#!/usr/bin/env python3
"""Keep a snapshot directory in sync with Stella's ROM database.

For each ROM, Stella shows '<Cart.Name>.png' (or .jpg, optionally with an
'_<n>' suffix) from the snapshot directory, so when ROMs are renamed in
stella.pro their snapshots have to follow.

The current names come from 'stella -listrominfo', the database compiled into
the binary; rebuild Stella after changing stella.pro.  Which ROM a snapshot
shows is worked out from, in order:

  1. the baseline: the ROM each snapshot name showed when the directory was
     last in sync.  That is the record the last --write left in the directory,
     or else stella.pro in git as of the newest snapshot (--since overrides)
  2. the current database, for names that are already current
  3. the 'ROM MD5' that Stella writes into its own snapshots, for names that
     were never a ROM's (such as a ROM's filename)
  4. older stella.pro revisions, for snapshots that are out of date for longer

Each snapshot is then renamed to its ROM's current name, in two phases so that
names which swap or shift along a chain don't overwrite each other.  Snapshots
of ROMs no longer in the database are redundant.  Files that were never a ROM
name are only listed, since Stella can also find a snapshot by ROM filename.

Run without arguments for usage and examples.
"""

import argparse
import datetime
import os
import re
import shutil
import struct
import subprocess
import sys
import tempfile
from collections import defaultdict
from pathlib import Path

# Locate 'romhunter' next to this script (like Perl's FindBin)
sys.path.insert(0, str(Path(__file__).resolve().parent))
from romhunter import hash_dir  # noqa: E402

REPO = Path(__file__).resolve().parents[2]
PRO = "src/emucore/stella.pro"
EXTENSIONS = (".png", ".jpg")
# Record of the ROM each snapshot name shows, written by --write
RECORD = ".sync_snapshots"
# Stella's own fallback image, never a ROM's snapshot
DEFAULT_SNAPSHOT = "default_snapshot"
# Characters that can't be in a filename on Windows; Stella itself already
# replaces '/' and '\'
BAD_CHARS = re.compile(r'[/\\:*?"<>|]')
# Both the current 'Cart.' and the old 'Cartridge.' property names
PRO_LINE = re.compile(r'^"Cart(?:ridge)?\.(MD5|Name)" "(.*)"$')


# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Sources of ROM names
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

def parse_rominfo(text):
    """'stella -listrominfo' output -> {md5: name}."""
    lines = text.splitlines()
    header = lines[0].split("|")
    mi, ni = header.index("Cart.MD5"), header.index("Cart.Name")
    roms = {}
    for line in lines[1:]:
        fields = line.split("|")
        if len(fields) == len(header):
            roms[fields[mi].lower()] = fields[ni]
    return roms


def run_stella(stella):
    """Run 'stella -listrominfo' with a throwaway base directory."""
    with tempfile.TemporaryDirectory() as basedir:
        result = subprocess.run([stella, "-basedir", basedir, "-listrominfo"],
                                capture_output=True, text=True, encoding="utf-8",
                                errors="replace")
    if result.returncode != 0 or not result.stdout.startswith("Cart.MD5|"):
        sys.exit(f"ERROR: '{stella} -listrominfo' failed:\n{result.stderr}")
    return result.stdout


def sanitize(name):
    """Same as BSPF::sanitizeFilename, which Stella applies to every Cart.Name."""
    return name.replace("/", "_").replace("\\", "_")


def parse_pro(text):
    """stella.pro text, current or old format -> {md5: name}."""
    roms, md5, name = {}, None, None
    for line in text.splitlines():
        m = PRO_LINE.match(line)
        if m:
            if m.group(1) == "MD5":
                md5 = m.group(2).lower()
            else:
                name = sanitize(m.group(2))
        elif line.startswith('""'):
            if md5 and name is not None:
                roms[md5] = name
            md5, name = None, None
    return roms


def git(*args):
    return subprocess.run(["git", "-C", str(REPO), *args], capture_output=True,
                          text=True, encoding="utf-8", errors="replace")


def pro_at(rev):
    """{md5: name} from stella.pro at a git revision, or None."""
    result = git("show", f"{rev}:{PRO}")
    return parse_pro(result.stdout) if result.returncode == 0 else None


