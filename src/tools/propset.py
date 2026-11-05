"""Property-set helpers for the Stella ROM properties tooling.

This module is the Python replacement for the old 'PropSet.pm'.  It loads,
saves and serializes the ROM properties used by 'create_props.py' and
'merge_props.py'.

Single source of truth
----------------------
The property *names*, their *order* and their *default values* are not
duplicated here.  Instead they are parsed directly from the canonical C++
definition in 'src/emucore/Props.hxx'.  That header already has to declare
the 'PropType' enum, the 'ourPropertyNames' table and the
'ourDefaultProperties' table for the emulator itself, so reusing it means the
tooling can never silently drift out of sync with the code (which is exactly
what used to happen with the hand-maintained tables in PropSet.pm).

On import this module also *validates* that the four related structures in
Props.hxx agree with each other:

  * 'enum class PropType'    - the canonical order
  * 'ourPropertyNames'       - name strings, in PropType order
  * 'ourDefaultProperties'   - default values, in PropType order
  * 'ourNameToPropType'      - the case-insensitively sorted lookup table

If any of them fall out of alignment, import fails with a clear error rather
than producing a subtly wrong DefProps.hxx.
"""

import json
import re
import sqlite3
from pathlib import Path


# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Parsing of src/emucore/Props.hxx (the single source of truth)
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

# Matches a single C++ string literal, honouring backslash escapes.  The
# captured group is the raw literal contents (escapes left intact), which is
# exactly what we want to compare against / emit verbatim.
_STRING_RE = re.compile(r'"((?:[^"\\]|\\.)*)"')


def _find_props_hxx():
    """Locate src/emucore/Props.hxx relative to this script's location."""
    here = Path(__file__).resolve().parent          # .../src/tools
    candidate = here.parent / "emucore" / "Props.hxx"  # .../src/emucore/Props.hxx
    if candidate.is_file():
        return candidate
    raise FileNotFoundError(
        f"Could not locate Props.hxx (looked for {candidate}). "
        "Run the tool from within the Stella source tree."
    )


def _extract_array_block(text, marker):
    """Return the text between the '{{' and matching '}}' following 'marker'."""
    start = text.find(marker)
    if start < 0:
        raise ValueError(f"'{marker}' not found in Props.hxx")
    open_pos = text.find("{{", start)
    if open_pos < 0:
        raise ValueError(f"opening '{{{{' for '{marker}' not found in Props.hxx")
    close_pos = text.find("}}", open_pos)
    if close_pos < 0:
        raise ValueError(f"closing '}}}}' for '{marker}' not found in Props.hxx")
    return text[open_pos + 2:close_pos]


def _extract_enum(text):
    """Return the PropType enumerators in declaration order, sans 'NumTypes'."""
    m = re.search(r'enum\s+class\s+PropType\s*:\s*\w+\s*\{(.*?)\}', text, re.DOTALL)
    if not m:
        raise ValueError("'enum class PropType' not found in Props.hxx")
    names = []
    for token in m.group(1).split(","):
        # strip trailing line comments and whitespace
        token = token.split("//", 1)[0].strip()
        if token:
            names.append(token)
    if not names or names[-1] != "NumTypes":
        raise ValueError("PropType enum did not end with 'NumTypes' as expected")
    return names[:-1]


def _parse_props_hxx():
    """Parse Props.hxx and return (names, defaults) as parallel lists.

    Also cross-validates the enum, name table, default table and lookup table
    so that an inconsistent edit to Props.hxx is caught immediately.
    """
    path = _find_props_hxx()
    text = path.read_text(encoding="utf-8")

    enum_names = _extract_enum(text)
    names = _STRING_RE.findall(_extract_array_block(text, "ourPropertyNames"))
    defaults = _STRING_RE.findall(_extract_array_block(text, "ourDefaultProperties"))

    n = len(enum_names)
    if not (len(names) == n and len(defaults) == n):
        raise ValueError(
            f"Props.hxx tables disagree in length: enum={n}, "
            f"ourPropertyNames={len(names)}, ourDefaultProperties={len(defaults)}"
        )

    # The enumerator 'Cart_MD5' must correspond to the name 'Cart.MD5', etc.
    for i, (enum_name, prop_name) in enumerate(zip(enum_names, names)):
        if prop_name.replace(".", "_") != enum_name:
            raise ValueError(
                f"Props.hxx entry {i} mismatch: enum '{enum_name}' "
                f"vs name '{prop_name}'"
            )

    _validate_name_to_proptype(text, names)

    return names, defaults


