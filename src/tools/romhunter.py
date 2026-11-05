#!/usr/bin/env python3
"""Update stella.pro from a new RomHunter ROM set.

RomHunter's filenames are a flattened database:

  Title[ - Subtitle] (AKA ...) (Alt names) (Controller) (Date)
    (Manufacturer, Programmers - Partner) (Number) (ModelNo)
    (Prototype) (PAL) (4K) [a] ~

This tool hashes every ROM, parses its filename into those fields and maps
them onto the stella.pro properties (Cart.Name, Cart.Manufacturer,
Cart.ModelNo, Cart.Note, Cart.Rarity).

Given the previous RomHunter set as well (--old), every field is merged three
ways: what stella.pro has now, what the old RomHunter name implies and what
the new one implies.  A field RomHunter didn't change keeps whatever stella.pro
has.  A field he did change is updated when stella.pro still matches his old
name, and reported as a conflict otherwise.  ROMs missing from stella.pro get
a proposed entry derived from the filename.  With --remove, entries for ROMs
dropped since the previous set are deleted, keeping stella.pro in sync with
his collection; by default they are kept.

How a filename maps onto Cart.Name (the short manufacturer, which subtitles
are kept) is learned from the existing stella.pro entries, not hardcoded.

The previous set must keep RomHunter's original filenames; a set renamed by
Stella's ROM Audit no longer says what he called each ROM.

Run without arguments for usage and examples.
"""

import argparse
import contextlib
import difflib
import hashlib
import re
import sys
from collections import Counter, defaultdict
from pathlib import Path

# Locate the 'propset' module next to this script (like Perl's FindBin)
sys.path.insert(0, str(Path(__file__).resolve().parent))
import propset  # noqa: E402

# Fields this tool owns; all other properties are left alone
FIELDS = ("Cart.Name", "Cart.Manufacturer", "Cart.ModelNo",
          "Cart.Note", "Cart.Rarity")
# Fields that must never be empty
REQUIRED = ("Cart.MD5", "Cart.Name")
# Fields that belong to one label of a ROM sold under several
LABEL_FIELDS = ("Cart.Name", "Cart.Manufacturer", "Cart.ModelNo")

TV = {"PAL", "PAL60", "SECAM", "NTSC"}
STATUS = {"Prototype", "Hack"}
SIZE = re.compile(r"^\d+K$")
DATE = re.compile(r"^((19|20)\d\d|(19|20)\d\d ?- ?(19|20)\d\d|"
                  r"\d\d-\d\d-(\d\d)?\d\d|\d\d-(19|20)\d\d)$")
CONTROLLERS = {"Paddle", "Kid's Controller", "Light Gun", "Keyboard Controller",
               "Joyboard", "Track & Field Controller", "Mindlink Controller",
               "Foot Craz", "Driving Controller", "Video Touch Pad",
               "TRON Joystick", "Booster Grip", "Kid Vid Voice Module",
               "Trak-Ball"}
# Groups before the date that stay in Cart.Name; all others are alternate names
KEEP_PRE = re.compile(r"^(\d+ in 1|\d+ of \d+|Preview|.* Beta|"
                      r"\d+ Game in One|Double-Game Package)$")


# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Filename parsing
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

def split_name(name):
    """Split a name into (title, [paren groups], [bracket groups], tilde)."""
    name = name.rstrip()
    tilde = name.endswith("~")
    if tilde:
        name = name[:-1].rstrip()
    brackets = []
    while name.endswith("]") and "[" in name:
        i = name.rindex("[")
        brackets.insert(0, name[i + 1:-1])
        name = name[:i].rstrip()

    spans, depth, start = [], 0, 0
    for j, c in enumerate(name):
        if c == "(":
            if depth == 0:
                start = j
            depth += 1
        elif c == ")" and depth > 0:
            depth -= 1
            if depth == 0:
                spans.append((start, j))
    title = name[:spans[0][0]].rstrip() if spans else name
    return title, [name[s + 1:e] for s, e in spans], brackets, tilde


