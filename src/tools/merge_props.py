#!/usr/bin/env python3
"""Merge the user properties in Stella's database into the system properties file.

Stella keeps the properties edited in its GUI in 'stella.sqlite3', one JSON
object per ROM in the 'properties' table.  For every entry that also exists in
the system file, the entry is removed from the database and overwritten in the
system file, where it is then built in.
"""

import os
import sqlite3
import sys
from pathlib import Path

# Locate the 'propset' module next to this script (like Perl's FindBin)
sys.path.insert(0, str(Path(__file__).resolve().parent))
import propset  # noqa: E402

USAGE = """\
merge_props.py <USER database> <SYSTEM properties file or database>

Scan Stella's database and the system properties file, and for every entry
found in both, remove it from the USER database and overwrite it in the SYSTEM
file.  Close Stella first, since it uses the database too.

The SYSTEM side can also be a properties master made by propsdb.py.  Without
arguments, ~/.config/stella/stella.sqlite3 and src/emucore/stella.pro are used."""


def remove_from_database(path, md5s):
    db = sqlite3.connect(path)
    try:
        with db:
            db.executemany("DELETE FROM properties WHERE md5 = ?",
                           [(md5,) for md5 in md5s])
    finally:
        db.close()


def main(argv):
    if len(argv) == 1 and argv[0] in ("-help", "--help", "-h"):
        print(USAGE)
        return 0

    if len(argv) == 2:
        usr_db, sys_file = argv[0], argv[1]
    elif len(argv) == 0:
        # Saves having to type these paths every single time
        usr_db = str(Path.home() / ".config" / "stella" / "stella.sqlite3")
        sys_file = "src/emucore/stella.pro"
    else:
        print(USAGE)
        return 1

    for path in (usr_db, sys_file):
        if not Path(path).is_file():
            print(f"ERROR: file '{path}' not found")
            return 1

    try:
        usr_propset = propset.load_prop_db(usr_db)
    except sqlite3.Error as err:
        print(f"ERROR: can't read properties from '{usr_db}': {err}")
        return 1
    sys_propset = propset.load(sys_file)

    print()
    print(f"Valid properties found in user database: {len(usr_propset)}")
    print(f"Valid properties found in system file: {len(sys_propset)}")

    # Move every entry present in both from the database into the system file
    merged = sorted(key for key in usr_propset if key in sys_propset)
    for key in merged:
        sys_propset[key] = usr_propset[key]

    print()
    print(f"Updated properties found in user database: {len(usr_propset) - len(merged)}")
    print(f"Updated properties found in system file: {len(sys_propset)}")
    print()

    if not merged:
        print("Nothing to merge")
        return 0

    for key in merged:
        print(f"Merged: {key}  {usr_propset[key][propset.prop_names().index('Cart.Name')]}")
    print()

    # Write the system file first, so a failure can't lose the user entries
    try:
        propset.save(sys_file, sys_propset)
    except ValueError as err:
        print(f"ERROR: not saving, {err}")
        return 1
    remove_from_database(usr_db, merged)
    print(f"Removed {len(merged)} merged entries from: {usr_db}")

    if input("\nRun create_props [yN]: ").strip() == "y":
        script = Path(__file__).resolve().parent / "create_props.py"
        os.system(f'"{sys.executable}" "{script}"')

    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