def _validate_name_to_proptype(text, names):
    """Verify ourNameToPropType covers every name and stays sorted."""
    block = _extract_array_block(text, "ourNameToPropType")
    entries = re.findall(r'\{\s*"([^"]+)"\s*,\s*PropType::(\w+)\s*\}', block)

    lookup_names = [name for name, _ in entries]
    if sorted(lookup_names) != sorted(names):
        raise ValueError(
            "ourNameToPropType does not list the same names as ourPropertyNames"
        )
    if lookup_names != sorted(lookup_names, key=str.casefold):
        raise ValueError(
            "ourNameToPropType is not in case-insensitive sorted order"
        )
    for name, proptype in entries:
        if name.replace(".", "_") != proptype:
            raise ValueError(
                f"ourNameToPropType maps '{name}' to 'PropType::{proptype}'"
            )


# Parsed once at import; these are the canonical, validated tables.
_PROP_NAMES, _PROP_DEFAULTS = _parse_props_hxx()
_NAME_TO_INDEX = {name: i for i, name in enumerate(_PROP_NAMES)}
_MD5_INDEX = _NAME_TO_INDEX["Cart.MD5"]

# A '"key" "value"' line from a properties file.  Keys never contain a quote;
# values may contain escaped quotes, so anchor the value at end-of-line.
_LINE_RE = re.compile(r'^"([^"]*)"\s+"(.*)"$')


# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Public API (mirrors the old PropSet.pm subroutines)
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

def prop_names():
    """Property names, in PropType order."""
    return _PROP_NAMES


def prop_defaults():
    """Default property values, in PropType order."""
    return _PROP_DEFAULTS


def num_prop_types():
    """Number of property tags in one PropSet element."""
    return len(_PROP_NAMES)


def load_prop_set(path):
    """Load and parse a properties file into a dict keyed by md5sum.

    Each value is a list of property strings in PropType order.
    """
    print(f"Loading properties from file: {path}")

    propset = {}
    props = [""] * len(_PROP_NAMES)

    with open(path, encoding="utf-8") as infile:
        for raw in infile:
            line = raw.rstrip("\r\n")

            # A bare '""' line terminates the current item
            if line.startswith('""'):
                key = props[_MD5_INDEX]
                if key in propset:
                    print(f"Duplicate: {key}")
                propset[key] = props
                props = [""] * len(_PROP_NAMES)
            elif line.strip():
                m = _LINE_RE.match(line)
                key = m.group(1) if m else None
                if key in _NAME_TO_INDEX:
                    props[_NAME_TO_INDEX[key]] = m.group(2)
                else:
                    print(f"ERROR: {line}")
                    print(f"Invalid key = '{key}' for md5 = "
                          f"'{props[_MD5_INDEX]}', ignoring ...")

    return propset


def save_prop_set(path, propset):
    """Write a properties dict back to disk, sorted by md5sum."""
    print(f"Saving {len(propset)} properties to file: {path}")

    with open(path, "w", encoding="utf-8") as outfile:
        for md5 in sorted(propset):
            for i, value in enumerate(propset[md5]):
                if value != "":
                    outfile.write(f'"{_PROP_NAMES[i]}" "{value}"\n')
            outfile.write('""\n\n')


def build_prop_string(values):
    """Convert a property value list into a C++ array initializer row.

    Values equal to their default are emitted as the empty string, matching
    how DefProps is consumed (setDefaults() runs first, so unset slots inherit
    the compiled-in default).
    """
    items = [
        f'"{value}"' if _PROP_DEFAULTS[i] != value else '""'
        for i, value in enumerate(values)
    ]
    return "  { " + ", ".join(items) + " }"


# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Stella's database format
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#
# The same layout as the 'properties' table of Stella's stella.sqlite3: one
# JSON object per ROM, keyed by md5sum.  Values that parse as JSON are stored
# as JSON and read back as compact JSON text, as KeyValueRepositoryJsonFile
# does.  Property-set values stay in their stella.pro form (quotes and
# backslashes escaped), so both formats load into the same structure.

_DB_TABLE = ("CREATE TABLE IF NOT EXISTS `{}` "
             "(`md5` TEXT PRIMARY KEY, `properties` TEXT) WITHOUT ROWID")
_DB_SUFFIXES = (".sqlite3", ".sqlite", ".db")