def pro_rev_before(when):
    """The last stella.pro revision committed before a datetime, or None."""
    rev = git("rev-list", "-1", f"--before={when.isoformat()}", "HEAD", "--", PRO).stdout.strip()
    return rev or None


def pro_history():
    """{name: md5s} from the newest stella.pro revision that used each name."""
    history = {}
    for rev in git("log", "--format=%H", "--", PRO).stdout.split():
        for name, md5s in by_name(pro_at(rev)).items():
            history.setdefault(name, md5s)
    return history


def read_record(path):
    """{name: md5s} from the record of the last --write."""
    names = defaultdict(set)
    for line in path.read_text(encoding="utf-8").splitlines():
        if line and not line.startswith("#"):
            md5, _, name = line.partition("|")
            names[name].add(md5)
    return names


def write_record(path, owners):
    lines = sorted(f"{md5}|{name}" for name, md5s in owners.items() for md5 in md5s)
    path.write_text("# Written by src/tools/sync_snapshots.py: the ROM each "
                    "snapshot name shows\n" + "\n".join(lines) + "\n", encoding="utf-8")


def png_md5(path):
    """The 'ROM MD5' text Stella writes into its snapshots, or None."""
    with open(path, "rb") as f:
        if f.read(8) != b"\x89PNG\r\n\x1a\n":
            return None
        while True:
            header = f.read(8)
            if len(header) < 8:
                return None
            size, kind = struct.unpack(">I4s", header)
            if kind in (b"IDAT", b"IEND"):
                return None
            data = f.read(size)
            f.seek(4, os.SEEK_CUR)
            if kind == b"tEXt":
                key, _, value = data.partition(b"\0")
                if key == b"ROM MD5":
                    return value.decode("latin-1").strip().lower()


def by_name(roms):
    names = defaultdict(set)
    for md5, name in roms.items():
        names[name].add(md5)
    return names


# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Planning
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

def split_snapshot(filename, known):
    """'Name_2.png' -> ('Name', '_2', '.png', doubled), using known names.

    'doubled' is True for a repeated suffix such as 'Name_2_2.png', which
    Stella doesn't recognize.
    """
    stem, ext = os.path.splitext(filename)
    if stem not in known:
        m = re.fullmatch(r"(.*?)(_\d+)(\2)?", stem)
        if m and m.group(1) in known:
            return m.group(1), m.group(2), ext, m.group(3) is not None
    return stem, "", ext, False


