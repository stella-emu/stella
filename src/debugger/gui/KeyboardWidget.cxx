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

#include "EventHandler.hxx"
#include "Layout.hxx"
#include "KeyboardWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
KeyboardWidget::KeyboardWidget(GuiObject* boss, const GUI::Font& font,
                               int x, int y, Controller& controller)
  : ControllerWidget(boss, font, x, y, controller)
{
  // Create the keys at a placeholder position; reflow() lays them out
  // NOLINTBEGIN(cppcoreguidelines-prefer-member-initializer)
  for(int i = 0; i < 12; ++i)
  {
    myBox[i] = new CheckboxWidget(boss, font, 0, 0, "",
                                  CheckboxWidget::kCheckActionCmd);
    myBox[i]->setID(i);
    myBox[i]->setTarget(this);
    addFocusWidget(myBox[i]);
  }
  // NOLINTEND(cppcoreguidelines-prefer-member-initializer)

  myEvent = isLeftPort() ? ourLeftEvents.data() : ourRightEvents.data();

  createHeader();
  reflow();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void KeyboardWidget::layoutContent(GUI::BoxLayout& col)
{
  using GUI::BoxLayout;
  using GUI::GridLayout;
  using GUI::anchoredItem;
  using Dir = BoxLayout::Dir;

  const int VGAP = _font.getFontHeight() / 4;

  // The twelve keys as a 3-wide keypad, centered
  auto keypad = std::make_unique<GridLayout>(3, 4, VGAP, VGAP);
  for(int c = 0; c < 3; ++c)
    keypad->columnAuto(c);
  for(int r = 0; r < 4; ++r)
    keypad->rowAuto(r);
  for(int i = 0; i < 12; ++i)
    keypad->place(i % 3, i / 3, anchoredItem(myBox[i]));

  auto row = std::make_unique<BoxLayout>(Dir::Horizontal);
  row->addStretchSpace();
  row->addAuto(std::move(keypad));
  row->addStretchSpace();
  col.addAuto(std::move(row));
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
  if(cmd == CheckboxWidget::kCheckActionCmd)
    instance().eventHandler().handleEvent(myEvent[id], myBox[id]->getState());
}
