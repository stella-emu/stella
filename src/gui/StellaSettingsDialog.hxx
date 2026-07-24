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

class StellaSettingsDialog : public Dialog
{
  public:
    StellaSettingsDialog(OSystem& osystem, DialogContainer& parent, AppMode mode);
    ~StellaSettingsDialog() override;

    void loadConfig() override;
    void saveConfig() override;
    void setDefaults() override;

    // The help dialog is a separate Dialog; only allocated on first use, and
    // this dialog can itself be a long-lived cached singleton (see
    // OverlayMenu::cached), so it must forward explicitly rather than assume
    // it will always be freshly reconstructed before the help box is stale
    void refreshFont() override;

  protected:
    void layout() override;
    void handleCommand(CommandSender* sender, int cmd, int data, int id) override;

  private:
    void createUIOptions(WidgetArray& wid);
    void createVideoOptions(WidgetArray& wid);
    void createGameOptions(WidgetArray& wid);

    void handleOverscanChange();

    // switch to advanced settings after user confirmation
    void switchSettingsMode();

    // load the properties for the controller settings
    void loadControllerProperties(const Properties& props);

    // convert internal setting values to user friendly levels
    static int levelToValue(int level);
    static int valueToLevel(int value);

    void openHelp();

    void updateControllerStates();

  private:
    // Top-row buttons and section headers
    ButtonWidget* myAdvancedButton{nullptr};
    ButtonWidget* myHelpButton{nullptr};
    LabelWidget*  myGlobalLbl{nullptr};

    // UI theme
    LabelWidget*  myThemePopupLbl{nullptr};
    PopUpWidget*  myThemePopup{nullptr};
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

    LabelWidget*  myLeftPortLbl{nullptr};
    LabelWidget*  myRightPortLbl{nullptr};
    PopUpWidget*  myLeftPort{nullptr};
    LabelWidget*  myLeftPortDetected{nullptr};
    PopUpWidget*  myRightPort{nullptr};
    LabelWidget*  myRightPortDetected{nullptr};

    unique_ptr<HelpDialog> myHelpDialog;

    // Indicates if this dialog is used for global (vs. in-game) settings
    AppMode myMode{AppMode::emulator};

    enum {
      kAdvancedSettings = 'SSad',
      kHelp             = 'SShl',
      kScanlinesChanged = 'SSsc',
      kPhosphorChanged  = 'SSph',
      kOverscanChanged  = 'SSov',
      kLeftCChanged     = 'LCch',
      kRightCChanged    = 'RCch',
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
