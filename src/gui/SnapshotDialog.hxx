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

#ifndef SNAPSHOT_DIALOG_HXX
#define SNAPSHOT_DIALOG_HXX

class OSystem;
class GuiObject;
class DialogContainer;
class ButtonWidget;
class CheckboxWidget;
class EditTextWidget;
class SliderWidget;
class LabelWidget;

#include "Dialog.hxx"
#include "Command.hxx"

/**
  Dialog for configuring snapshot saving: destination path, naming,
  single-file overwrite, scale, cropping, and continuous-snapshot
  interval.

  @author  Stephen Anthony and Thomas Jentzsch
*/
class SnapshotDialog : public Dialog
{
  public:
    SnapshotDialog(OSystem& osystem, DialogContainer& parent,
                   const GUI::Font& font);
    ~SnapshotDialog() override = default;

    void loadConfig() override;
    // Also flushes settings to disk and re-derives dependent config paths
    void saveConfig() override;
    void setDefaults() override;

  protected:
    // OK saves and closes; Defaults resets; the path button opens a directory
    // browser; the interval slider updates its own "second(s)" unit label
    void handleCommand(CommandSender* sender, GuiCmd::Code cmd, int data, int id) override;

    void layout() override;

  private:
    struct Cmd {
      static constexpr GuiCmd::Code
        ChooseSnapSaveDir = GuiCmd::of("SnapshotDialog.ChooseSnapSaveDir"),
        SnapshotInterval  = GuiCmd::of("SnapshotDialog.SnapshotInterval");
    };

    // Config paths
    ButtonWidget* mySnapSaveButton{nullptr};
    EditTextWidget* mySnapSavePath{nullptr};

    // Header for the checkbox group below
    LabelWidget* myWhenLbl{nullptr};
    // Name new snapshots after the ROM, rather than a sequential number
    CheckboxWidget* mySnapName{nullptr};
    // How often continuous-snapshot mode saves
    LabelWidget* mySnapIntervalLbl{nullptr};
    SliderWidget* mySnapInterval{nullptr};

    // Overwrite an existing file rather than adding a new one each time
    CheckboxWidget* mySnapSingle{nullptr};
    // Save at 1x scale, skipping zoom/post-processing
    CheckboxWidget* mySnap1x{nullptr};
    CheckboxWidget* mySnapCrop{nullptr};

  private:
    // Following constructors and assignment operators not supported
    SnapshotDialog() = delete;
    SnapshotDialog(const SnapshotDialog&) = delete;
    SnapshotDialog(SnapshotDialog&&) = delete;
    SnapshotDialog& operator=(const SnapshotDialog&) = delete;
    SnapshotDialog& operator=(SnapshotDialog&&) = delete;
};

#endif  // SNAPSHOT_DIALOG_HXX
