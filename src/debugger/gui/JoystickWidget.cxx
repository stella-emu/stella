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

#include "JoystickWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
JoystickWidget::JoystickWidget(GuiObject* boss, const GUI::Font& font,
                               Controller& controller,
                               bool embedded)
  : ControllerWidget(boss, font, controller)
{
  // Create the pins at a placeholder position; reflow() forms them into a cross
  // NOLINTBEGIN(cppcoreguidelines-prefer-member-initializer)
  for(int i = kJUp; i <= kJRight; ++i)
  {
    myPins[i] = new CheckboxWidget(boss, font, "",
                                   CheckboxWidget::kCheckActionCmd);
    myPins[i]->setID(i);
    myPins[i]->setTarget(this);
  }
  myPins[kJFire] = new CheckboxWidget(boss, font, "Fire",
                                      CheckboxWidget::kCheckActionCmd);
  myPins[kJFire]->setID(kJFire);
  myPins[kJFire]->setTarget(this);
  // NOLINTEND(cppcoreguidelines-prefer-member-initializer)

  addFocusWidget(myPins[kJUp]);
  addFocusWidget(myPins[kJLeft]);
  addFocusWidget(myPins[kJRight]);
  addFocusWidget(myPins[kJDown]);
  addFocusWidget(myPins[kJFire]);

  if(!embedded)
    createHeader();
  reflow();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void JoystickWidget::layoutContent(GUI::BoxLayout& col)
{
  using GUI::BoxLayout;
  using GUI::anchoredItem;
  using Dir = BoxLayout::Dir;

  const int VGAP = _font.getFontHeight() / 4;

  // The shared cross, with Fire left-aligned below it; the whole unit is
  // centered so the buttons line up with the cross's left column
  auto unit = std::make_unique<BoxLayout>(Dir::Vertical, VGAP);
  unit->addAuto(layoutCross(myPins[kJUp], myPins[kJDown],
                            myPins[kJLeft], myPins[kJRight]));
  unit->addAuto(anchoredItem(myPins[kJFire]));

  auto row = std::make_unique<BoxLayout>(Dir::Horizontal);
  row->addStretchSpace();
  row->addAuto(std::move(unit));
  row->addStretchSpace();
  col.addAuto(std::move(row));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void JoystickWidget::loadConfig()
{
  myPins[kJUp]->setState(!getPin(ourPinNo[kJUp]));
  myPins[kJDown]->setState(!getPin(ourPinNo[kJDown]));
  myPins[kJLeft]->setState(!getPin(ourPinNo[kJLeft]));
  myPins[kJRight]->setState(!getPin(ourPinNo[kJRight]));
  myPins[kJFire]->setState(!getPin(ourPinNo[kJFire]));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void JoystickWidget::handleCommand(
    CommandSender* sender, int cmd, int data, int id)
{
  if(cmd == CheckboxWidget::kCheckActionCmd)
    setPin(ourPinNo[id], !myPins[id]->getState());
}
