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

#ifndef LAYOUT_HXX
#define LAYOUT_HXX

#include "Rect.hxx"
#include "bspf.hxx"

class Widget;

namespace GUI {

/**
  Base class for the GUI layout manager.

  A Layout describes how a rectangular region of a dialog is subdivided.
  doLayout() assigns absolute geometry to the widgets it (transitively)
  contains; minSize() reports the smallest region in which nothing would clip,
  which the owning dialog uses to derive its window minimum; naturalSize()
  reports what the content would like to be.

  Layouts are intentionally cheap, throwaway descriptions: a dialog rebuilds
  its tree from the current font metrics whenever it (re)lays out, so window
  resizing and font-size changes are handled by exactly the same path.

  A dialog states alignment and sizing policy, never pixel offsets.  Text lines
  up because each widget centers its own text and the layout centers each widget
  in its cell (see HAlign/VAlign) — there is no per-widget nudge to get right,
  and none should be reintroduced.

  Things Qt has that this does not, left out on purpose — add them when a case
  demands one, rather than working around their absence:

    minimumSizeHint()  A widget reporting how far it may be squeezed, so a
                       resizable dialog could derive its window minimum from the
                       tree instead of the dialog hand-feeding it (the minW/minH
                       carried by each item).  It must stay separate from
                       naturalSize(): if a list's minimum were the size it wants
                       to be, the launcher could never be made smaller.
    heightForWidth()   A widget whose height depends on its width.  We do have
                       this case — WrappedTextWidget, which is why its width must
                       be set before the column holding it is built — but one
                       case does not pay for a two-pass layout.
    size policy        Fixed/Preferred/Expanding on the widget.  Not needed:
                       addFixed()/addAuto()/addStretch()/addPercent() say it at
                       the layout, where the dialog is already deciding.

  @author  Stephen Anthony
*/
class Layout
{
  public:
    Layout() = default;
    virtual ~Layout() = default;

    // Position this node's content within the given rectangle.  Plain ints are
    // used (rather than Common::Rect) since intermediate sizes may legitimately
    // be zero/negative, which Common::Rect would assert against.
    virtual void doLayout(int x, int y, int w, int h) = 0;

    // Report the smallest size at which the content does not clip.
    virtual Common::Size minSize() const = 0;

    // Report the size the content would like to be (Qt's size hint), which is
    // what BoxLayout::addAuto() sizes a cell from.  Deliberately separate from
    // minSize(): a list's natural size is far larger than the size it can be
    // squeezed to, and it is minSize() that a resizable dialog's window minimum
    // comes from (see Widget::naturalSize).
    virtual Common::Size naturalSize() const = 0;

    // Where this node's first line of text sits, and whether it asks to be put
    // on its row's baseline.  Only a widget can answer meaningfully, so these
    // default to "no text, not baseline-aligned" for the composite layouts.
    virtual int firstTextY() const { return 0; }
    virtual bool onBaseline() const { return false; }

  private:
    // Following constructors and assignment operators not supported
    Layout(const Layout&) = delete;
    Layout(Layout&&) = delete;
    Layout& operator=(const Layout&) = delete;
    Layout& operator=(Layout&&) = delete;
};

/**
  How a widget is placed within its cell, on each axis independently (this is
  Qt's Qt::Alignment).  Fill resizes the widget to the cell; every other value
  keeps the widget at its natural size (Widget::naturalSize) and positions it.

  Alignment is all a dialog needs to state, because every widget centers its own
  text within its own height (Widget::firstTextY): center two widgets in a row
  and their TEXT lines up, whatever each one's height and framing.  Baseline is
  the one case centering cannot express — a widget that is several rows of text
  in one box (data grid, toggle list), whose FIRST row a label beside it must sit
  on.  It is meaningful in a horizontal BoxLayout, where the row's baseline is
  the lowest first-text-line among its baseline-aligned items; elsewhere it
  places like Top.
*/
enum class HAlign: uInt8 { Fill, Left, Center, Right };
enum class VAlign: uInt8 { Fill, Top, Center, Baseline, Bottom };

/**
  An item that positions a single widget within its cell, per its alignment on
  each axis.  Carries the widget's minimum usable size (font-derived, supplied
  by the dialog).  A null widget makes the item an empty spacer that only
  reserves space.
*/
class WidgetLayout : public Layout
{
  public:
    explicit WidgetLayout(Widget* widget,
                          HAlign hAlign = HAlign::Fill,
                          VAlign vAlign = VAlign::Fill,
                          int minW = 0, int minH = 0)
      : myWidget{widget}, myHAlign{hAlign}, myVAlign{vAlign},
        myMinW{minW}, myMinH{minH} { }
    ~WidgetLayout() override = default;

