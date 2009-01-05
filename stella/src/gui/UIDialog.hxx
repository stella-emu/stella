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
// Copyright (c) 1995-2009 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: UIDialog.hxx,v 1.12 2009-01-05 22:05:35 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
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

#include "OSystem.hxx"
#include "bspf.hxx"

class UIDialog : public Dialog
{
  public:
    UIDialog(OSystem* osystem, DialogContainer* parent, const GUI::Font& font);
    ~UIDialog();

  protected:
    TabWidget* myTab;

    // Launcher options
    SliderWidget*     myLauncherWidthSlider;
    StaticTextWidget* myLauncherWidthLabel;
    SliderWidget*     myLauncherHeightSlider;
    StaticTextWidget* myLauncherHeightLabel;
    PopUpWidget*      myLauncherFontPopup;
    PopUpWidget*      myRomViewerPopup;

    // Debugger options
    SliderWidget*     myDebuggerWidthSlider;
    StaticTextWidget* myDebuggerWidthLabel;
    SliderWidget*     myDebuggerHeightSlider;
    StaticTextWidget* myDebuggerHeightLabel;

    // Misc options
    PopUpWidget*      myPalettePopup;
    SliderWidget*     myListDelaySlider;
    StaticTextWidget* myListDelayLabel;
    SliderWidget*     myWheelLinesSlider;
    StaticTextWidget* myWheelLinesLabel;
    
  private:
    void loadConfig();
    void saveConfig();
    void setDefaults();

    virtual void handleCommand(CommandSender* sender, int cmd, int data, int id);

    enum {
      kLWidthChanged  = 'UIlw',
      kLHeightChanged = 'UIlh',
      kDWidthChanged  = 'UIdw',
      kDHeightChanged = 'UIdh',
      kLQDelayChanged = 'UIqd',
      kWLinesChanged  = 'UIsl'
    };
};

#endif
