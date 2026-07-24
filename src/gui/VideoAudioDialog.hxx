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
class LabelWidget;
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
    VideoAudioDialog(OSystem& osystem, DialogContainer& parent, const GUI::Font& font);
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
    LabelWidget* myRendererLbl{nullptr};
    PopUpWidget*    myRenderer{nullptr};
    CheckboxWidget* myTIAInterpolate{nullptr};
    CheckboxWidget* myFullscreen{nullptr};
    CheckboxWidget* myUseStretch{nullptr};
    LabelWidget*    myTVOverscanLbl{nullptr};
    SliderWidget*   myTVOverscan{nullptr};
    CheckboxWidget* myRefreshAdapt{nullptr};
    LabelWidget*    myTIAZoomLbl{nullptr};
    SliderWidget*   myTIAZoom{nullptr};
    CheckboxWidget* myCorrectAspect{nullptr};
    LabelWidget*    myVSizeAdjustLbl{nullptr};
    SliderWidget*   myVSizeAdjust{nullptr};
    LabelWidget*    myDisplayInfo{nullptr};

    // TV effects adjustables (custom mode)
    LabelWidget*    myTVModeLbl{nullptr};
    PopUpWidget*    myTVMode{nullptr};
    LabelWidget*    myTVSharpLbl{nullptr};
    SliderWidget*   myTVSharp{nullptr};
    LabelWidget*    myTVResLbl{nullptr};
    SliderWidget*   myTVRes{nullptr};
    LabelWidget*    myTVArtifactsLbl{nullptr};
    SliderWidget*   myTVArtifacts{nullptr};
    LabelWidget*    myTVFringeLbl{nullptr};
    SliderWidget*   myTVFringe{nullptr};
    LabelWidget*    myTVBleedLbl{nullptr};
    SliderWidget*   myTVBleed{nullptr};

    // TV phosphor effect
    LabelWidget*    myTVPhosphorLbl{nullptr};
    PopUpWidget*    myTVPhosphor{nullptr};
    LabelWidget*    myTVPhosLevelLbl{nullptr};
    SliderWidget*   myTVPhosLevel{nullptr};

    // TV scanline intensity and interpolation
    LabelWidget*    myTVScanLbl{nullptr};
    LabelWidget*    myTVScanIntenseLbl{nullptr};
    SliderWidget*   myTVScanIntense{nullptr};
    LabelWidget*    myTVScanMaskLbl{nullptr};
    PopUpWidget*    myTVScanMask{nullptr};

    // TV effects adjustables presets (custom mode)
    ButtonWidget*   myCloneComposite{nullptr};
    ButtonWidget*   myCloneSvideo{nullptr};
    ButtonWidget*   myCloneRGB{nullptr};
    ButtonWidget*   myCloneBad{nullptr};
    ButtonWidget*   myCloneCustom{nullptr};

    // Palettes
    LabelWidget*    myTIAPaletteLbl{nullptr};
    PopUpWidget*    myTIAPalette{nullptr};
    CheckboxWidget* myDetectPal60{nullptr};
    CheckboxWidget* myDetectNtsc50{nullptr};
    LabelWidget*    myPhaseShiftLbl{nullptr};
    SliderWidget*   myPhaseShift{nullptr};
    LabelWidget*    myTVRedScaleLbl{nullptr};
    SliderWidget*   myTVRedScale{nullptr};
    SliderWidget*   myTVRedShift{nullptr};
    LabelWidget*    myTVGreenScaleLbl{nullptr};
    SliderWidget*   myTVGreenScale{nullptr};
    SliderWidget*   myTVGreenShift{nullptr};
    LabelWidget*    myTVBlueScaleLbl{nullptr};
    SliderWidget*   myTVBlueScale{nullptr};
    SliderWidget*   myTVBlueShift{nullptr};
    LabelWidget*    myTVHueLbl{nullptr};
    SliderWidget*   myTVHue{nullptr};
    LabelWidget*    myTVSaturLbl{nullptr};
    SliderWidget*   myTVSatur{nullptr};
    LabelWidget*    myTVBrightLbl{nullptr};
    SliderWidget*   myTVBright{nullptr};
    LabelWidget*    myTVContrastLbl{nullptr};
    SliderWidget*   myTVContrast{nullptr};
    LabelWidget*    myTVGammaLbl{nullptr};
    SliderWidget*   myTVGamma{nullptr};
    LabelWidget*    myAutodetectLbl{nullptr};
    // The palette: a chroma per row, a luminance per column
    static constexpr int NUM_CHROMA = 16;
    static constexpr int NUM_LUMA = 8;
    std::array<LabelWidget*, NUM_CHROMA> myColorLbl{};
    BSPF::array2D<ColorWidget*, NUM_CHROMA, NUM_LUMA> myColor{};

    // Bezels
    CheckboxWidget* myBezelEnableCheckbox{nullptr};
    ButtonWidget*   myOpenBrowserButton{nullptr};
    EditTextWidget* myBezelPath{nullptr};
    CheckboxWidget* myBezelShowWindowed{nullptr};
    CheckboxWidget* myManualWindow{nullptr};
    LabelWidget*    myWinLeftSliderLbl{nullptr};
    SliderWidget*   myWinLeftSlider{nullptr};
    LabelWidget*    myWinRightSliderLbl{nullptr};
    SliderWidget*   myWinRightSlider{nullptr};
    LabelWidget*    myWinTopSliderLbl{nullptr};
    SliderWidget*   myWinTopSlider{nullptr};
    LabelWidget*    myWinBottomSliderLbl{nullptr};
    SliderWidget*   myWinBottomSlider{nullptr};

    // Audio
    CheckboxWidget* mySoundEnableCheckbox{nullptr};
    LabelWidget*    myVolumeSliderLbl{nullptr};
    SliderWidget*   myVolumeSlider{nullptr};
    CheckboxWidget* myStereoSoundCheckbox{nullptr};
    LabelWidget*    myModePopupLbl{nullptr};
    PopUpWidget*    myModePopup{nullptr};
    LabelWidget*    myFreqPopupLbl{nullptr};
    PopUpWidget*    myFreqPopup{nullptr};
    LabelWidget*    myResamplingPopupLbl{nullptr};
    PopUpWidget*    myResamplingPopup{nullptr};
    LabelWidget*    myHeadroomSliderLbl{nullptr};
    SliderWidget*   myHeadroomSlider{nullptr};
    LabelWidget*    myBufferSizeSliderLbl{nullptr};
    SliderWidget*   myBufferSizeSlider{nullptr};
    LabelWidget*    myDpcPitchLbl{nullptr};
    SliderWidget*   myDpcPitch{nullptr};

    string myPalette;
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
      kAutoWindowChanged  = 'BZab',

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
