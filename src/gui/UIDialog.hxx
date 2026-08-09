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

#ifndef UI_DIALOG_HXX
#define UI_DIALOG_HXX

#include "Dialog.hxx"
#include "bspf.hxx"

/**
  Dialog for editing UI settings: look & feel (theme, palette, fonts,
  HiDPI) and launcher behavior, across two tabs.

  @author  Stephen Anthony and Thomas Jentzsch
*/
class UIDialog : public Dialog, public CommandSender
{
  public:
    UIDialog(OSystem& osystem, DialogContainer& parent, const GUI::Font& font,
             GuiObject* boss);
    ~UIDialog() override = default;

    // Populates both tabs' controls from current settings
    void loadConfig() override;
    // Writes both tabs' controls back to settings, applying palette/font/
    // HiDPI changes immediately
    void saveConfig() override;
    // Resets the currently active tab's controls to hardcoded defaults
    void setDefaults() override;

  protected:
    // OK saves and exits, then informs the boss of the settings that changed
    // (when global) and applies font/HiDPI changes immediately; Defaults
    // resets the active tab; slider/pop-up ids update their own labels; the
    // path buttons open browsers
    void handleCommand(CommandSender* sender, GuiCmd::Code cmd, int data, int id) override;
    void layout() override;

  private:
    // Derives the launcher's minimum width/height from the selected dialog
    // font, clamping the sliders to it
    void handleLauncherSize();
    // Updates the ROM-viewer slider's label/unit and enables/disables the
    // image-path controls based on its value
    void handleRomViewer();

  private:
    // Command ids dispatched in handleCommand()
    struct Cmd {
      static constexpr GuiCmd::Code
        DialogFont        = GuiCmd::of("UIDialog.DialogFont"),
        ListDelay         = GuiCmd::of("UIDialog.ListDelay"),
        MouseWheel        = GuiCmd::of("UIDialog.MouseWheel"),
        ControllerDelay   = GuiCmd::of("UIDialog.ControllerDelay"),
        ChooseRomDir      = GuiCmd::of("UIDialog.ChooseRomDir"),
        RomViewer         = GuiCmd::of("UIDialog.RomViewer"),
        ChooseSnapLoadDir = GuiCmd::of("UIDialog.ChooseSnapLoadDir");
    };

    // Hosts the two option tabs (Look & Feel, Launcher)
    TabWidget* myTab{nullptr};

    // Launcher options
    ButtonWidget*   myRomButton{nullptr};
    EditTextWidget* myRomPath{nullptr};
    CheckboxWidget* myFollowLauncherWidget{nullptr};
    LabelWidget*    myLauncherWidthSliderLbl{nullptr};
    SliderWidget*   myLauncherWidthSlider{nullptr};
    LabelWidget*    myLauncherHeightSliderLbl{nullptr};
    SliderWidget*   myLauncherHeightSlider{nullptr};
    LabelWidget*    myLauncherFontLbl{nullptr};
    PopUpWidget*    myLauncherFontPopup{nullptr};
    CheckboxWidget* myFavoritesWidget{nullptr};
    CheckboxWidget* myLauncherExtensionsWidget{nullptr};
    CheckboxWidget* myLauncherButtonsWidget{nullptr};
    LabelWidget*    myRomViewerSizeLbl{nullptr};
    SliderWidget*   myRomViewerSize{nullptr};
    ButtonWidget*   myOpenBrowserButton{nullptr};
    EditTextWidget* mySnapLoadPath{nullptr};
    CheckboxWidget* myLauncherExitWidget{nullptr};

    // Misc options
    LabelWidget*    myPalette1Lbl{nullptr};
    PopUpWidget*    myPalette1Popup{nullptr};
    LabelWidget*    myPalette2Lbl{nullptr};
    PopUpWidget*    myPalette2Popup{nullptr};
    CheckboxWidget* myAutoPalette{nullptr};
    LabelWidget*    myDialogFontLbl{nullptr};
    PopUpWidget*    myDialogFontPopup{nullptr};
    LabelWidget*    myHidpiLbl{nullptr};
    PopUpWidget*    myHidpiPopup{nullptr};
    LabelWidget*    myPositionLbl{nullptr};
    PopUpWidget*    myPositionPopup{nullptr};
    CheckboxWidget* myCenter{nullptr};
    LabelWidget*    myListDelaySliderLbl{nullptr};
    SliderWidget*   myListDelaySlider{nullptr};
    LabelWidget*    myWheelLinesSliderLbl{nullptr};
    SliderWidget*   myWheelLinesSlider{nullptr};
    LabelWidget*    myControllerRateSliderLbl{nullptr};
    SliderWidget*   myControllerRateSlider{nullptr};
    LabelWidget*    myControllerDelaySliderLbl{nullptr};
    SliderWidget*   myControllerDelaySlider{nullptr};
    LabelWidget*    myDoubleClickSliderLbl{nullptr};
    SliderWidget*   myDoubleClickSlider{nullptr};

    // Bottom-of-tab "(*) ..." info messages
    LabelWidget* myLauncherInfo{nullptr};

    // Indicates if this dialog is used for global (vs. in-game) settings
    bool myIsGlobal{false};

  private:
    // Following constructors and assignment operators not supported
    UIDialog() = delete;
    UIDialog(const UIDialog&) = delete;
    UIDialog(UIDialog&&) = delete;
    UIDialog& operator=(const UIDialog&) = delete;
    UIDialog& operator=(UIDialog&&) = delete;
};

#endif  // UI_DIALOG_HXX
