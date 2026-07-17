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

#include "Layout.hxx"

#include "BoosterWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BoosterWidget::BoosterWidget(GuiObject* boss, const GUI::Font& font,
                             int x, int y, Controller& controller)
  : ControllerWidget(boss, font, x, y, controller)
{
  // Create the pins at a placeholder position; reflow() lays them out
  // NOLINTBEGIN(cppcoreguidelines-prefer-member-initializer)
  const auto pin = [&](int id, string_view label) {
    myPins[id] = new CheckboxWidget(boss, font, 0, 0, label,
                                    CheckboxWidget::kCheckActionCmd);
    myPins[id]->setID(id);
    myPins[id]->setTarget(this);
  };
  pin(kJUp, "");  pin(kJDown, "");  pin(kJLeft, "");  pin(kJRight, "");
  pin(kJFire, "Fire");  pin(kJBooster, "Booster");  pin(kJTrigger, "Trigger");
  // NOLINTEND(cppcoreguidelines-prefer-member-initializer)

  addFocusWidget(myPins[kJUp]);
  addFocusWidget(myPins[kJLeft]);
  addFocusWidget(myPins[kJRight]);
  addFocusWidget(myPins[kJDown]);
  addFocusWidget(myPins[kJFire]);
  addFocusWidget(myPins[kJBooster]);
  addFocusWidget(myPins[kJTrigger]);

  createHeader();
  reflow();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BoosterWidget::layoutContent(GUI::BoxLayout& col)
{
  using GUI::BoxLayout;
  using GUI::anchoredItem;
  using Dir = BoxLayout::Dir;

  const int VGAP = _font.getFontHeight() / 4;

  // The shared cross, with the labeled buttons left-aligned below it; the whole
  // unit is centered so the buttons line up with the cross's left column
  auto unit = std::make_unique<BoxLayout>(Dir::Vertical, VGAP);
  unit->addAuto(layoutCross(myPins[kJUp], myPins[kJDown],
                            myPins[kJLeft], myPins[kJRight]));
  unit->addAuto(anchoredItem(myPins[kJFire]));
  unit->addAuto(anchoredItem(myPins[kJBooster]));
  unit->addAuto(anchoredItem(myPins[kJTrigger]));

  auto row = std::make_unique<BoxLayout>(Dir::Horizontal);
  row->addStretchSpace();
  row->addAuto(std::move(unit));
  row->addStretchSpace();
  col.addAuto(std::move(row));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BoosterWidget::loadConfig()
{
  myPins[kJUp]->setState(!getPin(ourPinNo[kJUp]));
  myPins[kJDown]->setState(!getPin(ourPinNo[kJDown]));
  myPins[kJLeft]->setState(!getPin(ourPinNo[kJLeft]));
  myPins[kJRight]->setState(!getPin(ourPinNo[kJRight]));
  myPins[kJFire]->setState(!getPin(ourPinNo[kJFire]));

  myPins[kJBooster]->setState(
    getPin(Controller::AnalogPin::Five) == AnalogReadout::connectToVcc());
  myPins[kJTrigger]->setState(
    getPin(Controller::AnalogPin::Nine) == AnalogReadout::connectToVcc());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BoosterWidget::handleCommand(
    CommandSender* sender, int cmd, int data, int id)
{
  if(cmd == CheckboxWidget::kCheckActionCmd)
  {
    switch(id)
    {
      case kJUp:
      case kJDown:
      case kJLeft:
      case kJRight:
      case kJFire:
        setPin(ourPinNo[id], !myPins[id]->getState());
        break;
      case kJBooster:
        setPin(Controller::AnalogPin::Five,
          myPins[id]->getState() ? AnalogReadout::connectToVcc() :
                                   AnalogReadout::disconnect());
        break;
      case kJTrigger:
        setPin(Controller::AnalogPin::Nine,
          myPins[id]->getState() ? AnalogReadout::connectToVcc() :
                                   AnalogReadout::disconnect());
        break;
      default:
        break;
    }
  }
}
