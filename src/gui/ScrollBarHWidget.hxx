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

#ifndef SCROLL_BAR_H_WIDGET_HXX
#define SCROLL_BAR_H_WIDGET_HXX

class GuiObject;

#include "Widget.hxx"
#include "Command.hxx"
#include "ScrollBarWidget.hxx"
#include "bspf.hxx"

/**
  A horizontal scrollbar: left/right arrows, a page track, and a draggable
  slider sized to the visible fraction of the content.  The sideways
  counterpart of ScrollBarWidget, with the same look and behaviour.

  @author  Stephen Anthony and Thomas Jentzsch
*/
class ScrollBarHWidget : public Widget, public CommandSender
{
  public:
    ScrollBarHWidget(GuiObject* boss, const GUI::Font& font);
    ~ScrollBarHWidget() override = default;

    // Configure what's being scrolled; called by the owner whenever its
    // contents or visible range change
    void setNumEntries(int numEntries)         { _numEntries = numEntries; }
    void setEntriesPerPage(int entriesPerPage) { _entriesPerPage = entriesPerPage; }
    void setCurrentPos(int pos)                { _currentPos = pos; }
    int  currentPos() const                    { return _currentPos; }
    // Per-owner override of the class-wide default; 0 means use it
    void setWheelLineCount(int lines)          { _wheel_lines = lines; }

    // Recomputes the slider's size/position from _numEntries/_entriesPerPage/_currentPos
    void recalc();
    // Clicking an arrow steps by one, the track pages, and the slider drags;
    // handleMouseClicks() lets the arrows/track keep firing while held
    void handleMouseDown(int x, int y, MouseButton b, int clickCount) override;
    void handleMouseUp(int x, int y, MouseButton b, int clickCount) override;
    void handleMouseMoved(int x, int y) override;
    void handleMouseWheel(int x, int y, int direction) override;
    bool handleMouseClicks(int x, int y, MouseButton b) override;
    void handleMouseLeft() override;

    // Same thickness as a vertical scrollbar's width, so the two match
    static int scrollBarHeight(const GUI::Font& font) {
      return ScrollBarWidget::scrollBarWidth(font);
    }

    // Re-pick the arrow sizes and the (font-derived) bar height when the font
    // changes at runtime
    void refreshFont() override;

  protected:
    // Draws the frame, both arrows, and (unless everything fits on one page) the slider
    void drawWidget(bool hilite) override;

  private:
    // Clamps _currentPos to [0, _numEntries - _entriesPerPage]; if it moved,
    // recalcs and reports the new position via GuiObject::Cmd::SetPosition
    void checkBounds(int old_pos);
    // Re-derives the arrow/left-right-box dimensions from the current font
    void setArrows();

    // What's being scrolled; set via setNumEntries()/setEntriesPerPage()/setCurrentPos()
    int _numEntries{0};
    int _entriesPerPage{0};
    int _currentPos{0};
    // Per-owner override of the wheel default; 0 means use it
    int _wheel_lines{0};

    // Which region of the bar an interaction refers to
    enum class Part: uInt8 { None, LeftArrow, RightArrow, Slider, PageLeft, PageRight };

    // Region currently under the mouse (for hover highlight)
    Part _part{Part::None};
    // Region a mouse-down started in, while the button stays held
    Part _draggingPart{Part::None};
    int _sliderWidth{0};
    int _sliderPos{0};
    // Offset from the slider's left edge to where a drag grabbed it
    int _sliderDeltaMouseDownPos{0};

    int _leftRightBoxWidth{0};
    // Font-derived dimensions (see setArrows())
    int _scrollBarHeight{0};
    int _arrowLength{0};
    int _arrowBreadth{0};
    int _arrowThickness{0};

  private:
    // Following constructors and assignment operators not supported
    ScrollBarHWidget() = delete;
    ScrollBarHWidget(const ScrollBarHWidget&) = delete;
    ScrollBarHWidget(ScrollBarHWidget&&) = delete;
    ScrollBarHWidget& operator=(const ScrollBarHWidget&) = delete;
    ScrollBarHWidget& operator=(ScrollBarHWidget&&) = delete;
};

#endif  // SCROLL_BAR_H_WIDGET_HXX
