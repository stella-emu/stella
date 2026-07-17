# Writing a Dialog with the Layout Engine

This document explains how to add a new dialog (or convert an existing one) to
Stella's GUI using the relative **Layout** engine in `src/gui/Layout.{hxx,cxx}`.
It is aimed at developers who have never touched the GUI code before.

If you only read one section, read [The pattern in one
picture](#the-pattern-in-one-picture) and [Step by step](#step-by-step).

---

## Read this before you write any code

Every dialog in `src/gui/` is already converted, so there is a precedent for
whatever you are about to do. Find it and follow it. These five rules are what the
rest of this document exists to explain:

1. **Read the exemplar.** Open the nearest already-converted file and follow its
   *shape* before designing anything. A summary of the engine gives you the
   vocabulary, not the design.
2. **No new seam without asking.** Don't invent a virtual, protocol, collector or
   helper that has no counterpart in `src/gui/`. If the existing seams can't express
   what you need, that is a question to raise, not a design to make.
3. **State intent, not pixels.** No pixel arithmetic, no `setPos()` after a layout,
   no hand-summed widths, no measured specimen strings
   (`getStringWidth("Manufacturer ")`), no labels padded with spaces to fake
   alignment. Extend the engine rather than working around it.
4. **One `align*` call per group**, made by the code that *builds* the group
   (`alignLabels` / `alignPopUps` / `alignButtons` / `alignTracks`). Size cells with
   `addAuto()`.
5. **Widgets own their size.** A constructor never bakes geometry: if a widget can
   derive it (button, pop-up, edit field, slider, static text), don't pass it. All
   geometry lives in `layout()` / `reflow()`.

---

## Why this exists

Historically every dialog hard-coded pixel positions in its **constructor**:

```cpp
// OLD STYLE — don't do this in new dialogs
xpos = 10;  ypos = 20;
myWidget = new PopUpWidget(this, font, xpos, ypos, 120, lineHeight, ...);
ypos += lineHeight + 4;
myOtherWidget = new CheckboxWidget(this, font, xpos, ypos, "Enable");
...
```

That has three problems:

1. **Font changes don't reflow.** Stella lets the user pick the dialog font
   (Small … Large) and even change it *live* while the app runs. Positions baked
   from one font are wrong for another.
2. **Window resizing doesn't reflow.** Resizable dialogs (the launcher, the
   debugger) need to re-place their contents when the window changes size.
3. **It's error-prone.** Manual `xpos += …` arithmetic drifts and is painful to
   review.

The layout engine solves all three with **one** rule:

> **The constructor only *creates* widgets. A `layout()` method *positions and
> sizes* them, computing every number from the current font each time it runs.**

Window resize and font change (including live re-font) then all funnel through
the same `layout()` call. You write the geometry once.

And it aims at **one goal**, which is worth stating before any of the API:

> **Layout code describes *intent*. The pixel positions are details, and they are
> deliberately hidden from you at this level.**

So a pixel computation in a dialog is a smell, and almost always means some
knowledge belongs one level down — in the engine, or in the widget itself:

| you were about to write | say this instead |
| ----------------------- | ---------------- |
| a row height (`lineHeight`, `buttonHeight`) | `addAuto` — the widget knows its own size |
| a dialog's height, summed over its rows | ask the layout: `naturalSize()` |
| `+2` to line up some text | nothing — widgets centre their own text (`firstTextY`) |
| a column that matches a sibling's `x` | a shared `GridLayout` track (`columnAuto`) |
| arithmetic keeping two groups in step | a stretch that absorbs the slack |
| a button's width, from its label | nothing — the button sizes itself; a *group* of them, `GUI::alignButtons` |
| a button's width, from its icon | nothing — pass it the icon; `setIcon()` re-sizes it too |
| a pop-up's width, from its items | nothing — pass it the items and let it size itself |
| a width for a caption that shows *loaded* text | nothing — **fill it**; the layout owns its width (see below) |

`addFixed` is still right where the number is genuinely the *dialog's* decision —
a minimum width for the whole dialog, how many characters of a path a field must
show. The test is simply: **whose choice is this number?** If it is the widget's
or the font's, do not write it down.

The same test applies to what you hand a widget's **constructor**. A widget that
can work a size out for itself should be left to: a `ButtonWidget` given only its
label sizes itself to it, and a `PopUpWidget` given only its item list sizes its
value box to the widest of them (and both take their height from the font). You
pass a size only when the widget genuinely cannot know it — because the label or
the item list is **not final** (the command menu re-labels its buttons; the game
properties pop-ups are refilled per ROM), in which case the dialog must say how
wide a label or entry it is prepared to show, or the control would resize under
the user. `ButtonWidget::calcWidth()` and `PopUpWidget::calcWidth()` exist for
exactly that, and for nothing else.

If the engine cannot express your intent, **extend the engine** rather than
working around it in the dialog. `VAlign::Baseline`, `addAuto` and
`SizePolicy::Auto` all arrived exactly that way.

---

## The pattern in one picture

```
  class MyDialog : public Dialog
  ┌───────────────────────────────────────────────────────────────┐
  │  MyDialog(...)                 ← CREATE widgets at (0,0) only   │
  │  {                                                              │
  │     myFoo = new PopUpWidget(this, font, 0,0, w, h, ...);        │
  │     myBar = new CheckboxWidget(this, font, 0,0, "Bar");         │
  │     addToFocusList(wid);                                        │
  │     addDefaultsOKCancelBGroup(wid, font);                       │
  │  }                                                              │
  │                                                                 │
  │  void layout() override       ← POSITION + SIZE everything     │
  │  {                                                              │
  │     _w = ...; _h = ...;        // from current font metrics     │
  │     auto root = make_unique<BoxLayout>(Dir::Vertical, ...);     │
  │     root->addFixed(anchoredItem(myFoo), lineHeight);            │
  │     root->addFixed(anchoredItem(myBar), lineHeight);            │
  │     root->doLayout(0, _th, _w, _h - _th);                      │
  │     layoutButtonGroup();       // the bottom OK/Cancel row      │
  │  }                                                              │
  └───────────────────────────────────────────────────────────────┘
```

