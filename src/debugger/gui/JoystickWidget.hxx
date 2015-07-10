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

#ifndef JOYSTICK_WIDGET_HXX
#define JOYSTICK_WIDGET_HXX

#include "Control.hxx"
#include "Event.hxx"
#include "ControllerWidget.hxx"

class JoystickWidget : public ControllerWidget
{
  public:
    JoystickWidget(GuiObject* boss, const GUI::Font& font, int x, int y,
                   Controller& controller);
    virtual ~JoystickWidget();

  private:
    enum { kJUp = 0, kJDown, kJLeft, kJRight, kJFire };

    CheckboxWidget* myPins[5];
    static Controller::DigitalPin ourPinNo[5];

  private:
    void loadConfig() override;
    void handleCommand(CommandSender* sender, int cmd, int data, int id) override;

    // Following constructors and assignment operators not supported
    JoystickWidget() = delete;
    JoystickWidget(const JoystickWidget&) = delete;
    JoystickWidget(JoystickWidget&&) = delete;
    JoystickWidget& operator=(const JoystickWidget&) = delete;
    JoystickWidget& operator=(JoystickWidget&&) = delete;
};

#endif
