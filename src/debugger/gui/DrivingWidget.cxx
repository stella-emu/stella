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
// Copyright (c) 1995-2026 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include "DataGridWidget.hxx"
#include "Layout.hxx"
#include "DrivingWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DrivingWidget::DrivingWidget(GuiObject* boss, const GUI::Font& font,
                             int x, int y, Controller& controller, bool embedded)
  : ControllerWidget(boss, font, x, y, controller)
{
  // Create the controls at a placeholder position; reflow() lays them out.
  // Embedded in a QuadTari there is only room for short button labels
  // NOLINTBEGIN(cppcoreguidelines-prefer-member-initializer)
  myGrayUp = new ButtonWidget(boss, font, 0, 0,
                              embedded ? "GC+" : "Gray code +", kGrayUpCmd);
  myGrayUp->setTarget(this);
  myGrayDown = new ButtonWidget(boss, font, 0, 0,
                                embedded ? "GC-" : "Gray code -", kGrayDownCmd);
  myGrayDown->setTarget(this);

  myGrayValue = new DataGridWidget(boss, font, 0, 0, 1, 1, 2, 8,
                                   Common::Base::Fmt::_16);
  myGrayValue->setTarget(this);
  myGrayValue->setEditable(false);

  myFire = new CheckboxWidget(boss, font, 0, 0, "Fire", kFireCmd);
  myFire->setTarget(this);
  // NOLINTEND(cppcoreguidelines-prefer-member-initializer)

  addFocusWidget(myGrayUp);
  addFocusWidget(myGrayDown);
  addFocusWidget(myFire);

  if(!embedded)
    createHeader();
  reflow();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DrivingWidget::layoutContent(GUI::BoxLayout& col)
{
  using GUI::BoxLayout;
  using GUI::anchoredItem;
  using Dir = BoxLayout::Dir;

  const int VGAP = _font.getFontHeight() / 4;

  // The two Gray-code buttons share a width
  GUI::alignButtons({myGrayUp, myGrayDown});

  // The buttons stacked, with the current Gray-code value centered beside them
  auto buttons = std::make_unique<BoxLayout>(Dir::Vertical, VGAP);
  buttons->addAuto(anchoredItem(myGrayUp));
  buttons->addAuto(anchoredItem(myGrayDown));

  auto row = std::make_unique<BoxLayout>(Dir::Horizontal, _font.getMaxCharWidth());
  row->addAuto(std::move(buttons));
  row->addAuto(anchoredItem(myGrayValue));
  col.addAuto(std::move(row));

  col.addSpace(VGAP);
  col.addAuto(anchoredItem(myFire));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DrivingWidget::loadConfig()
{
  uInt8 gray = 0;
  if(getPin(Controller::DigitalPin::One)) gray += 1;
  if(getPin(Controller::DigitalPin::Two)) gray += 2;

  for(myGrayIndex = 0; myGrayIndex < 4; ++myGrayIndex)
  {
    if(ourGrayTable[myGrayIndex] == gray)
    {
      setValue(myGrayIndex);
      break;
    }
  }

  myFire->setState(!getPin(Controller::DigitalPin::Six));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DrivingWidget::handleCommand(
    CommandSender* sender, int cmd, int data, int id)
{
  switch(cmd)
  {
    case kGrayUpCmd:
      myGrayIndex = (myGrayIndex + 1) % 4;
      setPin(Controller::DigitalPin::One, (ourGrayTable[myGrayIndex] & 0x1) != 0);
      setPin(Controller::DigitalPin::Two, (ourGrayTable[myGrayIndex] & 0x2) != 0);
      setValue(myGrayIndex);
      break;
    case kGrayDownCmd:
      myGrayIndex = myGrayIndex == 0 ? 3 : myGrayIndex - 1;
      setPin(Controller::DigitalPin::One, (ourGrayTable[myGrayIndex] & 0x1) != 0);
      setPin(Controller::DigitalPin::Two, (ourGrayTable[myGrayIndex] & 0x2) != 0);
      setValue(myGrayIndex);
      break;
    case kFireCmd:
      setPin(Controller::DigitalPin::Six, !myFire->getState());
      break;
    default:
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DrivingWidget::setValue(int idx)
{
  const int grayCode = ourGrayTable[idx];
  // FIXME  * 8 = a nasty hack, because the DataGridWidget does not support 2 digit binary output
  myGrayValue->setList(0, (grayCode & 0b01) + (grayCode & 0b10) * 8);
}