`layout()` may be called any number of times; `MyDialog(...)` runs once.

---

## The Layout engine

A `Layout` is a cheap, throwaway description of how a rectangle is subdivided.
You build a small tree of them each time `layout()` runs, call `doLayout(x, y,
w, h)` on the root, and the tree assigns absolute geometry to every widget it
contains. Then you throw the tree away.

There are two container primitives and one leaf.

### `BoxLayout` — a row or a column

Stacks children along one axis. Along the **main** axis each child is sized by
its policy; along the **cross** axis every child fills the box (minus margins).

```cpp
BoxLayout(Dir dir, int spacing = 0, int marginH = 0, int marginV = 0);
```

| Method                              | Meaning                                             |
| ----------------------------------- | --------------------------------------------------- |
| `addAuto(child)`                    | child gets what it *wants* along the main axis — **prefer this** |
| `addFixed(child, px)`               | child gets exactly `px` along the main axis         |
| `addStretch(child, weight=1, base=0)` | child gets `base`, then shares leftover (by weight) |
| `addPercent(child, pct, max=0)`     | child gets `pct`% of the available length           |
| `addSpace(px)`                      | an empty gap of `px`                                |
| `addStretchSpace(weight=1, base=0)` | an empty gap of at least `base`, which also grows   |

`addAuto` sizes the cell from `Widget::naturalSize()`, so a row is exactly as
tall as its tallest widget and you state no height at all. Reach for it before
`addFixed(item, lineHeight)`: a `lineHeight` cell is 2px too short for anything
that frames its text (edit field, pop-up) and far too short for a button, and it
stops following the font the moment the row's contents change. Use `addFixed`
only where the size is genuinely the *dialog's* choice (a shared button width, a
column width several rows must agree on), and `addStretch` for a widget with no
size of its own — a list, an image.

The `base` of a stretch cell is its **natural size**: it is reserved before any
leftover is shared out. That is what lets several cells grow from *different*
natural sizes in a chosen proportion, without computing the leftover yourself.
Two columns and the gap between them, growing 1 : 2 : 1 from their own minima:

```cpp
  root.addStretch(std::move(left), 1, leftNaturalW);
  root.addStretchSpace(2, minGap);
  root.addStretch(std::move(right), 1, rightNaturalW);
```

```
  BoxLayout(Dir::Vertical)                BoxLayout(Dir::Horizontal)
  ┌───────────────────────┐               ┌──────┬────────────┬──────┐
  │ addFixed(A, lineH)    │  A            │ Fixed│  Stretch   │Fixed │
  ├───────────────────────┤               │  A   │     B      │  C   │
  │ addSpace(VGAP)        │               └──────┴────────────┴──────┘
  ├───────────────────────┤                addFixed(A) addStretch(B) addFixed(C)
  │ addFixed(B, lineH)    │  B
  ├───────────────────────┤               B expands to fill whatever A and C
  │ addStretch(C)         │  C (fills)    leave over.
  └───────────────────────┘
```

A vertical box whose rows are each an inner **horizontal** box is the workhorse
for form dialogs (label + control per row).

### `GridLayout` — a real table

Use this **only** for genuine tables where cells must line up across both rows
*and* columns (e.g. `HighScoresDialog`). Do **not** use it for ordinary forms —
self-labeling popups/sliders already align themselves (see below), so a vertical
`BoxLayout` is simpler.

```cpp
GridLayout(int cols, int rows, int hSpacing=0, int vSpacing=0,
           int marginH=0, int marginV=0);

grid.columnFixed(0, labelW).columnStretch(1);   // size the tracks
grid.rowFixed(0, lineHeight). ... ;
grid.place(col, row, widgetItem(w), colspan, rowspan);
```

```
        col 0 (Fixed)   col 1 (Stretch)
      ┌───────────────┬────────────────────┐
 row0 │ Name          │ [ editable value ] │
      ├───────────────┼────────────────────┤
 row1 │ Score         │ [ editable value ] │
      ├───────────────┼────────────────────┤
 row2 │ Date          │ [ editable value ] │
      └───────────────┴────────────────────┘
   Every cell in col 0 shares col 0's width → labels line up.
   Every cell in col 1 shares col 1's width → values line up.
```

### `SizePolicy`

Both containers size tracks/children with the same three policies:

| Policy    | `value` means…                                    |
| --------- | ------------------------------------------------- |
| `Fixed`   | a fixed number of pixels (usually font-derived)   |
| `Percent` | a percentage 0..100 of the available length       |
| `Stretch` | a weight; shares the leftover length in proportion |

### Leaf items — how a single widget sits in its cell

You never construct `WidgetLayout` directly; use these `GUI::` helper builders:

| Helper                                   | Behaviour                                                                 |
| ---------------------------------------- | ------------------------------------------------------------------------- |
| `widgetItem(w, minW=0, minH=0)`          | **fills** the cell in both axes — stretches the widget to the cell size    |
| `stretchedItem(w, minW=0)`               | fills the cell's **width**, keeps its own **height**, vertically centered  |
| `anchoredItem(w, minW=0, minH=0)`        | the widget's **natural** size, left of the cell and vertically **centered** |
| `centeredItem(w, minW=0, minH=0)`        | the widget's **natural** size, **centered** on both axes — a heading over its content, or a symmetric grid cell (a joystick cross) |
| `alignedItem(w, hAlign, vAlign, …)`      | any other combination — the general form the four above are shorthand for |
| `indentedItem(w, indent, minW=0)`        | natural size, positioned `indent` px from the left                        |
| `indentedFill(w, indent, width=0)`       | indented and **filling** — an indented field/list/pop-up. Give a `width` to end flush with a **sibling** rather than with the row (see the Fill rule) |
| `labeledRow(label, control, labelW=0, indent=0, fill=false)` | a row pairing a **separate** label with a control; `fill=true` stretches the control to the rest of the row (edits/lists) instead of keeping its natural width |

> ⚠ **`widgetItem` vs `stretchedItem` — the one trap in the engine.** A *filled*
> axis reports **no natural size** (the cell decides it, so reporting back would
> feed the cell its own answer). That is right for a list or an image, which have
> no size of their own. It is wrong for a **button, field or checkbox**, which do:
> put one in a cell sized by `addAuto`/`rowAuto` and the cell takes its height
> from whatever *else* shares the row, squashing it (a 25px button in a row with a
> 22px edit field comes out 22px, and a row of nothing but filled items collapses
> to zero). Reach for `stretchedItem` whenever the **width** is the layout's to
> decide but the **height** is the widget's — which is almost every control.
> Keep `widgetItem` for content that genuinely has no size of its own.

> ⚠ **Fill means "end flush with your CELL" — so make the cell the thing you are
> lining up with.** This is the trap on the other side of the same coin, and it is
> subtle because it *looks* right until the container grows.
>
> A control that must end flush with a **sibling** — a pop-up above it, a list
> below it — cannot say so by stretching into whatever container it happens to be
> in. Stretch it and it will chase the container's edge, and the container is
> usually wider (a tab is as wide as the *widest* tab in the dialog, not as wide as
> its own content). The control then sails past the thing it was supposed to meet.
>
> Say it by giving it a cell that **is** that width:
>
> ```cpp
> // the filter pop-up ends flush with the list below it (EventMappingWidget)
> row->addFixed(stretchedItem(myFilterPopup), listArea);
>
> // NOT this: it would run out to the container's edge instead
> row->addStretch(stretchedItem(myFilterPopup));
> ```
>
> Filling is right when the cell genuinely *is* the anchor — a progress bar across
> the dialog, a path field to the right border, a pop-up to the tab edge. The test
> is the usual one: **what is it ending flush with?** If the answer is another
> widget, name that widget; if it is the edge you are already in, fill.
>
> For **sliders** this case has its own helper, because what must line up is the
> *track inside* the widget rather than its edge — see `GUI::alignTracks`.

Alignment is per axis, and it is *all* you state:

```cpp
enum class HAlign { Fill, Left, Center, Right };
enum class VAlign { Fill, Top, Center, Baseline, Bottom };
```

**Why you never nudge text by hand.** Every widget centers its own text within
its own height (`Widget::firstTextY()`), and the layout centers each widget in
its cell. Center two widgets in one row and their *text* lines up — whatever
their heights, and whichever of them frames its text (an edit field and a pop-up
are 2px taller than a label; a button is taller still). There is no anchor to
pick and nothing to get wrong. If a row looks 1px out, the fix is in the widget's
`firstTextY()`, never a `+2` in the dialog.

`VAlign::Baseline` is the one case centering cannot express: a `DataGridWidget`
or `ToggleWidget` is *several* rows of text in one box, and a label beside it
must sit on the grid's **first** row, not the middle of the whole grid. Mark both
items `Baseline` and the row puts the label's text on the grid's first line.
(This only works within a horizontal `BoxLayout`. A vertical *stack* of labels
beside a grid is one item to the row, so it cannot use it — start the column with
`addSpace(grid->firstTextY() - label->firstTextY())` instead; `CpuWidget` and
`RamWidget` do exactly this.)

Rules of thumb:

- **Self-labeling widgets** (`PopUpWidget`, `SliderWidget` — they draw their own
  label) → use `anchoredItem`. Filling them would stretch the *track/value box*
  and look wrong. Because they share a common label width, they line up across
  rows without a grid.
- **A control that should span the row** (an `EditTextWidget`) → `stretchedItem`
  in a row sized with `addAuto`: it widens with the dialog but keeps the height
  it built itself from the font. Only a **list** or an image — which have no
  height of their own — wants `widgetItem` and a stretching row.
- **A checkbox indented under a group header** → `indentedItem(cb, indent())`.
- **A separate label + control** (label is its own `StaticTextWidget`, control is
  not self-labeling) → `labeledRow(label, control, sharedLabelW)`.
- **Right-align a column of value fields** whose widths differ (say a 3-digit and
  an 8-digit field, which should still end flush) → do *not* compute per-row
  label widths. Stretch the label and fix the field:
  `row.addStretch(anchoredItem(label)); row.addFixed(anchoredItem(field), fieldW);`
  Every row then ends at the column's right edge, whatever its label or digits.
- **A standalone button centered on its row** (an OK/Close on its own) → a
  horizontal box of `addStretchSpace()` · `addAuto(anchoredItem(btn))` ·
  `addStretchSpace()`. The button already knows how wide its label needs it to be
  (see below), so the row states nothing.
- **A group of buttons that must be the same width** (a column of them, an
  OK/Cancel pair, a grid of option buttons) → `GUI::alignButtons({a, b, c})` and
  then `addAuto`. What a button *cannot* know is what its **neighbours'** labels
  need, and that is the only thing about a button a layout has to say. Pass
  `dialog().standardButtonWidth()` as the optional minimum for a group that should
  match the dialog's own buttons rather than shrink-wrap to its labels.
