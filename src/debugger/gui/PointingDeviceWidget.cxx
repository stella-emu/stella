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

#include "PointingDevice.hxx"
#include "DataGridWidget.hxx"
#include "Layout.hxx"
#include "PointingDeviceWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PointingDeviceWidget::PointingDeviceWidget(GuiObject* boss, const GUI::Font& font,
      Controller& controller)
  : ControllerWidget(boss, font, controller)
{
  // Create the controls at a placeholder position; reflow() lays them out
  // NOLINTBEGIN(cppcoreguidelines-prefer-member-initializer)
  const auto grayValue = [&]() {
    auto* g = new DataGridWidget(boss, font, 1, 1, 2, 8,
                                 Common::Base::Fmt::_16);
    g->setTarget(this);
    g->setEditable(false);
    return g;
  };
  const auto button = [&](string_view label, int cmd) {
    auto* b = new ButtonWidget(boss, font, label, cmd);
    b->setTarget(this);
    return b;
  };
  myGrayValueV = grayValue();
  myGrayUp     = button("+", kTBUp);
  myGrayLeft   = button("-", kTBLeft);
  myGrayRight  = button("+", kTBRight);
  myGrayValueH = grayValue();
  myGrayDown   = button("-", kTBDown);
  myFire = new CheckboxWidget(boss, font, "Fire", kTBFire);
  myFire->setTarget(this);
  // NOLINTEND(cppcoreguidelines-prefer-member-initializer)

  addFocusWidget(myGrayUp);
  addFocusWidget(myGrayLeft);
  addFocusWidget(myGrayRight);
  addFocusWidget(myGrayDown);
  addFocusWidget(myFire);

  createHeader();
  reflow();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PointingDeviceWidget::layoutContent(GUI::BoxLayout& col)
{
  using GUI::BoxLayout;
  using GUI::GridLayout;
  using GUI::centeredItem;
  using GUI::anchoredItem;
  using Dir = BoxLayout::Dir;

  const int VGAP = _font.getFontHeight() / 4;

  // The +/- buttons hold a single character, so keep them small
  const int bWidth = _font.getMaxCharWidth() * 2;
  for(auto* b: {myGrayUp, myGrayDown, myGrayLeft, myGrayRight})
    b->setWidth(bWidth);

  // A cross of +/- buttons with the gray-code readouts: the vertical value sits
  // above Up, the horizontal value beside Right, and Fire below.  Every cell is
  // centered so the cross stays symmetric though the value grids are wider
  auto grid = std::make_unique<GridLayout>(4, 5, VGAP, VGAP);
  for(int c = 0; c < 4; ++c)
    grid->columnAuto(c);
  for(int r = 0; r < 5; ++r)
    grid->rowAuto(r);
  grid->place(1, 0, centeredItem(myGrayValueV));
  grid->place(1, 1, centeredItem(myGrayUp));
  grid->place(0, 2, centeredItem(myGrayLeft));
  grid->place(2, 2, centeredItem(myGrayRight));
  grid->place(3, 2, centeredItem(myGrayValueH));
  grid->place(1, 3, centeredItem(myGrayDown));
  grid->place(0, 4, anchoredItem(myFire), 4);

  auto row = std::make_unique<BoxLayout>(Dir::Horizontal);
  row->addStretchSpace();
  row->addAuto(std::move(grid));
  row->addStretchSpace();
  col.addAuto(std::move(row));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PointingDeviceWidget::loadConfig()
{
  setGrayCodeH();
  setGrayCodeV();
  myFire->setState(!getPin(Controller::DigitalPin::Six));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PointingDeviceWidget::handleCommand(CommandSender* sender, int cmd, int data, int id)
{
  // Since the PointingDevice uses its own, internal state (not reading the
  // controller), we have to communicate directly with it
  auto& pDev = static_cast<PointingDevice&>(controller());

  switch(cmd)
  {
    case kTBLeft:
      ++pDev.myCountH;
      pDev.myTrackBallLeft = false;
      setGrayCodeH();
      break;
    case kTBRight:
      --pDev.myCountH;
      pDev.myTrackBallLeft = true;
      setGrayCodeH();
      break;
    case kTBUp:
      ++pDev.myCountV;
      pDev.myTrackBallDown = true;
      setGrayCodeV();
      break;
    case kTBDown:
      --pDev.myCountV;
      pDev.myTrackBallDown = false;
      setGrayCodeV();
      break;
    case kTBFire:
      setPin(Controller::DigitalPin::Six, !myFire->getState());
      break;
    default:
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PointingDeviceWidget::setGrayCodeH()
{
  auto& pDev = static_cast<PointingDevice&>(controller());

  pDev.myCountH &= 0b11;
  setValue(myGrayValueH, pDev.myCountH, pDev.myTrackBallLeft);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PointingDeviceWidget::setGrayCodeV()
{
  auto& pDev = static_cast<PointingDevice&>(controller());

  pDev.myCountV &= 0b11;
  setValue(myGrayValueV, pDev.myCountV, !pDev.myTrackBallDown);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PointingDeviceWidget::setValue(DataGridWidget* grayValue,
                                    int index, int direction)
{
  const uInt8 grayCode = getGrayCodeTable(index, direction);

  // FIXME  * 8 = a nasty hack, because the DataGridWidget does not support 2 digit binary output
  grayValue->setList(0, (grayCode & 0b01) + (grayCode & 0b10) * 8);
}