def _unescape(text):
    """stella.pro text -> real value, as KeyValueRepositoryPropertyFile reads it."""
    out, i = [], 0
    while i < len(text):
        if text[i] == "\\" and i + 1 < len(text) and text[i + 1] in '"\\':
            i += 1
        out.append(text[i])
        i += 1
    return "".join(out)


def _escape(value):
    """Real value -> stella.pro text, as KeyValueRepositoryPropertyFile writes it."""
    return value.replace("\\", "\\\\").replace('"', '\\"')


def _reject_constant(name):
    raise ValueError(name)


def _json_if_valid(value):
    """The JSON a value is stored as, or the value itself (jsonIfValid)."""
    try:
        return json.loads(value, parse_constant=_reject_constant)
    except ValueError:
        return value


def _json_text(value):
    """A stored JSON value as the text Stella reads back."""
    if isinstance(value, str):
        return value
    return json.dumps(value, separators=(",", ":"), ensure_ascii=False, sort_keys=True)


def to_db_value(text):
    """stella.pro text -> the JSON value Stella stores for it.

    Raises ValueError for text that wouldn't read back unchanged.
    """
    value = _unescape(text)
    stored = _json_if_valid(value)
    too_big = isinstance(stored, int) and not isinstance(stored, bool) and abs(stored) >= 2**63
    if _escape(value) != text or _json_text(stored) != value or too_big:
        raise ValueError(f"'{text}' doesn't survive the database format unchanged")
    return stored


def from_db_value(value):
    """A JSON value from the database -> stella.pro text."""
    return _escape(_json_text(value))


def load_prop_db(path, table="properties"):
    """Load a database in Stella's format into a dict keyed by md5sum.

    The same structure as load_prop_set() returns; a missing table is empty.
    """
    print(f"Loading properties from database: {path}")

    db = sqlite3.connect(f"file:{path}?mode=ro", uri=True)
    try:
        exists = db.execute("SELECT 1 FROM sqlite_master WHERE type='table' AND name=?",
                            (table,)).fetchone()
        rows = db.execute(f"SELECT md5, properties FROM `{table}`").fetchall() if exists else []
    finally:
        db.close()

    propset = {}
    for md5, text in rows:
        props = [""] * len(_PROP_NAMES)
        props[_MD5_INDEX] = md5
        for key, value in json.loads(text).items():
            if key in _NAME_TO_INDEX:
                props[_NAME_TO_INDEX[key]] = from_db_value(value)
            else:
                print(f"Invalid key = '{key}' for md5 = '{md5}', ignoring ...")
        propset[md5] = props
    return propset


def db_rows(propset):
    """(md5, JSON) rows for a properties dict.

    Raises ValueError if a value can't be stored unchanged, so this also
    checks a properties dict before anything is written.
    """
    rows = []
    for md5 in sorted(propset):
        entry = {}
        for i, text in enumerate(propset[md5]):
            if text != "":
                try:
                    entry[_PROP_NAMES[i]] = to_db_value(text)
                except ValueError as err:
                    raise ValueError(f"md5 = '{md5}', {_PROP_NAMES[i]}: {err}") from None
        rows.append((md5, json.dumps(entry, indent=2, sort_keys=True, ensure_ascii=False)))
    return rows


def save_prop_db(path, propset, table="properties"):
    """Replace a table of a database in Stella's format with a properties dict.

    Raises ValueError, before anything is written, if a value can't be stored.
    """
    rows = db_rows(propset)
    print(f"Saving {len(rows)} properties to database: {path}")
    db = sqlite3.connect(path)
    try:
        with db:
            db.execute(_DB_TABLE.format(table))
            db.execute(f"DELETE FROM `{table}`")
            db.executemany(f"INSERT INTO `{table}` VALUES (?, ?)", rows)
    finally:
        db.close()


def db_tables(path):
    """Names of the tables in a database."""
    db = sqlite3.connect(f"file:{path}?mode=ro", uri=True)
    try:
        return {row[0] for row in db.execute("SELECT name FROM sqlite_master WHERE type='table'")}
    finally:
        db.close()


def is_db(path):
    return Path(path).suffix.lower() in _DB_SUFFIXES


def load(path):
    """Load a properties file or database, by its extension."""
    return load_prop_db(path) if is_db(path) else load_prop_set(path)


def save(path, propset):
    """Save a properties file or database, by its extension."""
    if is_db(path):
        save_prop_db(path, propset)
    else:
        save_prop_set(path, propset)
