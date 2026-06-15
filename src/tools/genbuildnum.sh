#!/bin/sh
#============================================================================
#
#   SSSS    tt          lll  lll
#  SS  SS   tt           ll   ll
#  SS     tttttt  eeee   ll   ll   aaaa
#   SSSS    tt   ee  ee  ll   ll      aa
#      SS   tt   eeeeee  ll   ll   aaaaa  --  "An Atari 2600 VCS Emulator"
#  SS  SS   tt   ee      ll   ll  aa  aa
#   SSSS     ttt  eeeee llll llll  aaaaa
#
# Copyright (c) 1995-2026 by Bradford W. Mott, Stephen Anthony
# and the Stella Team
#
# See the file "License.txt" for information on usage and redistribution of
# this file, and for a DISCLAIMER OF ALL WARRANTIES.
#============================================================================

# Generate src/common/VersionBuild.hxx with the build number taken from the git
# commit count (the same value GitHub reports). The header is only rewritten
# when the value actually changes, so the few objects that include it aren't
# needlessly recompiled. When git is unavailable (e.g. building from a release
# tarball), the existing file is left untouched and the fallback value in
# Version.hxx is used instead.

dir=`dirname "$0"`
out="$dir/../common/VersionBuild.hxx"

build=`git -C "$dir" rev-list --count HEAD 2>/dev/null` || exit 0
[ -n "$build" ] || exit 0

line="#define STELLA_BUILD_NUMBER \"$build\""
if [ ! -f "$out" ] || [ "$(cat "$out" 2>/dev/null)" != "$line" ]; then
  echo "$line" > "$out"
fi
