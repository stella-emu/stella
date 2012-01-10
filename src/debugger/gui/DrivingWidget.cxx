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

#include "DataGridWidget.hxx"
#include "DrivingWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DrivingWidget::DrivingWidget(GuiObject* boss, const GUI::Font& font,
                             int x, int y, Controller& controller)
  : ControllerWidget(boss, font, x, y, controller),
    myGreyIndex(0)
{
  bool leftport = myController.jack() == Controller::Left;
  const string& label = leftport ? "Left (Driving):" : "Right (Driving):";

  const int fontHeight = font.getFontHeight(),
            bwidth = font.getStringWidth("Grey code +") + 10,
            bheight = font.getLineHeight() + 4;
  int xpos = x, ypos = y, lwidth = font.getStringWidth("Right (Driving):");
  StaticTextWidget* t;

  t = new StaticTextWidget(boss, font, xpos, ypos+2, lwidth,
                           fontHeight, label, kTextAlignLeft);

  ypos += t->getHeight() + 20;
  myGreyUp = new ButtonWidget(boss, font, xpos, ypos, bwidth, bheight,
                              "Grey code +", kGreyUpCmd);
  myGreyUp->setTarget(this);

  ypos += myGreyUp->getHeight() + 5;
  myGreyDown = new ButtonWidget(boss, font, xpos, ypos, bwidth, bheight,
                                "Grey code -", kGreyDownCmd);
  myGreyDown->setTarget(this);

  xpos += myGreyDown->getWidth() + 10;  ypos -= 10;
  myGreyValue = new DataGridWidget(boss, font, xpos, ypos,
                                   1, 1, 2, 8, kBASE_16);
  myGreyValue->setTarget(this);
  myGreyValue->setEditable(false);

  xpos = x + 30;  ypos += myGreyDown->getHeight() + 20;
  myFire = new CheckboxWidget(boss, font, xpos, ypos, "Fire", kFireCmd);
  myFire->setTarget(this);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DrivingWidget::~DrivingWidget()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DrivingWidget::loadConfig()
{
  uInt8 grey = 0;
  if(myController.read(Controller::One)) grey += 1;
  if(myController.read(Controller::Two)) grey += 2;

  for(myGreyIndex = 0; myGreyIndex < 4; ++myGreyIndex)
    if(ourGreyTable[myGreyIndex] == grey)
      break;

  myFire->setState(!myController.read(Controller::Six));
  myGreyValue->setList(0, grey);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DrivingWidget::handleCommand(
    CommandSender* sender, int cmd, int data, int id)
{
  switch(cmd)
  {
    case kGreyUpCmd:
      myGreyIndex = (myGreyIndex + 1) % 4;
      myController.set(Controller::One, (ourGreyTable[myGreyIndex] & 0x1) != 0);
      myController.set(Controller::Two, (ourGreyTable[myGreyIndex] & 0x2) != 0);
      myGreyValue->setList(0, ourGreyTable[myGreyIndex]);
      break;
    case kGreyDownCmd:
      myGreyIndex = myGreyIndex == 0 ? 3 : myGreyIndex - 1;
      myController.set(Controller::One, (ourGreyTable[myGreyIndex] & 0x1) != 0);
      myController.set(Controller::Two, (ourGreyTable[myGreyIndex] & 0x2) != 0);
      myGreyValue->setList(0, ourGreyTable[myGreyIndex]);
      break;
    case kFireCmd:
      myController.set(Controller::Six, !myFire->getState());
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 DrivingWidget::ourGreyTable[4] = { 0x03, 0x01, 0x00, 0x02 };