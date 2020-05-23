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
// Copyright (c) 1995-2020 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#ifndef VIDEOAUDIO_DIALOG_HXX
#define VIDEOAUDIO_DIALOG_HXX

class CommandSender;
class CheckboxWidget;
class ColorWidget;
class DialogContainer;
class PopUpWidget;
class RadioButtonGroup;
class SliderWidget;
class StaticTextWidget;
class TabWidget;
class OSystem;

#include "Dialog.hxx"
#include "PaletteHandler.hxx"
#include "NTSCFilter.hxx"
#include "bspf.hxx"

class VideoAudioDialog : public Dialog
{
  public:
    VideoAudioDialog(OSystem& osystem, DialogContainer& parent, const GUI::Font& font,
                int max_w, int max_h);
    virtual ~VideoAudioDialog() = default;

  private:
    void loadConfig() override;
    void saveConfig() override;
    void setDefaults() override;

    void addDisplayTab();
    void addPaletteTab();
    void addTVEffectsTab();
    void addAudioTab();
    void handleTVModeChange(NTSCFilter::Preset);
    void loadTVAdjustables(NTSCFilter::Preset preset);
    void handlePaletteChange();
    void handlePaletteUpdate();
    void handleFullScreenChange();
    void handleOverscanChange();
    void handlePhosphorChange();
    void handleCommand(CommandSender* sender, int cmd, int data, int id) override;
    void addPalette(int x, int y, int h, int w);
    void colorPalette();
    void updatePreset();
    void updateEnabledState();
    void updateSettingsWithPreset(AudioSettings&);

  private:
    TabWidget* myTab;

    // General options
    PopUpWidget*      myRenderer{nullptr};
    CheckboxWidget*   myTIAInterpolate{nullptr};
    CheckboxWidget*   myFullscreen{nullptr};
    CheckboxWidget*   myUseStretch{nullptr};
    SliderWidget*     myTVOverscan{nullptr};
  #ifndef BSPF_MACOS
    CheckboxWidget*   myRefreshAdapt{nullptr};
  #endif
    SliderWidget*     myTIAZoom{nullptr};
    SliderWidget*     myVSizeAdjust{nullptr};

    // TV effects adjustables (custom mode)
    PopUpWidget*      myTVMode{nullptr};
    SliderWidget*     myTVSharp{nullptr};
    SliderWidget*     myTVRes{nullptr};
    SliderWidget*     myTVArtifacts{nullptr};
    SliderWidget*     myTVFringe{nullptr};
    SliderWidget*     myTVBleed{nullptr};

    // TV phosphor effect
    CheckboxWidget*   myTVPhosphor{nullptr};
    SliderWidget*     myTVPhosLevel{nullptr};

    // TV scanline intensity and interpolation
    StaticTextWidget* myTVScanLabel{nullptr};
    SliderWidget*     myTVScanIntense{nullptr};

    // TV effects adjustables presets (custom mode)
    ButtonWidget*     myCloneComposite{nullptr};
    ButtonWidget*     myCloneSvideo{nullptr};
    ButtonWidget*     myCloneRGB{nullptr};
    ButtonWidget*     myCloneBad{nullptr};
    ButtonWidget*     myCloneCustom{nullptr};

    // Palettes
    PopUpWidget*      myTIAPalette{nullptr};
    SliderWidget*     myPhaseShiftNtsc{nullptr};
    SliderWidget*     myPhaseShiftPal{nullptr};
    SliderWidget*     myTVHue{nullptr};
    SliderWidget*     myTVSatur{nullptr};
    SliderWidget*     myTVBright{nullptr};
    SliderWidget*     myTVContrast{nullptr};
    SliderWidget*     myTVGamma{nullptr};
    std::array<StaticTextWidget*, 16> myColorLbl{nullptr};
    ColorWidget*      myColor[16][8]{{nullptr}};

    // Audio
    CheckboxWidget*   mySoundEnableCheckbox{nullptr};
    SliderWidget*     myVolumeSlider{nullptr};
    CheckboxWidget*   myStereoSoundCheckbox{nullptr};
    PopUpWidget*      myModePopup{nullptr};
    PopUpWidget*      myFragsizePopup{nullptr};
    PopUpWidget*      myFreqPopup{nullptr};
    PopUpWidget*      myResamplingPopup{nullptr};
    SliderWidget*     myHeadroomSlider{nullptr};
    SliderWidget*     myBufferSizeSlider{nullptr};
    SliderWidget*     myDpcPitch{nullptr};

    string            myPalette;
    PaletteHandler::Adjustable myPaletteAdj;

    enum {
      kZoomChanged        = 'VDZo',
      kVSizeChanged       = 'VDVs',
      kFullScreenChanged  = 'VDFs',
      kOverscanChanged    = 'VDOv',

      kPaletteChanged     = 'VDpl',
      kNtscShiftChanged   = 'VDns',
      kPalShiftChanged    = 'VDps',
      kPaletteUpdated     = 'VDpu',

      kTVModeChanged      = 'VDtv',
      kCloneCompositeCmd  = 'CLcp',
      kCloneSvideoCmd     = 'CLsv',
      kCloneRGBCmd        = 'CLrb',
      kCloneBadCmd        = 'CLbd',
      kCloneCustomCmd     = 'CLcu',
      kPhosphorChanged    = 'VDph',
      kPhosBlendChanged   = 'VDbl',
      kScanlinesChanged   = 'VDsc',

      kSoundEnableChanged = 'ADse',
      kModeChanged        = 'ADmc',
      kHeadroomChanged    = 'ADhc',
      kBufferSizeChanged  = 'ADbc'
    };

  private:
    // Following constructors and assignment operators not supported
    VideoAudioDialog() = delete;
    VideoAudioDialog(const VideoAudioDialog&) = delete;
    VideoAudioDialog(VideoAudioDialog&&) = delete;
    VideoAudioDialog& operator=(const VideoAudioDialog&) = delete;
    VideoAudioDialog& operator=(VideoAudioDialog&&) = delete;
};

#endif
