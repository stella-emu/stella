#!/usr/bin/env bash
# sync.sh -- re-vendor the FujiNet cartridge's shared sources from fn-2600.
#
# The protocol, the bus decode and the glyph compositor are the RP2040
# cartridge firmware's own files. They are shared rather than reimplemented
# for the reason fujimail.h states: two copies of a protocol drift, and every
# browse-and-boot run in Stella should exercise what the cartridge actually
# ships. MAME vendors the same files through pico/atari-2600/emu/apply.sh.
#
# Usage: ./sync.sh [path-to-fn-2600/pico/atari-2600]
#   default: $FN2600, else ~/Workspace/fn-2600/pico/atari-2600
#
# The only transformations are Stella's file-naming convention (.h -> .hxx,
# .c -> .cxx, and the #include lines that follow from it). The contents are
# otherwise byte-identical to upstream, so a diff against fn-2600 is a
# meaningful drift check. Idempotent: run it again after any upstream change.
#
# vcsmap.h is deliberately NOT vendored. Stella owns a booted game's mapper --
# it has sixty-odd schemes where vcsmap has nine -- which is what upstream's
# VCS_CART_HOST_MAPPER guard in vcs_cart.h selects.  That define is set for
# the whole build by configure.
set -euo pipefail
cd "$(dirname "$0")"

SRC="${1:-${FN2600:-$HOME/Workspace/fn-2600/pico/atari-2600}}"
[ -d "$SRC/firmware/include" ] || { echo "sync.sh: no firmware at $SRC" >&2; exit 1; }

HEADERS="fuji_mailbox fujibus fujimail vcs_cart vcs_render vcs_font"
SOURCES="fujibus fujimail vcs_render"

# The rename is the whole patch; keep it in one place so it cannot drift.
retarget() { sed -e 's/#include "\([A-Za-z0-9_]*\)\.h"/#include "\1.hxx"/'; }

for h in $HEADERS; do retarget < "$SRC/firmware/include/$h.h"  > "$h.hxx"; done
for c in $SOURCES; do retarget < "$SRC/firmware/src/$c.c"      > "$c.cxx"; done

echo "sync.sh: vendored $(echo $HEADERS | wc -w) headers and $(echo $SOURCES | wc -w) sources from $SRC"
