# Writing a Dialog with the Layout Engine

This document explains how to add a new dialog (or convert an existing one) to
Stella's GUI using the relative **Layout** engine in `src/gui/Layout.{hxx,cxx}`.
It is aimed at developers who have never touched the GUI code before.

If you only read one section, read [The pattern in one
picture](#the-pattern-in-one-picture) and [Step by step](#step-by-step).

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
| `addFixed(child, px)`               | child gets exactly `px` along the main axis         |
| `addStretch(child, weight=1, base=0)` | child gets `base`, then shares leftover (by weight) |
| `addPercent(child, pct, max=0)`     | child gets `pct`% of the available length           |
| `addSpace(px)`                      | an empty gap of `px`                                |
| `addStretchSpace(weight=1, base=0)` | an empty gap of at least `base`, which also grows   |

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

### Leaf items — how a single widget fills its cell

You never construct `WidgetLayout` directly; use these `GUI::` helper builders:

| Helper                                   | Behaviour                                                                 |
| ---------------------------------------- | ------------------------------------------------------------------------- |
| `widgetItem(w, minW=0, minH=0)`          | **fills** the cell — stretches the widget to the cell size                 |
| `anchoredItem(w, minW=0, minH=0)`        | keeps the widget's **natural** size, top-left of the cell                  |
| `vCentered(w, h, minW=0)`                | keeps natural height `h`, vertically centered in a taller cell             |
| `hCentered(w, width, minH=0)`            | keeps natural width, horizontally centered in a wider cell                 |
| `indentedItem(w, indent, minW=0)`        | natural size, positioned `indent` px from the left                        |
| `labelColumn(label, control)`            | a label centered on the (taller) control it names                         |
| `labeledRow(label, control, labelW=0, indent=0)` | a row pairing a **separate** label with a control                 |

Rules of thumb:

- **Self-labeling widgets** (`PopUpWidget`, `SliderWidget` — they draw their own
  label) → use `anchoredItem`. Filling them would stretch the *track/value box*
  and look wrong. Because they share a common label width, they line up across
  rows without a grid.
- **A control that should span the row** (an `EditTextWidget`, a list) → use
  `widgetItem` (fill), or `vCentered(edit, edit->getHeight())` for an edit field
  (see the EditTextWidget gotcha below).
- **A checkbox indented under a group header** → `indentedItem(cb, indent())`.
- **A separate label + control** (label is its own `StaticTextWidget`, control is
  not self-labeling) → `labeledRow(label, control, sharedLabelW)`.
- **Right-align a column of value fields** whose widths differ (say a 3-digit and
  an 8-digit field, which should still end flush) → do *not* compute per-row
  label widths. Stretch the label and fix the field:
  `row.addStretch(labelColumn(label, field)); row.addFixed(anchoredItem(field), fieldW);`
  Every row then ends at the column's right edge, whatever its label or digits.

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
center it yourself with the engine — `hCentered(_okWidget, w)` as a one-item row.

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

| Your dialog is…                                       | Use                                                                   |
| ----------------------------------------------------- | --------------------------------------------------------------------- |
| A vertical **form** of self-labeling popups/sliders   | `BoxLayout(Vertical)` of `anchoredItem`s                              |
| A form with **separate** label + control rows         | `BoxLayout(Vertical)` of `labeledRow(...)` (share a `labelW`)         |
| A genuine **table** (cells align in 2-D)              | `GridLayout`                                                          |
| A grid of **buttons** (OptionsDialog, CommandDialog)  | `GridLayout` with `widgetItem` cells                                  |
| **Tabbed**                                            | `TabWidget` + a per-tab `layoutXxxTab()` helper (see below)           |
| Mostly regular with an **irregular** sub-block        | engine for the regular part, `setPos`/`setWidth` for the rest         |

**Mixing is expected and fine.** Lay out the structured part with the engine and
`doLayout` it, then read back resolved positions (`getRight()`, `getTop()`,
`getBottom()`) to `setPos`/`setWidth` the irregular bits. `HighScoresDialog` and
`GlobalPropsDialog` do exactly this. What you must *not* do is a plain
`for(...) w->setPos(...)` loop over several regular widgets — that defeats the
whole exercise. Direct `setPos` is only for a genuinely one-off widget or a
back-referenced nudge after a `doLayout`.

If you write a row/column lambda inside `layout()` that other dialogs could
reuse (a label+control row, a centered item, …), promote it to a `GUI::` helper
in `Layout.{hxx,cxx}` rather than leaving it local. Keep dialog-*specific*
composition local.

---

## Tabbed dialogs

```cpp
// ctor: create the tab widget at a PLACEHOLDER size
myTab = new TabWidget(this, font, 0, 0, 1, 1);
const int tabID = myTab->addTab(" General ", TabWidget::AUTO_WIDTH);
// ... create this tab's widgets with parent = myTab ...
addTabWidget(myTab);
```

```cpp
void MyTabbedDialog::layout()
{
  _w = ...; _h = ...;

  // Size the tab widget…
  myTab->setPos(2, Dialog::vGap() + _th);
  myTab->setWidth(_w - 4);
  myTab->setHeight(_h - _th - Dialog::vGap()
                   - Dialog::buttonHeight() - Dialog::vBorder() * 2);
  // …then recompute the tab-bar geometry from the live font:
  myTab->updateTabSizes();

  layoutGeneralTab();     // per-tab helpers position each tab's contents
  layoutAdvancedTab();

  layoutButtonGroup();
}
```

```
  ┌─ General ─┬─ Advanced ─┐            ← tab bar (updateTabSizes())
  │           └────────────┴──────────┐
  │  Mode:      [ popup            ▼]  │
  │  [x] Enable feature               │  ← each tab laid out like a form,
  │  Speed:     [======O------]       │    via layoutGeneralTab()
  │                                   │
  ├───────────────────────────────────┤
  │ [Defaults]              [OK][Cancel]
  └───────────────────────────────────┘
```

Key points:

- Create `myTab` at `(0, 0, 1, 1)` and size it in `layout()`. **You must call
  `myTab->updateTabSizes()`** after sizing, or the tab headers render with a
  negative/placeholder width.
- Tab child coordinates are **tab-relative**; the tab bar offset is added for
  you. A per-tab helper builds that tab's contents exactly like a standalone
  form: `doLayout(0, 0, myTab->getWidth(), myTab->getHeight())`.
- `layout()` order: compute `_w`/`_h` → size `myTab` → `updateTabSizes()` →
  per-tab helpers → `layoutButtonGroup()`.

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

- **`EditTextWidget` is `h + 2` internally** (frame padding). Never force its
  height. Use `setWidth` + `setPos`, or `vCentered(edit, edit->getHeight())` in
  a row. Passing `lineHeight` as its height makes it 2px too short.
- **`PopUpWidget::setWidth(w)` takes the TOTAL width** (`value box + label +
  dropDownWidth`), not just the value box. To fill a row, pass the full span
  (`_w - HBORDER*2 - …`), not the ctor's value-box formula. `setWidth` also
  resizes the drop-down menu, so a placeholder-width popup sized in `layout()`
  gets a correct menu.
- **Anything that reads `_w`/`_h` in the ctor breaks** — they are 0 there. Move
  such code to `layout()`.
- **Don't fill self-labeling widgets** (`PopUpWidget`/`SliderWidget`) — anchor
  them, or their track/value box stretches.
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
