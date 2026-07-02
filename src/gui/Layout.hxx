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
  which the owning dialog uses to derive its window minimum.

  Layouts are intentionally cheap, throwaway descriptions: a dialog rebuilds
  its tree from the current font metrics whenever it (re)lays out, so window
  resizing and font-size changes are handled by exactly the same path.

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

  private:
    // Following constructors and assignment operators not supported
    Layout(const Layout&) = delete;
    Layout(Layout&&) = delete;
    Layout& operator=(const Layout&) = delete;
    Layout& operator=(Layout&&) = delete;
};

/**
  An item that positions a single widget within its cell.  By default the widget
  is resized to fill the cell; with fill=false it is instead anchored at the
  cell's top-left and keeps its own (natural) size — useful for table cells that
  should line up but not stretch, e.g. text labels.  Carries the widget's
  minimum usable size (font-derived, supplied by the dialog).  A null widget
  makes the item an empty spacer that only reserves space.
*/
class WidgetLayout : public Layout
{
  public:
    explicit WidgetLayout(Widget* widget, int minW = 0, int minH = 0,
                          bool fill = true)
      : myWidget{widget}, myMinW{minW}, myMinH{minH}, myFill{fill} { }
    ~WidgetLayout() override = default;

    void doLayout(int x, int y, int w, int h) override;

    Common::Size minSize() const override {
      return Common::Size(static_cast<uInt32>(myMinW), static_cast<uInt32>(myMinH));
    }

  private:
    Widget* myWidget{nullptr};
    int myMinW{0};
    int myMinH{0};
    bool myFill{true};

  private:
    // Following constructors and assignment operators not supported
    WidgetLayout(const WidgetLayout&) = delete;
    WidgetLayout(WidgetLayout&&) = delete;
    WidgetLayout& operator=(const WidgetLayout&) = delete;
    WidgetLayout& operator=(WidgetLayout&&) = delete;
};

/**
  How a BoxLayout child (along the main axis) or a GridLayout track is sized:

    Fixed    a fixed number of pixels (usually font-derived)
    Percent  a percentage (0..100) of the available length
    Stretch  shares the leftover length in proportion to a weight

  An optional maximum clamps the resulting size.
*/
enum class SizePolicy: uInt8 { Fixed, Percent, Stretch };

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
    BoxLayout& add(unique_ptr<Layout> child, SizePolicy policy, int value,
                   int maxMain = 0);

    // Convenience wrappers
    BoxLayout& addFixed(unique_ptr<Layout> child, int px)
      { return add(std::move(child), SizePolicy::Fixed, px); }
    BoxLayout& addPercent(unique_ptr<Layout> child, int pct, int maxMain = 0)
      { return add(std::move(child), SizePolicy::Percent, pct, maxMain); }
    BoxLayout& addStretch(unique_ptr<Layout> child, int weight = 1)
      { return add(std::move(child), SizePolicy::Stretch, weight); }
    // A fixed empty gap
    BoxLayout& addSpace(int px)
      { return add(std::make_unique<WidgetLayout>(nullptr), SizePolicy::Fixed, px); }

    void doLayout(int x, int y, int w, int h) override;

    Common::Size minSize() const override;

  private:
    struct Item {
      unique_ptr<Layout> layout;
      SizePolicy policy{SizePolicy::Fixed};
      int value{0};
      int maxMain{0};
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
    GridLayout& column(int idx, SizePolicy policy, int value, int maxSize = 0);
    GridLayout& row(int idx, SizePolicy policy, int value, int maxSize = 0);

    // Convenience wrappers
    GridLayout& columnFixed(int idx, int px)
      { return column(idx, SizePolicy::Fixed, px); }
    GridLayout& columnPercent(int idx, int pct, int maxSize = 0)
      { return column(idx, SizePolicy::Percent, pct, maxSize); }
    GridLayout& columnStretch(int idx, int weight = 1)
      { return column(idx, SizePolicy::Stretch, weight); }
    GridLayout& rowFixed(int idx, int px)
      { return row(idx, SizePolicy::Fixed, px); }
    GridLayout& rowPercent(int idx, int pct, int maxSize = 0)
      { return row(idx, SizePolicy::Percent, pct, maxSize); }
    GridLayout& rowStretch(int idx, int weight = 1)
      { return row(idx, SizePolicy::Stretch, weight); }

    // Place a child in the cell at (col, row), optionally spanning cells.
    GridLayout& place(int col, int row, unique_ptr<Layout> child,
                      int colspan = 1, int rowspan = 1);

    void doLayout(int x, int y, int w, int h) override;

    Common::Size minSize() const override;

  private:
    struct Track {
      SizePolicy policy{SizePolicy::Fixed};
      int value{0};
      int maxSize{0};
    };
    struct Cell {
      unique_ptr<Layout> layout;
      int col{0}, row{0};
      int colspan{1}, rowspan{1};
    };

    // Resolve one axis' track sizes into 'ext', given the length available to
    // the tracks (i.e. after the inter-track spacing has been removed).
    static void resolveTracks(const vector<Track>& tracks, int avail,
                              IntArray& ext);
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

// Wrap a widget as a layout item, carrying its minimum usable size.  A null
// widget yields an empty spacer that only reserves space.
inline unique_ptr<WidgetLayout>
widgetItem(Widget* widget, int minW = 0, int minH = 0)
{
  return std::make_unique<WidgetLayout>(widget, minW, minH);
}

// Like widgetItem(), but anchors the widget at its cell's top-left and keeps
// the widget's own size instead of resizing it to fill the cell — for table
// cells (labels/values) that should align but not stretch.
inline unique_ptr<WidgetLayout>
anchoredItem(Widget* widget, int minW = 0, int minH = 0)
{
  return std::make_unique<WidgetLayout>(widget, minW, minH, false);
}

// Wrap a widget so it keeps its natural height 'h' and is vertically centered
// within its (taller) cell, rather than filling it — used for plain text
// labels, which draw top-aligned.
unique_ptr<Layout> vCentered(Widget* widget, int h, int minW = 0);

// Wrap a widget so it keeps its natural width 'w' and is horizontally centered
// within its (wider) cell, rather than filling it — used e.g. for a button
// centered across a spanning grid cell.
unique_ptr<Layout> hCentered(Widget* widget, int w, int minH = 0);

// Wrap a widget so it keeps its natural size and is positioned 'indent' pixels
// from the left of its cell — used for options indented under a group header
// (e.g. the checkboxes below a "When saving:" label in the option dialogs).
unique_ptr<Layout> indentedItem(Widget* widget, int indent, int minW = 0);

// A horizontal form row pairing a separate label with a control: the label
// occupies a column 'labelW' wide (0 = the label's own width) and the control
// is anchored at its natural size just to its right, after an optional left
// 'indent'.  For label + PopUp/Slider/edit rows where the widget is not
// self-labeling; pass a shared 'labelW' to align controls across several rows.
unique_ptr<Layout> labeledRow(Widget* label, Widget* control,
                              int labelW = 0, int indent = 0);

}  // namespace GUI

#endif  // LAYOUT_HXX
