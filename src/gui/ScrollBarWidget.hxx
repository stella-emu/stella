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

#ifndef SCROLL_BAR_WIDGET_HXX
#define SCROLL_BAR_WIDGET_HXX

class GuiObject;

#include "Widget.hxx"
#include "Command.hxx"
#include "bspf.hxx"

class ScrollBarWidget : public Widget, public CommandSender
{
  public:
    ScrollBarWidget(GuiObject* boss, const GUI::Font& font);
    ~ScrollBarWidget() override = default;

    void recalc();
    void handleMouseDown(int x, int y, MouseButton b, int clickCount) override;
    void handleMouseUp(int x, int y, MouseButton b, int clickCount) override;
    void handleMouseMoved(int x, int y) override;
    void handleMouseWheel(int x, int y, int direction) override;
    bool handleMouseClicks(int x, int y, MouseButton b) override;
    void handleMouseLeft() override;

    static void setWheelLines(int lines) { S_WHEEL_LINES = lines; }
    static int  getWheelLines()          { return S_WHEEL_LINES;  }
    /**
      The arrow I draw at each end.  Odd, so it has a single-pixel tip, and
      derived from the font so it grows with the text rather than stepping
      between two hand-drawn bitmaps.  At the default 9x18 font this is 7,
      which is the width the old small arrow bitmap had.
    */
    static int arrowWidth(const GUI::Font& font) {
      return ((font.getMaxCharWidth() * 3 / 4) | 1);
    }

    static int scrollBarWidth(const GUI::Font& font) {
      // Wide enough for the arrow it must contain, with a margin either side
      return arrowWidth(font) * 2 + 1;
    }

    // Re-pick the arrow images/box sizes and the (font-derived) bar width when
    // the font changes at runtime, so the scrollbar grows/shrinks with it
    void refreshFontMetrics() override;

  protected:
    void drawWidget(bool hilite) override;

  private:
    void checkBounds(int old_pos);
    void setArrows();

  public:  // TODO: these shouldn't be public
    int _numEntries{0};
    int _entriesPerPage{0};
    int _currentPos{0};
    int _wheel_lines{0};

  private:
    enum class Part: uInt8 { None, UpArrow, DownArrow, Slider, PageUp, PageDown };

    Part _part{Part::None};
    Part _draggingPart{Part::None};
    int _sliderHeight{0};
    int _sliderPos{0};
    int _sliderDeltaMouseDownPos{0};

    int _upDownBoxHeight{0};
    int _scrollBarWidth{0};
    int _arrowWidth{0};
    int _arrowHeight{0};
    int _arrowThickness{0};

    static inline int S_WHEEL_LINES = 4;

  private:
    // Following constructors and assignment operators not supported
    ScrollBarWidget() = delete;
    ScrollBarWidget(const ScrollBarWidget&) = delete;
    ScrollBarWidget(ScrollBarWidget&&) = delete;
    ScrollBarWidget& operator=(const ScrollBarWidget&) = delete;
    ScrollBarWidget& operator=(ScrollBarWidget&&) = delete;
};

#endif  // SCROLL_BAR_WIDGET_HXX
