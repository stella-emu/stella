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
#include "BoosterWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BoosterWidget::BoosterWidget(GuiObject* boss, const GUI::Font& font,
                             int x, int y, Controller& controller)
  : ControllerWidget(boss, font, x, y, controller)
{
  bool leftport = myController.jack() == Controller::Left;
  const string& label = leftport ? "Left (Booster):" : "Right (Booster):";

  const int fontHeight = font.getFontHeight();
  int xpos = x, ypos = y, lwidth = font.getStringWidth("Right (Booster):");
  StaticTextWidget* t;

  t = new StaticTextWidget(boss, font, xpos, ypos+2, lwidth,
                           fontHeight, label, kTextAlignLeft);
  xpos += t->getWidth()/2 - 5;  ypos += t->getHeight() + 10;
  myPins[kJUp] = new CheckboxWidget(boss, font, xpos, ypos, "", kCheckActionCmd);
  myPins[kJUp]->setID(kJUp);
  myPins[kJUp]->setTarget(this);

  ypos += myPins[kJUp]->getHeight() * 2 + 10;
  myPins[kJDown] = new CheckboxWidget(boss, font, xpos, ypos, "", kCheckActionCmd);
  myPins[kJDown]->setID(kJDown);
  myPins[kJDown]->setTarget(this);

  xpos -= myPins[kJUp]->getWidth() + 5;
  ypos -= myPins[kJUp]->getHeight() + 5;
  myPins[kJLeft] = new CheckboxWidget(boss, font, xpos, ypos, "", kCheckActionCmd);
  myPins[kJLeft]->setID(kJLeft);
  myPins[kJLeft]->setTarget(this);

  xpos += (myPins[kJUp]->getWidth() + 5) * 2;
  myPins[kJRight] = new CheckboxWidget(boss, font, xpos, ypos, "", kCheckActionCmd);
  myPins[kJRight]->setID(kJRight);
  myPins[kJRight]->setTarget(this);

  xpos -= (myPins[kJUp]->getWidth() + 5) * 2;
  ypos = 20 + (myPins[kJUp]->getHeight() + 10) * 3;
  myPins[kJFire] = new CheckboxWidget(boss, font, xpos, ypos, "Fire", kCheckActionCmd);
  myPins[kJFire]->setID(kJFire);
  myPins[kJFire]->setTarget(this);

  ypos += myPins[kJFire]->getHeight() + 5;
  myPins[kJBooster] = new CheckboxWidget(boss, font, xpos, ypos, "Booster", kCheckActionCmd);
  myPins[kJBooster]->setID(kJBooster);
  myPins[kJBooster]->setTarget(this);

  ypos += myPins[kJBooster]->getHeight() + 5;
  myPins[kJTrigger] = new CheckboxWidget(boss, font, xpos, ypos, "Trigger", kCheckActionCmd);
  myPins[kJTrigger]->setID(kJTrigger);
  myPins[kJTrigger]->setTarget(this);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BoosterWidget::~BoosterWidget()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BoosterWidget::loadConfig()
{
  myPins[kJUp]->setState(!myController.read(ourPinNo[kJUp]));
  myPins[kJDown]->setState(!myController.read(ourPinNo[kJDown]));
  myPins[kJLeft]->setState(!myController.read(ourPinNo[kJLeft]));
  myPins[kJRight]->setState(!myController.read(ourPinNo[kJRight]));
  myPins[kJFire]->setState(!myController.read(ourPinNo[kJFire]));

  myPins[kJBooster]->setState(
    myController.read(Controller::Five) == Controller::minimumResistance);
  myPins[kJTrigger]->setState(
    myController.read(Controller::Nine) == Controller::minimumResistance);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BoosterWidget::handleCommand(
    CommandSender* sender, int cmd, int data, int id)
{
  if(cmd == kCheckActionCmd)
  {
    switch(id)
    {
      case kJUp:
      case kJDown:
      case kJLeft:
      case kJRight:
      case kJFire:
        myController.set(ourPinNo[id], !myPins[id]->getState());
        break;
      case kJBooster:
        myController.set(Controller::Five,
          myPins[id]->getState() ? Controller::minimumResistance :
                                   Controller::maximumResistance);
        break;
      case kJTrigger:
        myController.set(Controller::Nine,
          myPins[id]->getState() ? Controller::minimumResistance :
                                   Controller::maximumResistance);
        break;
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Controller::DigitalPin BoosterWidget::ourPinNo[5] = {
  Controller::One, Controller::Two, Controller::Three, Controller::Four,
  Controller::Six
};
