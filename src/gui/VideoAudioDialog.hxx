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

#ifndef VIDEO_AUDIO_DIALOG_HXX
#define VIDEO_AUDIO_DIALOG_HXX

class CommandSender;
class CheckboxWidget;
class ColorWidget;
class DialogContainer;
class PopUpWidget;
class RadioButtonGroup;
class SliderWidget;
class StaticTextWidget;
class EditTextWidget;
class TabWidget;
class TabPaneWidget;
class OSystem;

#include "Dialog.hxx"
#include "PaletteHandler.hxx"
#include "NTSCFilter.hxx"
#include "bspf.hxx"

class VideoAudioDialog : public Dialog
{
  public:
    VideoAudioDialog(OSystem& osystem, DialogContainer& parent,
                     const GUI::Font& font);
    ~VideoAudioDialog() override = default;

    void loadConfig() override;
    void saveConfig() override;
    void setDefaults() override;

  protected:
    void layout() override;
    void handleCommand(CommandSender* sender, int cmd, int data, int id) override;

  private:
    void addDisplayTab();
    void addPaletteTab();
    void addTVEffectsTab();
    void addBezelTab();
    void addAudioTab();

    void handleTVModeChange(NTSCFilter::Preset);
    void loadTVAdjustables(NTSCFilter::Preset preset);
    void handleRendererChanged();
    void handlePaletteChange();
    void handleShiftChanged(SliderWidget* widget);
    void handlePaletteUpdate();
    void handleFullScreenChange();
    void handleOverscanChange();
    void handlePhosphorChange();
    void handleBezelChange();

    void createPaletteWidgets(TabPaneWidget* pane);
    unique_ptr<GUI::Layout> paletteLayout();
    void colorPalette();

    void updatePreset();
    void updateAudioEnabledState();
    void updateSettingsWithPreset(AudioSettings&);

  private:
    TabWidget* myTab{nullptr};

    // General options
    StaticTextWidget* myRendererLabel{nullptr};
    PopUpWidget*      myRenderer{nullptr};
    CheckboxWidget*   myTIAInterpolate{nullptr};
    CheckboxWidget*   myFullscreen{nullptr};
    CheckboxWidget*   myUseStretch{nullptr};
    StaticTextWidget* myTVOverscanLabel{nullptr};
    SliderWidget*     myTVOverscan{nullptr};
    CheckboxWidget*   myRefreshAdapt{nullptr};
    StaticTextWidget* myTIAZoomLabel{nullptr};
    SliderWidget*     myTIAZoom{nullptr};
    CheckboxWidget*   myCorrectAspect{nullptr};
    StaticTextWidget* myVSizeAdjustLabel{nullptr};
    SliderWidget*     myVSizeAdjust{nullptr};
    StaticTextWidget* myDisplayInfo{nullptr};

    // TV effects adjustables (custom mode)
    StaticTextWidget* myTVModeLabel{nullptr};
    PopUpWidget*      myTVMode{nullptr};
    StaticTextWidget* myTVSharpLabel{nullptr};
    SliderWidget*     myTVSharp{nullptr};
    StaticTextWidget* myTVResLabel{nullptr};
    SliderWidget*     myTVRes{nullptr};
    StaticTextWidget* myTVArtifactsLabel{nullptr};
    SliderWidget*     myTVArtifacts{nullptr};
    StaticTextWidget* myTVFringeLabel{nullptr};
    SliderWidget*     myTVFringe{nullptr};
    StaticTextWidget* myTVBleedLabel{nullptr};
    SliderWidget*     myTVBleed{nullptr};

    // TV phosphor effect
    StaticTextWidget* myTVPhosphorLabel{nullptr};
    PopUpWidget*      myTVPhosphor{nullptr};
    StaticTextWidget* myTVPhosLevelLabel{nullptr};
    SliderWidget*     myTVPhosLevel{nullptr};

    // TV scanline intensity and interpolation
    StaticTextWidget* myTVScanLabel{nullptr};
    StaticTextWidget* myTVScanIntenseLabel{nullptr};
    SliderWidget*     myTVScanIntense{nullptr};
    StaticTextWidget* myTVScanMaskLabel{nullptr};
    PopUpWidget*      myTVScanMask{nullptr};

    // TV effects adjustables presets (custom mode)
    ButtonWidget*     myCloneComposite{nullptr};
    ButtonWidget*     myCloneSvideo{nullptr};
    ButtonWidget*     myCloneRGB{nullptr};
    ButtonWidget*     myCloneBad{nullptr};
    ButtonWidget*     myCloneCustom{nullptr};

