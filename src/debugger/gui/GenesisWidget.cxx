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
#include "GenesisWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
GenesisWidget::GenesisWidget(GuiObject* boss, const GUI::Font& font,
                             int x, int y, Controller& controller)
  : ControllerWidget(boss, font, x, y, controller)
{
  bool leftport = myController.jack() == Controller::Left;
  const string& label = leftport ? "Left (Genesis):" : "Right (Genesis):";

  const int fontHeight = font.getFontHeight();
  int xpos = x, ypos = y, lwidth = font.getStringWidth("Right (Genesis):");
  StaticTextWidget* t;

  t = new StaticTextWidget(boss, font, xpos, ypos+2, lwidth,
                           fontHeight, label, kTextAlignLeft);
  xpos += t->getWidth()/2 - 5;  ypos += t->getHeight() + 20;
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
  ypos = 30 + (myPins[kJUp]->getHeight() + 10) * 3;
  myPins[kJBbtn] = new CheckboxWidget(boss, font, xpos, ypos, "B button", kCheckActionCmd);
  myPins[kJBbtn]->setID(kJBbtn);
  myPins[kJBbtn]->setTarget(this);

  ypos += myPins[kJBbtn]->getHeight() + 5;
  myPins[kJCbtn] = new CheckboxWidget(boss, font, xpos, ypos, "C button", kCheckActionCmd);
  myPins[kJCbtn]->setID(kJCbtn);
  myPins[kJCbtn]->setTarget(this);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
GenesisWidget::~GenesisWidget()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GenesisWidget::loadConfig()
{
  myPins[kJUp]->setState(!myController.read(ourPinNo[kJUp]));
  myPins[kJDown]->setState(!myController.read(ourPinNo[kJDown]));
  myPins[kJLeft]->setState(!myController.read(ourPinNo[kJLeft]));
  myPins[kJRight]->setState(!myController.read(ourPinNo[kJRight]));
  myPins[kJBbtn]->setState(!myController.read(ourPinNo[kJBbtn]));

  myPins[kJCbtn]->setState(
    myController.read(Controller::Five) == Controller::maximumResistance);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GenesisWidget::handleCommand(
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
      case kJBbtn:
        myController.set(ourPinNo[id], !myPins[id]->getState());
        break;
      case kJCbtn:
        myController.set(Controller::Five,
          myPins[id]->getState() ? Controller::maximumResistance :
                                   Controller::minimumResistance);
        break;
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Controller::DigitalPin GenesisWidget::ourPinNo[5] = {
  Controller::One, Controller::Two, Controller::Three, Controller::Four,
  Controller::Six
};
