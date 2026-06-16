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

import re
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