    // Palettes
    StaticTextWidget* myTIAPaletteLabel{nullptr};
    PopUpWidget*      myTIAPalette{nullptr};
    CheckboxWidget*   myDetectPal60{nullptr};
    CheckboxWidget*   myDetectNtsc50{nullptr};
    StaticTextWidget* myPhaseShiftLabel{nullptr};
    SliderWidget*     myPhaseShift{nullptr};
    StaticTextWidget* myTVRedScaleLabel{nullptr};
    SliderWidget*     myTVRedScale{nullptr};
    SliderWidget*     myTVRedShift{nullptr};
    StaticTextWidget* myTVGreenScaleLabel{nullptr};
    SliderWidget*     myTVGreenScale{nullptr};
    SliderWidget*     myTVGreenShift{nullptr};
    StaticTextWidget* myTVBlueScaleLabel{nullptr};
    SliderWidget*     myTVBlueScale{nullptr};
    SliderWidget*     myTVBlueShift{nullptr};
    StaticTextWidget* myTVHueLabel{nullptr};
    SliderWidget*     myTVHue{nullptr};
    StaticTextWidget* myTVSaturLabel{nullptr};
    SliderWidget*     myTVSatur{nullptr};
    StaticTextWidget* myTVBrightLabel{nullptr};
    SliderWidget*     myTVBright{nullptr};
    StaticTextWidget* myTVContrastLabel{nullptr};
    SliderWidget*     myTVContrast{nullptr};
    StaticTextWidget* myTVGammaLabel{nullptr};
    SliderWidget*     myTVGamma{nullptr};
    StaticTextWidget* myAutodetectLabel{nullptr};
    // The palette: a chroma per row, a luminance per column
    static constexpr int NUM_CHROMA = 16;
    static constexpr int NUM_LUMA = 8;
    std::array<StaticTextWidget*, NUM_CHROMA> myColorLbl{};
    BSPF::array2D<ColorWidget*, NUM_CHROMA, NUM_LUMA> myColor{};

    // Bezels
    CheckboxWidget*   myBezelEnableCheckbox{nullptr};
    ButtonWidget*     myOpenBrowserButton{nullptr};
    EditTextWidget*   myBezelPath{nullptr};
    CheckboxWidget*   myBezelShowWindowed{nullptr};
    CheckboxWidget*   myManualWindow{nullptr};
    StaticTextWidget* myWinLeftSliderLabel{nullptr};
    SliderWidget*     myWinLeftSlider{nullptr};
    StaticTextWidget* myWinRightSliderLabel{nullptr};
    SliderWidget*     myWinRightSlider{nullptr};
    StaticTextWidget* myWinTopSliderLabel{nullptr};
    SliderWidget*     myWinTopSlider{nullptr};
    StaticTextWidget* myWinBottomSliderLabel{nullptr};
    SliderWidget*     myWinBottomSlider{nullptr};

    // Audio
    CheckboxWidget*   mySoundEnableCheckbox{nullptr};
    StaticTextWidget* myVolumeSliderLabel{nullptr};
    SliderWidget*     myVolumeSlider{nullptr};
    CheckboxWidget*   myStereoSoundCheckbox{nullptr};
    StaticTextWidget* myModePopupLabel{nullptr};
    PopUpWidget*      myModePopup{nullptr};
    StaticTextWidget* myFreqPopupLabel{nullptr};
    PopUpWidget*      myFreqPopup{nullptr};
    StaticTextWidget* myResamplingPopupLabel{nullptr};
    PopUpWidget*      myResamplingPopup{nullptr};
    StaticTextWidget* myHeadroomSliderLabel{nullptr};
    SliderWidget*     myHeadroomSlider{nullptr};
    StaticTextWidget* myBufferSizeSliderLabel{nullptr};
    SliderWidget*     myBufferSizeSlider{nullptr};
    StaticTextWidget* myDpcPitchLabel{nullptr};
    SliderWidget*     myDpcPitch{nullptr};

    string            myPalette;
    PaletteHandler::Adjustable myPaletteAdj;

    enum {
      kRendererChanged    = 'VDRe',
      kZoomChanged        = 'VDZo',
      kVSizeChanged       = 'VDVs',
      kFullScreenChanged  = 'VDFs',
      kOverscanChanged    = 'VDOv',

      kPaletteChanged     = 'VDpl',
      kPhaseShiftChanged  = 'VDps',
      kRedShiftChanged    = 'VDrs',
      kGreenShiftChanged  = 'VDgs',
      kBlueShiftChanged   = 'VDbs',
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

      kBezelEnableChanged = 'BZen',
      kChooseBezelDirCmd  = 'BZsl',
      kAutoWindowChanged = 'BZab',

      kSoundEnableChanged = 'ADse',
      kDeviceChanged      = 'ADdc',
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

#endif  // VIDEO_AUDIO_DIALOG_HXX
