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
// Copyright (c) 1995-2020 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include "OSystem.hxx"
#include "Console.hxx"
#include "TIA.hxx"
#include "QuadTari.hxx"
#include "JoystickWidget.hxx"
#include "NullControlWidget.hxx"
#include "QuadTariWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
QuadTariWidget::QuadTariWidget(GuiObject* boss, const GUI::Font& font,
                               int x, int y, Controller& controller)
  : ControllerWidget(boss, font, x, y, controller)
{
  string label = (isLeftPort() ? "Left" : "Right") + string(" (QuadTari)");
  StaticTextWidget* t = new StaticTextWidget(boss, font, x, y + 2, label);
  QuadTari& qt = (QuadTari&)controller;

  x += font.getMaxCharWidth() * 2;
  y = t->getBottom() + font.getFontHeight() * 1.25;

  // TODO: support multiple controller types
  switch(qt.myFirstController->type())
  {
    case Controller::Type::Joystick:
      myFirstControl = new JoystickWidget(boss, font, x, y, *qt.myFirstController, true);
      x = myFirstControl->getRight() - font.getMaxCharWidth() * 8;
      break;

    default:
      myFirstControl = new NullControlWidget(boss, font, x, y, *qt.myFirstController);
      x += font.getMaxCharWidth() * 8;
      break;
  }

  switch(qt.mySecondController->type())
  {
    case Controller::Type::Joystick:
      mySecondControl = new JoystickWidget(boss, font, x, y, *qt.mySecondController, true);
      break;

    default:
      mySecondControl = new NullControlWidget(boss, font, x, y, *qt.mySecondController);
      break;
  }

  myPointer = new StaticTextWidget(boss, font,
                                   x - font.getMaxCharWidth() * 5, y, "  ");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void QuadTariWidget::loadConfig()
{
  bool first = !(instance().console().tia().registerValue(VBLANK) & 0x80);

  myPointer->setLabel(first ? "<-" : "->");
}