    void doLayout(int x, int y, int w, int h) override;

    Common::Size minSize() const override {
      return Common::Size(static_cast<uInt32>(myMinW), static_cast<uInt32>(myMinH));
    }

    Common::Size naturalSize() const override;

    int firstTextY() const override;
    bool onBaseline() const override { return myVAlign == VAlign::Baseline; }

  private:
    Widget* myWidget{nullptr};
    HAlign myHAlign{HAlign::Fill};
    VAlign myVAlign{VAlign::Fill};
    int myMinW{0};
    int myMinH{0};

  private:
    // Following constructors and assignment operators not supported
    WidgetLayout(const WidgetLayout&) = delete;
    WidgetLayout(WidgetLayout&&) = delete;
    WidgetLayout& operator=(const WidgetLayout&) = delete;
    WidgetLayout& operator=(WidgetLayout&&) = delete;
};

/**
  How a BoxLayout child (along the main axis) or a GridLayout track is sized:

    Auto     as large as its content wants to be (Widget::naturalSize)
    Fixed    a fixed number of pixels (usually font-derived)
    Percent  a percentage (0..100) of the available length
    Stretch  shares the leftover length in proportion to a weight

  Auto is the one to reach for: a row is then as tall as its tallest widget and a
  column as wide as its widest, and the dialog states no pixel size at all.  Use
  Fixed only where the size is genuinely the dialog's choice (a shared button
  width), and Stretch for content with no size of its own (a list, an image).

  An optional maximum clamps the resulting size.
*/
enum class SizePolicy: uInt8 { Auto, Fixed, Percent, Stretch };

/**
  Stacks its children along one axis (horizontal or vertical).  Each child is
  sized along the box's main axis according to its policy; on the cross axis
  every child fills the box (minus margins).
*/
class BoxLayout : public Layout
{
  public:
    enum class Dir: uInt8 { Horizontal, Vertical };

    explicit BoxLayout(Dir dir, int spacing = 0, int marginH = 0, int marginV = 0)
      : myDir{dir}, mySpacing{spacing}, myMarginH{marginH}, myMarginV{marginV} { }
    ~BoxLayout() override = default;

    // value: Fixed -> pixels, Percent -> 0..100, Stretch -> weight.
    // maxMain: optional main-axis clamp (0 = unbounded).
    // minMain: main-axis floor reported by minSize().  A Fixed cell defaults
    //          to its fixed size; a dialog that recomputes the fixed value as
    //          the window shrinks declares here how far it can compress, which
    //          keeps minSize() independent of the current size.  A Stretch cell
    //          takes it as a BASE SIZE: the cell gets that much before the
    //          leftover is shared out by weight, so several cells can grow from
    //          differing natural sizes in a chosen proportion.
    BoxLayout& add(unique_ptr<Layout> child, SizePolicy policy, int value,
                   int maxMain = 0, int minMain = 0);

    // Convenience wrappers
    BoxLayout& addFixed(unique_ptr<Layout> child, int px, int minPx = 0)
      { return add(std::move(child), SizePolicy::Fixed, px, 0, minPx); }
    BoxLayout& addPercent(unique_ptr<Layout> child, int pct, int maxMain = 0)
      { return add(std::move(child), SizePolicy::Percent, pct, maxMain); }
    BoxLayout& addStretch(unique_ptr<Layout> child, int weight = 1, int basePx = 0)
      { return add(std::move(child), SizePolicy::Stretch, weight, 0, basePx); }

    // Size the cell from what the child wants to be (Widget::naturalSize), so a
    // row of controls is as tall as its tallest one and a column of them as wide
    // as its widest — the dialog states no pixel height at all.  Prefer this to
    // addFixed(item, lineHeight): the row then cannot be too short for a control
    // that frames its text, and it follows the font by construction.
    // Not for a widget that has no meaningful size of its own along the main
    // axis (a list, an image): stretch those.
    BoxLayout& addAuto(unique_ptr<Layout> child, int minPx = 0)
      { return add(std::move(child), SizePolicy::Auto, 0, 0, minPx); }

    // A fixed empty gap
    BoxLayout& addSpace(int px)
      { return add(std::make_unique<WidgetLayout>(nullptr), SizePolicy::Fixed, px); }
    // An empty gap of at least 'basePx', which grows with the leftover space
    BoxLayout& addStretchSpace(int weight = 1, int basePx = 0)
      { return add(std::make_unique<WidgetLayout>(nullptr), SizePolicy::Stretch,
                   weight, 0, basePx); }

    void doLayout(int x, int y, int w, int h) override;

