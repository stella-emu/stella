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
// Copyright (c) 1995-2015 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

#ifndef UI_DIALOG_HXX
#define UI_DIALOG_HXX

class CommandSender;
class Dialog;
class DialogContainer;
class CheckboxWidget;
class PopUpWidget;
class SliderWidget;
class StaticTextWidget;
class TabWidget;
class OSystem;

#include "bspf.hxx"

class UIDialog : public Dialog
{
  public:
    UIDialog(OSystem& osystem, DialogContainer& parent, const GUI::Font& font);
    virtual ~UIDialog();

  private:
    void loadConfig() override;
    void saveConfig() override;
    void setDefaults() override;

    void handleCommand(CommandSender* sender, int cmd, int data, int id) override;

  private:
    TabWidget* myTab;

    // Launcher options
    SliderWidget*     myLauncherWidthSlider;
    StaticTextWidget* myLauncherWidthLabel;
    SliderWidget*     myLauncherHeightSlider;
    StaticTextWidget* myLauncherHeightLabel;
    PopUpWidget*      myLauncherExitPopup;
    PopUpWidget*      myLauncherFontPopup;
    PopUpWidget*      myRomViewerPopup;

    // Debugger options
    SliderWidget*     myDebuggerWidthSlider;
    StaticTextWidget* myDebuggerWidthLabel;
    SliderWidget*     myDebuggerHeightSlider;
    StaticTextWidget* myDebuggerHeightLabel;
    PopUpWidget*      myDebuggerFontStyle;

    // Misc options
    PopUpWidget*      myPalettePopup;
    PopUpWidget*      myListDelayPopup;
    PopUpWidget*      myWheelLinesPopup;
    
    enum {
      kLWidthChanged  = 'UIlw',
      kLHeightChanged = 'UIlh',
      kDWidthChanged  = 'UIdw',
      kDHeightChanged = 'UIdh',

      kDSmallSize  = 'UIds',
      kDMediumSize = 'UIdm',
      kDLargeSize  = 'UIdl'
    };

  private:
    // Following constructors and assignment operators not supported
    UIDialog() = delete;
    UIDialog(const UIDialog&) = delete;
    UIDialog(UIDialog&&) = delete;
    UIDialog& operator=(const UIDialog&) = delete;
    UIDialog& operator=(UIDialog&&) = delete;
};

#endif
