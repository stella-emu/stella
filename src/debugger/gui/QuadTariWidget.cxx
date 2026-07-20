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

#include "OSystem.hxx"
#include "Console.hxx"
#include "TIA.hxx"
#include "QuadTari.hxx"
#include "AtariVoxWidget.hxx"
#include "DrivingWidget.hxx"
#include "JoystickWidget.hxx"
#include "NullControlWidget.hxx"
#include "PaddleWidget.hxx"
#include "SaveKeyWidget.hxx"
#include "Layout.hxx"

#include "QuadTariWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
QuadTariWidget::QuadTariWidget(GuiObject* boss, const GUI::Font& font,
                               Controller& controller)
  : ControllerWidget(boss, font, controller)
{
  const QuadTari& qt = static_cast<QuadTari&>(controller);

  // Create both embedded controllers and the pointer at a placeholder position;
  // reflow() lays them out
  // NOLINTBEGIN(cppcoreguidelines-prefer-member-initializer)
  myFirst   = addController(boss, *qt.myFirstController, false);
  mySecond  = addController(boss, *qt.mySecondController, true);
  myPointer = new StaticTextWidget(boss, font, 0, 0, "  ");
  // NOLINTEND(cppcoreguidelines-prefer-member-initializer)

  createHeader();
  reflow();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void QuadTariWidget::layoutContent(GUI::BoxLayout& col)
{
  using GUI::BoxLayout;
  using GUI::anchoredItem;
  using Dir = BoxLayout::Dir;

  // The two embedded controllers side by side, the pointer between them.  Each
  // is a full controller widget, so the row lays it out via its setArea(),
  // which re-flows its own content -- no need to position them by hand
  auto row = std::make_unique<BoxLayout>(Dir::Horizontal, _font.getMaxCharWidth());
  row->addAuto(anchoredItem(myFirst));
  row->addAuto(anchoredItem(myPointer));
  row->addAuto(anchoredItem(mySecond));
  col.addAuto(std::move(row));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ControllerWidget* QuadTariWidget::addController(GuiObject* boss,
                                                Controller& controller, bool second)
{
  ControllerWidget* widget = nullptr;

  switch(controller.type())
  {
    using enum Controller::Type;
    case Joystick:
      widget = new JoystickWidget(boss, _font, controller, true);
      break;
    case Driving:
      widget = new DrivingWidget(boss, _font, controller, true);
      break;
    case Paddles:
      widget = new PaddleWidget(boss, _font, controller, true, second);
      break;
    case AtariVox:
      widget = new AtariVoxWidget(boss, _font, controller, true);
      break;
    case SaveKey:
      widget = new SaveKeyWidget(boss, _font, controller, true);
      break;
    default:
      widget = new NullControlWidget(boss, _font, controller, true);
      break;
  }
  const WidgetArray& focusList = widget->getFocusList();
  if(!focusList.empty())
    addToFocusList(focusList);
  return widget;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void QuadTariWidget::loadConfig()
{
  const bool first = !(instance().console().tia().registerValue(VBLANK) & 0x80);

  myPointer->setLabel(first ? "<-" : "->");
}
