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

#include "JoystickWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
JoystickWidget::JoystickWidget(GuiObject* boss, const GUI::Font& font,
                               int x, int y, Controller& controller)
  : ControllerWidget(boss, font, x, y, controller)
{
  const string& label = getHeader();
  const int fontHeight = font.getFontHeight();
  int xpos = x, ypos = y, lwidth = font.getStringWidth("Right (Joystick)");
  StaticTextWidget* t;

  t = new StaticTextWidget(boss, font, xpos, ypos+2, lwidth,
                           fontHeight, label, TextAlign::Left);
  xpos += t->getWidth()/2 - 5;  ypos += t->getHeight() + 20;
  myPins[kJUp] = new CheckboxWidget(boss, font, xpos, ypos, "",
                                    CheckboxWidget::kCheckActionCmd);
  myPins[kJUp]->setID(kJUp);
  myPins[kJUp]->setTarget(this);

  ypos += myPins[kJUp]->getHeight() * 2 + 10;
  myPins[kJDown] = new CheckboxWidget(boss, font, xpos, ypos, "",
                                      CheckboxWidget::kCheckActionCmd);
  myPins[kJDown]->setID(kJDown);
  myPins[kJDown]->setTarget(this);

  xpos -= myPins[kJUp]->getWidth() + 5;
  ypos -= myPins[kJUp]->getHeight() + 5;
  myPins[kJLeft] = new CheckboxWidget(boss, font, xpos, ypos, "",
                                      CheckboxWidget::kCheckActionCmd);
  myPins[kJLeft]->setID(kJLeft);
  myPins[kJLeft]->setTarget(this);

  xpos += (myPins[kJUp]->getWidth() + 5) * 2;
  myPins[kJRight] = new CheckboxWidget(boss, font, xpos, ypos, "",
                                       CheckboxWidget::kCheckActionCmd);
  myPins[kJRight]->setID(kJRight);
  myPins[kJRight]->setTarget(this);

  xpos -= (myPins[kJUp]->getWidth() + 5) * 2;
  ypos = 30 + (myPins[kJUp]->getHeight() + 10) * 3;
  myPins[kJFire] = new CheckboxWidget(boss, font, xpos, ypos, "Fire",
                                      CheckboxWidget::kCheckActionCmd);
  myPins[kJFire]->setID(kJFire);
  myPins[kJFire]->setTarget(this);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void JoystickWidget::loadConfig()
{
  myPins[kJUp]->setState(!myController.read(ourPinNo[kJUp]));
  myPins[kJDown]->setState(!myController.read(ourPinNo[kJDown]));
  myPins[kJLeft]->setState(!myController.read(ourPinNo[kJLeft]));
  myPins[kJRight]->setState(!myController.read(ourPinNo[kJRight]));
  myPins[kJFire]->setState(!myController.read(ourPinNo[kJFire]));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void JoystickWidget::handleCommand(
    CommandSender* sender, int cmd, int data, int id)
{
  if(cmd == CheckboxWidget::kCheckActionCmd)
    myController.set(ourPinNo[id], !myPins[id]->getState());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Controller::DigitalPin JoystickWidget::ourPinNo[5] = {
  Controller::One, Controller::Two, Controller::Three, Controller::Four,
  Controller::Six
};
