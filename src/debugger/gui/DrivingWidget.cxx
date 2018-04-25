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

#include "DataGridWidget.hxx"
#include "DrivingWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DrivingWidget::DrivingWidget(GuiObject* boss, const GUI::Font& font,
                             int x, int y, Controller& controller)
  : ControllerWidget(boss, font, x, y, controller),
    myGrayIndex(0)
{
  const string& label = getHeader();

  const int fontHeight = font.getFontHeight(),
            bwidth = font.getStringWidth("Gray code +") + 10,
            bheight = font.getLineHeight() + 4;
  int xpos = x, ypos = y, lwidth = font.getStringWidth("Right (Driving)");
  StaticTextWidget* t;

  t = new StaticTextWidget(boss, font, xpos, ypos+2, lwidth,
                           fontHeight, label, TextAlign::Left);

  ypos += t->getHeight() + 20;
  myGrayUp = new ButtonWidget(boss, font, xpos, ypos, bwidth, bheight,
                              "Gray code +", kGrayUpCmd);
  myGrayUp->setTarget(this);

  ypos += myGrayUp->getHeight() + 5;
  myGrayDown = new ButtonWidget(boss, font, xpos, ypos, bwidth, bheight,
                                "Gray code -", kGrayDownCmd);
  myGrayDown->setTarget(this);

  xpos += myGrayDown->getWidth() + 10;  ypos -= 10;
  myGrayValue = new DataGridWidget(boss, font, xpos, ypos,
                                   1, 1, 2, 8, Common::Base::F_16);
  myGrayValue->setTarget(this);
  myGrayValue->setEditable(false);

  xpos = x + 30;  ypos += myGrayDown->getHeight() + 20;
  myFire = new CheckboxWidget(boss, font, xpos, ypos, "Fire", kFireCmd);
  myFire->setTarget(this);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DrivingWidget::loadConfig()
{
  uInt8 gray = 0;
  if(myController.read(Controller::One)) gray += 1;
  if(myController.read(Controller::Two)) gray += 2;

  for(myGrayIndex = 0; myGrayIndex < 4; ++myGrayIndex)
  {
    if(ourGrayTable[myGrayIndex] == gray)
    {
      setValue(myGrayIndex);
      break;
    }
  }

  myFire->setState(!myController.read(Controller::Six));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DrivingWidget::handleCommand(
    CommandSender* sender, int cmd, int data, int id)
{
  switch(cmd)
  {
    case kGrayUpCmd:
      myGrayIndex = (myGrayIndex + 1) % 4;
      myController.set(Controller::One, (ourGrayTable[myGrayIndex] & 0x1) != 0);
      myController.set(Controller::Two, (ourGrayTable[myGrayIndex] & 0x2) != 0);
      setValue(myGrayIndex);
      break;
    case kGrayDownCmd:
      myGrayIndex = myGrayIndex == 0 ? 3 : myGrayIndex - 1;
      myController.set(Controller::One, (ourGrayTable[myGrayIndex] & 0x1) != 0);
      myController.set(Controller::Two, (ourGrayTable[myGrayIndex] & 0x2) != 0);
      setValue(myGrayIndex);
      break;
    case kFireCmd:
      myController.set(Controller::Six, !myFire->getState());
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DrivingWidget::setValue(int idx)
{
  int grayCode = ourGrayTable[idx];
  // FIXME  * 8 = a nasty hack, because the DataGridWidget does not support 2 digit binary output
  myGrayValue->setList(0, (grayCode & 0b01) + (grayCode & 0b10) * 8);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 DrivingWidget::ourGrayTable[4] = { 0x03, 0x01, 0x00, 0x02 };