def company(manu):
    """'Atari - GCC, Ed Riddle - Sears' -> 'Atari'"""
    return re.split(r" - |, ", manu, maxsplit=1)[0]


def chain(manu):
    """'Atari - GCC, Ed Riddle - Sears' -> 'Atari - GCC'"""
    return manu.split(", ", 1)[0]


def has_digit(s):
    return any(c.isdigit() for c in s)


def parse(name, companies):
    """Parse a RomHunter name into its fields."""
    title, g, brackets, tilde = split_name(name)
    r = {"title": title, "date": "", "manu": "", "num": "", "model": "",
         "status": [], "tv": "", "size": "", "brackets": brackets,
         "tilde": tilde, "aka": [], "alt": [], "keep": [], "ctrl": [],
         "extra": []}

    # Trailing status/TV/size groups
    while g and (g[-1] in TV or g[-1] in STATUS or SIZE.match(g[-1])):
        x = g.pop()
        if x in TV:
            r["tv"] = x
        elif x in STATUS:
            r["status"].insert(0, x)
        else:
            r["size"] = x

    # The manufacturer follows the date; without one, look for a known company
    di = next((i for i, x in enumerate(g[:-1]) if DATE.match(x)), None)
    if di is not None:
        r["date"] = g[di]
        pre, mi = g[:di], di + 1
        # Skip stray groups between date and manufacturer
        moved = []
        while (mi + 1 < len(g) and company(g[mi]) not in companies
               and company(g[mi + 1]) in companies):
            moved.append(g[mi])
            mi += 1
        if moved and all(has_digit(x) for x in moved):
            # A model number placed before the manufacturer
            g = g[:di + 1] + [g[mi]] + moved + g[mi + 1:]
            mi = di + 1
        else:
            pre += moved
    else:
        mi = next((i for i in range(len(g) - 1, -1, -1)
                   if g[i] == "Unknown" or company(g[i]) in companies), None)
        if mi is None and g:
            mi = len(g) - 1
            if mi > 0 and has_digit(g[mi]):
                mi -= 1
        pre = g[:mi] if mi is not None else g

    if mi is not None and mi < len(g):
        r["manu"] = g[mi]
        post = g[mi + 1:]
        # Starpath/Arcadia series number, e.g. '(9) (AR-4105)'
        if len(post) >= 2 and post[0].isdigit():
            r["num"] = post.pop(0)
        if post:
            r["model"] = post.pop(0)
        r["extra"] = post

    for x in pre:
        if x in STATUS:
            r["status"].append(x)
        elif x.startswith("AKA "):
            r["aka"].append(x[4:])
        elif x in CONTROLLERS:
            r["ctrl"].append(x)
        elif KEEP_PRE.match(x):
            r["keep"].append(x)
        else:
            r["alt"].append(x)
    return r


# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Deriving stella.pro fields, using conventions learned from stella.pro
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