- **Sliders that must end flush with the pop-up above them** →
  `GUI::alignTracks({s1, s2, …}, popup)` (add the indent as a third argument if the
  sliders sit a level in from it). A slider is a *label + track + readout* drawn in
  one rect, and what lines up down a form is the **track inside it** — which is
  below the level a layout can see, since a layout only sizes whole widgets. The
  slider knows what its own label and readout take; what it cannot know is the box
  it should span. Never compute a track width in a dialog.
  For two sliders **sharing one row** (a value and its offset, side by side), the
  other form tiles their tracks across a span:
  `GUI::alignTracks({scale, shift}, span, gap)`.
- **How many rows should this list show?** → `ListWidget::calcHeight(font, rows)`
  as the item's `minH`, the same way `EditTextWidget::calcWidth(font, chars)`
  says how wide a field must be. Say what you mean in rows and characters; do not
  arrive at a pixel size and hope the rows come out even.

`nullptr` as the widget makes any item an empty spacer that only reserves space
(this is exactly what `addSpace` does internally).

---

## Dialog metric helpers — never hard-code pixels

`Dialog` exposes font-derived metric helpers. **Use these instead of magic
numbers** so everything scales with the font:

| Helper                | Value                                    | Use for                      |
| --------------------- | ---------------------------------------- | ---------------------------- |
| `lineHeight()`        | `font.getLineHeight()`                   | row pitch for text/controls  |
| `fontHeight()`        | `font.getFontHeight()`                   |                              |
| `fontWidth()`         | `font.getMaxCharWidth()`                 | horizontal spacing units     |
| `buttonHeight()`      | `lineHeight() * 1.25`                     | button/edit row height       |
| `buttonWidth(label)`  | `getStringWidth(label) + fontWidth()*2.5`| a button sized to its text   |
| `buttonGap()`         | `fontWidth()`                            | gap between buttons          |
| `hBorder()`           | `fontWidth() * 1.25`                      | left/right dialog margin     |
| `vBorder()`           | `fontHeight() / 2`                       | top/bottom dialog margin     |
| `vGap()`              | `fontHeight() / 4`                       | small vertical gaps          |
| `indent()`            | `fontWidth() * 2`                        | indentation step             |

Convention at the top of most `layout()` bodies:

```cpp
const int lineHeight   = Dialog::lineHeight(),
          fontWidth    = Dialog::fontWidth(),
          buttonHeight = Dialog::buttonHeight(),
          VBORDER      = Dialog::vBorder(),
          HBORDER      = Dialog::hBorder(),
          VGAP         = Dialog::vGap();
```

`_th` is the title-bar height (set by `setTitle`). Content starts at `y = _th`,
so the typical root call is `root->doLayout(0, _th, _w, _h - _th)`.

---

## The bottom button group

Never hand-place OK/Cancel, and **never** hard-code the platform button order
(`#ifdef BSPF_MACOS` etc.) in a dialog — the base class already encapsulates it.

1. In the **ctor**, create the standard group:

   ```cpp
   addDefaultsOKCancelBGroup(wid, font);        // Defaults / OK / Cancel
   // or: addOKCancelBGroup(wid, font);          // OK / Cancel
   // or: addOKBGroup(wid, font);                // single OK (centered)
   ```

   These create the buttons at a placeholder position and register them as
   `_defaultWidget` / `_okWidget` / `_cancelWidget`.

2. In **`layout()`**, after laying out the content, call:

   ```cpp
   layoutButtonGroup();
   ```

   It positions Defaults (and optional Extra) at the bottom-left and OK/Cancel
   at the bottom-right, in the correct platform order, for the current `_w`/`_h`.

```
  ┌───────────────────────────────────────────────┐
  │  ... dialog content ...                        │
  │                                                │
  ├───────────────────────────────────────────────┤
  │ [Defaults]                        [OK][Cancel] │  ← layoutButtonGroup()
  └───────────────────────────────────────────────┘
```

**Custom left-hand buttons** (e.g. a "Save log to disk" button): register your
button with `addDefaultWidget(btn)` / `addExtraWidget(btn)` and `layoutButtonGroup()`
places it on the left. **Nav buttons** (Prev/Next, Go-up/Home) that sit *beside*
OK/Cancel go in their own left-aligned `BoxLayout` HBox positioned at the same
bottom `y`, letting `layoutButtonGroup()` handle the right side.

**Single centered OK** (from `addOKBGroup`): the group helper right-aligns, so
center it yourself with the engine — a horizontal box of `addStretchSpace()` ·
`addFixed(widgetItem(_okWidget), Dialog::buttonWidth("OK"))` · `addStretchSpace()`.

### The title-bar "?" help button

Call `setHelpAnchor("YourAnchor")` in the ctor. The base class creates the "?"
button and **auto-positions it** after every `layout()` via `layoutHelp()` — do
**not** place it yourself.

---

## Step by step

### 1. Header (`MyDialog.hxx`)

```cpp
class MyDialog : public Dialog
{
  public:
    MyDialog(OSystem& osystem, DialogContainer& parent, const GUI::Font& font);
    ~MyDialog() override = default;

    void loadConfig() override;
    void saveConfig() override;

  protected:
    void handleCommand(CommandSender* sender, int cmd, int data, int id) override;
    void layout() override;                 // no doc comment — base documents it

  private:
    // Promote to members ANY widget layout() must position.
    PopUpWidget*   myMode{nullptr};
    CheckboxWidget* myEnable{nullptr};
    // ... deleted copy/move ctors ...
};
```

- Put `void layout() override;` in the **protected** section, with **no** doc
  comment (the base `Dialog::layout()` already documents the contract).
- Any widget that `layout()` needs to touch must be a **member** — promote
  anonymous locals from the old ctor.

