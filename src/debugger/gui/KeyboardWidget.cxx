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
#include "KeyboardWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
KeyboardWidget::KeyboardWidget(GuiObject* boss, const GUI::Font& font,
                               int x, int y, Controller& controller)
  : ControllerWidget(boss, font, x, y, controller)
{
  bool leftport = myController.jack() == Controller::Left;
  const string& label = leftport ? "Left (Keyboard):" : "Right (Keyboard):";

  const int fontHeight = font.getFontHeight();
  int xpos = x, ypos = y, lwidth = font.getStringWidth("Right (Keyboard):");
  StaticTextWidget* t;

  t = new StaticTextWidget(boss, font, xpos, ypos+2, lwidth,
                           fontHeight, label, kTextAlignLeft);

  xpos += 30;  ypos += t->getHeight() + 10;

  for(int i = 0; i < 12; ++i)
  {
    myBox[i] = new CheckboxWidget(boss, font, xpos, ypos, "", kCheckActionCmd);
    myBox[i]->setID(i);
    myBox[i]->setTarget(this);
    xpos += myBox[i]->getWidth() + 5;
    if((i+1) % 3 == 0)
    {
      xpos = x + 30;
      ypos += myBox[i]->getHeight() + 5;
    }
  }
  myEvent = leftport ? ourLeftEvents : ourRightEvents;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
KeyboardWidget::~KeyboardWidget()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void KeyboardWidget::loadConfig()
{
  const Event& event = instance().eventHandler().event();
  for(int i = 0; i < 12; ++i)
    myBox[i]->setState(event.get(myEvent[i]));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void KeyboardWidget::handleCommand(
    CommandSender* sender, int cmd, int data, int id)
{
  if(cmd == kCheckActionCmd)
    instance().eventHandler().handleEvent(myEvent[id], myBox[id]->getState());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Event::Type KeyboardWidget::ourLeftEvents[12] = {
  Event::KeyboardZero1,    Event::KeyboardZero2,  Event::KeyboardZero3,
  Event::KeyboardZero4,    Event::KeyboardZero5,  Event::KeyboardZero6,
  Event::KeyboardZero7,    Event::KeyboardZero8,  Event::KeyboardZero9,
  Event::KeyboardZeroStar, Event::KeyboardZero0,  Event::KeyboardZeroPound
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Event::Type KeyboardWidget::ourRightEvents[12] = {
  Event::KeyboardOne1,    Event::KeyboardOne2,  Event::KeyboardOne3,
  Event::KeyboardOne4,    Event::KeyboardOne5,  Event::KeyboardOne6,
  Event::KeyboardOne7,    Event::KeyboardOne8,  Event::KeyboardOne9,
  Event::KeyboardOneStar, Event::KeyboardOne0,  Event::KeyboardOnePound
};