class Conventions:
    """What stella.pro does with RomHunter names, learned from its entries."""

    def __init__(self, pro, rom_names):
        self.companies = set()
        for p in pro.values():
            if p["Cart.Manufacturer"]:
                self.companies.add(company(p["Cart.Manufacturer"]))
        for names in rom_names:
            for n in names:
                _, g, _, _ = split_name(n)
                di = next((i for i, x in enumerate(g[:-1]) if DATE.match(x)), None)
                if di is not None and re.search(r"[A-Za-z]{2}", company(g[di + 1])):
                    self.companies.add(company(g[di + 1]))

        self.titles = {split_name(p["Cart.Name"])[0] for p in pro.values()}
        # Series whose subtitles are kept, e.g. 'Star Wars - Jedi Arena'
        self.series = {t.split(" - ")[0] for t in self.titles if " - " in t}

        # Short manufacturer in Cart.Name, by full string, chain and company
        self.short = [defaultdict(Counter) for _ in range(3)]
        for names in rom_names:
            for md5, ns in names.items():
                if len(ns) != 1 or md5 not in pro:
                    continue
                r = parse(ns[0], self.companies)
                if not r["manu"] or r["manu"] == "Unknown":
                    continue
                _, g, _, _ = split_name(pro[md5]["Cart.Name"])
                known = {r["date"], r["tv"], r["size"], *r["status"], *r["keep"]}
                rest = [x for x in g if x not in known]
                if len(rest) == 1:
                    for level, key in zip(self.short, self.short_keys(r["manu"])):
                        level[key][rest[0]] += 1

    @staticmethod
    def short_keys(manu):
        return manu, chain(manu), company(manu)

    def short_name(self, manu):
        """Returns (short name, True if it was learned rather than guessed)."""
        if manu == "Unknown":
            return manu, True
        for level, key in zip(self.short, self.short_keys(manu)):
            if key in level:
                return level[key].most_common(1)[0][0], True
        return company(manu), False

    def title(self, title):
        """Returns (title for Cart.Name, True if a subtitle was guessed away)."""
        if " - " not in title or title in self.titles:
            return title, False
        if title.split(" - ")[0] in self.series:
            return title, True
        return title.split(" - ")[0], True

    def tokens(self, r):
        """Cart.Name as a token list: title, then '(group)' and '[bracket]'."""
        g = list(r["keep"])
        if r["date"]:
            g.append(r["date"])
        if r["manu"]:
            g.append(self.short_name(r["manu"])[0])
        if "Prototype" in r["status"]:
            g.append("Prototype")
        if r["tv"]:
            g.append(r["tv"])
        if "Hack" in r["status"]:
            g.append("Hack")
        if r["size"]:
            g.append(r["size"])
        return ([self.title(r["title"])[0]] + [f"({x})" for x in g]
                + [f"[{x}]" for x in r["brackets"]])

    def derive(self, r):
        rarity = ("Prototype" if "Prototype" in r["status"] else
                  "Hack" if "Hack" in r["status"] else "")
        return {"Cart.Name": " ".join(self.tokens(r)),
                "Cart.Manufacturer": "" if r["manu"] == "Unknown" else r["manu"],
                "Cart.ModelNo": r["model"],
                "Cart.Note": "AKA " + ", ".join(r["aka"]) if r["aka"] else "",
                "Cart.Rarity": rarity}

    def checks(self, r):
        """Things a human should look at in a derived entry."""
        out = []
        title, guessed = self.title(r["title"])
        if guessed:
            out.append(f"subtitle kept, as for other '{title.split(' - ')[0]}' titles"
                       if title == r["title"] else f"subtitle dropped from '{r['title']}'")
        if r["manu"] and not self.short_name(r["manu"])[1]:
            out.append(f"short manufacturer '{self.short_name(r['manu'])[0]}' is a guess")
        if r["alt"]:
            out.append("alternate names: " + "; ".join(r["alt"]))
        if r["ctrl"]:
            out.append("controller: " + ", ".join(r["ctrl"]))
        if r["num"]:
            out.append(f"series number ({r['num']}) not in Cart.ModelNo")
        if r["extra"]:
            out.append("unparsed: " + "; ".join(f"({x})" for x in r["extra"]))
        return out


def name_tokens(name):
    title, g, b, _ = split_name(name)
    return [title] + [f"({x})" for x in g] + [f"[{x}]" for x in b]


def find(seq, sub):
    for i in range(len(seq) - len(sub) + 1):
        if seq[i:i + len(sub)] == sub:
            return i
    return -1


