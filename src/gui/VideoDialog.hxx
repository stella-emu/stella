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
// Copyright (c) 1995-2012 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
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
class OSystem;

#include "Dialog.hxx"
#include "bspf.hxx"

class VideoDialog : public Dialog
{
  public:
    VideoDialog(OSystem* osystem, DialogContainer* parent, const GUI::Font& font,
                int max_w, int max_h);
    ~VideoDialog();

  private:
    void loadConfig();
    void saveConfig();
    void setDefaults();

    void handleFullscreenChange(bool enable);
    void handleTVModeChange(NTSCFilter::Preset);
    void loadTVAdjustables(NTSCFilter::Preset preset);
    virtual void handleCommand(CommandSender* sender, int cmd, int data, int id);

  private:
    TabWidget* myTab;

    // General options
    StaticTextWidget* myRenderer;
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
    CheckboxWidget*   myFastSCBiosCheckbox;

    // TV effects adjustables (custom mode)
    PopUpWidget*      myTVMode;
    SliderWidget*     myTVSharp;
    StaticTextWidget* myTVSharpLabel;
    SliderWidget*     myTVHue;
    StaticTextWidget* myTVHueLabel;
    SliderWidget*     myTVRes;
    StaticTextWidget* myTVResLabel;
    SliderWidget*     myTVArtifacts;
    StaticTextWidget* myTVArtifactsLabel;
    SliderWidget*     myTVFringe;
    StaticTextWidget* myTVFringeLabel;
    SliderWidget*     myTVBleed;
    StaticTextWidget* myTVBleedLabel;
    SliderWidget*     myTVBright;
    StaticTextWidget* myTVBrightLabel;
    SliderWidget*     myTVContrast;
    StaticTextWidget* myTVContrastLabel;
    SliderWidget*     myTVSatur;
    StaticTextWidget* myTVSaturLabel;
    SliderWidget*     myTVGamma;
    StaticTextWidget* myTVGammaLabel;

    // TV scanline intensity and interpolation
    StaticTextWidget* myTVScanLabel;
    SliderWidget*     myTVScanIntense;
    StaticTextWidget* myTVScanIntenseLabel;
    CheckboxWidget*   myTVScanInterpolate;

    // TV effects adjustables presets (custom mode)
    ButtonWidget*     myCloneComposite;
    ButtonWidget*     myCloneSvideo;
    ButtonWidget*     myCloneRGB;
    ButtonWidget*     myCloneBad;
    ButtonWidget*     myCloneCustom;

    enum {
      kNAspectRatioChanged = 'VDan',
      kPAspectRatioChanged = 'VDap',
      kFrameRateChanged    = 'VDfr',
      kFullScrChanged      = 'VDfs',

      kTVModeChanged       = 'VDtv',
      kTVSharpChanged      = 'TVsh',
      kTVHueChanged        = 'TVhu',
      kTVResChanged        = 'TVrs',
      kTVArtifactsChanged  = 'TVar',
      kTVFringeChanged     = 'TVfr',
      kTVBleedChanged      = 'TVbl',
      kTVBrightChanged     = 'TVbr',
      kTVContrastChanged   = 'TVct',
      kTVSaturChanged      = 'TVsa',
      kTVGammaChanged      = 'TVga',
      kTVScanIntenseChanged= 'TVsc',

      kCloneCompositeCmd   = 'CLcp',
      kCloneSvideoCmd      = 'CLsv',
      kCloneRGBCmd         = 'CLrb',
      kCloneBadCmd         = 'CLbd',
      kCloneCustomCmd      = 'CLcu'
    };
};

#endif
