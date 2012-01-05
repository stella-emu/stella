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
// Copyright (c) 1995-2012 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

#include "OSystem.hxx"
#include "EventHandler.hxx"
#include "JoystickWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
JoystickWidget::JoystickWidget(GuiObject* boss, const GUI::Font& font,
                               int x, int y, Controller& controller)
  : ControllerWidget(boss, font, x, y, controller)
{
  bool leftport = _controller.jack() == Controller::Left;
  if(leftport)
  {
    myPinEvent[kJUp]    = Event::JoystickZeroUp;
    myPinEvent[kJDown]  = Event::JoystickZeroDown;
    myPinEvent[kJLeft]  = Event::JoystickZeroLeft;
    myPinEvent[kJRight] = Event::JoystickZeroRight;
    myPinEvent[kJFire]  = Event::JoystickZeroFire1;
  }
  else
  {
    myPinEvent[kJUp]    = Event::JoystickOneUp;
    myPinEvent[kJDown]  = Event::JoystickOneDown;
    myPinEvent[kJLeft]  = Event::JoystickOneLeft;
    myPinEvent[kJRight] = Event::JoystickOneRight;
    myPinEvent[kJFire]  = Event::JoystickOneFire1;
  }
  const string& label = leftport ? "Left (Joystick):" : "Right (Joystick):";

  const int fontWidth  = font.getMaxCharWidth(),
            fontHeight = font.getFontHeight(),
            lineHeight = font.getLineHeight();
  int xpos = x, ypos = y, lwidth = font.getStringWidth("Right (Joystick):");
  StaticTextWidget* t;

  t = new StaticTextWidget(boss, font, xpos, ypos+2, lwidth,
                           fontHeight, label, kTextAlignLeft);
  xpos += t->getWidth()/2 - 5;  ypos += t->getHeight() + 5;
  myPins[kJUp] = new CheckboxWidget(boss, font, xpos, ypos, "", kCheckActionCmd);
  myPins[kJUp]->setID(kJUp);
  myPins[kJUp]->setTarget(this);
  addFocusWidget(myPins[kJUp]);

  ypos += myPins[kJUp]->getHeight() * 2 + 10;
  myPins[kJDown] = new CheckboxWidget(boss, font, xpos, ypos, "", kCheckActionCmd);
  myPins[kJDown]->setID(kJDown);
  myPins[kJDown]->setTarget(this);
  addFocusWidget(myPins[kJDown]);

  xpos -= myPins[kJUp]->getWidth() + 5;
  ypos -= myPins[kJUp]->getHeight() + 5;
  myPins[kJLeft] = new CheckboxWidget(boss, font, xpos, ypos, "", kCheckActionCmd);
  myPins[kJLeft]->setID(kJLeft);
  myPins[kJLeft]->setTarget(this);
  addFocusWidget(myPins[kJLeft]);

  _w = xpos;

  xpos += (myPins[kJUp]->getWidth() + 5) * 2;
  myPins[kJRight] = new CheckboxWidget(boss, font, xpos, ypos, "", kCheckActionCmd);
  myPins[kJRight]->setID(kJRight);
  myPins[kJRight]->setTarget(this);
  addFocusWidget(myPins[kJRight]);

  xpos -= (myPins[kJUp]->getWidth() + 5) * 2;
  ypos = 20 + (myPins[kJUp]->getHeight() + 10) * 3;
  myPins[kJFire] = new CheckboxWidget(boss, font, xpos, ypos, "Fire", kCheckActionCmd);
  myPins[kJFire]->setID(kJFire);
  myPins[kJFire]->setTarget(this);
  addFocusWidget(myPins[kJFire]);

  _h = ypos;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
JoystickWidget::~JoystickWidget()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void JoystickWidget::loadConfig()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void JoystickWidget::handleCommand(
    CommandSender* sender, int cmd, int data, int id)
{
}
