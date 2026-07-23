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

#include "Paddles.hxx"
#include "Layout.hxx"
#include "PaddleWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PaddleWidget::PaddleWidget(GuiObject* boss, const GUI::Font& font,
                           Controller& controller, bool embedded, bool second)
  : ControllerWidget(boss, font, controller),
    myEmbedded{embedded}
{
  const bool leftport = isLeftPort();

  // Create the controls at a placeholder position; reflow() lays them out.
  // Embedded in a QuadTari the resistance is not shown, just a short pot label
  // NOLINTBEGIN(cppcoreguidelines-prefer-member-initializer)
  if(embedded)
  {
    myP0Label = new StaticTextWidget(boss, font,
      leftport ? second ? "P1b" : "P1a" : second ? "P3b" : "P3a");
    myP1Label = new StaticTextWidget(boss, font,
      leftport ? second ? "P2b" : "P2a" : second ? "P4b" : "P4a");

    myP0Resistance = new SliderWidget(boss, font);
    myP0Resistance->setEnabled(false);
    myP0Resistance->setFlags(Widget::FLAG_INVISIBLE);
    myP1Resistance = new SliderWidget(boss, font);
    myP1Resistance->setEnabled(false);
    myP1Resistance->setFlags(Widget::FLAG_INVISIBLE);
  }
  else
  {
    myP0Label = new StaticTextWidget(boss, font, leftport ? "P1 pot" : "P3 pot");
    myP0Resistance = new SliderWidget(boss, font, 0, kP0Changed);
    myP1Label = new StaticTextWidget(boss, font, leftport ? "P2 pot" : "P4 pot");
    myP1Resistance = new SliderWidget(boss, font, 0, kP1Changed);
  }
  myP0Fire = new CheckboxWidget(boss, font, "Fire", kP0Fire);
  myP1Fire = new CheckboxWidget(boss, font, "Fire", kP1Fire);
  // NOLINTEND(cppcoreguidelines-prefer-member-initializer)

  for(auto* s: {myP0Resistance, myP1Resistance})
  {
    s->setMinValue(0);
    s->setMaxValue(uInt32{AnalogReadout::MAX_POT_RESISTANCE});
    s->setStepValue(uInt32{AnalogReadout::MAX_POT_RESISTANCE / 100});
    s->setTarget(this);
  }
  myP0Fire->setTarget(this);
  myP1Fire->setTarget(this);

  addFocusWidget(myP0Resistance);
  addFocusWidget(myP0Fire);
  addFocusWidget(myP1Resistance);
  addFocusWidget(myP1Fire);

  if(!embedded)
    createHeader();
  reflow();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PaddleWidget::layoutContent(GUI::BoxLayout& col)
{
  using GUI::anchoredItem;
  using GUI::indentedItem;
  using GUI::labeledRow;

  const int VGAP   = _font.getFontHeight() / 4,
            INDENT = _font.getMaxCharWidth() * 2;

  if(myEmbedded)
  {
    // Just the pot label and its fire button, twice (the sliders are hidden)
    col.addAuto(anchoredItem(myP0Label));
    col.addAuto(anchoredItem(myP0Fire));
    col.addSpace(VGAP);
    col.addAuto(anchoredItem(myP1Label));
    col.addAuto(anchoredItem(myP1Fire));
  }
  else
  {
    // A resistance slider beside its label, with the fire button indented below
    col.addAuto(labeledRow(myP0Label, myP0Resistance));
    col.addAuto(indentedItem(myP0Fire, INDENT));
    col.addSpace(VGAP);
    col.addAuto(labeledRow(myP1Label, myP1Resistance));
    col.addAuto(indentedItem(myP1Fire, INDENT));
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PaddleWidget::loadConfig()
{
  myP0Resistance->setValue(static_cast<Int32>(AnalogReadout::MAX_POT_RESISTANCE -
      getPin(Controller::AnalogPin::Five).resistance));
  myP1Resistance->setValue(static_cast<Int32>(AnalogReadout::MAX_POT_RESISTANCE -
      getPin(Controller::AnalogPin::Nine).resistance));
  myP0Fire->setState(!getPin(Controller::DigitalPin::Four));
  myP1Fire->setState(!getPin(Controller::DigitalPin::Three));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PaddleWidget::handleCommand(
    CommandSender* sender, int cmd, int data, int id)
{
  switch(cmd)
  {
    case kP0Changed:
      setPin(Controller::AnalogPin::Five,
             AnalogReadout::connectToVcc(AnalogReadout::MAX_POT_RESISTANCE - myP0Resistance->getValue()));
      break;
    case kP1Changed:
      setPin(Controller::AnalogPin::Nine,
             AnalogReadout::connectToVcc(AnalogReadout::MAX_POT_RESISTANCE - myP1Resistance->getValue()));
      break;
    case kP0Fire:
      setPin(Controller::DigitalPin::Four, !myP0Fire->getState());
      break;
    case kP1Fire:
      setPin(Controller::DigitalPin::Three, !myP1Fire->getState());
      break;
    default:
      break;
  }
}