### 2. Constructor — create only

```cpp
MyDialog::MyDialog(OSystem& osystem, DialogContainer& parent, const GUI::Font& font)
  : Dialog(osystem, parent, font, "My Dialog")   // title sets _th
{
  const int lineHeight = Dialog::lineHeight();
  WidgetArray wid;

  // Create every widget at placeholder (0,0), with its real font-derived WIDTH.
  myMode = new PopUpWidget(this, font, 0, 0, popupWidth, lineHeight, items, "Mode ");
  wid.push_back(myMode);

  myEnable = new CheckboxWidget(this, font, 0, 0, "Enable feature");
  wid.push_back(myEnable);

  addDefaultsOKCancelBGroup(wid, font);   // creates the button group
  addToFocusList(wid);                     // tab order
  setHelpAnchor("MyDialog");               // enables the "?" button
}
```

> ⚠ **A widget's ctor width is its *natural* width.** A widget built at a
> placeholder width (`, 1,`) reports **1px**, so anything that reads its natural
> size — `anchoredItem`, `addAuto`, `columnAuto` — will size a cell or a whole
> column to 1px and clip it away. Only a *filled* axis (`widgetItem`,
> `stretchedItem`) overrides that width, which is why a placeholder is safe there
> and nowhere else. So: **a label or a text field the layout does not stretch must
> be built with its real, font-derived width** — for a `StaticTextWidget` that
> means the short ctor (which derives it from the text), and for an
> `EditTextWidget`, `EditTextWidget::calcWidth(font, chars)`. Only the widget's
> *position*, and any size the layout will assign, belong in `layout()`.
>
> This misbehaves *intermittently*, which makes it nasty: `refreshFontMetrics()`
> recomputes a `StaticTextWidget`'s width from its label, so a clipped label
> springs back into view after any live font change and looks fine — until the
> dialog is next opened from scratch.

What stays in the ctor: `VarList` items, tooltips, `setHelpAnchor`, enable/disable,
command IDs, focus registration. What does **not**: any `xpos`/`ypos` arithmetic,
any `setSize`, any use of `_w`/`_h` (they are still 0 here).

