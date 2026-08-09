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

#ifndef TIME_LINE_WIDGET_HXX
#define TIME_LINE_WIDGET_HXX

#include "Widget.hxx"

/**
  A draggable scrubber/track control (like a slider, but with
  unevenly-spaced steps) used for the Time Machine's rewind timeline.

  @author  Stephen Anthony and Thomas Jentzsch
*/
class TimeLineWidget : public ButtonWidget
{
  public:
    TimeLineWidget(GuiObject* boss, const GUI::Font& font, GuiCmd::Code cmd = GuiCmd::None);
    ~TimeLineWidget() override = default;

    // Height of the timeline track, including its handle overhang
    static int calcHeight(const GUI::Font& font)
    {
      return font.getLineHeight() / 2 + 6;
    }

    // My height is mine (it follows my font); my WIDTH is the dialog's, and
    // TimeMachineDialog's layout re-applies it with HAlign::Fill
    void refreshFont() override;

    // Clamps to [min, max] and sends _cmd if the value actually changed
    void setValue(int value) override;
    uInt32 getValue() const { return _value; }

    void setMinValue(uInt32 value);
    void setMaxValue(uInt32 value);
    uInt32 getMinValue() const { return _valueMin; }
    uInt32 getMaxValue() const { return _valueMax; }

    /**
      Steps are not necessarily linear in a timeline, so we need info
      on each interval instead.
    */
    void setStepValues(const IntArray& steps);

    // Dragging the handle (mouse down + move) and the wheel both resolve to a
    // new value via setValue()
    void handleMouseMoved(int x, int y) override;
    void handleMouseDown(int x, int y, MouseButton b, int clickCount) override;
    void handleMouseUp(int x, int y, MouseButton b, int clickCount) override;
    void handleMouseWheel(int x, int y, int direction) override;

  protected:
    // Draws the track frame, filled bar, tickmarks, and handle
    void drawWidget(bool hilite) override;

    // Converts between a step value and its pixel offset along the track,
    // via the precomputed _stepValue table (steps are not evenly spaced)
    uInt32 valueToPos(uInt32 value) const;
    uInt32 posToValue(uInt32 pos) const;

  protected:
    // Current value
    uInt32  _value{0};
    uInt32  _valueMin{0}, _valueMax{0};
    // True while the handle is being dragged
    bool    _isDragging{false};

    // Pixel offset of each step along the track, set by setStepValues()
    uIntArray _stepValue;

  private:
    // Following constructors and assignment operators not supported
    TimeLineWidget() = delete;
    TimeLineWidget(const TimeLineWidget&) = delete;
    TimeLineWidget(TimeLineWidget&&) = delete;
    TimeLineWidget& operator=(const TimeLineWidget&) = delete;
    TimeLineWidget& operator=(TimeLineWidget&&) = delete;
};

#endif  // TIME_LINE_WIDGET_HXX
