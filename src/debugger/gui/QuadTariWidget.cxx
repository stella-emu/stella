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
#include "DrivingWidget.hxx"
#include "JoystickWidget.hxx"
#include "NullControlWidget.hxx"
#include "QuadTariWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
QuadTariWidget::QuadTariWidget(GuiObject* boss, const GUI::Font& font,
                               int x, int y, Controller& controller)
  : ControllerWidget(boss, font, x, y, controller)
{
  const int fontWidth = font.getMaxCharWidth(),
    fontHeight = font.getFontHeight();
  string label = (isLeftPort() ? "Left" : "Right") + string(" (QuadTari)");
  StaticTextWidget* t = new StaticTextWidget(boss, font, x, y + 2, label);
  QuadTari& qt = static_cast<QuadTari&>(controller);

  // TODO: support multiple controller types
  switch(qt.myFirstController->type())
  {
    case Controller::Type::Joystick:
      x += fontWidth * 2;
      y = t->getBottom() + fontHeight;
      myFirstControl = new JoystickWidget(boss, font, x, y, *qt.myFirstController, true);
      break;

    case Controller::Type::Driving:
      y = t->getBottom() + font.getFontHeight() * 1;
      myFirstControl = new DrivingWidget(boss, font, x, y, *qt.myFirstController, true);
      break;

    default:
      y = t->getBottom() + font.getFontHeight() * 1.25;
      myFirstControl = new NullControlWidget(boss, font, x, y, *qt.myFirstController, true);
      break;
  }

  x = t->getLeft() + fontWidth * 10;
  switch(qt.mySecondController->type())
  {
    case Controller::Type::Joystick:
      x += fontWidth * 2;
      y = t->getBottom() + fontHeight;
      mySecondControl = new JoystickWidget(boss, font, x, y, *qt.mySecondController, true);
      break;

    case Controller::Type::Driving:
      y = t->getBottom() + font.getFontHeight();
      myFirstControl = new DrivingWidget(boss, font, x, y, *qt.mySecondController, true);
      break;

    default:
      y = t->getBottom() + font.getFontHeight() * 1.25;
      mySecondControl = new NullControlWidget(boss, font, x, y, *qt.mySecondController, true);
      break;
  }

  myPointer = new StaticTextWidget(boss, font,
                                   t->getLeft() + font.getMaxCharWidth() * 7, y, "  ");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void QuadTariWidget::loadConfig()
{
  bool first = !(instance().console().tia().registerValue(VBLANK) & 0x80);

  myPointer->setLabel(first ? "<-" : "->");
}