**clang-tidy — suppress `prefer-member-initializer`, don't obey it.** Creating the
widgets in the ctor body (rather than the member-initializer list) makes
`cppcoreguidelines-prefer-member-initializer` flag every widget whose arguments are
literals/params/members only (a widget sized from a body-local like `lineHeight`
isn't flagged). **Do not hoist them into the initializer list.** That is wrong for
this pattern: it scatters the labels away from the size-bearing siblings that *must*
stay in the body, and it silently reorders widget registration — the init list runs
before the body, in header-declaration order, so draw/focus order would change.
Bracket the create-only block with a `NOLINTBEGIN`/`NOLINTEND` pair instead (place
`NOLINTBEGIN` right after the create-only comment and `NOLINTEND` before the
constructor's closing brace):

```cpp
  // NOLINTBEGIN(cppcoreguidelines-prefer-member-initializer)
  myMode = new PopUpWidget(this, font, 0, 0, popupWidth, lineHeight, items, "Mode ");
  wid.push_back(myMode);
  myEnable = new CheckboxWidget(this, font, 0, 0, "Enable feature");
  wid.push_back(myEnable);
  // NOLINTEND(cppcoreguidelines-prefer-member-initializer)
```

(For a lone flagged line — e.g. one whose value depends on a body-local so it can't
be a member initializer anyway — a single `// NOLINTNEXTLINE(...)` above it is
enough.)

### 3. `layout()` — size and position

```cpp
void MyDialog::layout()
{
  using GUI::BoxLayout;
  using GUI::anchoredItem;
  using Dir = BoxLayout::Dir;

  const int lineHeight   = Dialog::lineHeight(),
            buttonHeight = Dialog::buttonHeight(),
            VBORDER      = Dialog::vBorder(),
            HBORDER      = Dialog::hBorder(),
            VGAP         = Dialog::vGap();

  // 1. Size the dialog from the current font.
  _w = 40 * Dialog::fontWidth() + HBORDER * 2;
  _h = _th + VBORDER * 2 + 2 * (lineHeight + VGAP) + buttonHeight;

  // 2. Build a throwaway layout tree and apply it below the title bar.
  auto root = std::make_unique<BoxLayout>(Dir::Vertical, VGAP, HBORDER, VBORDER);
  root->addFixed(anchoredItem(myMode),   lineHeight);
  root->addFixed(anchoredItem(myEnable), lineHeight);
  root->doLayout(0, _th, _w, _h - _th);

  // 3. Standard bottom button row.
  layoutButtonGroup();
}
```

- Move the old `setSize(...)` formula here as direct `_w = …; _h = …;`. Drop
  `setSize` for a fixed-size dialog. (Fill-available/resizable dialogs are the
  exception — see below.)
- Content sits below the title bar → `doLayout(0, _th, _w, _h - _th)`.

### 4. Add to the build

Add `MyDialog.cxx` to `src/gui/module.mk` (follow the existing entries), and
`#include "Layout.hxx"` in the `.cxx`.

---

## Choosing an engine shape

**Before you design anything, open the converted dialog closest to yours and copy
its shape.** These are the worked examples, and reading one will save you
re-deriving (badly) a rule that is already settled:

| Your dialog is…                                       | Use                                                                   | Read first          |
| ----------------------------------------------------- | --------------------------------------------------------------------- | ------------------- |
| A vertical **form** of self-labeling popups/sliders   | `BoxLayout(Vertical)` of `anchoredItem`s                              | `EmulationDialog`   |
| A form with **separate** label + control rows         | `BoxLayout(Vertical)` of `labeledRow(...)`, columns via `GUI::alignLabels` | `GlobalPropsDialog` |
| A genuine **table** (cells align in 2-D)              | `GridLayout`                                                          | `HighScoresDialog`  |
| A grid of **buttons** (OptionsDialog, CommandDialog)  | `GridLayout` with `widgetItem` cells                                  | `OptionsDialog`     |
| **Tabbed**                                            | `TabWidget` + a `TabPaneWidget` per tab (see below)                   | `InputDialog`       |

**Everything goes through the engine — there is no "irregular" escape hatch.** A
`for(...) w->setPos(...)` loop over several widgets defeats the whole exercise,
and so does reading back a resolved position to nudge the next widget by hand. If
the engine cannot express what you want, **extend the engine**: an effect that
looks like arithmetic is usually a layout policy in disguise ("these two end
flush" is a right-align; "the extra width splits 1:2:1" is a weighted stretch;
"this group is centred" is a stretch either side of it).

If you write a row/column lambda inside `layout()` that other dialogs could
reuse (a label+control row, a centered item, …), promote it to a `GUI::` helper
in `Layout.{hxx,cxx}` rather than leaving it local. Keep dialog-*specific*
composition local.

---

## Tabbed dialogs

`TabWidget` is a **container**: it owns laying out the content of its tabs. The
dialog sizes the tab widget and the content follows — **a tabbed dialog writes no
per-tab layout or resize code at all.**

Every tab's content is **one widget**. There are two kinds, and picking the right
one is the whole trick:

| Your tab holds… | Use | Register with |
|---|---|---|
| loose controls the dialog owns (the usual form of checkboxes, popups, sliders) | a **`TabPaneWidget`** — a transparent container you parent the controls to, plus a layout builder | `myTab->setPaneWidget(tabID, pane)` |
| a self-contained widget that already owns its controls (`EventMappingWidget`, the debugger's `TiaWidget`/`RiotWidget`/`Cart*Widget`) | **that widget itself** — it *is* the content | `myTab->setParentWidget(tabID, widget)` |

```
  ┌─ General ─┬─ Advanced ─┬─ Mapping ─┐   ← tab bar (updateTabSizes())
  │           └────────────┴───────────┴──┐
  │  Mode:      [ popup               ▼]  │  ← "General" = loose controls, so
  │  [x] Enable feature                   │    they are parented to a
  │  Speed:     [======O------]           │    TabPaneWidget and described in
  │                                       │    its setLayout() builder
  ├───────────────────────────────────────┤
  │ [Defaults]                 [OK][Cancel]
  └───────────────────────────────────────┘

  ┌─ General ─┬─ Advanced ─┬─ Mapping ─┐
  │                        └───────────┴──┐
  │  ┌─────────────────────────────────┐  │  ← "Mapping" = one self-contained
  │  │                                 │  │    widget (EventMappingWidget).
  │  │   EventMappingWidget            │  │    It IS the content: no pane, and
  │  │   (owns its list, buttons, …)   │  │    it reflows in its own setArea()
  │  └─────────────────────────────────┘  │
  ├───────────────────────────────────────┤
  │ [Defaults]                 [OK][Cancel]
  └───────────────────────────────────────┘
```

A self-contained widget needs **no pane** — it already is the thing the tab holds.
Do not wrap it in one, and do not derive it from `TabPaneWidget`: **a widget must
never know whether its parent is a tab.** It takes a plain `GuiObject*` boss like
any other widget, and reflows itself in its own `setArea()` override. The *dialog*
does the tab wiring.

### A tab of loose controls — `TabPaneWidget`

```cpp
// ctor: create the tab widget at a PLACEHOLDER size
myTab = new TabWidget(this, font, 0, 0, 1, 1);
addTabWidget(myTab);

const int tabID = myTab->addTab(" General ", TabWidget::AUTO_WIDTH);
auto* pane = new TabPaneWidget(myTab, _font);
myTab->setPaneWidget(tabID, pane);

// this tab's controls are parented to the PANE, not to myTab
myMode  = new PopUpWidget(pane, _font, 0, 0, pwidth, lineHeight, items, "Mode ", lwidth);
mySpeed = new SliderWidget(pane, _font, 0, 0, swidth, lineHeight, "Speed", lwidth);
addToFocusList(wid, myTab, tabID);
pane->setHelpAnchor("General");

// describe the layout ONCE; the pane re-runs it on every resize/font change.
// The box is vertical and already inset by the standard borders
pane->setLayout([this](GUI::BoxLayout& col) {
  const int lh = Dialog::lineHeight(), VGAP = Dialog::vGap();

  col.addFixed(GUI::anchoredItem(myMode), lh);
  col.addSpace(VGAP);
  col.addFixed(GUI::anchoredItem(mySpeed), lh);
});
```

`setLayout()` also takes an optional second `postLayout` callback, for the rare
overlay/cross-referencing widget the box cannot express. Reach for it last: a
right-hand group with its own vertical rhythm is usually just **two parallel
columns** — an `addStretch`ed left column beside a fixed-width right one (see
`InputDialog`'s "Devices & Ports" tab).

### `layout()` — no per-tab code

```cpp
void MyTabbedDialog::layout()
{
  _w = ...; _h = ...;

  // Size the tab widget…
  myTab->setPos(2, Dialog::vGap() + _th);
  myTab->setWidth(_w - 4);
  myTab->setHeight(_h - _th - Dialog::vGap()
                   - Dialog::buttonHeight() - Dialog::vBorder() * 2);
  // …then recompute the tab-bar geometry from the live font.  This also re-lays
  // out the tabs' content — that is the container's job, not the dialog's
  myTab->updateTabSizes();

  layoutButtonGroup();
}
```

Key points:

- Create `myTab` at `(0, 0, 1, 1)` and size it in `layout()`. **You must call
  `myTab->updateTabSizes()`** after sizing, or the tab headers render with a
  negative/placeholder width and the content is never laid out.
- A pane's children are **pane-relative**, and the builder's box is already inset
  by `hBorder()`/`vBorder()` — so just add rows; never offset by the borders
  yourself.
- **Panes are laid out even while their tab is hidden**, and they must be: a
  dialog's `loadConfig()` feeds the controls of *every* tab, not just the visible
  one, so they all have to be at their real size by then. (Feeding a control that
  is still at its placeholder size is how you get a `Rect::valid()` assert.)
  Self-contained content is laid out only when its tab is **active**, because a
  child it creates lazily (`RomListWidget`'s checkbox pool) would otherwise attach
  itself to whichever tab is currently showing.
- `layout()` order: compute `_w`/`_h` → size `myTab` → `updateTabSizes()` →
  `layoutButtonGroup()`.

---

## Resizable / fill-available dialogs

Fixed-size dialogs compute `_w`/`_h` in `layout()` and drop `setSize`. A dialog
that should take **all available space** (e.g. `LoggerDialog`, the launcher) is
different:

- Keep `setSize(4000, 4000, max_w, max_h)` in the ctor (it claims the space).
- Keep the `max_w`/`max_h` ctor params.
- Give its main list/content widget a **stretch** item so it grows:
  `root->addStretch(widgetItem(myList))`.
- If it is truly user-resizable, it also reports a minimum window size via
  `instance().frameBuffer().setWindowMinSize(...)` (see `LauncherDialog`).
  Ordinary fixed dialogs do not.

If a ctor param becomes genuinely unused after conversion (a `max_w`/`max_h` that
only fed a dropped `setSize`), delete it and fix the call sites — don't leave
dead parameters.

---

## Live font change (context — you don't implement this)

An already-open dialog re-fonts *live* when the user changes the dialog font,
with no restart. The base class handles this: `FrameBuffer::changeDialogFont`
mutates the font in place, then `Dialog::refreshFont()` refreshes widget metrics
and calls `relayout()` → your `layout()` runs again with the new font.

Your only responsibility is the golden rule: **`layout()` must recompute every
number from the current font**, and hold no state baked from a previous font.
Two corollaries:

- **Don't rely on a widget's auto-sized ctor width in `layout()`.** It goes
  stale on a live font change. If you need a label's width, recompute it:
  `label->setWidth(font.getStringWidth(label->getLabel()))` — this both de-dups
  the string literal and stays font-reactive.
- A dialog that ends up **larger than the screen** at a big font is detected
  automatically (`Dialog::exceedsScreen()` shows a "Dialog too large for screen"
  message on open). You don't need to guard against it.

---

## Gotcha checklist

- **`EditTextWidget` and `PopUpWidget` are `h + 2` internally** (frame padding).
  Never force their height. Size their row with `addAuto`, which takes the height
  from the widget, or give them `VAlign::Center` so they keep it. Passing
  `lineHeight` as the cell height makes them 2px too short — including via
  `widgetItem()`, which fills its cell in **both** axes.
- **`PopUpWidget::setWidth(w)` takes the TOTAL width** (`value box + label +
  dropDownWidth`), not just the value box. To fill a row, pass the full span
  (`_w - HBORDER*2 - …`), not the ctor's value-box formula. `setWidth` also
  resizes the drop-down menu, so a placeholder-width popup sized in `layout()`
  gets a correct menu.
- **Never nudge text to make it line up.** Text alignment is not something a
  dialog states: each widget centers its own text and the layout centers each
  widget in its cell, so centered items on a row always agree. A stray `+2` in a
  dialog is a bug in some widget's `firstTextY()`, not a fix. The single
  exception is a *multi-row* widget (`DataGridWidget`, `ToggleWidget`), whose
  first row a label must sit on: that is `VAlign::Baseline` (see above).
- **A box built to show several lines is not centered.** An `EditTextWidget`
  taller than one line (EventMappingWidget's two-line "Action" box) starts its
  text at the top, so the lines below it have somewhere to go — and a label
  beside it belongs on that *first* line, so pair them with `VAlign::Baseline`,
  not `labeledRow`. Centering is only ever right for a *single* line of text.
- **A label's box is `lineHeight` tall and clears its background**, so it carries
  ~2px of padding *below* its glyphs. Stacking one directly above another widget
  by measuring up `getFontHeight()` makes that padding erase the widget's top
  border. Hang it from the widget instead: `y = target->getTop() -
  label->getHeight()`.
- **Anything that reads `_w`/`_h` in the ctor breaks** — they are 0 there. Move
  such code to `layout()`.
- **A tab's content is one widget, and it never knows it is in a tab.** Loose
  controls go in a `TabPaneWidget`; a widget that already owns its controls *is*
  the content and needs no pane — don't wrap it in one, and don't derive it from
  `TabPaneWidget` (that would couple it to `TabWidget` and stop it being used
  anywhere else). See "Tabbed dialogs".
- **Never size a widget by subtracting what you think the other rows use.** A
  formula like `listHeight = h - 3*lineHeight - VBORDER - …` is right for exactly
  one height and silently drifts the moment anything above or below it changes.
  Make it an `addStretch()` item and let it take whatever is left over.
- **Filling a self-labeling widget** (`PopUpWidget`/`SliderWidget`) stretches its
  value box / track, so only fill one when its **cell is the thing it must end
  flush with** (a pop-up handed the list's width). Anchor it otherwise — and never
  stretch it into a container that is wider than its anchor, or it will chase the
  container's edge and sail past what it was meant to meet. A slider lining up with
  a pop-up wants `GUI::alignTracks`, not a fill: what has to meet is the *track*,
  not the widget's edge.
- **A widget that shows text loaded LATER must FILL.** A caption filled in by
  `loadConfig()` — a detected controller, a score, a date, an MD5 — must be given
  its width by the layout (`stretchedItem`, or `HAlign::Fill`), and built with no
  text at all. Never let it take its width from the text it happens to hold: that
  text is *data*, and the layout would then change shape with it — a long value
  stretching a column, an empty one collapsing it, and a `StaticTextWidget`
  re-deriving its width from its current label after any font change. Where the
  space such a value needs is wider than its heading, that width is the *dialog's*
  to state — from the model, not from a specimen widget:

  ```cpp
  // the score column shows MAX_SCORE_DIGITS digits, whatever is in it today
  table->columnFixed(COL_SCORE, fontWidth * HSM::MAX_SCORE_DIGITS);
  table->place(COL_SCORE, row, stretchedItem(myScoreWidgets[r]));  // fills it
  ```

  A value that must be centred (or right-aligned) in that space says so on the
  **widget** (`TextAlign::Center`) — a widget aligns its own text.
- **Keep irregular gaps explicit** with `addSpace(VGAP * n)`, traced from the
  original spacing.
- **Radio buttons are two objects with different ownership.** Each
  `RadioButtonWidget` is a widget, so the dialog owns it automatically like any
  other — but the `RadioButtonGroup` that links the buttons is *not* a widget,
  and nothing owns it for you. Hold it as a `unique_ptr<RadioButtonGroup>`
  member, create it with `std::make_unique<RadioButtonGroup>()`, and pass
  `myGroup.get()` to each button's constructor (the buttons only observe the
  group). A raw `new RadioButtonGroup()` into a plain pointer is a leak.
- **Destructor placement:** if you define `~MyDialog() = default;` in the `.cxx`
  (needed when members are `unique_ptr` to forward-declared types), put it
  immediately after the constructor. An inline `= default` in the header needs
  nothing.
- **Wrap the create-only block in `NOLINTBEGIN/END(cppcoreguidelines-prefer-member-initializer)`**
  — the create-only ctor deliberately triggers this check; suppress it, never hoist
  the widgets into the initializer list (see step 2).

---

## Build and verify

```bash
make -j$(nproc)          # plain build; objects in the build dir, binary ./stella
./stella -help           # smoke test: must exit 0 with no warnings
```

CI treats warnings as errors (`-Wall -Wextra -Wunused -Woverloaded-virtual`), so
a clean build is mandatory. Because positioning can't be fully verified without
running the GUI, **trace the new positions against the old ctor arithmetic** for
byte-parity, then test each dialog interactively — resize the window and switch
the dialog font (Small ↔ Large) and confirm nothing clips or overlaps.

---

## A complete minimal example

`ComboDialog` (`src/gui/ComboDialog.cxx`) is the smallest fully-converted dialog
— a vertical stack of eight self-labeling event popups over a button row. Its
`layout()` is the canonical template:

```cpp
void ComboDialog::layout()
{
  using GUI::BoxLayout;
  using GUI::anchoredItem;
  using Dir = BoxLayout::Dir;

  const int lineHeight   = Dialog::lineHeight(),
            fontWidth    = Dialog::fontWidth(),
            buttonHeight = Dialog::buttonHeight(),
            VBORDER      = Dialog::vBorder(),
            HBORDER      = Dialog::hBorder(),
            VGAP         = Dialog::vGap();

  // Size the (fixed) dialog from the current font so it reflows on font change
  _w = 8 * fontWidth + myPopupWidth + PopUpWidget::dropDownWidth(_font) + HBORDER * 2;
  _h = 8 * (lineHeight + VGAP) + VGAP + buttonHeight + VBORDER * 2 + _th;

  // Vertical stack of the eight self-labeling event popups; the button group
  // sits below, positioned by layoutButtonGroup().
  auto root = std::make_unique<BoxLayout>(Dir::Vertical, 0, HBORDER, VBORDER);
  for(auto* e: myEvents)
  {
    root->addFixed(anchoredItem(e), lineHeight);
    root->addSpace(VGAP);
  }
  root->doLayout(0, _th, _w, _h - _th);

  // Standard button group (Defaults / OK / Cancel) along the bottom edge
  layoutButtonGroup();
}
```

```
  ┌───────────── Add events for … ─────────────┐   ← title bar (_th)
  │  Event 1 [ Fire            ▼]               │  ┐
  │  Event 2 [ Up              ▼]               │  │ VBox of
  │  Event 3 [ Down            ▼]               │  │ anchoredItem(popup)
  │  Event 4 [ Left            ▼]               │  │ on a lineHeight+VGAP
  │  Event 5 [ Right           ▼]               │  │ pitch
  │  Event 6 [ None            ▼]               │  │
  │  Event 7 [ None            ▼]               │  │
  │  Event 8 [ None            ▼]               │  ┘
  ├─────────────────────────────────────────────┤
  │ [Defaults]                       [OK][Cancel]│  ← layoutButtonGroup()
  └─────────────────────────────────────────────┘
```

For richer examples in the tree:

- **Form with mixed rows:** `EmulationDialog`, `SnapshotDialog`
- **Two-column form:** `QuadTariDialog`
- **Grid table:** `HighScoresDialog`
- **Button grid:** `OptionsDialog`, `CommandDialog`
- **Tabbed:** `InputDialog`, `GameInfoDialog`, `DeveloperDialog`, `UIDialog`
- **Fill-available/resizable:** `LoggerDialog`, `LauncherDialog`
- **Irregular + engine mix:** `GlobalPropsDialog`
