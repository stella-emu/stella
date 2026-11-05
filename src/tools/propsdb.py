#!/usr/bin/env python3
"""Keep a local properties database in sync with stella.pro.

stella.pro is what's committed and shipped, and what DefProps.hxx is built
from.  This tool keeps a local copy of it in an SQLite database, in the same
format as the 'properties' table of Stella's own stella.sqlite3, where entries
are easier to query and edit: with SQL, or with romhunter.py and merge_props.py,
which accept the database in place of stella.pro.

The database also keeps stella.pro as of the last sync (table 'sync_base'), so
it can tell which side changed since, and nothing is silently overwritten.

Run without arguments for usage and examples.
"""

import argparse
import contextlib
import io
import sys
from pathlib import Path

# Locate the 'propset' module next to this script (like Perl's FindBin)
sys.path.insert(0, str(Path(__file__).resolve().parent))
import propset  # noqa: E402

DEFAULT_DB = Path.home() / ".config" / "stella" / "props-master.sqlite3"
DEFAULT_PRO = Path(__file__).resolve().parent.parent / "emucore" / "stella.pro"
# stella.pro as of the last import, export or merge
BASE = "sync_base"
NAMES = propset.prop_names()
NAME = NAMES.index("Cart.Name")


def quiet(fn, *args):
    """Run a propset function without its progress lines; warnings go to stderr."""
    captured = io.StringIO()
    try:
        with contextlib.redirect_stdout(captured):
            return fn(*args)
    finally:
        for line in captured.getvalue().splitlines():
            if not line.startswith(("Loading properties", "Saving ")):
                print(line, file=sys.stderr)


def changes(old, new):
    """{md5: (kind, [fields])} for entries that differ between two property sets."""
    out = {}
    for md5 in old.keys() | new.keys():
        a, b = old.get(md5), new.get(md5)
        if a == b:
            continue
        if a is None:
            out[md5] = ("added", [])
        elif b is None:
            out[md5] = ("removed", [])
        else:
            out[md5] = ("changed", [NAMES[i] for i in range(len(NAMES)) if a[i] != b[i]])
    return out


def name_of(md5, *sets):
    return next((s[md5][NAME] for s in sets if md5 in s), "")


def write_changes(title, diff, *sets):
    print(f"{title}: {len(diff)}")
    for md5 in sorted(diff, key=lambda m: name_of(m, *sets).casefold()):
        kind, fields = diff[md5]
        print(f"  {kind:8} {md5}  {name_of(md5, *sets)}"
              + (f"  ({', '.join(fields)})" if fields else ""))


class Sync:
    def __init__(self, db, pro):
        self.db, self.pro = Path(db).expanduser(), Path(pro).expanduser()
        if not self.pro.is_file():
            sys.exit(f"ERROR: properties file '{self.pro}' not found")
        # Stella's own database holds the user's settings next to its properties
        if self.db.is_file() and "settings" in quiet(propset.db_tables, self.db):
            sys.exit(f"ERROR: '{self.db}' is Stella's own database, not a properties master")

    def need_db(self):
        if not self.db.is_file():
            sys.exit(f"ERROR: no database '{self.db}'; run 'import' first")

    def load(self):
        """(database, base, stella.pro) property sets."""
        local = quiet(propset.load_prop_db, self.db) if self.db.is_file() else {}
        base = quiet(propset.load_prop_db, self.db, BASE) if self.db.is_file() else {}
        return local, base, quiet(propset.load_prop_set, self.pro)

    def store(self, local, base):
        try:
            quiet(propset.save_prop_db, self.db, local)
            quiet(propset.save_prop_db, self.db, base, BASE)
        except ValueError as err:
            sys.exit(f"ERROR: {err}")


def cmd_import(sync, args):
    local, base, pro = sync.load()
    edits = changes(base, local)
    if edits and not args.force:
        write_changes("Database entries not exported yet", edits, local, base)
        sys.exit("\nERROR: importing would discard these; run 'export' or 'merge', "
                 "or 'import --force' to discard them")
    if sync.db.is_file():
        write_changes("Entries changed by the import", changes(local, pro), pro, local)
    sync.store(pro, pro)
    print(f"\nImported {len(pro)} entries into {sync.db}")


def cmd_export(sync, args):
    sync.need_db()
    local, base, pro = sync.load()
    upstream = changes(base, pro)
    if upstream and not args.force:
        write_changes("stella.pro entries changed since the last sync", upstream, pro, base)
        sys.exit("\nERROR: exporting would overwrite these; run 'merge' first, "
                 "or 'export --force' to overwrite them")
    diff = changes(pro, local)
    if not diff:
        print("stella.pro is already up to date")
    else:
        write_changes("Entries changed in stella.pro", diff, local, pro)
        try:
            propset.db_rows(local)
        except ValueError as err:
            sys.exit(f"ERROR: {err}")
        # stella.pro first: the sync record must never claim a write that failed
        quiet(propset.save_prop_set, sync.pro, local)
        print(f"\nWrote {len(local)} entries to {sync.pro}; now run create_props.py")
    sync.store(local, local)


