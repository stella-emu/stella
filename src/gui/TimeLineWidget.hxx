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
// Copyright (c) 1995-2018 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#ifndef TIMELINE_WIDGET_HXX
#define TIMELINE_WIDGET_HXX

#include "Widget.hxx"

class TimeLineWidget : public ButtonWidget
{
  public:
    TimeLineWidget(GuiObject* boss, const GUI::Font& font,
                   int x, int y, int w, int h, const string& label = "",
                   int labelWidth = 0, int cmd = 0);

    void setValue(int value);
    int getValue() const      { return _value; }

    void setMinValue(int value);
    int  getMinValue() const      { return _valueMin; }
    void setMaxValue(int value);
    int  getMaxValue() const      { return _valueMax; }
#if 0
    void setStepValue(int value);
    int  getStepValue() const     { return _stepValue; }
#endif

  protected:
    void handleMouseMoved(int x, int y) override;
    void handleMouseDown(int x, int y, MouseButton b, int clickCount) override;
    void handleMouseUp(int x, int y, MouseButton b, int clickCount) override;
    void handleMouseWheel(int x, int y, int direction) override;
    bool handleEvent(Event::Type event) override;

    void drawWidget(bool hilite) override;

    int valueToPos(int value);
    int posToValue(int pos);

  protected:
    int  _value, _stepValue;
    int  _valueMin, _valueMax;
    bool _isDragging;
    int  _labelWidth;

  private:
    // Following constructors and assignment operators not supported
    TimeLineWidget() = delete;
    TimeLineWidget(const TimeLineWidget&) = delete;
    TimeLineWidget(TimeLineWidget&&) = delete;
    TimeLineWidget& operator=(const TimeLineWidget&) = delete;
    TimeLineWidget& operator=(TimeLineWidget&&) = delete;
};

#endif
