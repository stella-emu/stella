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
// Copyright (c) 1995-2021 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#ifndef KEYBOARD_WIDGET_HXX
#define KEYBOARD_WIDGET_HXX

#include "Event.hxx"
#include "ControllerWidget.hxx"

class KeyboardWidget : public ControllerWidget
{
  public:
    KeyboardWidget(GuiObject* boss, const GUI::Font& font, int x, int y,
                   Controller& controller);
    ~KeyboardWidget() override = default;

  private:
    std::array<CheckboxWidget*, 12> myBox{nullptr};
    const Event::Type* myEvent{nullptr};

    static constexpr std::array<Event::Type, 12> ourLeftEvents = {{
      Event::KeyboardZero1,    Event::KeyboardZero2,  Event::KeyboardZero3,
      Event::KeyboardZero4,    Event::KeyboardZero5,  Event::KeyboardZero6,
      Event::KeyboardZero7,    Event::KeyboardZero8,  Event::KeyboardZero9,
      Event::KeyboardZeroStar, Event::KeyboardZero0,  Event::KeyboardZeroPound
    }};
    static constexpr std::array<Event::Type, 12> ourRightEvents = {{
      Event::KeyboardOne1,    Event::KeyboardOne2,  Event::KeyboardOne3,
      Event::KeyboardOne4,    Event::KeyboardOne5,  Event::KeyboardOne6,
      Event::KeyboardOne7,    Event::KeyboardOne8,  Event::KeyboardOne9,
      Event::KeyboardOneStar, Event::KeyboardOne0,  Event::KeyboardOnePound
    }};

  private:
    void loadConfig() override;
    void handleCommand(CommandSender* sender, int cmd, int data, int id) override;

    // Following constructors and assignment operators not supported
    KeyboardWidget() = delete;
    KeyboardWidget(const KeyboardWidget&) = delete;
    KeyboardWidget(KeyboardWidget&&) = delete;
    KeyboardWidget& operator=(const KeyboardWidget&) = delete;
    KeyboardWidget& operator=(KeyboardWidget&&) = delete;
};

#endif
