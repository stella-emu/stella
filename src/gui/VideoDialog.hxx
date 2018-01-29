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
// Copyright (c) 1995-2018 by Bradford W. Mott, Stephen Anthony
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
class PopUpWidget;
class SliderWidget;
class StaticTextWidget;
class TabWidget;
class OSystem;

#include "Dialog.hxx"
#include "NTSCFilter.hxx"
#include "bspf.hxx"

class VideoDialog : public Dialog
{
  public:
    VideoDialog(OSystem& osystem, DialogContainer& parent, const GUI::Font& font,
                int max_w, int max_h);
    virtual ~VideoDialog() = default;

  private:
    void loadConfig() override;
    void saveConfig() override;
    void setDefaults() override;

    void handleTVModeChange(NTSCFilter::Preset);
    void loadTVAdjustables(NTSCFilter::Preset preset);
    void handleCommand(CommandSender* sender, int cmd, int data, int id) override;

  private:
    TabWidget* myTab;

    // General options
    PopUpWidget*      myRenderer;
    PopUpWidget*      myTIAZoom; // TODO: SliderWidget
    PopUpWidget*      myTIAPalette;
    CheckboxWidget*   myFrameTiming;
    CheckboxWidget*   myTIAInterpolate;
    SliderWidget*     myNAspectRatio;
    SliderWidget*     myPAspectRatio;
    SliderWidget*     myFrameRate;

    CheckboxWidget*   myFullscreen;
    //PopUpWidget*      myFullScreenMode;
    CheckboxWidget*   myUseStretch;
    CheckboxWidget*   myUseVSync;
    CheckboxWidget*   myUIMessages;
    CheckboxWidget*   myCenter;
    CheckboxWidget*   myFastSCBios;
    CheckboxWidget*   myUseThreads;

    // TV effects adjustables (custom mode)
    PopUpWidget*      myTVMode;
    SliderWidget*     myTVSharp;
    SliderWidget*     myTVHue;
    SliderWidget*     myTVRes;
    SliderWidget*     myTVArtifacts;
    SliderWidget*     myTVFringe;
    SliderWidget*     myTVBleed;
    SliderWidget*     myTVBright;
    SliderWidget*     myTVContrast;
    SliderWidget*     myTVSatur;
    SliderWidget*     myTVGamma;

    // TV phosphor effect
    CheckboxWidget*   myTVPhosphor;
    SliderWidget*     myTVPhosLevel;

    // TV scanline intensity and interpolation
    StaticTextWidget* myTVScanLabel;
    SliderWidget*     myTVScanIntense;
    CheckboxWidget*   myTVScanInterpolate;

    // TV effects adjustables presets (custom mode)
    ButtonWidget*     myCloneComposite;
    ButtonWidget*     myCloneSvideo;
    ButtonWidget*     myCloneRGB;
    ButtonWidget*     myCloneBad;
    ButtonWidget*     myCloneCustom;

    enum {
      kFrameRateChanged    = 'VDfr',

      kTVModeChanged       = 'VDtv',

      kCloneCompositeCmd   = 'CLcp',
      kCloneSvideoCmd      = 'CLsv',
      kCloneRGBCmd         = 'CLrb',
      kCloneBadCmd         = 'CLbd',
      kCloneCustomCmd      = 'CLcu'
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
