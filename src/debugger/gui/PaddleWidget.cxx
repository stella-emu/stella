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
#include "PaddleWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PaddleWidget::PaddleWidget(GuiObject* boss, const GUI::Font& font,
                           int x, int y, Controller& controller)
  : ControllerWidget(boss, font, x, y, controller)
{
  bool leftport = myController.jack() == Controller::Left;
  const string& label = leftport ? "Left (Paddles):" : "Right (Paddles):";

  const int fontWidth  = font.getMaxCharWidth(),
            fontHeight = font.getFontHeight(),
            lineHeight = font.getLineHeight();
  int xpos = x, ypos = y, lwidth = font.getStringWidth("Right (Paddles):");

  new StaticTextWidget(boss, font, xpos, ypos+2, lwidth,
                       fontHeight, label, kTextAlignLeft);

  ypos += lineHeight + 10;
  const string& p0string = leftport ? "P0 pot: " : "P2 pot: ";
  const string& p1string = leftport ? "P1 pot: " : "P3 pot: ";
  lwidth = font.getStringWidth("P3 pot: ");
  myP0Resistance =
    new SliderWidget(boss, font, xpos, ypos, 10*fontWidth, lineHeight,
                     p0string, lwidth, kP0Changed);
  myP0Resistance->setMinValue(0); myP0Resistance->setMaxValue(1400000);
  myP0Resistance->setStepValue(1400000/100);
  myP0Resistance->setTarget(this);

  xpos += 20;  ypos += myP0Resistance->getHeight() + 4;
  myP0Fire = new CheckboxWidget(boss, font, xpos, ypos,
                      "Fire", kP0Fire);
  myP0Fire->setTarget(this);

  xpos = x;  ypos += 2*lineHeight;
  myP1Resistance =
    new SliderWidget(boss, font, xpos, ypos, 10*fontWidth, lineHeight,
                     p1string, lwidth, kP1Changed);
  myP1Resistance->setMinValue(0); myP1Resistance->setMaxValue(1400000);
  myP1Resistance->setStepValue(1400000/100);
  myP1Resistance->setTarget(this);

  xpos += 20;  ypos += myP1Resistance->getHeight() + 4;
  myP1Fire = new CheckboxWidget(boss, font, xpos, ypos,
                      "Fire", kP1Fire);
  myP1Fire->setTarget(this);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PaddleWidget::~PaddleWidget()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PaddleWidget::loadConfig()
{
  myP0Resistance->setValue(1400000 - (Int32)myController.read(Controller::Nine));
  myP1Resistance->setValue(1400000 - (Int32)myController.read(Controller::Five));
  myP0Fire->setState(!myController.read(Controller::Four));
  myP1Fire->setState(!myController.read(Controller::Three));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PaddleWidget::handleCommand(
    CommandSender* sender, int cmd, int data, int id)
{
  switch(cmd)
  {
    case kP0Changed:
      myController.set(Controller::Nine, 1400000 - myP0Resistance->getValue());
      break;
    case kP1Changed:
      myController.set(Controller::Five, 1400000 - myP1Resistance->getValue());
      break;
    case kP0Fire:
      myController.set(Controller::Four, !myP0Fire->getState());
      break;
    case kP1Fire:
      myController.set(Controller::Three, !myP1Fire->getState());
      break;
  }
}
