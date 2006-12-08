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
// Copyright (c) 1995-2006 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: VideoDialog.hxx,v 1.16 2006-12-08 16:49:37 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#ifndef VIDEO_DIALOG_HXX
#define VIDEO_DIALOG_HXX

class CommandSender;
class DialogContainer;
class PopUpWidget;
class SliderWidget;
class StaticTextWidget;
class CheckboxWidget;

#include "OSystem.hxx"
#include "Dialog.hxx"
#include "bspf.hxx"

class VideoDialog : public Dialog
{
  public:
    VideoDialog(OSystem* osystem, DialogContainer* parent,
                const GUI::Font& font, int x, int y, int w, int h);
    ~VideoDialog();

  private:
    void loadConfig();
    void saveConfig();
    void setDefaults();

    void handleRendererChange(int item);
    virtual void handleCommand(CommandSender* sender, int cmd, int data, int id);

  private:
    PopUpWidget*      myDirtyPopup;
    PopUpWidget*      myRendererPopup;
    PopUpWidget*      myFilterPopup;
    SliderWidget*     myAspectRatioSlider;
    StaticTextWidget* myAspectRatioLabel;
    PopUpWidget*      myPalettePopup;
    SliderWidget*     myFrameRateSlider;
    StaticTextWidget* myFrameRateLabel;
    PopUpWidget*      myScalerPopup;
    CheckboxWidget*   myFullscreenCheckbox;
    CheckboxWidget*   myTiaDefaultsCheckbox;
    CheckboxWidget*   myUseDeskResCheckbox;
    CheckboxWidget*   myUseVSyncCheckbox;

    enum {
      kRendererChanged    = 'VDrd',
      kAspectRatioChanged = 'VDar',
      kFrameRateChanged   = 'VDfr'
    };
};

#endif
