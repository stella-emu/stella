#!/usr/bin/env python3
"""Merge a user properties file into the system properties file.

Python replacement for the old 'merge_props.pl'.  For every entry that exists
in both files, the entry is removed from the user file and overwritten in the
system file.
"""

import os
import sys
from pathlib import Path

# Locate the 'propset' module next to this script (like Perl's FindBin)
sys.path.insert(0, str(Path(__file__).resolve().parent))
import propset  # noqa: E402

USAGE = """\
merge_props.py <USER properties file> <SYSTEM properties file>

Scan both properties files, and for every entry found in both files,
remove it from the USER file and overwrite it in the SYSTEM file."""


def main(argv):
    if len(argv) == 1 and argv[0] in ("-help", "--help", "-h"):
        print(USAGE)
        return 0

    if len(argv) == 2:
        usr_file, sys_file = argv[0], argv[1]
    elif len(argv) == 0:
        # Saves having to type these paths every single time
        usr_file = str(Path.home() / ".config" / "stella" / "stella.pro")
        sys_file = "src/emucore/stella.pro"
    else:
        print(USAGE)
        return 1

    print(usr_file)

    usr_propset = propset.load_prop_set(usr_file)
    sys_propset = propset.load_prop_set(sys_file)

    print()
    print(f"Valid properties found in user file: {len(usr_propset)}")
    print(f"Valid properties found in system file: {len(sys_propset)}")

    # Move every entry present in both files from the user file into the system file
    for key in list(usr_propset):
        if key in sys_propset:
            sys_propset[key] = usr_propset.pop(key)

    print()
    print(f"Updated properties found in user file: {len(usr_propset)}")
    print(f"Updated properties found in system file: {len(sys_propset)}")
    print()

    # Write both files back to disk
    propset.save_prop_set(usr_file, usr_propset)
    propset.save_prop_set(sys_file, sys_propset)

    if input("\nRun create_props [yN]: ").strip() == "y":
        script = Path(__file__).resolve().parent / "create_props.py"
        os.system(f'"{sys.executable}" "{script}"')

    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
