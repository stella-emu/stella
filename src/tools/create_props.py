#!/usr/bin/env python3
"""Convert a Stella properties file (stella.pro) into the C++ DefProps.hxx header.

Python replacement for the old 'create_props.pl'.  The property names, order
and defaults are taken from src/emucore/Props.hxx (see propset.py), so this
tool can never drift out of sync with the emulator.
"""

import sys
from pathlib import Path

# Locate the 'propset' module next to this script (like Perl's FindBin)
sys.path.insert(0, str(Path(__file__).resolve().parent))
import propset  # noqa: E402

USAGE = """\
create_props.py <INPUT properties file> <OUTPUT C++ header>

Convert the given properties file into a C++ compatible header file."""

HEADER = """\
//============================================================================
//
//   SSSS    tt          lll  lll
//  SS  SS   tt           ll   ll
//  SS     tttttt  eeee   ll   ll   aaaa
//   SSSS    tt   ee  ee  ll   ll      aa
//      SS   tt   eeeeee  ll   ll   aaaaa  --  "An Atari 2600 VCS Emulator"
//  SS  SS   tt   ee      ll   ll  aa  aa
//   SSSS     ttt  eeeee llll llll  aaaaa
//
// Copyright (c) 1995-2026 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#ifndef DEF_PROPS_HXX
#define DEF_PROPS_HXX

// Static analyzer can't tell the following is data
// NOLINTBEGIN(modernize-raw-string-literal)

#include "bspf.hxx"

/**
  This code is generated using the 'create_props.py' script,
  located in the src/tools directory.  All properties changes
  should be made in stella.pro, and then this file should be
  regenerated and the application recompiled.
*/
"""


def _default_path(primary, fallback):
    # Saves having to type these paths every single time
    return primary if Path(primary).is_file() else fallback


def main(argv):
    if len(argv) == 1 and argv[0] in ("-help", "--help", "-h"):
        print(USAGE)
        return 0

    if len(argv) == 2:
        infile, outfile = argv[0], argv[1]
    elif len(argv) == 0:
        infile = _default_path("src/emucore/stella.pro", "../emucore/stella.pro")
        outfile = _default_path("src/emucore/DefProps.hxx", "../emucore/DefProps.hxx")
    else:
        print(USAGE)
        return 1

    propset_map = propset.load_prop_set(infile)
    setsize = len(propset_map)
    typesize = propset.num_prop_types()

    print(f"Valid properties found: {setsize}")

    # Walk the map in md5sum order and build each row
    rows = [
        propset.build_prop_string(propset_map[key])
        for key in sorted(propset_map)
    ]

    with open(outfile, "w", encoding="utf-8") as out:
        out.write(HEADER)
        out.write(
            f"static constexpr BSPF::array2D<const char*, "
            f"{setsize}, {typesize}> DefProps = {{{{\n"
        )
        out.write(",\n".join(rows))
        out.write("\n}};\n")
        out.write("\n// NOLINTEND(modernize-raw-string-literal)\n\n")
        out.write("#endif  // DEF_PROPS_HXX\n")

    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