    Common::Size minSize() const override;
    Common::Size naturalSize() const override;

  private:
    struct Item {
      unique_ptr<Layout> layout;
      SizePolicy policy{SizePolicy::Fixed};
      int value{0};
      int maxMain{0};
      int minMain{0};
    };

    Dir myDir{Dir::Vertical};
    int mySpacing{0};
    int myMarginH{0};  // left/right inset
    int myMarginV{0};  // top/bottom inset
    vector<Item> myItems;

  private:
    // Following constructors and assignment operators not supported
    BoxLayout(const BoxLayout&) = delete;
    BoxLayout(BoxLayout&&) = delete;
    BoxLayout& operator=(const BoxLayout&) = delete;
    BoxLayout& operator=(BoxLayout&&) = delete;
};

/**
  Arranges its children in a table of fixed rows and columns.  Each column and
  each row is a "track" sized along its axis by a SizePolicy (the same
  Fixed/Percent/Stretch rules BoxLayout uses); a child is then placed into a
  cell and may span several columns and/or rows.

  Because every cell in a column shares that column's resolved width (and every
  cell in a row its height), controls line up across rows — which is what the
  form-style option dialogs need for their label / control columns.
*/
class GridLayout : public Layout
{
  public:
    GridLayout(int cols, int rows, int hSpacing = 0, int vSpacing = 0,
               int marginH = 0, int marginV = 0);
    ~GridLayout() override = default;

    // Define a column's / row's sizing policy.
    // value: Fixed -> pixels, Percent -> 0..100, Stretch -> weight.
    // maxSize: optional clamp on the resolved track size (0 = unbounded).
    // minSize: a Stretch track's BASE size — what it gets before the leftover is
    //          shared out.  A stretching column of fields has no size of its own,
    //          so this is where a dialog says how much room they need, and it is
    //          what lets the dialog's own width be derived rather than guessed.
    GridLayout& column(int idx, SizePolicy policy, int value, int maxSize = 0,
                       int minSize = 0);
    GridLayout& row(int idx, SizePolicy policy, int value, int maxSize = 0,
                    int minSize = 0);

    // Convenience wrappers.  An Auto track is as large as the largest thing
    // placed in it wants to be — the counterpart of BoxLayout::addAuto(), and
    // what makes a form's label column line up without anyone measuring labels
    GridLayout& columnAuto(int idx)
      { return column(idx, SizePolicy::Auto, 0); }
    GridLayout& columnFixed(int idx, int px)
      { return column(idx, SizePolicy::Fixed, px); }
    GridLayout& columnPercent(int idx, int pct, int maxSize = 0)
      { return column(idx, SizePolicy::Percent, pct, maxSize); }
    GridLayout& columnStretch(int idx, int weight = 1, int minPx = 0)
      { return column(idx, SizePolicy::Stretch, weight, 0, minPx); }
    GridLayout& rowAuto(int idx)
      { return row(idx, SizePolicy::Auto, 0); }
    GridLayout& rowFixed(int idx, int px)
      { return row(idx, SizePolicy::Fixed, px); }
    GridLayout& rowPercent(int idx, int pct, int maxSize = 0)
      { return row(idx, SizePolicy::Percent, pct, maxSize); }
    GridLayout& rowStretch(int idx, int weight = 1, int minPx = 0)
      { return row(idx, SizePolicy::Stretch, weight, 0, minPx); }

    // Place a child in the cell at (col, row), optionally spanning cells.
    GridLayout& place(int col, int row, unique_ptr<Layout> child,
                      int colspan = 1, int rowspan = 1);

    void doLayout(int x, int y, int w, int h) override;

    Common::Size minSize() const override;
    Common::Size naturalSize() const override;

  private:
    struct Track {
      SizePolicy policy{SizePolicy::Fixed};
      int value{0};
      int maxSize{0};
      int minSize{0};
    };
    struct Cell {
      unique_ptr<Layout> layout;
      int col{0}, row{0};
      int colspan{1}, rowspan{1};
    };

    // Resolve one axis' track sizes into 'ext', given the length available to
    // the tracks (i.e. after the inter-track spacing has been removed) and what
    // each track's content wants to be (which sizes the Auto tracks).
    static void resolveTracks(const vector<Track>& tracks, int avail,
                              const IntArray& naturals, IntArray& ext);

    // What each track's content wants to be, along the given axis
    void trackNaturals(bool horiz, IntArray& naturals) const;
    // Grow the 'span' tracks starting at 'start' so their combined size (plus
    // the spacing they subsume) can hold 'need' pixels of content.
    static void growSpan(IntArray& mins, int start, int span, int need,
                         int spacing);