class Plan:
    def __init__(self, files, current, baseline, history, metadata, remove):
        cur_names = by_name(current)
        base_md5s = set().union(*baseline.values()) if baseline else set()
        known = set(cur_names) | set(baseline) | set(history)
        # A ROM MD5 in a PNG identifies every image under that name
        tagged = {split_snapshot(f, known)[0]: {md5} for f, md5 in metadata.items()
                  if md5 in current or md5 in base_md5s}
        known |= set(tagged)

        self.keep, self.moves = {}, {}
        self.redundant, self.unknown, self.ambiguous, self.blocked = {}, {}, {}, {}
        # The ROM(s) each file shows, and the snapshot name it ends up under
        self.owners, self.final = {}, {}

        target = {}
        for f in files:
            name, suffix, ext, doubled = split_snapshot(f, known)
            if name == DEFAULT_SNAPSHOT:
                continue
            # The name was chosen deliberately, so it beats the ROM MD5 the
            # image was captured from
            if name in baseline:
                owners, how = baseline[name], ""
            elif name in cur_names:
                owners, how = cur_names[name], ""
            elif name in tagged:
                owners, how = tagged[name], "ROM MD5 in the PNG"
            elif name in history:
                owners, how = history[name], "name from an older stella.pro"
            else:
                self.unknown[f] = "not a ROM name in any stella.pro"
                continue
            if doubled:
                how = "repairs a doubled suffix" + (f"; {how}" if how else "")
            self.owners[f], self.final[f] = owners, name

            dests = {current[m] for m in owners if m in current}
            # Of several ROMs that shared a name, the one keeping it keeps the images
            if len(dests) > 1 and name in dests:
                dests = {name}
            if not dests:
                self.redundant[f] = "ROM no longer in the database"
            elif len(dests) > 1:
                self.ambiguous[f] = "name was shared by ROMs now called: " + "; ".join(sorted(dests))
            else:
                dest = dests.pop()
                target[f] = (dest + suffix + ext, how, dest)

        # Every destination can hold only one file
        claims = defaultdict(list)
        for f, (dest, _, _) in target.items():
            claims[dest].append(f)
        for dest, fs in claims.items():
            if dest in fs:
                self.keep[dest] = target[dest][1]
                for f in fs:
                    if f == dest:
                        continue
                    # The same ROM's image under its current name wins
                    if self.owners[f] & self.owners[dest]:
                        self.redundant[f] = f"superseded by '{dest}'"
                    else:
                        self.blocked[f] = (dest, "the new name is shared with a different "
                                           "ROM; give them distinct names in stella.pro")
            elif len(fs) > 1:
                for f in fs:
                    self.blocked[f] = (dest, "several snapshots want this name: " + "; ".join(fs))
            elif BAD_CHARS.search(os.path.splitext(dest)[0]):
                self.blocked[fs[0]] = (dest, "the new name can't be a filename")
            else:
                self.moves[fs[0]] = target[fs[0]][:2]

        # A destination still held by a file that isn't moving away; blocking
        # one move keeps its file in place, which can block another.  All
        # images under one name move together, or none of them do.
        changed = True
        while changed:
            staying = set(files) - set(self.moves) - (set(self.redundant) if remove else set())
            held = {self.final[f] for f in (*self.blocked, *self.ambiguous)}
            changed = False
            for f, (dest, _) in list(self.moves.items()):
                if dest in staying:
                    why = ("the new name holds a redundant snapshot; needs --remove"
                           if dest in self.redundant else
                           "the new name is taken by a snapshot that stays")
                elif self.final[f] in held:
                    why = "another image under the same name can't move"
                else:
                    continue
                self.blocked[f] = (dest, why)
                del self.moves[f]
                changed = True
        self.replaces = {f for f, (dest, _) in self.moves.items() if dest in self.redundant}
        for f in self.moves:
            self.final[f] = target[f][2]

        # Names that end up without a plain '<name>.png' or '.jpg'
        final = set(self.keep) | {dest for dest, _ in self.moves.values()}
        self.missing, self.unusable = [], []
        for name in sorted(cur_names, key=str.casefold):
            if BAD_CHARS.search(name):
                self.unusable.append(name)
            elif not any(name + ext in final for ext in EXTENSIONS):
                self.missing.append(name)
        self.shared = {n: sorted(m) for n, m in cur_names.items() if len(m) > 1}

    def record(self, remove):
        """{name: md5s} each snapshot name shows once the plan is applied."""
        names = defaultdict(set)
        for f, owners in self.owners.items():
            if not (remove and f in self.redundant):
                names[self.final[f]] |= owners
        return names


def apply(plan, snapdir, remove):
    """Delete (with --remove) and rename in two phases; returns error count."""
    errors = 0
    if remove:
        for f in plan.redundant:
            os.remove(snapdir / f)
    temps = {}
    for i, (f, (dest, _)) in enumerate(sorted(plan.moves.items())):
        tmp = snapdir / f".sync_snapshots.{i}.tmp"
        os.rename(snapdir / f, tmp)
        temps[tmp] = dest
    for tmp, dest in temps.items():
        if (snapdir / dest).exists():
            print(f"ERROR: '{dest}' exists, left as '{tmp.name}'", file=sys.stderr)
            errors += 1
        else:
            os.rename(tmp, snapdir / dest)
    return errors


# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

