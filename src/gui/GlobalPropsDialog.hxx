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

#ifndef GLOBAL_PROPS_DIALOG_HXX
#define GLOBAL_PROPS_DIALOG_HXX

class CommandSender;
class DialogContainer;
class CheckboxWidget;
class PopUpWidget;
class LabelWidget;
class OSystem;

#include "Dialog.hxx"
#include "bspf.hxx"

/**
  Dialog for temporarily overriding a ROM's power-on properties
  (bankswitch/difficulty/TV type, held buttons) without changing its
  saved Properties.

  @author  Stephen Anthony and Thomas Jentzsch
*/
class GlobalPropsDialog : public Dialog, public CommandSender
{
  public:
    GlobalPropsDialog(GuiObject* boss, const GUI::Font& font);
    ~GlobalPropsDialog() override = default;

    // Populates all controls from settings
    void loadConfig() override;
    // Writes all controls back to settings
    void saveConfig() override;
    void setDefaults() override;

  protected:
    void layout() override;
    // OK saves, closes, and asks the launcher to load the ROM; Defaults
    // resets every control and saves immediately
    void handleCommand(CommandSender* sender, GuiCmd::Code cmd, int data, int id) override;

  private:
    // Builds the joystick direction/fire checkboxes and the console Select/Reset ones
    void createHoldWidgets(const GUI::Font& font, WidgetArray& wid);

    // The three groups of buttons which can be held down at power-on: a
    // directional cross per joystick, and the console's Select/Reset
    unique_ptr<GUI::Layout> holdLayout();
    unique_ptr<GUI::Layout> joyLayout(LabelWidget* label, int base);
    unique_ptr<GUI::Layout> consoleLayout();

  private:
    // Indices into myJoy/ourJoyState for each joystick's directions and fire button
    enum: uInt8 {
      kJ0Up, kJ0Down, kJ0Left, kJ0Right, kJ0Fire,
      kJ1Up, kJ1Down, kJ1Left, kJ1Right, kJ1Fire
    };

    // Power-on option pop-ups, each labelled below
    PopUpWidget* myBSType{nullptr};
    PopUpWidget* myLeftDiff{nullptr};
    PopUpWidget* myRightDiff{nullptr};
    PopUpWidget* myTVType{nullptr};

    // Direction/fire checkboxes for both joysticks, indexed by the enum above
    std::array<CheckboxWidget*, 10> myJoy{nullptr};
    // Console Select/Reset checkboxes
    CheckboxWidget* myHoldSelect{nullptr};
    CheckboxWidget* myHoldReset{nullptr};
    CheckboxWidget* myDebug{nullptr};

    LabelWidget* myBSLbl{nullptr};
    LabelWidget* myTVLbl{nullptr};
    LabelWidget* myLeftDiffLbl{nullptr};
    LabelWidget* myRightDiffLbl{nullptr};
    LabelWidget* myHeldLbl{nullptr};
    LabelWidget* myReleasedLbl{nullptr};
    LabelWidget* myLeftJoyLbl{nullptr};
    LabelWidget* myRightJoyLbl{nullptr};
    LabelWidget* myConsoleLbl{nullptr};
    LabelWidget* myInfo1{nullptr};
    LabelWidget* myInfo2{nullptr};

    // Single-letter codes stored in the holdjoy0/holdjoy1 settings, indexed
    // the same as myJoy
    static constexpr std::array<string_view, 10> ourJoyState = {
      "U", "D", "L", "R", "F", "U", "D", "L", "R", "F"
    };

  private:
    // Following constructors and assignment operators not supported
    GlobalPropsDialog() = delete;
    GlobalPropsDialog(const GlobalPropsDialog&) = delete;
    GlobalPropsDialog(GlobalPropsDialog&&) = delete;
    GlobalPropsDialog& operator=(const GlobalPropsDialog&) = delete;
    GlobalPropsDialog& operator=(GlobalPropsDialog&&) = delete;
};

#endif  // GLOBAL_PROPS_DIALOG_HXX
