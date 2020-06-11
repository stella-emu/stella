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

#include <cmath>

#include "bspf.hxx"
#include "Base.hxx"
#include "Control.hxx"
#include "Cart.hxx"
#include "CartDPC.hxx"
#include "Dialog.hxx"
#include "Menu.hxx"
#include "OSystem.hxx"
#include "EditTextWidget.hxx"
#include "PopUpWidget.hxx"
#include "ColorWidget.hxx"
#include "Console.hxx"
#include "PaletteHandler.hxx"
#include "TIA.hxx"
#include "Settings.hxx"
#include "Sound.hxx"
#include "AudioSettings.hxx"
#include "Widget.hxx"
#include "Font.hxx"
#include "TabWidget.hxx"
#include "NTSCFilter.hxx"
#include "TIASurface.hxx"

#include "VideoAudioDialog.hxx"

#define CREATE_CUSTOM_SLIDERS(obj, desc, cmd)                            \
  myTV ## obj =                                                          \
    new SliderWidget(myTab, _font, xpos, ypos-1, swidth, lineHeight,     \
                     desc, lwidth, cmd, fontWidth*4, "%");               \
  myTV ## obj->setMinValue(0); myTV ## obj->setMaxValue(100);            \
  myTV ## obj->setStepValue(1);                                          \
  myTV ## obj->setTickmarkIntervals(2);                                  \
  wid.push_back(myTV ## obj);                                            \
  ypos += lineHeight + VGAP;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
VideoAudioDialog::VideoAudioDialog(OSystem& osystem, DialogContainer& parent,
                         const GUI::Font& font, int max_w, int max_h)
  : Dialog(osystem, parent, font, "Video & Audio settings")
{
  const int lineHeight   = _font.getLineHeight(),
            fontHeight   = _font.getFontHeight(),
            fontWidth    = _font.getMaxCharWidth(),
            buttonHeight = _font.getLineHeight() * 1.25;
  const int VGAP = fontHeight / 4;
  const int VBORDER = fontHeight / 2;
  const int HBORDER = fontWidth * 1.25;
  int xpos, ypos;

  // Set real dimensions
  setSize(44 * fontWidth + HBORDER * 2 + PopUpWidget::dropDownWidth(font) * 2,
          _th + VGAP * 6 + lineHeight + 10 * (lineHeight + VGAP) + buttonHeight + VBORDER * 3,
          max_w, max_h);

  // The tab widget
  xpos = 2;  ypos = VGAP;
  myTab = new TabWidget(this, font, xpos, ypos + _th,
                        _w - 2*xpos,
                        _h - _th - VGAP - buttonHeight - VBORDER * 2);
  addTabWidget(myTab);

  addDisplayTab();
  addPaletteTab();
  addTVEffectsTab();
  addAudioTab();

  // Add Defaults, OK and Cancel buttons
  WidgetArray wid;
  addDefaultsOKCancelBGroup(wid, _font);
  addBGroupToFocusList(wid);

  // Activate the first tab
  myTab->setActiveTab(0);

  // Disable certain functions when we know they aren't present
#ifndef WINDOWED_SUPPORT
  myFullscreen->clearFlags(Widget::FLAG_ENABLED);
  myUseStretch->clearFlags(Widget::FLAG_ENABLED);
  myTVOverscan->clearFlags(Widget::FLAG_ENABLED);
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VideoAudioDialog::addDisplayTab()
{
  const int lineHeight   = _font.getLineHeight(),
            fontHeight   = _font.getFontHeight(),
            fontWidth    = _font.getMaxCharWidth();
  const int VGAP = fontHeight / 4;
  const int VBORDER = fontHeight / 2;
  const int HBORDER = fontWidth * 1.25;
  const int INDENT = CheckboxWidget::prefixSize(_font);
  const int lwidth = _font.getStringWidth("V-Size adjust "),
            pwidth = _font.getStringWidth("OpenGLES2");
  int xpos = HBORDER,
      ypos = VBORDER;
  WidgetArray wid;
  VariantList items;
  const int tabID = myTab->addTab(" Display ", TabWidget::AUTO_WIDTH);

  // Video renderer
  myRenderer = new PopUpWidget(myTab, _font, xpos, ypos, pwidth, lineHeight,
                               instance().frameBuffer().supportedRenderers(),
                               "Renderer ", lwidth);
  wid.push_back(myRenderer);
  const int swidth = myRenderer->getWidth() - lwidth;
  ypos += lineHeight + VGAP;

  // TIA interpolation
  myTIAInterpolate = new CheckboxWidget(myTab, _font, xpos, ypos + 1, "Interpolation ");
  wid.push_back(myTIAInterpolate);  ypos += lineHeight + VGAP * 4;

  // TIA zoom levels (will be dynamically filled later)
  myTIAZoom = new SliderWidget(myTab, _font, xpos, ypos - 1, swidth, lineHeight,
                               "Zoom ", lwidth, 0, fontWidth * 4, "%");
  myTIAZoom->setMinValue(200); myTIAZoom->setStepValue(FrameBuffer::ZOOM_STEPS * 100);
  wid.push_back(myTIAZoom);
  ypos += lineHeight + VGAP;

  // Fullscreen
  myFullscreen = new CheckboxWidget(myTab, _font, xpos, ypos + 1, "Fullscreen", kFullScreenChanged);
  wid.push_back(myFullscreen);
  ypos += lineHeight + VGAP;

  // FS stretch
  myUseStretch = new CheckboxWidget(myTab, _font, xpos + INDENT, ypos + 1, "Stretch");
  wid.push_back(myUseStretch);

#ifdef ADAPTABLE_REFRESH_SUPPORT
  // Adapt refresh rate
  ypos += lineHeight + VGAP;
  myRefreshAdapt = new CheckboxWidget(myTab, _font, xpos + INDENT, ypos + 1, "Adapt display refresh rate");
  wid.push_back(myRefreshAdapt);
#else
  myRefreshAdapt = nullptr;
#endif

  // FS overscan
  ypos += lineHeight + VGAP;
  myTVOverscan = new SliderWidget(myTab, _font, xpos + INDENT, ypos - 1, swidth, lineHeight,
                                  "Overscan", lwidth - INDENT, kOverscanChanged, fontWidth * 3, "%");
  myTVOverscan->setMinValue(0); myTVOverscan->setMaxValue(10);
  myTVOverscan->setTickmarkIntervals(2);
  wid.push_back(myTVOverscan);

  // Vertical size
  ypos += lineHeight + VGAP;
  myVSizeAdjust =
    new SliderWidget(myTab, _font, xpos, ypos-1, swidth, lineHeight,
                     "V-Size adjust", lwidth, kVSizeChanged, fontWidth * 7, "%", 0, true);
  myVSizeAdjust->setMinValue(-5); myVSizeAdjust->setMaxValue(5);
  myVSizeAdjust->setTickmarkIntervals(2);
  wid.push_back(myVSizeAdjust);


  // Add items for tab 0
  addToFocusList(wid, myTab, tabID);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VideoAudioDialog::addPaletteTab()
{
  const int lineHeight = _font.getLineHeight(),
            fontHeight = _font.getFontHeight(),
            fontWidth = _font.getMaxCharWidth();
  const int VBORDER = fontHeight / 2;
  const int HBORDER = fontWidth * 1.25;
  const int INDENT = fontWidth * 2;
  const int VGAP = fontHeight / 4;
  const int lwidth = _font.getStringWidth("  NTSC phase ");
  const int pwidth = _font.getStringWidth("Standard");
  int xpos = HBORDER,
      ypos = VBORDER;
  WidgetArray wid;
  VariantList items;
  const int tabID = myTab->addTab(" Palettes ", TabWidget::AUTO_WIDTH);

  // TIA Palette
  items.clear();
  VarList::push_back(items, "Standard", PaletteHandler::SETTING_STANDARD);
  VarList::push_back(items, "z26", PaletteHandler::SETTING_Z26);
  if (instance().checkUserPalette())
    VarList::push_back(items, "User", PaletteHandler::SETTING_USER);
  VarList::push_back(items, "Custom", PaletteHandler::SETTING_CUSTOM);
  myTIAPalette = new PopUpWidget(myTab, _font, xpos, ypos, pwidth,
                                 lineHeight, items, "Palette ", lwidth, kPaletteChanged);
  wid.push_back(myTIAPalette);
  ypos += lineHeight + VGAP;

  const int swidth = myTIAPalette->getWidth() - lwidth;
  const int plWidth = _font.getStringWidth("NTSC phase ");
  const int pswidth = swidth - INDENT + lwidth - plWidth;

  myPhaseShiftNtsc =
    new SliderWidget(myTab, _font, xpos + INDENT, ypos-1, pswidth, lineHeight,
                     "NTSC phase", plWidth, kNtscShiftChanged, fontWidth * 5);
  myPhaseShiftNtsc->setMinValue((PaletteHandler::DEF_NTSC_SHIFT - PaletteHandler::MAX_SHIFT) * 10);
  myPhaseShiftNtsc->setMaxValue((PaletteHandler::DEF_NTSC_SHIFT + PaletteHandler::MAX_SHIFT) * 10);
  myPhaseShiftNtsc->setTickmarkIntervals(4);
  wid.push_back(myPhaseShiftNtsc);
  ypos += lineHeight + VGAP;

  myPhaseShiftPal =
    new SliderWidget(myTab, _font, xpos + INDENT, ypos-1, pswidth, lineHeight,
                     "PAL phase", plWidth, kPalShiftChanged, fontWidth * 5);
  myPhaseShiftPal->setMinValue((PaletteHandler::DEF_PAL_SHIFT - PaletteHandler::MAX_SHIFT) * 10);
  myPhaseShiftPal->setMaxValue((PaletteHandler::DEF_PAL_SHIFT + PaletteHandler::MAX_SHIFT) * 10);
  myPhaseShiftPal->setTickmarkIntervals(4);
  wid.push_back(myPhaseShiftPal);
  ypos += lineHeight + VGAP;

  CREATE_CUSTOM_SLIDERS(Hue, "Hue ", kPaletteUpdated)
  CREATE_CUSTOM_SLIDERS(Satur, "Saturation ", kPaletteUpdated)
  CREATE_CUSTOM_SLIDERS(Contrast, "Contrast ", kPaletteUpdated)
  CREATE_CUSTOM_SLIDERS(Bright, "Brightness ", kPaletteUpdated)
  CREATE_CUSTOM_SLIDERS(Gamma, "Gamma ", kPaletteUpdated)

  // The resulting palette
  xpos = myPhaseShiftNtsc->getRight() + fontWidth * 2;
  addPalette(xpos, VBORDER, _w - 2 * 2 - HBORDER - xpos,
             myTVGamma->getBottom() -  myTIAPalette->getTop());

  // Add items for tab 2
  addToFocusList(wid, myTab, tabID);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VideoAudioDialog::addTVEffectsTab()
{
  const int lineHeight = _font.getLineHeight(),
            fontHeight = _font.getFontHeight(),
            fontWidth = _font.getMaxCharWidth(),
            buttonHeight = _font.getLineHeight() * 1.25;
  const int VBORDER = fontHeight / 2;
  const int HBORDER = fontWidth * 1.25;
  const int INDENT = CheckboxWidget::prefixSize(_font);// fontWidth * 2;
  const int VGAP = fontHeight / 4;
  int xpos = HBORDER,
      ypos = VBORDER;
  const int lwidth = _font.getStringWidth("Saturation ");
  const int pwidth = _font.getStringWidth("Bad adjust  ");
  WidgetArray wid;
  VariantList items;
  const int tabID = myTab->addTab(" TV Effects ", TabWidget::AUTO_WIDTH);

  items.clear();
  VarList::push_back(items, "Disabled", static_cast<uInt32>(NTSCFilter::Preset::OFF));
  VarList::push_back(items, "RGB", static_cast<uInt32>(NTSCFilter::Preset::RGB));
  VarList::push_back(items, "S-Video", static_cast<uInt32>(NTSCFilter::Preset::SVIDEO));
  VarList::push_back(items, "Composite", static_cast<uInt32>(NTSCFilter::Preset::COMPOSITE));
  VarList::push_back(items, "Bad adjust", static_cast<uInt32>(NTSCFilter::Preset::BAD));
  VarList::push_back(items, "Custom", static_cast<uInt32>(NTSCFilter::Preset::CUSTOM));
  myTVMode = new PopUpWidget(myTab, _font, xpos, ypos, pwidth, lineHeight,
                             items, "TV mode ", 0, kTVModeChanged);
  wid.push_back(myTVMode);
  ypos += lineHeight + VGAP;

  // Custom adjustables (using macro voodoo)
  const int swidth = myTVMode->getWidth() - INDENT - lwidth;
  xpos += INDENT;

  CREATE_CUSTOM_SLIDERS(Sharp, "Sharpness ", 0)
  CREATE_CUSTOM_SLIDERS(Res, "Resolution ", 0)
  CREATE_CUSTOM_SLIDERS(Artifacts, "Artifacts ", 0)
  CREATE_CUSTOM_SLIDERS(Fringe, "Fringing ", 0)
  CREATE_CUSTOM_SLIDERS(Bleed, "Bleeding ", 0)

  ypos += VGAP * 3;

  xpos = HBORDER;

  // TV Phosphor effect
  myTVPhosphor = new CheckboxWidget(myTab, _font, xpos, ypos + 1, "Phosphor for all ROMs", kPhosphorChanged);
  wid.push_back(myTVPhosphor);
  ypos += lineHeight + VGAP / 2;

  // TV Phosphor blend level
  xpos += INDENT;
  CREATE_CUSTOM_SLIDERS(PhosLevel, "Blend", kPhosBlendChanged)
  ypos += VGAP;

  // Scanline intensity and interpolation
  xpos -= INDENT;
  myTVScanLabel = new StaticTextWidget(myTab, _font, xpos, ypos, "Scanlines:");
  ypos += lineHeight + VGAP / 2;

  xpos += INDENT;
  CREATE_CUSTOM_SLIDERS(ScanIntense, "Intensity", kScanlinesChanged)

  // Create buttons in 2nd column
  int cloneWidth = _font.getStringWidth("Clone Bad Adjust") + fontWidth * 2.5;
  xpos = _w - HBORDER - 2 * 2 - cloneWidth;
  ypos = VBORDER - VGAP / 2;

  // Adjustable presets
#define CREATE_CLONE_BUTTON(obj, desc)                                 \
  myClone ## obj =                                                     \
    new ButtonWidget(myTab, _font, xpos, ypos, cloneWidth, buttonHeight,\
                     desc, kClone ## obj ##Cmd);                       \
  wid.push_back(myClone ## obj);                                       \
  ypos += buttonHeight + VGAP;

  ypos += VGAP;
  CREATE_CLONE_BUTTON(RGB, "Clone RGB")
  CREATE_CLONE_BUTTON(Svideo, "Clone S-Video")
  CREATE_CLONE_BUTTON(Composite, "Clone Composite")
  CREATE_CLONE_BUTTON(Bad, "Clone Bad adjust")
  CREATE_CLONE_BUTTON(Custom, "Revert")

  // Add items for tab 3
  addToFocusList(wid, myTab, tabID);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VideoAudioDialog::addAudioTab()
{
  const int lineHeight   = _font.getLineHeight(),
            fontHeight   = _font.getFontHeight(),
            fontWidth    = _font.getMaxCharWidth();
  const int VBORDER = fontHeight / 2;
  const int HBORDER = fontWidth * 1.25;
  const int INDENT = CheckboxWidget::prefixSize(_font);
  const int VGAP = fontHeight / 4;

  int xpos, ypos;
  int lwidth = _font.getStringWidth("Volume "),
    pwidth;
  WidgetArray wid;
  VariantList items;
  const int tabID = myTab->addTab("  Audio  ", TabWidget::AUTO_WIDTH);

  xpos = HBORDER;  ypos = VBORDER;

  // Enable sound
  mySoundEnableCheckbox = new CheckboxWidget(myTab, _font, xpos, ypos,
                                             "Enable sound", kSoundEnableChanged);
  wid.push_back(mySoundEnableCheckbox);
  ypos += lineHeight + VGAP;
  xpos += CheckboxWidget::prefixSize(_font);

  // Volume
  myVolumeSlider = new SliderWidget(myTab, _font, xpos, ypos,
                                    "Volume", lwidth, 0, 4 * fontWidth, "%");
  myVolumeSlider->setMinValue(1); myVolumeSlider->setMaxValue(100);
  myVolumeSlider->setTickmarkIntervals(4);
  wid.push_back(myVolumeSlider);
  ypos += lineHeight + VGAP;

  // Mode
  items.clear();
  VarList::push_back(items, "Low quality, medium lag", static_cast<int>(AudioSettings::Preset::lowQualityMediumLag));
  VarList::push_back(items, "High quality, medium lag", static_cast<int>(AudioSettings::Preset::highQualityMediumLag));
  VarList::push_back(items, "High quality, low lag", static_cast<int>(AudioSettings::Preset::highQualityLowLag));
  VarList::push_back(items, "Ultra quality, minimal lag", static_cast<int>(AudioSettings::Preset::ultraQualityMinimalLag));
  VarList::push_back(items, "Custom", static_cast<int>(AudioSettings::Preset::custom));
  myModePopup = new PopUpWidget(myTab, _font, xpos, ypos,
                                _font.getStringWidth("Ultry quality, minimal lag"), lineHeight,
                                items, "Mode", lwidth, kModeChanged);
  wid.push_back(myModePopup);
  ypos += lineHeight + VGAP;
  xpos += INDENT;

  // Fragment size
  lwidth = _font.getStringWidth("Resampling quality ");
  pwidth = myModePopup->getRight() - xpos - lwidth - PopUpWidget::dropDownWidth(_font);
  items.clear();
  VarList::push_back(items, "128 samples", 128);
  VarList::push_back(items, "256 samples", 256);
  VarList::push_back(items, "512 samples", 512);
  VarList::push_back(items, "1k samples", 1024);
  VarList::push_back(items, "2k samples", 2048);
  VarList::push_back(items, "4K samples", 4096);
  myFragsizePopup = new PopUpWidget(myTab, _font, xpos, ypos,
                                    pwidth, lineHeight,
                                    items, "Fragment size", lwidth);
  wid.push_back(myFragsizePopup);
  ypos += lineHeight + VGAP;

  // Output frequency
  items.clear();
  VarList::push_back(items, "44100 Hz", 44100);
  VarList::push_back(items, "48000 Hz", 48000);
  VarList::push_back(items, "96000 Hz", 96000);
  myFreqPopup = new PopUpWidget(myTab, _font, xpos, ypos,
                                pwidth, lineHeight,
                                items, "Sample rate", lwidth);
  wid.push_back(myFreqPopup);
  ypos += lineHeight + VGAP;

  // Resampling quality
  items.clear();
  VarList::push_back(items, "Low", static_cast<int>(AudioSettings::ResamplingQuality::nearestNeightbour));
  VarList::push_back(items, "High", static_cast<int>(AudioSettings::ResamplingQuality::lanczos_2));
  VarList::push_back(items, "Ultra", static_cast<int>(AudioSettings::ResamplingQuality::lanczos_3));
  myResamplingPopup = new PopUpWidget(myTab, _font, xpos, ypos,
                                      pwidth, lineHeight,
                                      items, "Resampling quality ", lwidth);
  wid.push_back(myResamplingPopup);
  ypos += lineHeight + VGAP;

  // Param 1
  int swidth = pwidth + PopUpWidget::dropDownWidth(_font);
  myHeadroomSlider = new SliderWidget(myTab, _font, xpos, ypos, swidth, lineHeight,
                                      "Headroom           ", 0, kHeadroomChanged, 10 * fontWidth);
  myHeadroomSlider->setMinValue(0); myHeadroomSlider->setMaxValue(AudioSettings::MAX_HEADROOM);
  myHeadroomSlider->setTickmarkIntervals(5);
  wid.push_back(myHeadroomSlider);
  ypos += lineHeight + VGAP;

  // Param 2
  myBufferSizeSlider = new SliderWidget(myTab, _font, xpos, ypos, swidth, lineHeight,
                                        "Buffer size        ", 0, kBufferSizeChanged, 10 * fontWidth);
  myBufferSizeSlider->setMinValue(0); myBufferSizeSlider->setMaxValue(AudioSettings::MAX_BUFFER_SIZE);
  myBufferSizeSlider->setTickmarkIntervals(5);
  wid.push_back(myBufferSizeSlider);
  ypos += lineHeight + VGAP;

  // Stereo sound
  xpos -= INDENT;
  myStereoSoundCheckbox = new CheckboxWidget(myTab, _font, xpos, ypos,
                                             "Stereo for all ROMs");
  wid.push_back(myStereoSoundCheckbox);
  ypos += lineHeight + VGAP;

  swidth += INDENT - fontWidth * 4;
  myDpcPitch = new SliderWidget(myTab, _font, xpos, ypos, swidth, lineHeight,
                                "Pitfall II music pitch ", 0, 0, 5 * fontWidth);
  myDpcPitch->setMinValue(10000); myDpcPitch->setMaxValue(30000);
  myDpcPitch->setStepValue(100);
  myDpcPitch->setTickmarkIntervals(2);
  wid.push_back(myDpcPitch);

  // Add items for tab 4
  addToFocusList(wid, myTab, tabID);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VideoAudioDialog::loadConfig()
{
  // Display tab
  // Renderer settings
  myRenderer->setSelected(instance().settings().getString("video"), "default");

  // TIA interpolation
  myTIAInterpolate->setState(instance().settings().getBool("tia.inter"));

  // TIA zoom levels
  // These are dynamically loaded, since they depend on the size of
  // the desktop and which renderer we're using
  float minZoom = instance().frameBuffer().supportedTIAMinZoom(); // or 2 if we allow lower values
  float maxZoom = instance().frameBuffer().supportedTIAMaxZoom();

  myTIAZoom->setMinValue(minZoom * 100);
  myTIAZoom->setMaxValue(maxZoom * 100);
  myTIAZoom->setTickmarkIntervals((maxZoom - minZoom) * 2); // every ~50%
  myTIAZoom->setValue(instance().settings().getFloat("tia.zoom") * 100);

  // Fullscreen
  myFullscreen->setState(instance().settings().getBool("fullscreen"));
  // Fullscreen stretch setting
  myUseStretch->setState(instance().settings().getBool("tia.fs_stretch"));
#ifdef ADAPTABLE_REFRESH_SUPPORT
  // Adapt refresh rate
  myRefreshAdapt->setState(instance().settings().getBool("tia.fs_refresh"));
#endif
  // Fullscreen overscan setting
  myTVOverscan->setValue(instance().settings().getInt("tia.fs_overscan"));
  handleFullScreenChange();

  // Aspect ratio setting (NTSC and PAL)
  myVSizeAdjust->setValue(instance().settings().getInt("tia.vsizeadjust"));

  /////////////////////////////////////////////////////////////////////////////
  // Palettes tab
  // TIA Palette
  myPalette = instance().settings().getString("palette");
  myTIAPalette->setSelected(myPalette, PaletteHandler::SETTING_STANDARD);

  // Palette adjustables
  instance().frameBuffer().tiaSurface().paletteHandler().getAdjustables(myPaletteAdj);
  myPhaseShiftNtsc->setValue(myPaletteAdj.phaseNtsc);
  myPhaseShiftPal->setValue(myPaletteAdj.phasePal);
  myTVHue->setValue(myPaletteAdj.hue);
  myTVBright->setValue(myPaletteAdj.brightness);
  myTVContrast->setValue(myPaletteAdj.contrast);
  myTVSatur->setValue(myPaletteAdj.saturation);
  myTVGamma->setValue(myPaletteAdj.gamma);
  handlePaletteChange();
  colorPalette();

  /////////////////////////////////////////////////////////////////////////////
  // TV Effects tab
  // TV Mode
  myTVMode->setSelected(
    instance().settings().getString("tv.filter"), "0");
  int preset = instance().settings().getInt("tv.filter");
  handleTVModeChange(NTSCFilter::Preset(preset));

  // TV Custom adjustables
  loadTVAdjustables(NTSCFilter::Preset::CUSTOM);

  // TV phosphor mode
  myTVPhosphor->setState(instance().settings().getString("tv.phosphor") == "always");

  // TV phosphor blend
  myTVPhosLevel->setValue(instance().settings().getInt("tv.phosblend"));
  handlePhosphorChange();

  // TV scanline intensity and interpolation
  myTVScanIntense->setValue(instance().settings().getInt("tv.scanlines"));

  /////////////////////////////////////////////////////////////////////////////
  // Audio tab
  AudioSettings& audioSettings = instance().audioSettings();

  // Enable sound
#ifdef SOUND_SUPPORT
  mySoundEnableCheckbox->setState(audioSettings.enabled());
#else
  mySoundEnableCheckbox->setState(false);
#endif

  // Volume
  myVolumeSlider->setValue(audioSettings.volume());

  // Stereo
  myStereoSoundCheckbox->setState(audioSettings.stereo());

  // DPC Pitch
  myDpcPitch->setValue(audioSettings.dpcPitch());

  // Preset / mode
  myModePopup->setSelected(static_cast<int>(audioSettings.preset()));

  updateSettingsWithPreset(instance().audioSettings());

  updateEnabledState();

  myTab->loadConfig();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VideoAudioDialog::updateSettingsWithPreset(AudioSettings& audioSettings)
{
  // Fragsize
  myFragsizePopup->setSelected(audioSettings.fragmentSize());

  // Output frequency
  myFreqPopup->setSelected(audioSettings.sampleRate());

  // Headroom
  myHeadroomSlider->setValue(audioSettings.headroom());

  // Buffer size
  myBufferSizeSlider->setValue(audioSettings.bufferSize());

  // Resampling quality
  myResamplingPopup->setSelected(static_cast<int>(audioSettings.resamplingQuality()));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VideoAudioDialog::saveConfig()
{
  /////////////////////////////////////////////////////////////////////////////
  // Display tab
  // Renderer setting
  instance().settings().setValue("video",
                                 myRenderer->getSelectedTag().toString());

  // TIA interpolation
  instance().settings().setValue("tia.inter", myTIAInterpolate->getState());

  // Fullscreen
  instance().settings().setValue("fullscreen", myFullscreen->getState());
  // Fullscreen stretch setting
  instance().settings().setValue("tia.fs_stretch", myUseStretch->getState());
#ifdef ADAPTABLE_REFRESH_SUPPORT
  // Adapt refresh rate
  instance().settings().setValue("tia.fs_refresh", myRefreshAdapt->getState());
#endif
  // Fullscreen overscan
  instance().settings().setValue("tia.fs_overscan", myTVOverscan->getValueLabel());

  // TIA zoom levels
  instance().settings().setValue("tia.zoom", myTIAZoom->getValue() / 100.0);

  // Aspect ratio setting (NTSC and PAL)
  const int oldAdjust = instance().settings().getInt("tia.vsizeadjust");
  const int newAdjust = myVSizeAdjust->getValue();
  const bool vsizeChanged = oldAdjust != newAdjust;

  instance().settings().setValue("tia.vsizeadjust", newAdjust);


  // Note: Palette values are saved directly when changed!

  /////////////////////////////////////////////////////////////////////////////
  // TV Effects tab
  // TV Mode
  instance().settings().setValue("tv.filter",
                                 myTVMode->getSelectedTag().toString());
  // TV Custom adjustables
  NTSCFilter::Adjustable ntscAdj;
  ntscAdj.sharpness = myTVSharp->getValue();
  ntscAdj.resolution = myTVRes->getValue();
  ntscAdj.artifacts = myTVArtifacts->getValue();
  ntscAdj.fringing = myTVFringe->getValue();
  ntscAdj.bleed = myTVBleed->getValue();
  instance().frameBuffer().tiaSurface().ntsc().setCustomAdjustables(ntscAdj);

  // TV phosphor mode
  instance().settings().setValue("tv.phosphor",
                                 myTVPhosphor->getState() ? "always" : "byrom");
  // TV phosphor blend
  instance().settings().setValue("tv.phosblend", myTVPhosLevel->getValueLabel() == "Off"
                                 ? "0" : myTVPhosLevel->getValueLabel());

  // TV scanline intensity
  instance().settings().setValue("tv.scanlines", myTVScanIntense->getValueLabel());

  if(instance().hasConsole())
  {
    instance().console().setTIAProperties();

    if(vsizeChanged)
    {
      instance().console().tia().clearFrameBuffer();
      instance().console().initializeVideo();
    }
  }

  // Finally, issue a complete framebuffer re-initialization...
  instance().createFrameBuffer();

  // ... and apply potential setting changes to the TIA surface
  instance().frameBuffer().tiaSurface().updateSurfaceSettings();

  /////////////////////////////////////////////////////////////////////////////
  // Audio tab
  AudioSettings& audioSettings = instance().audioSettings();

  // Enabled
  audioSettings.setEnabled(mySoundEnableCheckbox->getState());
  instance().sound().setEnabled(mySoundEnableCheckbox->getState());

  // Volume
  audioSettings.setVolume(myVolumeSlider->getValue());
  instance().sound().setVolume(myVolumeSlider->getValue());

  // Stereo
  audioSettings.setStereo(myStereoSoundCheckbox->getState());

  // DPC Pitch
  audioSettings.setDpcPitch(myDpcPitch->getValue());
  // update if current cart is Pitfall II
  if (instance().hasConsole() && instance().console().cartridge().name() == "CartridgeDPC")
  {
    CartridgeDPC& cart = static_cast<CartridgeDPC&>(instance().console().cartridge());
    cart.setDpcPitch(myDpcPitch->getValue());
  }

  AudioSettings::Preset preset = static_cast<AudioSettings::Preset>(myModePopup->getSelectedTag().toInt());
  audioSettings.setPreset(preset);

  if (preset == AudioSettings::Preset::custom) {
    // Fragsize
    audioSettings.setFragmentSize(myFragsizePopup->getSelectedTag().toInt());
    audioSettings.setSampleRate(myFreqPopup->getSelectedTag().toInt());
    audioSettings.setHeadroom(myHeadroomSlider->getValue());
    audioSettings.setBufferSize(myBufferSizeSlider->getValue());
    audioSettings.setResamplingQuality(static_cast<AudioSettings::ResamplingQuality>(myResamplingPopup->getSelectedTag().toInt()));
  }

  // Only force a re-initialization when necessary, since it can
  // be a time-consuming operation
  if(instance().hasConsole())
    instance().console().initializeAudio();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VideoAudioDialog::setDefaults()
{
  switch(myTab->getActiveTab())
  {
    case 0:  // General
    {
      myRenderer->setSelectedIndex(0);
      myTIAInterpolate->setState(false);
      // screen size
      myFullscreen->setState(false);
      myUseStretch->setState(false);
    #ifdef ADAPTABLE_REFRESH_SUPPORT
      myRefreshAdapt->setState(false);
    #endif
      myTVOverscan->setValue(0);
      myTIAZoom->setValue(300);
      myVSizeAdjust->setValue(0);

      handleFullScreenChange();
      break;
    }

    case 1:  // Palettes
      myTIAPalette->setSelected(PaletteHandler::SETTING_STANDARD);
      myPhaseShiftNtsc->setValue(PaletteHandler::DEF_NTSC_SHIFT * 10);
      myPhaseShiftPal->setValue(PaletteHandler::DEF_PAL_SHIFT * 10);
      myTVHue->setValue(50);
      myTVSatur->setValue(50);
      myTVContrast->setValue(50);
      myTVBright->setValue(50);
      myTVGamma->setValue(50);
      handlePaletteChange();
      handlePaletteUpdate();
      break;

    case 2:  // TV effects
    {
      myTVMode->setSelected("0", "0");

      // TV phosphor mode
      myTVPhosphor->setState(false);

      // TV phosphor blend
      myTVPhosLevel->setValue(50);

      // TV scanline intensity and interpolation
      myTVScanIntense->setValue(25);

      // Make sure that mutually-exclusive items are not enabled at the same time
      handleTVModeChange(NTSCFilter::Preset::OFF);
      handlePhosphorChange();
      loadTVAdjustables(NTSCFilter::Preset::CUSTOM);
      break;
    }
    case 3:  // Audio
      mySoundEnableCheckbox->setState(AudioSettings::DEFAULT_ENABLED);
      myVolumeSlider->setValue(AudioSettings::DEFAULT_VOLUME);
      myStereoSoundCheckbox->setState(AudioSettings::DEFAULT_STEREO);
      myDpcPitch->setValue(AudioSettings::DEFAULT_DPC_PITCH);
      myModePopup->setSelected(static_cast<int>(AudioSettings::DEFAULT_PRESET));

      if (AudioSettings::DEFAULT_PRESET == AudioSettings::Preset::custom) {
        myResamplingPopup->setSelected(static_cast<int>(AudioSettings::DEFAULT_RESAMPLING_QUALITY));
        myFragsizePopup->setSelected(AudioSettings::DEFAULT_FRAGMENT_SIZE);
        myFreqPopup->setSelected(AudioSettings::DEFAULT_SAMPLE_RATE);
        myHeadroomSlider->setValue(AudioSettings::DEFAULT_HEADROOM);
        myBufferSizeSlider->setValue(AudioSettings::DEFAULT_BUFFER_SIZE);
      }
      else updatePreset();

      updateEnabledState();
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VideoAudioDialog::handleTVModeChange(NTSCFilter::Preset preset)
{
  bool enable = preset == NTSCFilter::Preset::CUSTOM;

  myTVSharp->setEnabled(enable);
  myTVRes->setEnabled(enable);
  myTVArtifacts->setEnabled(enable);
  myTVFringe->setEnabled(enable);
  myTVBleed->setEnabled(enable);
  myCloneComposite->setEnabled(enable);
  myCloneSvideo->setEnabled(enable);
  myCloneRGB->setEnabled(enable);
  myCloneBad->setEnabled(enable);
  myCloneCustom->setEnabled(enable);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VideoAudioDialog::loadTVAdjustables(NTSCFilter::Preset preset)
{
  NTSCFilter::Adjustable adj;
  instance().frameBuffer().tiaSurface().ntsc().getAdjustables(
      adj, NTSCFilter::Preset(preset));
  myTVSharp->setValue(adj.sharpness);
  myTVRes->setValue(adj.resolution);
  myTVArtifacts->setValue(adj.artifacts);
  myTVFringe->setValue(adj.fringing);
  myTVBleed->setValue(adj.bleed);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VideoAudioDialog::handlePaletteChange()
{
  bool enable = myTIAPalette->getSelectedTag().toString() == "custom";

  myPhaseShiftNtsc->setEnabled(enable);
  myPhaseShiftPal->setEnabled(enable);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VideoAudioDialog::handlePaletteUpdate()
{
  // TIA Palette
  instance().settings().setValue("palette",
                                  myTIAPalette->getSelectedTag().toString());
  // Palette adjustables
  PaletteHandler::Adjustable paletteAdj;
  paletteAdj.phaseNtsc  = myPhaseShiftNtsc->getValue();
  paletteAdj.phasePal   = myPhaseShiftPal->getValue();
  paletteAdj.hue        = myTVHue->getValue();
  paletteAdj.saturation = myTVSatur->getValue();
  paletteAdj.contrast   = myTVContrast->getValue();
  paletteAdj.brightness = myTVBright->getValue();
  paletteAdj.gamma      = myTVGamma->getValue();
  instance().frameBuffer().tiaSurface().paletteHandler().setAdjustables(paletteAdj);

  if(instance().hasConsole())
    instance().frameBuffer().tiaSurface().paletteHandler().setPalette();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VideoAudioDialog::handleFullScreenChange()
{
  bool enable = myFullscreen->getState();
  myUseStretch->setEnabled(enable);
#ifdef ADAPTABLE_REFRESH_SUPPORT
  myRefreshAdapt->setEnabled(enable);
#endif
  myTVOverscan->setEnabled(enable);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VideoAudioDialog::handleOverscanChange()
{
  if (myTVOverscan->getValue() == 0)
  {
    myTVOverscan->setValueLabel("Off");
    myTVOverscan->setValueUnit("");
  }
  else
    myTVOverscan->setValueUnit("%");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VideoAudioDialog::handlePhosphorChange()
{
  myTVPhosLevel->setEnabled(myTVPhosphor->getState());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VideoAudioDialog::handleCommand(CommandSender* sender, int cmd,
                                int data, int id)
{
  switch (cmd)
  {
    case GuiObject::kOKCmd:
      saveConfig();
      close();
      break;

    case GuiObject::kCloseCmd:
      // restore palette settings
      instance().frameBuffer().tiaSurface().paletteHandler().setAdjustables(myPaletteAdj);
      instance().frameBuffer().tiaSurface().paletteHandler().setPalette(myPalette);
      Dialog::handleCommand(sender, cmd, data, 0);
      break;

    case GuiObject::kDefaultsCmd:
      setDefaults();
      break;

    case kPaletteChanged:
      handlePaletteChange();
      handlePaletteUpdate();
      break;

    case kPaletteUpdated:
      handlePaletteUpdate();
      break;

    case kNtscShiftChanged:
    {
      std::ostringstream ss;

      ss << std::setw(4) << std::fixed << std::setprecision(1)
        << (0.1 * abs(myPhaseShiftNtsc->getValue())) << DEGREE;
      myPhaseShiftNtsc->setValueLabel(ss.str());
      handlePaletteUpdate();
      break;
    }
    case kPalShiftChanged:
    {
      std::ostringstream ss;

      ss << std::setw(4) << std::fixed << std::setprecision(1)
        << (0.1 * abs(myPhaseShiftPal->getValue())) << DEGREE;
      myPhaseShiftPal->setValueLabel(ss.str());
      handlePaletteUpdate();
      break;
    }
    case kVSizeChanged:
    {
      int adjust = myVSizeAdjust->getValue();

      if (!adjust)
      {
        myVSizeAdjust->setValueLabel("Default");
        myVSizeAdjust->setValueUnit("");
      }
      else
        myVSizeAdjust->setValueUnit("%");
      break;
    }
    case kFullScreenChanged:
      handleFullScreenChange();
      break;

    case kOverscanChanged:
      handleOverscanChange();
      break;

    case kTVModeChanged:
      handleTVModeChange(NTSCFilter::Preset(myTVMode->getSelectedTag().toInt()));
      break;

    case kCloneCompositeCmd: loadTVAdjustables(NTSCFilter::Preset::COMPOSITE);
      break;
    case kCloneSvideoCmd: loadTVAdjustables(NTSCFilter::Preset::SVIDEO);
      break;
    case kCloneRGBCmd: loadTVAdjustables(NTSCFilter::Preset::RGB);
      break;
    case kCloneBadCmd: loadTVAdjustables(NTSCFilter::Preset::BAD);
      break;
    case kCloneCustomCmd: loadTVAdjustables(NTSCFilter::Preset::CUSTOM);
      break;

    case kScanlinesChanged:
      if (myTVScanIntense->getValue() == 0)
      {
        myTVScanIntense->setValueLabel("Off");
        myTVScanIntense->setValueUnit("");
      }
      else
        myTVScanIntense->setValueUnit("%");
      break;

    case kPhosphorChanged:
      handlePhosphorChange();
      break;

    case kPhosBlendChanged:
      if (myTVPhosLevel->getValue() == 0)
      {
        myTVPhosLevel->setValueLabel("Off");
        myTVPhosLevel->setValueUnit("");
      }
      else
        myTVPhosLevel->setValueUnit("%");
      break;

    case kSoundEnableChanged:
      updateEnabledState();
      break;

    case kModeChanged:
      updatePreset();
      updateEnabledState();
      break;

    case kHeadroomChanged:
    {
      std::ostringstream ss;
      ss << std::fixed << std::setprecision(1) << (0.5 * myHeadroomSlider->getValue()) << " frames";
      myHeadroomSlider->setValueLabel(ss.str());
      break;
    }
    case kBufferSizeChanged:
    {
      std::ostringstream ss;
      ss << std::fixed << std::setprecision(1) << (0.5 * myBufferSizeSlider->getValue()) << " frames";
      myBufferSizeSlider->setValueLabel(ss.str());
      break;
    }

    default:
      Dialog::handleCommand(sender, cmd, data, 0);
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VideoAudioDialog::addPalette(int x, int y, int w, int h)
{
  if(instance().hasConsole())
  {
    constexpr int NUM_LUMA = 8;
    constexpr int NUM_CHROMA = 16;
    const GUI::Font& ifont = instance().frameBuffer().infoFont();
    const int lwidth = ifont.getMaxCharWidth() * 1.5;
    const float COLW = float(w - lwidth) / NUM_LUMA;
    const float COLH = float(h) / NUM_CHROMA;
    const int yofs = (COLH - ifont.getFontHeight() + 1) / 2;

    for(int idx = 0; idx < NUM_CHROMA; ++idx)
    {
      myColorLbl[idx] = new StaticTextWidget(myTab, ifont, x, y + yofs + idx * COLH, " ");
      for(int lum = 0; lum < NUM_LUMA; ++lum)
      {
        myColor[idx][lum] = new ColorWidget(myTab, _font, x + lwidth + lum * COLW, y + idx * COLH,
                                            COLW + 1, COLH + 1, 0, false);
      }
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VideoAudioDialog::colorPalette()
{
  if(instance().hasConsole())
  {
    constexpr int NUM_LUMA = 8;
    constexpr int NUM_CHROMA = 16;
    const int order[2][NUM_CHROMA] =
    {
      {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15},
      {0, 1, 2, 4, 6, 8, 10, 12, 13, 11, 9, 7, 5, 3, 14, 15}
    };
    const int type = instance().console().timing() == ConsoleTiming::pal ? 1 : 0;

    for(int idx = 0; idx < NUM_CHROMA; ++idx)
    {
      ostringstream ss;
      const int color = order[type][idx];

      ss << Common::Base::HEX1 << std::uppercase << color;
      myColorLbl[idx]->setLabel(ss.str());
      for(int lum = 0; lum < NUM_LUMA; ++lum)
      {
        myColor[idx][lum]->setColor(color * NUM_CHROMA + lum * 2); // skip grayscale colors
      }
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VideoAudioDialog::updateEnabledState()
{
  bool active = mySoundEnableCheckbox->getState();
  AudioSettings::Preset preset = static_cast<AudioSettings::Preset>(myModePopup->getSelectedTag().toInt());
  bool userMode = preset == AudioSettings::Preset::custom;

  myVolumeSlider->setEnabled(active);
  myStereoSoundCheckbox->setEnabled(active);
  myModePopup->setEnabled(active);
  // enable only for Pitfall II cart
  myDpcPitch->setEnabled(active && instance().hasConsole() && instance().console().cartridge().name() == "CartridgeDPC");

  myFragsizePopup->setEnabled(active && userMode);
  myFreqPopup->setEnabled(active && userMode);
  myResamplingPopup->setEnabled(active && userMode);
  myHeadroomSlider->setEnabled(active && userMode);
  myBufferSizeSlider->setEnabled(active && userMode);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VideoAudioDialog::updatePreset()
{
  AudioSettings::Preset preset = static_cast<AudioSettings::Preset>(myModePopup->getSelectedTag().toInt());

  // Make a copy that does not affect the actual settings...
  AudioSettings audioSettings = instance().audioSettings();
  audioSettings.setPersistent(false);
  // ... and set the requested preset
  audioSettings.setPreset(preset);

  updateSettingsWithPreset(audioSettings);
}