def main(argv):
    ap = argparse.ArgumentParser(
        description="Sync a snapshot directory with Stella's ROM database.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=f"""\
The report goes to stdout.  With --write, snapshots are renamed to their ROM's
current name (blocked renames never are), and '{RECORD}' in the snapshot
directory records which ROM each snapshot name shows, as the baseline for the
next run.  Rebuild Stella after changing stella.pro, since the names come from
the database compiled into it.

examples, from the top of the source tree:
  # Report only; nothing is changed
  src/tools/sync_snapshots.py /path/to/snapshots > report.txt

  # Same, listing missing snapshots of RomHunter's ROMs separately
  src/tools/sync_snapshots.py /path/to/snapshots --romhunter ~/RomHunter_v20 > report.txt

  # Rename snapshots to their ROM's current name
  src/tools/sync_snapshots.py /path/to/snapshots --write

  # Same, but also delete snapshots of ROMs no longer in the database
  src/tools/sync_snapshots.py /path/to/snapshots --write --remove

  # No record yet, and the snapshots' dates don't tell when they were synced
  src/tools/sync_snapshots.py /path/to/snapshots --since <git revision>
""")
    ap.add_argument("snapdir", help="snapshot directory")
    ap.add_argument("--stella", help="Stella binary to ask for the database "
                    "(default: ./stella, else 'stella' on the PATH)")
    ap.add_argument("--rominfo", help="read saved 'stella -listrominfo' output instead")
    ap.add_argument("--since", help="git revision of stella.pro the snapshots were last "
                    f"in sync with (default: the '{RECORD}' record, else the revision "
                    "as of the newest snapshot)")
    ap.add_argument("--romhunter", help="RomHunter ROM set; lists missing snapshots of "
                    "his ROMs separately from the rest")
    ap.add_argument("--write", action="store_true", help="rename snapshots")
    ap.add_argument("--remove", action="store_true",
                    help="also delete redundant snapshots, freeing their names "
                         "for renames (applied with --write)")
    if not argv:
        ap.print_help()
        return 0
    args = ap.parse_args(argv)

    snapdir = Path(args.snapdir).expanduser()
    if not snapdir.is_dir():
        sys.exit(f"ERROR: '{snapdir}' is not a directory")

    if args.rominfo:
        source = args.rominfo
        current = parse_rominfo(Path(args.rominfo).read_text(encoding="utf-8"))
    else:
        stella = args.stella or (str(Path("stella").resolve()) if Path("stella").is_file()
                                 else shutil.which("stella"))
        if not stella:
            sys.exit("ERROR: no Stella binary found; use --stella or --rominfo")
        source = f"{stella} -listrominfo"
        current = parse_rominfo(run_stella(stella))

    files = sorted(f for f in os.listdir(snapdir)
                   if f.endswith(EXTENSIONS) and not f.startswith("."))
    in_git = git("rev-parse", "--git-dir").returncode == 0

    # Baseline: --since, else the record of the last --write, else stella.pro
    # as of the newest snapshot
    baseline, origin = {}, "none"
    if args.since:
        roms = pro_at(args.since) if in_git else None
        if roms is None:
            sys.exit(f"ERROR: no {PRO} at git revision '{args.since}'")
        baseline, origin = by_name(roms), f"{PRO} at {args.since}"
    elif (snapdir / RECORD).is_file():
        baseline, origin = read_record(snapdir / RECORD), f"{RECORD} (from the last --write)"
    elif in_git and files:
        newest = datetime.datetime.fromtimestamp(
            max((snapdir / f).stat().st_mtime for f in files)).replace(microsecond=0)
        rev = pro_rev_before(newest)
        if rev:
            baseline = by_name(pro_at(rev))
            origin = f"{PRO} at {rev[:10]}, as of the newest snapshot ({newest})"
    history = pro_history() if in_git else {}

    # The binary must have been built from the stella.pro in this tree
    stale = 0
    if (REPO / PRO).is_file():
        tree = parse_pro((REPO / PRO).read_text(encoding="utf-8"))
        stale = sum(1 for m in tree.keys() | current.keys() if tree.get(m) != current.get(m))

    metadata = {f: png_md5(snapdir / f) for f in files if f.endswith(".png")}
    metadata = {f: m for f, m in metadata.items() if m}
    plan = Plan(files, current, baseline, history, metadata, args.remove)

    # Missing snapshots split by whether the ROM is in RomHunter's set
    romhunter = set(hash_dir(args.romhunter)) if args.romhunter else None
    if romhunter is not None:
        roms_named = by_name(current)
        missing_rh = [n for n in plan.missing if roms_named[n] & romhunter]
        missing_other = [n for n in plan.missing if not roms_named[n] & romhunter]

    # - - - - - - - - - - - - - - - - - - report - - - - - - - - - - - - - - -
    out = sys.stdout
    out.write(f"\nSnapshots:  {snapdir}  ({len(files)} files)\n")
    out.write(f"Database:   {source}  ({len(current)} ROMs)\n")
    out.write(f"Baseline:   {origin}\n")
    if stale:
        out.write(f"\nWARNING: the database differs from {REPO / PRO} in {stale} "
                  "entries; rebuild Stella first\n")
    out.write(f"\n  in sync    {len(plan.keep):5}\n")
    out.write(f"  renamed    {len(plan.moves):5}   applied with --write\n")
    out.write(f"  redundant  {len(plan.redundant):5}   ROM gone, or image superseded; "
              "deleted with --write --remove\n")
    out.write(f"  unknown    {len(plan.unknown):5}   not a ROM name; never deleted\n")
    out.write(f"  blocked    {len(plan.blocked) + len(plan.ambiguous):5}   never applied\n")
    if romhunter is None:
        out.write(f"  missing    {len(plan.missing):5}   names without a snapshot\n")
    else:
        out.write(f"  missing    {len(missing_rh):5}   RomHunter's ROMs without a snapshot\n")
        out.write(f"  missing    {len(missing_other):5}   other ROMs without a snapshot\n")
    out.write(f"  shared     {len(plan.shared):5}   names used by more than one ROM\n")
    out.write(f"  unusable   {len(plan.unusable):5}   names that can't be a filename\n")

    def section(title, n):
        out.write(f"\n{'=' * 78}\n{title} ({n})\n{'=' * 78}\n")

    def by_file(d):
        return sorted(d.items(), key=lambda x: x[0].casefold())

    section("RENAMED", len(plan.moves))
    for f, (dest, how) in by_file(plan.moves):
        out.write(f"\n   {f}\n-> {dest}\n")
        if how:
            out.write(f"   check: {how}\n")
        if f in plan.replaces:
            out.write("   check: replaces a redundant snapshot\n")

    section("REDUNDANT (deleted with --write --remove)", len(plan.redundant))
    for f, why in by_file(plan.redundant):
        out.write(f"{f}\n" + (f"   {why}\n" if why.startswith("superseded") else ""))

    section("UNKNOWN (never deleted; may be found by ROM filename)", len(plan.unknown))
    for f, _ in by_file(plan.unknown):
        out.write(f"{f}\n")

    section("BLOCKED (never applied)", len(plan.blocked) + len(plan.ambiguous))
    for f, (dest, why) in by_file(plan.blocked):
        out.write(f"\n   {f}\n-> {dest}\n   {why}\n")
    for f, why in by_file(plan.ambiguous):
        out.write(f"\n   {f}\n   {why}\n")

    if romhunter is None:
        section("MISSING", len(plan.missing))
        for name in plan.missing:
            out.write(f"{name}\n")
    else:
        section("MISSING (RomHunter's ROMs)", len(missing_rh))
        for name in missing_rh:
            out.write(f"{name}\n")
        section("MISSING (other ROMs)", len(missing_other))
        for name in missing_other:
            out.write(f"{name}\n")

    section("SHARED (one snapshot shows for all of them)", len(plan.shared))
    for name, md5s in sorted(plan.shared.items(), key=lambda x: x[0].casefold()):
        out.write(f"\n{name}\n" + "".join(f"   {m}\n" for m in md5s))

    section("UNUSABLE (can't be a filename)", len(plan.unusable))
    for name in plan.unusable:
        out.write(f"{name}\n")

    if args.write:
        if stale:
            print("\nERROR: not changing anything, the database is out of date; "
                  "rebuild Stella first", file=sys.stderr)
            return 1
        errors = apply(plan, snapdir, args.remove)
        print(f"\nRenamed {len(plan.moves) - errors} snapshots"
              + (f", deleted {len(plan.redundant)}" if args.remove else ""),
              file=sys.stderr)
        if errors:
            print(f"Not updating {RECORD}; fix the errors above first", file=sys.stderr)
            return 1
        write_record(snapdir / RECORD, plan.record(args.remove))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