def merge_name(mine, base, theirs, strict=False):
    """Apply the base->theirs Cart.Name change to mine, token by token.

    Returns the merged name, or None if a change can't be placed.  Removing
    tokens mine doesn't have is fine, unless 'strict'.
    """
    out = name_tokens(mine)
    ops = difflib.SequenceMatcher(a=base, b=theirs, autojunk=False).get_opcodes()
    for tag, i1, i2, j1, j2 in ops:
        if tag == "equal":
            continue
        old, new = base[i1:i2], theirs[j1:j2]
        if old:
            at = find(out, old)
            if at < 0:
                if tag == "delete" and not strict:
                    continue
                return None
            out[at:at + len(old)] = new
        else:
            # Insert between the same two tokens as in base, or give up
            before = base[i1 - 1] if i1 > 0 else None
            after = base[i1] if i1 < len(base) else None
            at = out.index(before) + 1 if before in out else 0 if before is None else -1
            if at < 0 or (out[at] if at < len(out) else None) != after:
                return None
            out[at:at] = new
    return " ".join(out)


def merge_field(field, mine, base, theirs):
    """Three-way merge of one field. Returns (value, conflict)."""
    if base == theirs or mine == theirs:
        return mine, False
    if mine == base:
        return theirs, False
    if field == "Cart.Name":
        b, t = name_tokens(base), name_tokens(theirs)
        merged = merge_name(mine, b, t)
        if merged is not None:
            return merged, False
        # The change is already in mine if undoing it works
        return mine, merge_name(mine, t, b, strict=True) is None
    if field == "Cart.Note":
        if mine == "" or (theirs and theirs in mine):
            return mine or theirs, False
        if base and theirs and base in mine:
            return mine.replace(base, theirs), False
    if field == "Cart.Rarity" and mine == "":
        return theirs, False
    return mine, True


# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Matching ROM sets against stella.pro
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

def hash_dir(path):
    """md5 -> sorted list of names (no extension) for every ROM in path."""
    roms = defaultdict(list)
    for f in sorted(Path(path).iterdir()):
        if f.is_file() and f.suffix.lower() not in (".txt", ".nfo"):
            roms[hashlib.md5(f.read_bytes()).hexdigest()].append(f.stem)
    return roms


def pick(names, entry, conv):
    """Pick the name stella.pro's entry was made from; flags a tie."""
    def score(n):
        d = conv.derive(parse(n, conv.companies))
        s = 4 * (d["Cart.Name"] == entry["Cart.Name"])
        s += 2 * (name_tokens(d["Cart.Name"])[0] == name_tokens(entry["Cart.Name"])[0])
        s += d["Cart.Manufacturer"] == entry["Cart.Manufacturer"]
        s += d["Cart.ModelNo"] == entry["Cart.ModelNo"]
        return s
    scores = [score(n) for n in names]
    best = max(scores)
    return names[scores.index(best)], scores.count(best) > 1


def counterpart(base, old_names, new_names):
    """The new name for 'base', or None if that's a choice for a human.

    Only names that weren't in the old set can be a rename of 'base'.  If
    'base' was dropped without one, the ROM's only remaining name is used.
    """
    if base in new_names:
        return base
    fresh = [n for n in new_names if n not in old_names]
    if fresh:
        return max(fresh, key=lambda n: difflib.SequenceMatcher(a=base, b=n).ratio())
    return new_names[0] if len(new_names) == 1 else None


def load_pro(path):
    with contextlib.redirect_stdout(sys.stderr):
        raw = propset.load(path)
    names = propset.prop_names()
    return {md5: dict(zip(names, values)) for md5, values in raw.items()}


def save_pro(path, pro):
    names = propset.prop_names()
    with contextlib.redirect_stdout(sys.stderr):
        propset.save(path, {md5: [p[n] for n in names] for md5, p in pro.items()})


