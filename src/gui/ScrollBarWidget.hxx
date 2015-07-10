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
// Copyright (c) 1995-2015 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

#ifndef SCROLL_BAR_WIDGET_HXX
#define SCROLL_BAR_WIDGET_HXX

class GuiObject;

#include "Widget.hxx"
#include "Command.hxx"
#include "bspf.hxx"

enum {
  kScrollBarWidth = 14
};

class ScrollBarWidget : public Widget, public CommandSender
{
  public:
    ScrollBarWidget(GuiObject* boss, const GUI::Font& font,
                    int x, int y, int w, int h);

    void recalc();
    void handleMouseWheel(int x, int y, int direction) override;

    static void setWheelLines(int lines) { _WHEEL_LINES = lines; }
    static int  getWheelLines()          { return _WHEEL_LINES;  }

  private:
    void drawWidget(bool hilite) override;
    void checkBounds(int old_pos);

    void handleMouseDown(int x, int y, int button, int clickCount) override;
    void handleMouseUp(int x, int y, int button, int clickCount) override;
    void handleMouseMoved(int x, int y, int button) override;
    bool handleMouseClicks(int x, int y, int button) override;
    void handleMouseEntered(int button) override;
    void handleMouseLeft(int button) override;

  public:
    int _numEntries;
    int _entriesPerPage;
    int _currentPos;
    int _wheel_lines;

  private:
    enum Part {
      kNoPart,
      kUpArrowPart,
      kDownArrowPart,
      kSliderPart,
      kPageUpPart,
      kPageDownPart
    };

    Part _part;
    Part _draggingPart;
    int _sliderHeight;
    int _sliderPos;
    int _sliderDeltaMouseDownPos;

    static int _WHEEL_LINES;

  private:
    // Following constructors and assignment operators not supported
    ScrollBarWidget() = delete;
    ScrollBarWidget(const ScrollBarWidget&) = delete;
    ScrollBarWidget(ScrollBarWidget&&) = delete;
    ScrollBarWidget& operator=(const ScrollBarWidget&) = delete;
    ScrollBarWidget& operator=(ScrollBarWidget&&) = delete;
};

#endif
