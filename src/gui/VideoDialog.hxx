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
// Copyright (c) 1995-2017 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#ifndef VIDEO_DIALOG_HXX
#define VIDEO_DIALOG_HXX

class CommandSender;
class CheckboxWidget;
class DialogContainer;
class ColorWidget;
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
    VideoDialog(OSystem& osystem, DialogContainer& parent, const GUI::Font& font,
                int max_w, int max_h, bool isGlobal);
    virtual ~VideoDialog() = default;

  private:
    void loadConfig() override;
    void saveConfig() override;
    void setDefaults() override;

    void handleFullscreenChange(bool enable);
    void handleTVModeChange(NTSCFilter::Preset);
    void handleTVJitterChange(bool enable);
    void handleDebugColours(int cmd, int color);
    void handleDebugColours(const string& colors);
    void loadTVAdjustables(NTSCFilter::Preset preset);
    void handleCommand(CommandSender* sender, int cmd, int data, int id) override;

  private:
    TabWidget* myTab;

    // General options
    PopUpWidget*      myRenderer;
    PopUpWidget*      myTIAZoom;
    PopUpWidget*      myTIAPalette;
    PopUpWidget*      myFrameTiming;
    PopUpWidget*      myTIAInterpolate;
    SliderWidget*     myNAspectRatio;
    StaticTextWidget* myNAspectRatioLabel;
    SliderWidget*     myPAspectRatio;
    StaticTextWidget* myPAspectRatioLabel;

    SliderWidget*     myFrameRate;
    StaticTextWidget* myFrameRateLabel;
    CheckboxWidget*   myFullscreen;
    CheckboxWidget*   myUseStretch;
    CheckboxWidget*   myUseVSync;
    CheckboxWidget*   myColorLoss;
    CheckboxWidget*   myUIMessages;
    CheckboxWidget*   myCenter;
    CheckboxWidget*   myFastSCBios;

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

    // TV jitter effects
    CheckboxWidget*   myTVJitter;
    SliderWidget*     myTVJitterRec;
    StaticTextWidget* myTVJitterRecLabel;

    // TV phosphor effect
    PopUpWidget*      myTVPhosphor;
    SliderWidget*     myTVPhosLevel;
    StaticTextWidget* myTVPhosLevelLabel;

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

    // Debug colours selection
    PopUpWidget* myDbgColour[6];
    ColorWidget* myDbgColourSwatch[6];

    enum {
      kNAspectRatioChanged = 'VDan',
      kPAspectRatioChanged = 'VDap',
      kFrameRateChanged    = 'VDfr',

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

      kTVJitterChanged     = 'TVjt',
      kTVJitterRecChanged  = 'TVjr',

      kTVPhosLevelChanged  = 'TVpl',

      kCloneCompositeCmd   = 'CLcp',
      kCloneSvideoCmd      = 'CLsv',
      kCloneRGBCmd         = 'CLrb',
      kCloneBadCmd         = 'CLbd',
      kCloneCustomCmd      = 'CLcu',

      kP0ColourChangedCmd  = 'GOp0',
      kM0ColourChangedCmd  = 'GOm0',
      kP1ColourChangedCmd  = 'GOp1',
      kM1ColourChangedCmd  = 'GOm1',
      kPFColourChangedCmd  = 'GOpf',
      kBLColourChangedCmd  = 'GObl'
    };

  private:
    // Following constructors and assignment operators not supported
    VideoDialog() = delete;
    VideoDialog(const VideoDialog&) = delete;
    VideoDialog(VideoDialog&&) = delete;
    VideoDialog& operator=(const VideoDialog&) = delete;
    VideoDialog& operator=(VideoDialog&&) = delete;
};

#endif