def main(argv):
    ap = argparse.ArgumentParser(
        description="Compare a RomHunter ROM set against stella.pro.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""\
The report goes to stdout.  With --write, clean updates and new entries are
saved to stella.pro (conflicts never are); then run create_props.py.

examples, from the top of the source tree:
  # Report only; stella.pro is not changed
  src/tools/romhunter.py ~/RomHunter_v20 --old ~/RomHunter_v17 > report.txt

  # Apply the report to stella.pro, then regenerate DefProps.hxx
  src/tools/romhunter.py ~/RomHunter_v20 --old ~/RomHunter_v17 --write
  src/tools/create_props.py

  # Same, but also delete entries for ROMs RomHunter dropped
  src/tools/romhunter.py ~/RomHunter_v20 --old ~/RomHunter_v17 --write --remove
""")
    ap.add_argument("new", help="directory with the new RomHunter set")
    ap.add_argument("--old", help="directory with the previous RomHunter set "
                    "(original filenames); enables the three-way merge")
    ap.add_argument("--pro", help="properties file, or a propsdb.py database "
                    "(default: src/emucore/stella.pro)")
    ap.add_argument("--write", action="store_true", help="save changes to the properties file")
    ap.add_argument("--remove", action="store_true",
                    help="with --write, also delete entries for ROMs that are "
                         "in the previous set but not the new one")
    ap.add_argument("--audit", action="store_true",
                    help="also list Manufacturer/ModelNo differences in "
                         "entries RomHunter didn't change")
    if not argv:
        ap.print_help()
        return 0
    args = ap.parse_args(argv)

    pro_path = args.pro or next((p for p in ("src/emucore/stella.pro", "../emucore/stella.pro")
                                 if Path(p).is_file()), "src/emucore/stella.pro")
    pro = load_pro(pro_path)
    new = hash_dir(args.new)
    old = hash_dir(args.old) if args.old else {}
    conv = Conventions(pro, [new, old])

    merged, review, added, audit = [], [], [], []
    unchanged = 0
    for md5, names in sorted(new.items(), key=lambda kv: kv[1][0].casefold()):
        if md5 not in pro:
            n = next((x for x in names if x.rstrip().endswith("~")), names[0])
            r = parse(n, conv.companies)
            checks = conv.checks(r)
            others = [x for x in names if x != n]
            if others:
                checks.append("also named: " + "; ".join(others))
            d = conv.derive(r)
            if not d["Cart.Name"]:
                d["Cart.Name"] = n
                checks.append("no name could be derived; RomHunter's name is used as is")
            added.append((md5, n, r, d, checks))
            continue

        mine = pro[md5]
        if md5 in old:
            base, tie = pick(old[md5], mine, conv)
            theirs = counterpart(base, old[md5], names)
            if theirs is None:
                notes = ["RomHunter dropped this name; pick one of: " + "; ".join(names)]
                merged.append({"md5": md5, "cur": mine["Cart.Name"], "base": base,
                               "theirs": "(dropped)", "changes": [], "clash": [],
                               "notes": notes})
                continue
            b = conv.derive(parse(base, conv.companies))
            t = conv.derive(parse(theirs, conv.companies))
            if base == theirs:
                unchanged += 1
                if args.audit:
                    diffs = [(f, mine[f], t[f]) for f in ("Cart.Manufacturer", "Cart.ModelNo")
                             if mine[f] != t[f]]
                    if diffs:
                        audit.append((md5, mine["Cart.Name"], theirs, diffs))
                continue
            # A switch to another label replaces everything that names the label
            switched = theirs in old[md5]
            changes, clash = [], []
            for f in FIELDS:
                if switched and f in LABEL_FIELDS:
                    value, conflict = t[f], False
                else:
                    value, conflict = merge_field(f, mine[f], b[f], t[f])
                if f in REQUIRED and not value:
                    conflict = True
                if conflict:
                    clash.append((f, mine[f], b[f], t[f]))
                elif value != mine[f]:
                    changes.append((f, mine[f], value))
            notes = ["several names; the match to stella.pro is a guess"] if tie else []
            if switched:
                notes.append("RomHunter dropped this name; switched to the ROM's only remaining one")
            if clash or changes:
                merged.append({"md5": md5, "cur": mine["Cart.Name"], "base": base,
                               "theirs": theirs, "changes": changes, "clash": clash,
                               "notes": notes, "switched": switched, "b": b, "t": t})
            else:
                unchanged += 1
            for f, _, value in changes:
                mine[f] = value
        else:
            n, _ = pick(names, mine, conv)
            d = conv.derive(parse(n, conv.companies))
            diffs = [(f, mine[f], d[f]) for f in FIELDS if mine[f] != d[f]]
            if diffs:
                review.append((md5, mine["Cart.Name"], n, diffs))
            else:
                unchanged += 1

    # Entries for ROMs RomHunter dropped, with the other properties lost with them
    removed = []
    for md5, ns in old.items():
        if md5 not in new and md5 in pro:
            lost = [f for f, v in pro[md5].items()
                    if v and f not in FIELDS and f != "Cart.MD5"]
            removed.append((md5, pro[md5]["Cart.Name"], ns[0], lost))
    gone = {md5 for md5, *_ in removed} if args.remove else set()

    # A rename must never give two ROMs the same name; undo it as a conflict
    def name_users():
        users = defaultdict(set)
        for md5, p in pro.items():
            if md5 not in gone:
                users[p["Cart.Name"]].add(md5)
        return users

    undone = True
    while undone:
        users, undone = name_users(), False
        for item in merged:
            md5 = item["md5"]
            name = pro[md5]["Cart.Name"]
            if len(users[name]) < 2 or not any(f == "Cart.Name" for f, _, _ in item["changes"]):
                continue
            fields = LABEL_FIELDS if item["switched"] else ("Cart.Name",)
            for change in [c for c in item["changes"] if c[0] in fields]:
                f, before, _ = change
                pro[md5][f] = before
                item["changes"].remove(change)
                item["clash"].append((f, before, item["b"][f], item["t"][f]))
            others = users[name] - {md5}
            note = f"not renamed: '{name}' is already the name of {', '.join(sorted(others))}"
            if others <= {m for m, *_ in removed}:
                note += ", which RomHunter dropped; --remove frees the name"
            item["notes"].append(note)
            undone = True

    conflicts = [i for i in merged if i["clash"] or i["theirs"] == "(dropped)"]
    updated = [i for i in merged if i not in conflicts]

    # New entries get a name no other ROM has: keep a dropped subtitle, or else
    # add RomHunter's alternate marker.  Names that collide with nothing are
    # reserved first, so RomHunter's own '[a]' isn't taken by another ROM.
    users = name_users()
    wanted = Counter(d["Cart.Name"] for _, _, _, d, _ in added)
    reserved = {name for name, n in wanted.items() if n == 1 and not users.get(name)}
    def taken(name):
        return bool(users.get(name)) or name in reserved

    for md5, n, r, d, checks in added:
        name = d["Cart.Name"]
        if users.get(name):
            short = conv.title(r["title"])[0]
            options = [r["title"] + name[len(short):]] if short != r["title"] else []
            plain = re.sub(r" \[a\d*\]$", "", name)
            options += [f"{plain} [a]"] + [f"{plain} [a{i}]" for i in range(1, 100)]
            alt = next(o for o in options if not taken(o))
            if alt == options[0] and short != r["title"]:
                checks[:] = [c for c in checks if not c.startswith("subtitle dropped")]
            checks.append(f"'{name}' is already the name of "
                          f"{', '.join(sorted(users[name]))}; named '{alt}' instead")
            d["Cart.Name"] = alt
        users[d["Cart.Name"]].add(md5)

    # - - - - - - - - - - - - - - - - - - report - - - - - - - - - - - - - - -
    out = sys.stdout
    total = sum(len(v) for v in new.values())
    out.write(f"\nRomHunter set: {args.new}  ({total} ROMs, {len(new)} unique)\n")
    if args.old:
        out.write(f"Previous set:  {args.old}  ({sum(len(v) for v in old.values())} ROMs, "
                  f"{len(old)} unique)\n")
    out.write(f"Properties:    {pro_path}\n\n")
    out.write(f"  unchanged  {unchanged:5}\n")
    out.write(f"  updated    {len(updated):5}   applied with --write\n")
    out.write(f"  conflicts  {len(conflicts):5}   conflicting fields never applied, clean ones are\n")
    out.write(f"  new        {len(added):5}   applied with --write\n")
    out.write(f"  review     {len(review):5}   in stella.pro, not in the previous set; never applied\n")
    if args.old:
        out.write(f"  removed    {len(removed):5}   in the previous set only; deleted with --write --remove\n")

    def q(s):
        return f'"{s}"'

    def section(title, items):
        out.write(f"\n{'=' * 78}\n{title} ({len(items)})\n{'=' * 78}\n")

    def merged_item(item):
        out.write(f"\n{item['md5']}  {item['cur']}\n  was: {item['base']}\n  now: {item['theirs']}\n")
        for f, a, b in item["changes"]:
            out.write(f"  {f:18} {q(a)}\n  {'->':>18} {q(b)}\n")
        for f, m, b, t in item["clash"]:
            out.write(f"  {f:18} stella.pro: {q(m)}\n"
                      f"  {'':18} old name:   {q(b)}\n"
                      f"  {'':18} new name:   {q(t)}\n")
        for n in item["notes"]:
            out.write(f"  check: {n}\n")

    section("UPDATED", updated)
    for item in updated:
        merged_item(item)

    section("CONFLICTS (fields shown with three values are not applied)", conflicts)
    for item in conflicts:
        merged_item(item)

    section("NEW", added)
    for md5, n, _, d, checks in added:
        out.write(f"\n{n}\n")
        out.write(f'  "Cart.MD5" "{md5}"\n')
        for f in FIELDS:
            if d[f]:
                out.write(f"  {q(f)} {q(d[f])}\n")
        for c in checks:
            out.write(f"  check: {c}\n")

    section("REVIEW (not applied)", review)
    for md5, cur, n, diffs in review:
        out.write(f"\n{md5}  {cur}\n  name: {n}\n")
        for f, a, b in diffs:
            out.write(f"  {f:18} stella.pro: {q(a)}\n  {'':18} RomHunter:  {q(b)}\n")

    if args.old:
        section("REMOVED (deleted with --write --remove)", removed)
        for md5, cur, n, lost in sorted(removed, key=lambda x: x[2].casefold()):
            out.write(f"\n{md5}  {cur}\n  was: {n}\n")
            if lost:
                out.write(f"  also deletes: {', '.join(lost)}\n")

    if args.audit:
        section("AUDIT (RomHunter unchanged, stella.pro differs)", audit)
        for md5, cur, n, diffs in audit:
            out.write(f"\n{md5}  {cur}\n  name: {n}\n")
            for f, a, b in diffs:
                out.write(f"  {f:18} stella.pro: {q(a)}\n  {'':18} RomHunter:  {q(b)}\n")

    if args.write:
        empty = {f: "" for f in propset.prop_names()}
        for md5, _, _, d, _ in added:
            pro[md5] = {**empty, **d, "Cart.MD5": md5}
        if args.remove:
            for md5, _, _, _ in removed:
                del pro[md5]
        # Last line of defence: never save an entry missing a required field
        bad = [md5 for md5, p in pro.items() if not all(p[f] for f in REQUIRED)]
        if bad:
            print(f"\nERROR: not saving, {len(bad)} entries would lack "
                  f"{' or '.join(REQUIRED)}: {', '.join(bad)}", file=sys.stderr)
            return 1
        print(file=sys.stderr)
        try:
            save_pro(pro_path, pro)
        except ValueError as err:
            print(f"\nERROR: not saving, {err}", file=sys.stderr)
            return 1
        print("Now run " + ("propsdb.py export, then " if propset.is_db(pro_path) else "")
              + "create_props.py to regenerate DefProps.hxx", file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
