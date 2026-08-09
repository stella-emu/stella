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
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#ifndef RADIO_BUTTON_WIDGET_HXX
#define RADIO_BUTTON_WIDGET_HXX

#include "bspf.hxx"
#include "Widget.hxx"

class Dialog;
class RadioButtonGroup;

/**
  One button in a mutually-exclusive RadioButtonGroup; selecting it
  deselects the others.

  @author  Thomas Jentzsch
*/
class RadioButtonWidget : public CheckboxWidget
{
  public:
    // Registers itself with 'group', which selects it as the default if it's
    // the group's first member
    RadioButtonWidget(GuiObject* boss, const GUI::Font& font,
                      const string& label, RadioButtonGroup* group,
                      GuiCmd::Code cmd = GuiCmd::None);
    ~RadioButtonWidget() override = default;

    // Selects this button on click (radio buttons don't toggle off by clicking)
    void handleMouseUp(int x, int y, MouseButton b, int clickCount) override;
    // Sets the checked state; 'send' additionally sends _cmd when turning on;
    // turning on also tells myGroup to deselect the others
    void setState(bool state, bool send = true) override;
    // Picks the active/inactive dot icon; a radio button never uses FillType::Circle
    void setFill(FillType type) override;

    void refreshFont() override;

  protected:
    // Draws the outer/inner ring, the dot if checked, and the label
    void drawWidget(bool hilite) override;
    // Diameter of the (round) button for the given font
    static uInt32 buttonSize(const GUI::Font& font) {
      return font.isLarge() ? 22 : 14; // box is square
    }

  private:
    // The group this button belongs to; notified on selection (see setState())
    RadioButtonGroup* myGroup{nullptr};
    uInt32 _buttonSize{14};

  private:
    // Following constructors and assignment operators not supported
    RadioButtonWidget() = delete;
    RadioButtonWidget(const RadioButtonWidget&) = delete;
    RadioButtonWidget(RadioButtonWidget&&) = delete;
    RadioButtonWidget& operator=(const RadioButtonWidget&) = delete;
    RadioButtonWidget& operator=(RadioButtonWidget&&) = delete;
};

/**
  Tracks which RadioButtonWidget in a set is currently selected,
  deselecting the rest when one is chosen.

  @author  Thomas Jentzsch
*/
class RadioButtonGroup
{
  public:
    RadioButtonGroup() = default;
    ~RadioButtonGroup() = default;

    // add widget to group
    void addWidget(RadioButtonWidget* widget);
    // tell the group which widget was selected
    void select(const RadioButtonWidget* widget);
    // Selects myWidgets[selected], deselecting every other member
    void setSelected(uInt32 selected);
    uInt32 getSelected() const { return mySelected; }

  private:
    // Every button in the group, in the order they were added
    WidgetArray myWidgets;
    // Index into myWidgets of the currently selected button
    uInt32 mySelected{0};

  private:
    // Following constructors and assignment operators not supported
    RadioButtonGroup(const RadioButtonGroup&) = delete;
    RadioButtonGroup(RadioButtonGroup&&) = delete;
    RadioButtonGroup& operator=(const RadioButtonGroup&) = delete;
    RadioButtonGroup& operator=(RadioButtonGroup&&) = delete;
};

#endif  // RADIO_BUTTON_WIDGET_HXX
