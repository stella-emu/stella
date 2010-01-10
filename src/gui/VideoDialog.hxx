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
// Copyright (c) 1995-2010 by Bradford W. Mott and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#ifndef VIDEO_DIALOG_HXX
#define VIDEO_DIALOG_HXX

class CommandSender;
class CheckboxWidget;
class DialogContainer;
class EditTextWidget;
class PopUpWidget;
class SliderWidget;
class StaticTextWidget;
class TabWidget;

#include "OSystem.hxx"
#include "Dialog.hxx"
#include "bspf.hxx"

class VideoDialog : public Dialog
{
  public:
    VideoDialog(OSystem* osystem, DialogContainer* parent, const GUI::Font& font);
    ~VideoDialog();

  private:
    void loadConfig();
    void saveConfig();
    void setDefaults();

    void handleFullscreenChange(bool enable);
    virtual void handleCommand(CommandSender* sender, int cmd, int data, int id);

  private:
    TabWidget* myTab;

    // General options
    EditTextWidget*   myRenderer;
    PopUpWidget*      myRendererPopup;
    PopUpWidget*      myTIAFilterPopup;
    PopUpWidget*      myTIAPalettePopup;
    PopUpWidget*      myFSResPopup;
    PopUpWidget*      myFrameTimingPopup;
    PopUpWidget*      myGLFilterPopup;
    SliderWidget*     myNAspectRatioSlider;
    StaticTextWidget* myNAspectRatioLabel;
    SliderWidget*     myPAspectRatioSlider;
    StaticTextWidget* myPAspectRatioLabel;

    SliderWidget*     myFrameRateSlider;
    StaticTextWidget* myFrameRateLabel;
    PopUpWidget*      myFullscreenPopup;
    CheckboxWidget*   myColorLossCheckbox;
    CheckboxWidget*   myGLStretchCheckbox;
    CheckboxWidget*   myUseVSyncCheckbox;
    CheckboxWidget*   myUIMessagesCheckbox;
    CheckboxWidget*   myCenterCheckbox;
    CheckboxWidget*   myGrabmouseCheckbox;
    CheckboxWidget*   myFastSCBiosCheckbox;

    // TV effects options
    PopUpWidget*	  myTexturePopup;
    PopUpWidget*      myBleedPopup;
    PopUpWidget*      myNoisePopup;
    CheckboxWidget*   myPhosphorCheckbox;

    StaticTextWidget* myGLVersionInfo;
    StaticTextWidget* myGLTexRectInfo;

    enum {
      kNAspectRatioChanged = 'VDan',
      kPAspectRatioChanged = 'VDap',
      kFrameRateChanged    = 'VDfr',
      kFullScrChanged      = 'VDfs'
    };
};

#endif
