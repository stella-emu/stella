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

class UIDialog : public Dialog, public CommandSender
{
  public:
    UIDialog(OSystem& osystem, DialogContainer& parent, const GUI::Font& font,
             GuiObject* boss);
    ~UIDialog() override = default;

    void loadConfig() override;
    void saveConfig() override;
    void setDefaults() override;

  protected:
    void handleCommand(CommandSender* sender, int cmd, int data, int id) override;

    void layout() override;

  private:

    void handleLauncherSize();
    void handleRomViewer();

  private:
    enum
    {
      kDialogFont = 'UIDf',
      kListDelay  = 'UILd',
      kMouseWheel = 'UIMw',
      kControllerDelay = 'UIcd',
      kChooseRomDirCmd = 'LOrm', // rom select
      kRomViewer = 'UIRv',
      kChooseSnapLoadDirCmd = 'UIsl' // snapshot dir (load files)
    };

    TabWidget* myTab{nullptr};

    // Launcher options
    ButtonWidget*     myRomButton{nullptr};
    EditTextWidget*   myRomPath{nullptr};
    CheckboxWidget*   myFollowLauncherWidget{nullptr};
    StaticTextWidget* myLauncherWidthSliderLabel{nullptr};
    SliderWidget*     myLauncherWidthSlider{nullptr};
    StaticTextWidget* myLauncherHeightSliderLabel{nullptr};
    SliderWidget*     myLauncherHeightSlider{nullptr};
    StaticTextWidget* myLauncherFontLabel{nullptr};
    PopUpWidget*      myLauncherFontPopup{nullptr};
    CheckboxWidget*   myFavoritesWidget{nullptr};
    CheckboxWidget*   myLauncherExtensionsWidget{nullptr};
    CheckboxWidget*   myLauncherButtonsWidget{nullptr};
    StaticTextWidget* myRomViewerSizeLabel{nullptr};
    SliderWidget*     myRomViewerSize{nullptr};
    ButtonWidget*     myOpenBrowserButton{nullptr};
    EditTextWidget*   mySnapLoadPath{nullptr};
    CheckboxWidget*   myLauncherExitWidget{nullptr};

    // Misc options
    StaticTextWidget* myPalette1Label{nullptr};
    PopUpWidget*      myPalette1Popup{nullptr};
    StaticTextWidget* myPalette2Label{nullptr};
    PopUpWidget*      myPalette2Popup{nullptr};
    CheckboxWidget*   myAutoPalette{nullptr};
    StaticTextWidget* myDialogFontLabel{nullptr};
    PopUpWidget*      myDialogFontPopup{nullptr};
    CheckboxWidget*   myHidpiWidget{nullptr};
    StaticTextWidget* myPositionLabel{nullptr};
    PopUpWidget*      myPositionPopup{nullptr};
    CheckboxWidget*   myCenter{nullptr};
    StaticTextWidget* myListDelaySliderLabel{nullptr};
    SliderWidget*     myListDelaySlider{nullptr};
    StaticTextWidget* myWheelLinesSliderLabel{nullptr};
    SliderWidget*     myWheelLinesSlider{nullptr};
    StaticTextWidget* myControllerRateSliderLabel{nullptr};
    SliderWidget*     myControllerRateSlider{nullptr};
    StaticTextWidget* myControllerDelaySliderLabel{nullptr};
    SliderWidget*     myControllerDelaySlider{nullptr};
    StaticTextWidget* myDoubleClickSliderLabel{nullptr};
    SliderWidget*     myDoubleClickSlider{nullptr};

    // Bottom-of-tab "(*) ..." info messages
    StaticTextWidget* myLookFeelInfo{nullptr};
    StaticTextWidget* myLauncherInfo{nullptr};

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