    int myHSpacing{0};
    int myVSpacing{0};
    int myMarginH{0};  // left/right inset
    int myMarginV{0};  // top/bottom inset
    vector<Track> myColumns;
    vector<Track> myRows;
    vector<Cell> myCells;

  private:
    // Following constructors and assignment operators not supported
    GridLayout(const GridLayout&) = delete;
    GridLayout(GridLayout&&) = delete;
    GridLayout& operator=(const GridLayout&) = delete;
    GridLayout& operator=(GridLayout&&) = delete;
};

// - - - - - Convenience builders for assembling a layout tree - - - - -

// Wrap a widget as a layout item, aligned within its cell as asked.  Carries the
// widget's minimum usable size, which the dialog supplies (only a resizable
// dialog, whose window minimum comes from it, need bother).
inline unique_ptr<WidgetLayout>
alignedItem(Widget* widget, HAlign hAlign, VAlign vAlign,
            int minW = 0, int minH = 0)
{
  return std::make_unique<WidgetLayout>(widget, hAlign, vAlign, minW, minH);
}

// A widget that fills its cell in both axes: lists, images, and any control that
// should widen (and deepen) with the dialog.  A null widget yields an empty
// spacer that only reserves space.
inline unique_ptr<WidgetLayout>
widgetItem(Widget* widget, int minW = 0, int minH = 0)
{
  return std::make_unique<WidgetLayout>(widget, HAlign::Fill, VAlign::Fill,
                                        minW, minH);
}

// A widget that keeps its natural size, at the left of its cell and centered in
// it vertically.  This is the workhorse: centering is what makes its text line
// up with that of everything else on the row (see HAlign/VAlign above), so most
// controls want exactly this and need say nothing further.
inline unique_ptr<WidgetLayout>
anchoredItem(Widget* widget, int minW = 0, int minH = 0)
{
  return std::make_unique<WidgetLayout>(widget, HAlign::Left, VAlign::Center,
                                        minW, minH);
}

// Wrap a widget so it keeps its natural size and is positioned 'indent' pixels
// from the left of its cell — used for options indented under a group header
// (e.g. the checkboxes below a "When saving:" label in the option dialogs).
// An indent is spacing, not alignment, hence a composition rather than a flag.
unique_ptr<Layout> indentedItem(Widget* widget, int indent, int minW = 0);

// A self-labeling control, plus any indent the layout gives the row it sits in.
struct LabeledControl {
  Widget* control{nullptr};
  int indent{0};
};

// Give a group of self-labeling controls (SliderWidget, PopUpWidget — they draw
// their own label, see Widget::naturalLabelWidth) ONE label column, as wide as
// the longest of their labels plus a character of clearance.  Their tracks and
// value boxes then line up down the group, and a control the layout indents says
// so, so its column is narrowed to match and the tracks still meet.
//
// This is the width counterpart of BoxLayout::addAuto(): no one names a label in
// a string literal to measure it, and adding a longer label simply widens the
// column.  Call it from the layout (a pane's builder), so it follows the font.
//
// ⚠ The clearance comes from HERE.  A self-labeling control left out of a group
// derives its own column from its label text alone, so its box sits flush
// against it — which is why some labels still carry a trailing space to fake the
// gap ("Rate ", "X "). Put such a control in a group of its own rather than
// padding its label: the padding is alignment hidden in a string, and it breaks
// silently when the label is reworded.
void alignLabels(std::initializer_list<LabeledControl> controls);

// A horizontal form row pairing a separate label with a control: the label
// occupies a column 'labelW' wide (0 = the label's own width) and the control
// sits just to its right, after an optional left 'indent'.  With fill=false
// (the default) the control keeps its natural width; with fill=true it stretches
// to fill the rest of the row — for edit/list fields that should widen with the
// dialog.  For label + PopUp/Slider/edit rows where the widget is not
// self-labeling; pass a shared 'labelW' to align controls across several rows.
// Note that 'labelW' is a column width, not the label's: leave room in it for
// some clearance before the control.
// The row is as tall as the taller of the two, and both are centered in it, so
// their texts line up whichever control it is.  Give the row an addAuto() cell.
// NOT for a control that shows SEVERAL lines (a multi-line EditTextWidget, a
// data grid): the label belongs on the control's first line, not the middle of
// the box, so pair them with VAlign::Baseline instead — see EventMappingWidget's
// "Action" row.
unique_ptr<Layout> labeledRow(Widget* label, Widget* control,
                              int labelW = 0, int indent = 0, bool fill = false);

}  // namespace GUI

#endif  // LAYOUT_HXX
