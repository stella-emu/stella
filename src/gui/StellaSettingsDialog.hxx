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

#ifndef STELLA_SETTINGS_DIALOG_HXX
#define STELLA_SETTINGS_DIALOG_HXX

class PopUpWidget;
class ButtonWidget;

#include "Props.hxx"
#include "Dialog.hxx"

#include "HelpDialog.hxx"

namespace GUI {
  class Font;
}  // namespace GUI

/**
  A simplified settings dialog (UI theme/position, TV mode, controller
  ports) for first-time/basic use, with a link to the advanced dialogs.

  @author  Thomas Jentzsch
*/
class StellaSettingsDialog : public Dialog
{
  public:
    StellaSettingsDialog(OSystem& osystem, DialogContainer& parent, AppMode mode);
    ~StellaSettingsDialog() override;

    // Populates all controls from current settings and controller properties
    void loadConfig() override;
    // Writes all controls back to settings, then re-inits the framebuffer
    // and TIA surface so the changes take effect immediately
    void saveConfig() override;
    // Resets UI/video controls to hardcoded defaults, and reloads this ROM's
    // default controller properties
    void setDefaults() override;

  protected:
    void layout() override;
    // OK saves and exits; Advanced/Help open their dialogs; slider changes
    // update their value labels; controller pop-up changes refresh the
    // detected-controller readouts
    void handleCommand(CommandSender* sender, GuiCmd::Code cmd, int data, int id) override;

  private:
    // Builds the UI theme and dialog-position pop-ups
    void createUIOptions(WidgetArray& wid);
    // Builds the TV mode/scanline/phosphor/overscan controls
    void createVideoOptions(WidgetArray& wid);
    // Builds the left/right controller-port pop-ups and their
    // detected-controller labels
    void createGameOptions(WidgetArray& wid);

    // Blanks the value label and unit when overscan is 0, else shows it as
    // a percentage
    void handleOverscanChange();

    // switch to advanced settings after user confirmation
    void switchSettingsMode();

    // load the properties for the controller settings
    void loadControllerProperties(const Properties& props);

    // convert internal setting values to user friendly levels
    static int levelToValue(int level);
    static int valueToLevel(int value);

    // Opens (creating it if necessary) the basic-settings help dialog
    void openHelp();

    // Refreshes the detected-controller labels for both ports, and enables/
    // disables port selection (CompuMate has no selectable controllers)
    void updateControllerStates();

  private:
    // Top-row buttons and section headers
    ButtonWidget* myAdvancedButton{nullptr};
    ButtonWidget* myHelpButton{nullptr};
    LabelWidget*  myGlobalLbl{nullptr};

    // UI theme
    LabelWidget*  myThemePopupLbl{nullptr};
    PopUpWidget*  myThemePopup{nullptr};
    // Dialog position
    LabelWidget*  myPositionPopupLbl{nullptr};
    PopUpWidget*  myPositionPopup{nullptr};

    // TV effects
    LabelWidget*  myTVModeLbl{nullptr};
    PopUpWidget*  myTVMode{nullptr};

    // TV scanline intensity
    LabelWidget*  myTVScanIntenseLbl{nullptr};
    SliderWidget* myTVScanIntense{nullptr};

    // TV phosphor effect
    LabelWidget*  myTVPhosLevelLbl{nullptr};
    SliderWidget* myTVPhosLevel{nullptr};

    // TV Overscan
    LabelWidget*  myTVOverscanLbl{nullptr};
    SliderWidget* myTVOverscan{nullptr};
    LabelWidget*  myOverscanInfo{nullptr};

    // Controller properties
    LabelWidget*  myGameSettings{nullptr};

    // Left/right controller-port pop-ups, and the auto-detected controller
    // name shown beneath each (see updateControllerStates())
    LabelWidget*  myLeftPortLbl{nullptr};
    LabelWidget*  myRightPortLbl{nullptr};
    PopUpWidget*  myLeftPort{nullptr};
    LabelWidget*  myLeftPortDetected{nullptr};
    PopUpWidget*  myRightPort{nullptr};
    LabelWidget*  myRightPortDetected{nullptr};

    // Lazily created on the first Help click (see openHelp())
    unique_ptr<HelpDialog> myHelpDialog;

    // Indicates if this dialog is used for global (vs. in-game) settings
    AppMode myMode{AppMode::emulator};

    // Command ids dispatched in handleCommand()
    struct Cmd {
      static constexpr GuiCmd::Code
        AdvancedSettings       = GuiCmd::of("StellaSettingsDialog.AdvancedSettings"),
        Help                   = GuiCmd::of("StellaSettingsDialog.Help"),
        ScanlinesChanged       = GuiCmd::of("StellaSettingsDialog.ScanlinesChanged"),
        PhosphorChanged        = GuiCmd::of("StellaSettingsDialog.PhosphorChanged"),
        OverscanChanged        = GuiCmd::of("StellaSettingsDialog.OverscanChanged"),
        LeftControllerChanged  = GuiCmd::of("StellaSettingsDialog.LeftControllerChanged"),
        RightControllerChanged = GuiCmd::of("StellaSettingsDialog.RightControllerChanged");
    };

    // Game properties for currently loaded ROM
    Properties myGameProperties;

  private:
    // Following constructors and assignment operators not supported
    StellaSettingsDialog() = delete;
    StellaSettingsDialog(const StellaSettingsDialog&) = delete;
    StellaSettingsDialog(StellaSettingsDialog&&) = delete;
    StellaSettingsDialog& operator=(const StellaSettingsDialog&) = delete;
    StellaSettingsDialog& operator=(StellaSettingsDialog&&) = delete;
};

#endif  // STELLA_SETTINGS_DIALOG_HXX