def cmd_status(sync, args):
    sync.need_db()
    local, base, pro = sync.load()
    edits, upstream = changes(base, local), changes(base, pro)
    write_changes("Database changes since the last sync", edits, local, base)
    print()
    write_changes("stella.pro changes since the last sync", upstream, pro, base)
    print()
    if not edits and not upstream:
        print("In sync")
    elif not upstream:
        print("Run 'export' to write the database's changes to stella.pro")
    elif not edits:
        print("Run 'import' to bring stella.pro's changes into the database")
    else:
        print("Both changed: run 'merge', then 'export'")


def cmd_merge(sync, args):
    sync.need_db()
    local, base, pro = sync.load()
    merged, taken, conflicts = {}, [], []
    for md5 in local.keys() | base.keys() | pro.keys():
        mine, theirs, was = local.get(md5), pro.get(md5), base.get(md5)
        if mine == theirs or theirs == was:
            result = mine
        elif mine == was:
            result = theirs
            taken.append(md5)
        elif mine is None or theirs is None:
            # Removed on one side, changed on the other: the database's side stays
            result = mine
            conflicts.append((md5, "entry", "removed" if mine is None else "kept",
                              "removed" if theirs is None else "changed"))
        else:
            result, was = list(mine), was or [""] * len(NAMES)
            for i in range(len(NAMES)):
                if mine[i] == theirs[i] or theirs[i] == was[i]:
                    continue
                if mine[i] == was[i]:
                    result[i] = theirs[i]
                else:
                    conflicts.append((md5, NAMES[i], mine[i], theirs[i]))
            if result != mine:
                taken.append(md5)
        if result is not None:
            merged[md5] = result

    sets = (merged, pro, local, base)
    print(f"Entries updated from stella.pro: {len(taken)}")
    for md5 in sorted(taken, key=lambda m: name_of(m, *sets).casefold()):
        print(f"  {md5}  {name_of(md5, *sets)}")
    print(f"\nConflicts, database value kept: {len(conflicts)}")
    for md5, field, mine, theirs in conflicts:
        print(f"  {md5}  {name_of(md5, *sets)}\n"
              f"    {field:18} database:   \"{mine}\"\n"
              f"    {'':18} stella.pro: \"{theirs}\"")
    sync.store(merged, pro)
    print("\nMerged into the database; run 'status' to review, then 'export'")


def main(argv):
    ap = argparse.ArgumentParser(
        description="Keep a local properties database in sync with stella.pro.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=f"""\
commands:
  import   stella.pro -> database; refused if the database has edits that
           aren't exported yet
  export   database -> stella.pro; refused if stella.pro changed since the
           last sync (a pull, say)
  status   what changed on each side since the last sync
  merge    bring stella.pro's changes into the database, field by field; where
           both sides changed a field, the database's value is kept and listed

The database has the same 'properties' table as Stella's stella.sqlite3, so it
can be queried and edited with SQL, e.g.
  sqlite3 {DEFAULT_DB} \\
    "SELECT md5, json_extract(properties, '$.\\"Cart.Name\\"') FROM properties"

examples, from the top of the source tree:
  # Start, or refresh after a pull
  src/tools/propsdb.py import

  # Edit the database, e.g. with RomHunter's latest set
  src/tools/romhunter.py ~/RomHunter_v21 --old ~/RomHunter_v20 --pro {DEFAULT_DB} --write

  # Write stella.pro and regenerate DefProps.hxx
  src/tools/propsdb.py export
  src/tools/create_props.py

  # stella.pro changed while the database had edits
  src/tools/propsdb.py merge
  src/tools/propsdb.py export
""")
    ap.add_argument("command", choices=("import", "export", "status", "merge"))
    ap.add_argument("--db", default=str(DEFAULT_DB), help=f"database (default: {DEFAULT_DB})")
    ap.add_argument("--pro", default=str(DEFAULT_PRO),
                    help="properties file (default: src/emucore/stella.pro)")
    ap.add_argument("--force", action="store_true",
                    help="import or export even though it overwrites the other side's changes")
    if not argv:
        ap.print_help()
        return 0
    args = ap.parse_args(argv)

    sync = Sync(args.db, args.pro)
    {"import": cmd_import, "export": cmd_export,
     "status": cmd_status, "merge": cmd_merge}[args.command](sync, args)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
