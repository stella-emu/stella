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

#ifndef COMMAND_DIALOG_HXX
#define COMMAND_DIALOG_HXX

class CommandSender;
class DialogContainer;
class OSystem;

#include "Dialog.hxx"

/**
  The in-game command menu: one button per quick action (save/load
  state, TV format, palette, phosphor, sound, ...).

  @author  Stephen Anthony and Thomas Jentzsch
*/
class CommandDialog : public Dialog
{
  public:
    CommandDialog(OSystem& osystem, DialogContainer& parent);
    ~CommandDialog() override = default;

    // Relabels the buttons whose text reflects live console/settings state
    // (color mode, difficulty switches, TV format, palette, phosphor, sound, ...)
    void loadConfig() override;

  protected:
    // Dispatches a button to its console/state event, then (for an immediate
    // console command) leaves menu mode and applies it right away
    void handleCommand(CommandSender* sender, GuiCmd::Code cmd, int data, int id) override;

    void layout() override;

    // Relabel one button from current state; called from loadConfig() and
    // again after the corresponding command fires
    void updateSlot(int slot);
    void updateTVFormat();
    void updatePalette();
    // Leaves menu mode instead of closing (this dialog has no cancel widget)
    void processCancel() override;

    // All buttons in grid order (each column top-to-bottom)
    vector<ButtonWidget*> myButtons;

    // column 0
    ButtonWidget* myColorButton{nullptr};
    ButtonWidget* myLeftDiffButton{nullptr};
    ButtonWidget* myRightDiffButton{nullptr};
    // column 1
    ButtonWidget* mySaveStateButton{nullptr};
    ButtonWidget* myStateSlotButton{nullptr};
    ButtonWidget* myLoadStateButton{nullptr};
    ButtonWidget* myTimeMachineButton{nullptr};
    // column 2
    ButtonWidget* myTVFormatButton{nullptr};
    ButtonWidget* myPaletteButton{nullptr};
    ButtonWidget* myPhosphorButton{nullptr};
    ButtonWidget* mySoundButton{nullptr};

    // Column 1/2/3 button commands, dispatched in handleCommand()
    struct Cmd {
      static constexpr GuiCmd::Code
        Select          = GuiCmd::of("CommandDialog.Select"),
        Reset           = GuiCmd::of("CommandDialog.Reset"),
        Color           = GuiCmd::of("CommandDialog.Color"),
        LeftDifficulty  = GuiCmd::of("CommandDialog.LeftDifficulty"),
        RightDifficulty = GuiCmd::of("CommandDialog.RightDifficulty"),
        SaveState       = GuiCmd::of("CommandDialog.SaveState"),
        StateSlot       = GuiCmd::of("CommandDialog.StateSlot"),
        LoadState       = GuiCmd::of("CommandDialog.LoadState"),
        Snapshot        = GuiCmd::of("CommandDialog.Snapshot"),
        TimeMachine     = GuiCmd::of("CommandDialog.TimeMachine"),
        Format          = GuiCmd::of("CommandDialog.Format"),
        Palette         = GuiCmd::of("CommandDialog.Palette"),
        Phosphor        = GuiCmd::of("CommandDialog.Phosphor"),
        Sound           = GuiCmd::of("CommandDialog.Sound"),
        ReloadRom       = GuiCmd::of("CommandDialog.ReloadRom"),
        Exit            = GuiCmd::of("CommandDialog.Exit");
    };

  private:
    // Following constructors and assignment operators not supported
    CommandDialog() = delete;
    CommandDialog(const CommandDialog&) = delete;
    CommandDialog(CommandDialog&&) = delete;
    CommandDialog& operator=(const CommandDialog&) = delete;
    CommandDialog& operator=(CommandDialog&&) = delete;
};

#endif  // COMMAND_DIALOG_HXX
