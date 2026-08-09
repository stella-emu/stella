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

#ifndef EMULATION_DIALOG_HXX
#define EMULATION_DIALOG_HXX

class RadioButtonGroup;
class RadioButtonWidget;
class SliderWidget;
class LabelWidget;

#include "Dialog.hxx"

/**
  Dialog for editing general emulation settings: speed, threading,
  auto-pause, and state-save behavior on entering/exiting emulation.

  @author  Thomas Jentzsch
*/
class EmulationDialog : public Dialog
{
  public:
    EmulationDialog(OSystem& osystem, DialogContainer& parent, const GUI::Font& font);
    // Out-of-line: mySaveOnExitGroup (unique_ptr<RadioButtonGroup>) needs its
    // complete type here
    ~EmulationDialog() override;

    // Populates all controls from current settings
    void loadConfig() override;
    // Writes all controls back to settings, re-initializing audio/video
    // (and NTSC threading) if a console is running
    void saveConfig() override;
    // Resets all controls to hardcoded defaults, including the state
    // directory to its default location
    void setDefaults() override;

  protected:
    // OK saves and exits; Defaults resets to hardcoded values; the speed
    // slider updates its own label; the state-path button opens a browser;
    // toggling 'load/save in ROM directory' enables/disables the path controls
    void handleCommand(CommandSender* sender, GuiCmd::Code cmd, int data, int id) override;

    void layout() override;

  private:
    // Enable/disable the state path widgets based on the ROM-directory toggle
    void updateStatePathEnabled();

  private:
    // Emulation speed slider and its label
    LabelWidget*      mySpeedLbl{nullptr};
    SliderWidget*     mySpeed{nullptr};

    // General emulation toggles
    CheckboxWidget*   myUseVSync{nullptr};
    CheckboxWidget*   myTurbo{nullptr};
    CheckboxWidget*   myUIMessages{nullptr};
    // Skips the progress bars shown while a Supercharger BIOS loads
    CheckboxWidget*   myFastSCBios{nullptr};
    CheckboxWidget*   myUseThreads{nullptr};
    CheckboxWidget*   myAutoPauseWidget{nullptr};
    CheckboxWidget*   myConfirmExitWidget{nullptr};

    // What to do with the emulation state on entering/exiting emulation:
    // do nothing, save to the current slot, or use Time Machine states
    LabelWidget*      mySaveOnExitLbl{nullptr};
    unique_ptr<RadioButtonGroup> mySaveOnExitGroup;
    std::array<RadioButtonWidget*, 3> mySaveOnExitButtons{nullptr, nullptr, nullptr};
    CheckboxWidget*   myAutoSlotWidget{nullptr};

    // Where state files are read from/written to
    ButtonWidget*     myStatePathButton{nullptr};
    EditTextWidget*   myStatePath{nullptr};
    // Overrides myStatePath with the current ROM's own directory
    CheckboxWidget*   myStateWithRom{nullptr};

    // Command ids dispatched in handleCommand()
    struct Cmd {
      static constexpr GuiCmd::Code
        SpeedupChanged = GuiCmd::of("EmulationDialog.SpeedupChanged"),
        ChooseStateDir = GuiCmd::of("EmulationDialog.ChooseStateDir"),
        StateWithRom   = GuiCmd::of("EmulationDialog.StateWithRom");
    };

  private:
    // Following constructors and assignment operators not supported
    EmulationDialog() = delete;
    EmulationDialog(const EmulationDialog&) = delete;
    EmulationDialog(EmulationDialog&&) = delete;
    EmulationDialog& operator=(const EmulationDialog&) = delete;
    EmulationDialog& operator=(EmulationDialog&&) = delete;
};

#endif  // EMULATION_DIALOG_HXX
