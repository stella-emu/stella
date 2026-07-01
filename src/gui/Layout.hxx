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
  An item that positions a single widget so it fills its cell.  Carries the
  widget's minimum usable size (font-derived, supplied by the dialog).  A null
  widget makes the item an empty spacer that only reserves space.
*/
class WidgetLayout : public Layout
{
  public:
    explicit WidgetLayout(Widget* widget, int minW = 0, int minH = 0)
      : myWidget{widget}, myMinW{minW}, myMinH{minH} { }
    ~WidgetLayout() override = default;

    void doLayout(int x, int y, int w, int h) override;
    Common::Size minSize() const override {
      return Common::Size(static_cast<uInt32>(myMinW), static_cast<uInt32>(myMinH));
    }

  private:
    Widget* myWidget{nullptr};
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
  Stacks its children along one axis (horizontal or vertical).  Each child is
  sized along the box's main axis according to its policy; on the cross axis
  every child fills the box (minus margins).

    Fixed    main-axis size is a fixed number of pixels (usually font-derived)
    Percent  main-axis size is a percentage (0..100) of the available length
    Stretch  child shares the leftover length in proportion to its weight

  An optional per-child maximum clamps the resulting main-axis size.
*/
class BoxLayout : public Layout
{
  public:
    enum class Dir: uInt8    { Horizontal, Vertical };
    enum class Policy: uInt8 { Fixed, Percent, Stretch };

    explicit BoxLayout(Dir dir, int spacing = 0, int marginH = 0, int marginV = 0)
      : myDir{dir}, mySpacing{spacing}, myMarginH{marginH}, myMarginV{marginV} { }
    ~BoxLayout() override = default;

    // value: Fixed -> pixels, Percent -> 0..100, Stretch -> weight.
    // maxMain: optional main-axis clamp (0 = unbounded).
    BoxLayout& add(unique_ptr<Layout> child, Policy policy, int value,
                   int maxMain = 0);

    // Convenience wrappers
    BoxLayout& addFixed(unique_ptr<Layout> child, int px)
      { return add(std::move(child), Policy::Fixed, px); }
    BoxLayout& addPercent(unique_ptr<Layout> child, int pct, int maxMain = 0)
      { return add(std::move(child), Policy::Percent, pct, maxMain); }
    BoxLayout& addStretch(unique_ptr<Layout> child, int weight = 1)
      { return add(std::move(child), Policy::Stretch, weight); }
    // A fixed empty gap
    BoxLayout& addSpace(int px)
      { return add(std::make_unique<WidgetLayout>(nullptr), Policy::Fixed, px); }

    void doLayout(int x, int y, int w, int h) override;
    Common::Size minSize() const override;

  private:
    struct Item {
      unique_ptr<Layout> layout;
      Policy policy{Policy::Fixed};
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

// - - - - - Convenience builders for assembling a layout tree - - - - -

// Wrap a widget as a layout item, carrying its minimum usable size.  A null
// widget yields an empty spacer that only reserves space.
inline unique_ptr<WidgetLayout>
widgetItem(Widget* widget, int minW = 0, int minH = 0)
{
  return std::make_unique<WidgetLayout>(widget, minW, minH);
}

// Wrap a widget so it keeps its natural height 'h' and is vertically centered
// within its (taller) cell, rather than filling it — used for plain text
// labels, which draw top-aligned.
unique_ptr<Layout> vCentered(Widget* widget, int h, int minW = 0);

}  // namespace GUI

#endif  // LAYOUT_HXX
