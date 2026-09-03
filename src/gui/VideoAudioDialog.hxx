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

/**
  Dialog for editing video and audio settings across five tabs:
  Display, Palettes, TV Effects, Bezels, and Audio.

  @author  Stephen Anthony and Thomas Jentzsch
*/
class VideoAudioDialog : public Dialog
{
  public:
    VideoAudioDialog(OSystem& osystem, DialogContainer& parent, const GUI::Font& font);
    ~VideoAudioDialog() override = default;

    // Populates every tab from current settings
    void loadConfig() override;
    // Writes every tab back to settings, re-initializing the framebuffer/
    // TIA surface/audio as needed
    void saveConfig() override;
    // Resets the currently active tab to hardcoded defaults
    void setDefaults() override;

  protected:
    void layout() override;
    // OK saves and exits; Close reverts the live palette preview back to
    // what was loaded (see myPalette/myPaletteAdj); Defaults resets the
    // active tab; the remaining ids update slider/pop-up labels and enabled
    // state, or open the bezel-path browser
    void handleCommand(CommandSender* sender, GuiCmd::Code cmd, int data, int id) override;

  private:
    // Builds the 'Display' tab: renderer, zoom, fullscreen/stretch,
    // overscan, aspect ratio, V-Size
    void addDisplayTab();
    // Builds the 'Palettes' tab: TIA palette choice, custom R/G/B and
    // phase/hue/saturation/etc. adjustables, and the resulting color grid
    void addPaletteTab();
    // Builds the 'TV Effects' tab: NTSC filter mode and its custom
    // adjustables, phosphor, scanlines
    void addTVEffectsTab();
    // Builds the 'Bezels' tab: enable/path, and the manual
    // emulation-window sliders
    void addBezelTab();
    // Builds the 'Audio' tab: volume, mode preset, sample rate/resampling/
    // headroom/buffer size, DPC pitch
    void addAudioTab();

    // Enables the custom TV-effect sliders and clone buttons only when
    // 'Custom' is selected
    void handleTVModeChange(NTSCFilter::Preset);
    // Loads the given preset's sharpness/resolution/artifacts/fringing/
    // bleed into the custom sliders
    void loadTVAdjustables(NTSCFilter::Preset preset);
    // Disables interpolation when the software renderer is selected
    void handleRendererChanged();
    // Enables the custom-palette adjustables only when the 'Custom'
    // palette is selected
    void handlePaletteChange();
    // Formats a phase/R/G/B shift slider's value as degrees, then applies
    // the live palette preview
    void handleShiftChanged(SliderWidget* widget);
    // Applies the current adjustables as a live palette preview, and
    // refreshes the color grid
    void handlePaletteUpdate();
    // Enables the stretch/refresh-rate/overscan controls only in
    // fullscreen mode
    void handleFullScreenChange();
    // Blanks the value label and unit when overscan is 0, else shows it
    // as a percentage
    void handleOverscanChange();
    // Enables the phosphor blend slider unless phosphor mode is 'by ROM'
    void handlePhosphorChange();
    // Enables the bezel path/windowed controls when bezels are on, and
    // the manual window sliders when auto window detection is off
    void handleBezelChange();

    // Creates the chroma-digit labels and the color swatch grid
    void createPaletteWidgets(TabPaneWidget* pane);
    // Lays out the color grid: a hex-digit column plus one column per
    // luminance
    unique_ptr<GUI::Layout> paletteLayout();
    // Fills the color grid from the running console's palette, or blanks
    // it when no console is active
    void colorPalette();

    // Previews the selected audio preset's values, without persisting
    // them, in the custom controls
    void updatePreset();
    // Enables/disables every audio control based on the enable checkbox,
    // the preset, and whether Pitfall II is loaded
    void updateAudioEnabledState();
    // Loads sample rate/headroom/buffer size/resampling quality from the
    // given settings
    void updateSettingsWithPreset(AudioSettings&);

  private:
    // Hosts the settings tabs (Display, Palettes, TV Effects, Bezels when
    // built with image support, Audio)
    TabWidget* myTab{nullptr};

    // General options
    LabelWidget* myRendererLbl{nullptr};
    PopUpWidget*    myRenderer{nullptr};
    LabelWidget*    myRendererDetected{nullptr};
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

    // The palette name and adjustables in effect when the dialog opened;
    // restored on Close (see handleCommand())
    string myPalette;
    PaletteHandler::Adjustable myPaletteAdj;

    // Command ids dispatched in handleCommand()
    struct Cmd {
      static constexpr GuiCmd::Code
        RendererChanged      = GuiCmd::of("VideoAudioDialog.RendererChanged"),
        ZoomChanged          = GuiCmd::of("VideoAudioDialog.ZoomChanged"),
        VSizeChanged         = GuiCmd::of("VideoAudioDialog.VSizeChanged"),
        FullScreenChanged    = GuiCmd::of("VideoAudioDialog.FullScreenChanged"),
        OverscanChanged      = GuiCmd::of("VideoAudioDialog.OverscanChanged"),
        PaletteChanged       = GuiCmd::of("VideoAudioDialog.PaletteChanged"),
        PhaseShiftChanged    = GuiCmd::of("VideoAudioDialog.PhaseShiftChanged"),
        RedShiftChanged      = GuiCmd::of("VideoAudioDialog.RedShiftChanged"),
        GreenShiftChanged    = GuiCmd::of("VideoAudioDialog.GreenShiftChanged"),
        BlueShiftChanged     = GuiCmd::of("VideoAudioDialog.BlueShiftChanged"),
        PaletteUpdated       = GuiCmd::of("VideoAudioDialog.PaletteUpdated"),
        TvModeChanged        = GuiCmd::of("VideoAudioDialog.TvModeChanged"),
        CloneComposite       = GuiCmd::of("VideoAudioDialog.CloneComposite"),
        CloneSvideo          = GuiCmd::of("VideoAudioDialog.CloneSvideo"),
        CloneRGB             = GuiCmd::of("VideoAudioDialog.CloneRGB"),
        CloneBad             = GuiCmd::of("VideoAudioDialog.CloneBad"),
        CloneCustom          = GuiCmd::of("VideoAudioDialog.CloneCustom"),
        PhosphorChanged      = GuiCmd::of("VideoAudioDialog.PhosphorChanged"),
        PhosphorBlendChanged = GuiCmd::of("VideoAudioDialog.PhosphorBlendChanged"),
        ScanlinesChanged     = GuiCmd::of("VideoAudioDialog.ScanlinesChanged"),
        BezelEnableChanged   = GuiCmd::of("VideoAudioDialog.BezelEnableChanged"),
        ChooseBezelDir       = GuiCmd::of("VideoAudioDialog.ChooseBezelDir"),
        AutoWindowChanged    = GuiCmd::of("VideoAudioDialog.AutoWindowChanged"),
        SoundEnableChanged   = GuiCmd::of("VideoAudioDialog.SoundEnableChanged"),
        DeviceChanged        = GuiCmd::of("VideoAudioDialog.DeviceChanged"),
        ModeChanged          = GuiCmd::of("VideoAudioDialog.ModeChanged"),
        HeadroomChanged      = GuiCmd::of("VideoAudioDialog.HeadroomChanged"),
        BufferSizeChanged    = GuiCmd::of("VideoAudioDialog.BufferSizeChanged");
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
