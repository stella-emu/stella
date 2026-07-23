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

#include "bspf.hxx"
#include "Base.hxx"
#include "Control.hxx"
#include "Cart.hxx"
#include "CartDPC.hxx"
#include "Dialog.hxx"
#include "BrowserDialog.hxx"
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
#include "TabPaneWidget.hxx"
#include "Layout.hxx"
#include "NTSCFilter.hxx"
#include "TIASurface.hxx"

#include "VideoAudioDialog.hxx"

// A custom-adjustable slider: 0-100%, in 1% steps.  The track width is the
// dialog's choice and is set when the tab lays itself out
#define CREATE_CUSTOM_SLIDER(obj, desc, cmd)                             \
  myTV ## obj ## Label = new StaticTextWidget(pane, _font, desc);        \
  myTV ## obj =                                                          \
    new SliderWidget(pane, _font, 1, cmd, fontWidth*4, "%");             \
  myTV ## obj->setMinValue(0); myTV ## obj->setMaxValue(100);            \
  myTV ## obj->setStepValue(1);                                          \
  myTV ## obj->setTickmarkIntervals(2);                                  \
  wid.push_back(myTV ## obj);

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
VideoAudioDialog::VideoAudioDialog(OSystem& osystem, DialogContainer& parent,
                                   const GUI::Font& font)
  : Dialog(osystem, parent, font, "Video & Audio settings")
{
  // Widgets are only created here (at placeholder geometry); each tab's pane
  // lays its own out, and layout() sizes the dialog to the largest of them
  // NOLINTBEGIN(cppcoreguidelines-prefer-member-initializer)
  myTab = new TabWidget(this, _font);
  addTabWidget(myTab);

  addDisplayTab();
  addPaletteTab();
  addTVEffectsTab();
#ifdef IMAGE_SUPPORT
  addBezelTab();
#endif
  addAudioTab();

  // Finalize the tabs, and activate the first
  myTab->activateTabs();
  myTab->setActiveTab(0);

  // Add Defaults, OK and Cancel buttons
  WidgetArray wid;
  addDefaultsOKCancelBGroup(wid, _font);
  addBGroupToFocusList(wid);

  setHelpAnchor("VideoAudio");

  // Disable certain functions when we know they aren't present
#ifndef WINDOWED_SUPPORT
  myFullscreen->clearFlags(Widget::FLAG_ENABLED);
  myUseStretch->clearFlags(Widget::FLAG_ENABLED);
  myTVOverscan->clearFlags(Widget::FLAG_ENABLED);
#endif
  // NOLINTEND(cppcoreguidelines-prefer-member-initializer)
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VideoAudioDialog::layout()
{
  const int buttonHeight = Dialog::buttonHeight(),
            VBORDER      = Dialog::vBorder(),
            VGAP         = Dialog::vGap();

  // Both dimensions come from the tab widget: it reports what its largest tab's
  // content asks for, so nothing here counts rows, gaps or columns
  constexpr int xpos = 2;
  const Common::Size tabSize = myTab->naturalSize();

  myTab->setPos(xpos, VGAP + _th);
  myTab->setWidth(static_cast<int>(tabSize.w));
  myTab->setHeight(static_cast<int>(tabSize.h));

  _w = myTab->getWidth() + 2 * xpos;
  _h = _th + VGAP + myTab->getHeight() + VBORDER + buttonHeight + VBORDER;

  // Recompute the tab-bar geometry for the current font/width
  myTab->updateTabSizes();

  // Standard button group (Defaults / OK / Cancel) along the bottom edge
  layoutButtonGroup();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VideoAudioDialog::addDisplayTab()
{
  const GUI::Font& ifont = instance().frameBuffer().infoFont();
  const int fontWidth = Dialog::fontWidth();
  WidgetArray wid;

  const int tabID = myTab->addTab(" Display ", TabWidget::AUTO_WIDTH);
  auto* pane = new TabPaneWidget(myTab, _font);
  myTab->setPaneWidget(tabID, pane);

  // Video renderer.  The list is fixed for this dialog's life, so it sizes
  // itself to the widest renderer the platform offers
  myRendererLabel = new StaticTextWidget(pane, _font, "Renderer");
  myRenderer = new PopUpWidget(pane, _font,
                               instance().frameBuffer().supportedRenderers(),
                               kRendererChanged);
  myRenderer->setToolTip("Select renderer used for displaying screen.");
  wid.push_back(myRenderer);

  // TIA interpolation
  myTIAInterpolate = new CheckboxWidget(pane, _font, "Interpolation");
  myTIAInterpolate->setToolTip("Blur emulated display.", Event::ToggleInter);
  wid.push_back(myTIAInterpolate);

  // TIA zoom levels (will be dynamically filled later).  The sliders take their
  // track width from the renderer pop-up beside them, in the layout below
  myTIAZoomLabel = new StaticTextWidget(pane, _font, "Zoom");
  myTIAZoom = new SliderWidget(pane, _font, 1,
                               0, fontWidth * 4, "%");
  myTIAZoom->setMinValue(200); myTIAZoom->setStepValue(FrameBuffer::ZOOM_STEPS * 100);
  myTIAZoom->setToolTip(Event::VidmodeDecrease, Event::VidmodeIncrease);
  wid.push_back(myTIAZoom);

  // Fullscreen
  myFullscreen = new CheckboxWidget(pane, _font, "Fullscreen", kFullScreenChanged);
  myFullscreen->setToolTip(Event::ToggleFullScreen);
  wid.push_back(myFullscreen);

  // FS stretch
  myUseStretch = new CheckboxWidget(pane, _font, "Stretch");
  myUseStretch->setToolTip("Stretch emulated display to fill whole screen.");
  wid.push_back(myUseStretch);

#ifdef ADAPTABLE_REFRESH_SUPPORT
  // Adapt refresh rate
  myRefreshAdapt = new CheckboxWidget(pane, _font, "Adapt display refresh rate");
  myRefreshAdapt->setToolTip("Select optimal display refresh rate for each ROM.", Event::ToggleAdaptRefresh);
  wid.push_back(myRefreshAdapt);
#else
  myRefreshAdapt = nullptr;
#endif

  // FS overscan
  myTVOverscanLabel = new StaticTextWidget(pane, _font, "Overscan");
  myTVOverscan = new SliderWidget(pane, _font, 1,
                                  kOverscanChanged, fontWidth * 3, "%");
  myTVOverscan->setMinValue(0); myTVOverscan->setMaxValue(10);
  myTVOverscan->setTickmarkIntervals(2);
  myTVOverscan->setToolTip(Event::OverscanDecrease, Event::OverscanIncrease);
  wid.push_back(myTVOverscan);

  // Aspect ratio correction
  myCorrectAspect = new CheckboxWidget(pane, _font, "Correct aspect ratio (*)");
  myCorrectAspect->setToolTip("Uncheck to disable real world aspect ratio correction.",
    Event::ToggleCorrectAspectRatio);
  wid.push_back(myCorrectAspect);

  // Vertical size
  myVSizeAdjustLabel = new StaticTextWidget(pane, _font, "V-Size adjust");
  myVSizeAdjust =
    new SliderWidget(pane, _font, 1,
                     kVSizeChanged, fontWidth * 7, "%", 0, true);
  myVSizeAdjust->setMinValue(-5); myVSizeAdjust->setMaxValue(5);
  myVSizeAdjust->setTickmarkIntervals(2);
  myVSizeAdjust->setToolTip("Adjust vertical size to match emulated TV display.",
    Event::VSizeAdjustDecrease, Event::VSizeAdjustIncrease);
  wid.push_back(myVSizeAdjust);

  // Message concerning usage; it sits at the foot of the tab
  myDisplayInfo = new StaticTextWidget(pane, ifont,
                       "(*) Change may require an application restart");

  addToFocusList(wid, myTab, tabID);
  pane->setHelpAnchor("VideoAudioDisplay");

  // Describe the layout once; the pane runs it on every resize
  pane->setLayout([this](GUI::BoxLayout& col) {
    using GUI::anchoredItem;
    using GUI::labeledRow;
    using GUI::indentedItem;
    using GUI::indentedFill;
    const int VGAP = Dialog::vGap();
    const int INDENT = CheckboxWidget::prefixSize(_font);

    // The renderer's and the sliders' labels share one label column; the
    // indented one declares its indent, which narrows its column to match
    // so that all the tracks still line up
    GUI::alignLabels({{myRendererLabel}, {myTIAZoomLabel},
                      {myTVOverscanLabel, INDENT}, {myVSizeAdjustLabel}});

    // The sliders' tracks span the renderer pop-up's box, so they end flush
    GUI::alignTracks({myTIAZoom, myTVOverscan, myVSizeAdjust}, myRenderer);

    col.addAuto(labeledRow(myRendererLabel, myRenderer));
    col.addSpace(VGAP);
    col.addAuto(anchoredItem(myTIAInterpolate));
    col.addSpace(VGAP * 4);
    col.addAuto(labeledRow(myTIAZoomLabel, myTIAZoom));
    col.addSpace(VGAP);
    col.addAuto(anchoredItem(myFullscreen));
    col.addSpace(VGAP);
    col.addAuto(indentedItem(myUseStretch, INDENT));
#ifdef ADAPTABLE_REFRESH_SUPPORT
    col.addSpace(VGAP);
    col.addAuto(indentedItem(myRefreshAdapt, INDENT));
#endif
    col.addSpace(VGAP);
    col.addAuto(labeledRow(myTVOverscanLabel, myTVOverscan, 0, INDENT));
    col.addSpace(VGAP * 4);
    col.addAuto(anchoredItem(myCorrectAspect));
    col.addSpace(VGAP);
    col.addAuto(labeledRow(myVSizeAdjustLabel, myVSizeAdjust));
    // The note is anchored to the foot of the tab, whatever height it ends up
    col.addStretchSpace();
    col.addAuto(anchoredItem(myDisplayInfo));
  });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VideoAudioDialog::addPaletteTab()
{
  const int fontWidth = Dialog::fontWidth();
  WidgetArray wid;
  VariantList items;

  const int tabID = myTab->addTab(" Palettes ", TabWidget::AUTO_WIDTH);
  auto* pane = new TabPaneWidget(myTab, _font);
  myTab->setPaneWidget(tabID, pane);

  // TIA Palette
  items.clear();
  VarList::push_back(items, "Standard", PaletteHandler::SETTING_STANDARD);
  VarList::push_back(items, "z26", PaletteHandler::SETTING_Z26);
  if(instance().checkUserPalette())
    VarList::push_back(items, "User", PaletteHandler::SETTING_USER);
  VarList::push_back(items, "Custom", PaletteHandler::SETTING_CUSTOM);
  myTIAPaletteLabel = new StaticTextWidget(pane, _font, "Palette");
  myTIAPalette = new PopUpWidget(pane, _font, items, kPaletteChanged);
  myTIAPalette->setToolTip(Event::PaletteDecrease, Event::PaletteIncrease);
  wid.push_back(myTIAPalette);

  // The phase shift and the R/G/B pairs are indented under the palette; every
  // track width is set in the layout below, from the pop-up they sit beneath
  myPhaseShiftLabel = new StaticTextWidget(pane, _font, "NTSC phase");
  myPhaseShift =
    new SliderWidget(pane, _font, 1,
                     kPhaseShiftChanged, fontWidth * 5);
  wid.push_back(myPhaseShift);

  // Each R/G/B row is a saturation slider and a shift slider sharing the row
  const auto scaleSlider = [&](int cmd, string_view tip) {
    auto* s = new SliderWidget(pane, _font, 1, cmd,
                               fontWidth * 4, "%");
    s->setMinValue(0);
    s->setMaxValue(100);
    s->setTickmarkIntervals(2);
    s->setToolTip(tip);
    wid.push_back(s);
    return s;
  };
  const auto shiftSlider = [&](int cmd, string_view tip) {
    auto* s = new SliderWidget(pane, _font, 1, cmd, fontWidth * 6);
    s->setMinValue((PaletteHandler::DEF_RGB_SHIFT - PaletteHandler::MAX_RGB_SHIFT) * 10);
    s->setMaxValue((PaletteHandler::DEF_RGB_SHIFT + PaletteHandler::MAX_RGB_SHIFT) * 10);
    s->setTickmarkIntervals(2);
    s->setToolTip(tip);
    wid.push_back(s);
    return s;
  };

  myTVRedScaleLabel = new StaticTextWidget(pane, _font, "R");
  myTVRedScale   = scaleSlider(kPaletteUpdated,
                               "Adjust red saturation of 'Custom' palette.");
  myTVRedShift   = shiftSlider(kRedShiftChanged,
                               "Adjust red shift of 'Custom' palette.");
  myTVGreenScaleLabel = new StaticTextWidget(pane, _font, "G");
  myTVGreenScale = scaleSlider(kPaletteUpdated,
                               "Adjust green saturation of 'Custom' palette.");
  myTVGreenShift = shiftSlider(kGreenShiftChanged,
                               "Adjust green shift of 'Custom' palette.");
  myTVBlueScaleLabel = new StaticTextWidget(pane, _font, "B");
  myTVBlueScale  = scaleSlider(kPaletteUpdated,
                               "Adjust blue saturation of 'Custom' palette.");
  myTVBlueShift  = shiftSlider(kBlueShiftChanged,
                               "Adjust blue shift of 'Custom' palette.");

  CREATE_CUSTOM_SLIDER(Hue, "Hue", kPaletteUpdated)
  CREATE_CUSTOM_SLIDER(Satur, "Saturation", kPaletteUpdated)
  CREATE_CUSTOM_SLIDER(Contrast, "Contrast", kPaletteUpdated)
  CREATE_CUSTOM_SLIDER(Bright, "Brightness", kPaletteUpdated)
  CREATE_CUSTOM_SLIDER(Gamma, "Gamma", kPaletteUpdated)

  myAutodetectLabel = new StaticTextWidget(pane, _font, "Autodetection");

  myDetectPal60 = new CheckboxWidget(pane, _font, "PAL-60");
  myDetectPal60->setToolTip("Enable autodetection of PAL-60 based on colors used.");
  wid.push_back(myDetectPal60);

  myDetectNtsc50 = new CheckboxWidget(pane, _font, "NTSC-50");
  myDetectNtsc50->setToolTip("Enable autodetection of NTSC-50 based on colors used.");
  wid.push_back(myDetectNtsc50);

  // The resulting palette, shown beside the controls
  createPaletteWidgets(pane);

  addToFocusList(wid, myTab, tabID);
  pane->setHelpAnchor("VideoAudioPalettes");

  pane->setLayout([this](GUI::BoxLayout& col) {
    using GUI::BoxLayout;
    using GUI::anchoredItem;
    using GUI::labeledRow;
    using GUI::stretchedItem;
    using GUI::indentedFill;
    using Dir = BoxLayout::Dir;
    const int fontWidth = Dialog::fontWidth(),
              VGAP      = Dialog::vGap(),
              INDENT    = Dialog::indent();

    // One label column for the palette pop-up and everything that has its
    // own label; the indented ones say so, so their columns narrow and all
    // the tracks still line up
    GUI::alignLabels({{myTIAPaletteLabel}, {myPhaseShiftLabel, INDENT},
                      {myTVHueLabel}, {myTVSaturLabel}, {myTVContrastLabel},
                      {myTVBrightLabel}, {myTVGammaLabel}});
    // The R/G/B saturation sliders are a column of their own (the shift sliders
    // beside them have no label at all)
    GUI::alignLabels({{myTVRedScaleLabel}, {myTVGreenScaleLabel}, {myTVBlueScaleLabel}});

    // Every track spans the pop-up's value box, so the controls end flush
    GUI::alignTracks({myPhaseShift, myTVHue, myTVSatur, myTVContrast,
                      myTVBright, myTVGamma}, myTIAPalette);

    // Each R/G/B row's two sliders SHARE the span under the pop-up, so the shift
    // slider's track still ends where the pop-up does -- lining up with the phase
    // slider above it
    const int rgbSpan = GUI::flushSpan(myTIAPalette, myTIAPaletteLabel, INDENT);
    GUI::alignTracks({myTVRedScale, myTVRedShift}, {myTVRedScaleLabel, nullptr},
                     rgbSpan, fontWidth);
    GUI::alignTracks({myTVGreenScale, myTVGreenShift}, {myTVGreenScaleLabel, nullptr},
                     rgbSpan, fontWidth);
    GUI::alignTracks({myTVBlueScale, myTVBlueShift}, {myTVBlueScaleLabel, nullptr},
                     rgbSpan, fontWidth);

    const auto rgbRow = [&](StaticTextWidget* scaleLabel, SliderWidget* scale,
                            SliderWidget* shift) {
      auto row = std::make_unique<BoxLayout>(Dir::Horizontal);
      row->addSpace(INDENT);
      row->addAuto(labeledRow(scaleLabel, scale));
      row->addSpace(fontWidth);
      row->addAuto(anchoredItem(shift));
      return row;
    };

    // The controls, with the palette itself beside them
    auto controls = std::make_unique<BoxLayout>(Dir::Vertical);
    controls->addAuto(labeledRow(myTIAPaletteLabel, myTIAPalette));
    controls->addSpace(VGAP);
    controls->addAuto(labeledRow(myPhaseShiftLabel, myPhaseShift, 0, INDENT));
    controls->addSpace(VGAP);
    controls->addAuto(rgbRow(myTVRedScaleLabel, myTVRedScale, myTVRedShift));
    controls->addSpace(VGAP);
    controls->addAuto(rgbRow(myTVGreenScaleLabel, myTVGreenScale, myTVGreenShift));
    controls->addSpace(VGAP);
    controls->addAuto(rgbRow(myTVBlueScaleLabel, myTVBlueScale, myTVBlueShift));
    controls->addSpace(VGAP * 2);
    controls->addAuto(labeledRow(myTVHueLabel, myTVHue));
    controls->addSpace(VGAP);
    controls->addAuto(labeledRow(myTVSaturLabel, myTVSatur));
    controls->addSpace(VGAP);
    controls->addAuto(labeledRow(myTVContrastLabel, myTVContrast));
    controls->addSpace(VGAP);
    controls->addAuto(labeledRow(myTVBrightLabel, myTVBright));
    controls->addSpace(VGAP);
    controls->addAuto(labeledRow(myTVGammaLabel, myTVGamma));

    // The palette takes the width left over, but it says how much room it needs
    // -- a couple of characters per luminance -- and that is what gives the tab
    // (and so the dialog) a width of its own
    auto main = std::make_unique<BoxLayout>(Dir::Horizontal);
    main->addAuto(std::move(controls));
    main->addSpace(fontWidth * 2);
    main->addStretch(paletteLayout(), 1, NUM_LUMA * fontWidth * 2);

    // The autodetection options sit below, on one line
    auto detect = std::make_unique<BoxLayout>(Dir::Horizontal);
    detect->addAuto(anchoredItem(myAutodetectLabel));
    detect->addSpace(fontWidth * 2);
    detect->addAuto(anchoredItem(myDetectPal60));
    detect->addSpace(fontWidth * 2);
    detect->addAuto(anchoredItem(myDetectNtsc50));

    col.addAuto(std::move(main));
    col.addSpace(VGAP * 2);
    col.addAuto(std::move(detect));
  });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VideoAudioDialog::addTVEffectsTab()
{
  const int fontWidth = Dialog::fontWidth();
  WidgetArray wid;
  VariantList items;

  const int tabID = myTab->addTab("TV Effects", TabWidget::AUTO_WIDTH);
  auto* pane = new TabPaneWidget(myTab, _font);
  myTab->setPaneWidget(tabID, pane);

  items.clear();
  VarList::push_back(items, "Disabled", static_cast<uInt32>(NTSCFilter::Preset::OFF));
  VarList::push_back(items, "RGB", static_cast<uInt32>(NTSCFilter::Preset::RGB));
  VarList::push_back(items, "S-Video", static_cast<uInt32>(NTSCFilter::Preset::SVIDEO));
  VarList::push_back(items, "Composite", static_cast<uInt32>(NTSCFilter::Preset::COMPOSITE));
  VarList::push_back(items, "Bad adjust", static_cast<uInt32>(NTSCFilter::Preset::BAD));
  VarList::push_back(items, "Custom", static_cast<uInt32>(NTSCFilter::Preset::CUSTOM));
  myTVModeLabel = new StaticTextWidget(pane, _font, "TV mode");
  myTVMode = new PopUpWidget(pane, _font, items, kTVModeChanged);
  myTVMode->setToolTip(Event::PreviousVideoMode, Event::NextVideoMode);
  wid.push_back(myTVMode);

  // Custom adjustables
  CREATE_CUSTOM_SLIDER(Sharp, "Sharpness", 0)
  CREATE_CUSTOM_SLIDER(Res, "Resolution", 0)
  CREATE_CUSTOM_SLIDER(Artifacts, "Artifacts", 0)
  CREATE_CUSTOM_SLIDER(Fringe, "Fringing", 0)
  CREATE_CUSTOM_SLIDER(Bleed, "Bleeding", 0)

  // TV Phosphor effect
  items.clear();
  VarList::push_back(items, "by ROM", PhosphorHandler::VALUE_BYROM);
  VarList::push_back(items, "always", PhosphorHandler::VALUE_ALWAYS);
  VarList::push_back(items, "auto on", PhosphorHandler::VALUE_AUTO_ON);
  VarList::push_back(items, "auto on/off", PhosphorHandler::VALUE_AUTO);
  myTVPhosphorLabel = new StaticTextWidget(pane, _font, "Phosphor");
  myTVPhosphor = new PopUpWidget(pane, _font, items, kPhosphorChanged);
  myTVPhosphor->setToolTip(Event::PhosphorModeDecrease, Event::PhosphorModeIncrease);
  wid.push_back(myTVPhosphor);

  // TV Phosphor blend level
  CREATE_CUSTOM_SLIDER(PhosLevel, "Blend", kPhosBlendChanged)

  // Scanline intensity and interpolation
  myTVScanLabel = new StaticTextWidget(pane, _font, "Scanlines:");

  CREATE_CUSTOM_SLIDER(ScanIntense, "Intensity", kScanlinesChanged)
  myTVScanIntense->setToolTip(Event::ScanlinesDecrease, Event::ScanlinesIncrease);

  items.clear();
  VarList::push_back(items, "Standard", TIASurface::SETTING_STANDARD);
  VarList::push_back(items, "Thin lines", TIASurface::SETTING_THIN);
  VarList::push_back(items, "Pixelated", TIASurface::SETTING_PIXELS);
  VarList::push_back(items, "Aperture Gr.", TIASurface::SETTING_APERTURE);
  VarList::push_back(items, "MAME", TIASurface::SETTING_MAME);
  myTVScanMaskLabel = new StaticTextWidget(pane, _font, "Mask");
  myTVScanMask = new PopUpWidget(pane, _font, items);
  myTVScanMask->setToolTip(Event::PreviousScanlineMask, Event::NextScanlineMask);
  wid.push_back(myTVScanMask);

  // Adjustable presets, in a column of their own
#define CREATE_CLONE_BUTTON(obj, desc)                                 \
  myClone ## obj =                                                     \
    new ButtonWidget(pane, _font, desc, kClone ## obj ##Cmd);    \
  wid.push_back(myClone ## obj);

  CREATE_CLONE_BUTTON(RGB, "Clone RGB")
  CREATE_CLONE_BUTTON(Svideo, "Clone S-Video")
  CREATE_CLONE_BUTTON(Composite, "Clone Composite")
  CREATE_CLONE_BUTTON(Bad, "Clone Bad adjust")
  CREATE_CLONE_BUTTON(Custom, "Revert")

  addToFocusList(wid, myTab, tabID);
  pane->setHelpAnchor("VideoAudioEffects");

  pane->setLayout([this](GUI::BoxLayout& col) {
    using GUI::BoxLayout;
    using GUI::anchoredItem;
    using GUI::labeledRow;
    using GUI::indentedFill;
    using Dir = BoxLayout::Dir;
    const int fontWidth = Dialog::fontWidth(),
              VGAP      = Dialog::vGap();
    const int INDENT = CheckboxWidget::prefixSize(_font);

    // The two pop-ups read as one column and must END at the same place, so they
    // share both a label column and a box width.  (The old code did this by
    // padding a specimen -- "Bad adjust  " -- until the two came out equal.)
    GUI::alignLabels({{myTVModeLabel}, {myTVPhosphorLabel}});
    GUI::alignPopUps({myTVMode, myTVPhosphor});

    // Every slider sits a level in from those pop-ups and reads as one column of
    // its own -- a SEPARATE alignLabels group from TV mode/Phosphor's, so its
    // label column is its own width, not theirs; naming one label from each
    // group lets alignTracks() cross that gap itself
    GUI::alignLabels({{myTVSharpLabel}, {myTVResLabel}, {myTVArtifactsLabel},
                      {myTVFringeLabel}, {myTVBleedLabel}, {myTVPhosLevelLabel},
                      {myTVScanIntenseLabel}});
    GUI::alignTracks({myTVSharp, myTVRes, myTVArtifacts, myTVFringe, myTVBleed,
                      myTVPhosLevel, myTVScanIntense}, myTVMode, INDENT,
                     myTVSharpLabel, myTVModeLabel);

    // The mask pop-up is on its own, and a control in no group gets no clearance
    // between its label and its box
    GUI::alignLabels({{myTVScanMaskLabel}});

    // Only the TV mode block and the clone buttons stand side by side; everything
    // below them runs the full width of the tab
    GUI::alignButtons({myCloneRGB, myCloneSvideo, myCloneComposite,
                       myCloneBad, myCloneCustom});
    auto clones = std::make_unique<BoxLayout>(Dir::Vertical, VGAP);
    for(auto* b: {myCloneRGB, myCloneSvideo, myCloneComposite,
                  myCloneBad, myCloneCustom})
      clones->addAuto(anchoredItem(b));
    clones->addStretchSpace();

    auto modes = std::make_unique<BoxLayout>(Dir::Vertical);
    modes->addAuto(labeledRow(myTVModeLabel, myTVMode));
    modes->addSpace(VGAP);
    modes->addAuto(labeledRow(myTVSharpLabel, myTVSharp, 0, INDENT));
    modes->addSpace(VGAP);
    modes->addAuto(labeledRow(myTVResLabel, myTVRes, 0, INDENT));
    modes->addSpace(VGAP);
    modes->addAuto(labeledRow(myTVArtifactsLabel, myTVArtifacts, 0, INDENT));
    modes->addSpace(VGAP);
    modes->addAuto(labeledRow(myTVFringeLabel, myTVFringe, 0, INDENT));
    modes->addSpace(VGAP);
    modes->addAuto(labeledRow(myTVBleedLabel, myTVBleed, 0, INDENT));

    // Neither column fills, so nothing here claims the tab's leftover width
    // (the widest tab in the dialog, not this one, may demand more than
    // "modes" and "clones" need) -- send it past the clone buttons instead of
    // letting "modes" swallow it invisibly, which would push the buttons out
    auto main = std::make_unique<BoxLayout>(Dir::Horizontal);
    main->addAuto(std::move(modes));
    main->addSpace(fontWidth * 2);
    main->addAuto(std::move(clones));
    main->addStretchSpace();

    // The scanline row: its intensity slider, then the mask pop-up filling the
    // rest of the tab -- which is what runs it out under the buttons
    auto scanRow = std::make_unique<BoxLayout>(Dir::Horizontal);
    scanRow->addSpace(INDENT);
    scanRow->addAuto(labeledRow(myTVScanIntenseLabel, myTVScanIntense));
    scanRow->addSpace(fontWidth * 2);
    scanRow->addStretch(labeledRow(myTVScanMaskLabel, myTVScanMask, 0, 0, true));

    col.addAuto(std::move(main));
    col.addSpace(VGAP * 4);
    col.addAuto(labeledRow(myTVPhosphorLabel, myTVPhosphor));
    col.addSpace(VGAP);
    col.addAuto(labeledRow(myTVPhosLevelLabel, myTVPhosLevel, 0, INDENT));
    col.addSpace(VGAP * 2);
    col.addAuto(anchoredItem(myTVScanLabel));
    col.addSpace(VGAP);
    col.addAuto(std::move(scanRow));
  });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VideoAudioDialog::addBezelTab()
{
  const int fontWidth = Dialog::fontWidth();
  WidgetArray wid;

  const int tabID = myTab->addTab(" Bezels ", TabWidget::AUTO_WIDTH);
  auto* pane = new TabPaneWidget(myTab, _font);
  myTab->setPaneWidget(tabID, pane);

  // Enable bezels
  myBezelEnableCheckbox = new CheckboxWidget(pane, _font,
                                             "Enable bezels", kBezelEnableChanged);
  myBezelEnableCheckbox->setToolTip(Event::ToggleBezel);
  wid.push_back(myBezelEnableCheckbox);

  // Bezel path
  myOpenBrowserButton = new ButtonWidget(pane, _font,
                                         "Bezel path" + ELLIPSIS, kChooseBezelDirCmd);
  myOpenBrowserButton->setToolTip("Select path for bezels.");
  wid.push_back(myOpenBrowserButton);

  // The path widens with the tab, but it says how much room a path needs
  myBezelPath = new EditTextWidget(pane, _font,
                                   EditTextWidget::calcWidth(_font, 24));
  wid.push_back(myBezelPath);

  myBezelShowWindowed = new CheckboxWidget(pane, _font, "Windowed modes");
  myBezelShowWindowed->setToolTip("Enable bezels in windowed modes as well.");
  wid.push_back(myBezelShowWindowed);

  // Disable auto borders
  myManualWindow = new CheckboxWidget(pane, _font,
                                      "Manual emulation window", kAutoWindowChanged);
  myManualWindow->setToolTip("Enable if automatic window detection fails.");
  wid.push_back(myManualWindow);

  const auto winSlider = [&]() {
    auto* s = new SliderWidget(pane, _font, 1, 0,
                               4 * fontWidth, "%");
    s->setMinValue(0);
    s->setMaxValue(40);
    s->setTickmarkIntervals(4);
    wid.push_back(s);
    return s;
  };
  myWinLeftSliderLabel = new StaticTextWidget(pane, _font, "Left");
  myWinLeftSlider   = winSlider();
  myWinRightSliderLabel = new StaticTextWidget(pane, _font, "Right");
  myWinRightSlider  = winSlider();
  myWinTopSliderLabel = new StaticTextWidget(pane, _font, "Top");
  myWinTopSlider    = winSlider();
  myWinBottomSliderLabel = new StaticTextWidget(pane, _font, "Bottom");
  myWinBottomSlider = winSlider();

  addToFocusList(wid, myTab, tabID);
  pane->setHelpAnchor("VideoAudioBezels");

  pane->setLayout([this](GUI::BoxLayout& col) {
    using GUI::BoxLayout;
    using GUI::anchoredItem;
    using GUI::indentedItem;
    using GUI::labeledRow;
    using GUI::stretchedItem;
    using Dir = BoxLayout::Dir;
    const int fontWidth = Dialog::fontWidth(),
              VGAP      = Dialog::vGap();
    const int INDENT = CheckboxWidget::prefixSize(_font);

    // The four window sliders share one label column
    GUI::alignLabels({{myWinLeftSliderLabel}, {myWinRightSliderLabel},
                      {myWinTopSliderLabel}, {myWinBottomSliderLabel}});

    // The path row: the browse button, then a field filling the rest
    auto pathRow = std::make_unique<BoxLayout>(Dir::Horizontal);
    pathRow->addSpace(INDENT);
    pathRow->addAuto(anchoredItem(myOpenBrowserButton));
    pathRow->addSpace(fontWidth);
    pathRow->addStretch(stretchedItem(myBezelPath));

    col.addAuto(anchoredItem(myBezelEnableCheckbox));
    col.addSpace(VGAP);
    col.addAuto(std::move(pathRow));
    col.addSpace(VGAP * 3);
    col.addAuto(indentedItem(myBezelShowWindowed, INDENT));
    col.addSpace(VGAP);
    col.addAuto(indentedItem(myManualWindow, INDENT));
    col.addSpace(VGAP);
    // The sliders fill the width they are given, so they end flush with the
    // path field above them and no track width is stated
    col.addAuto(labeledRow(myWinLeftSliderLabel, myWinLeftSlider, 0, INDENT * 2, true));
    col.addSpace(VGAP);
    col.addAuto(labeledRow(myWinRightSliderLabel, myWinRightSlider, 0, INDENT * 2, true));
    col.addSpace(VGAP);
    col.addAuto(labeledRow(myWinTopSliderLabel, myWinTopSlider, 0, INDENT * 2, true));
    col.addSpace(VGAP);
    col.addAuto(labeledRow(myWinBottomSliderLabel, myWinBottomSlider, 0, INDENT * 2, true));
  });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VideoAudioDialog::addAudioTab()
{
  const int fontWidth = Dialog::fontWidth();
  WidgetArray wid;
  VariantList items;

  const int tabID = myTab->addTab(" Audio ", TabWidget::AUTO_WIDTH);
  auto* pane = new TabPaneWidget(myTab, _font);
  myTab->setPaneWidget(tabID, pane);

  // Enable sound
  mySoundEnableCheckbox = new CheckboxWidget(pane, _font,
                                             "Enable sound", kSoundEnableChanged);
  mySoundEnableCheckbox->setToolTip(Event::SoundToggle);
  wid.push_back(mySoundEnableCheckbox);

  // Volume: it sizes its own track (it is not one of the controls that must end
  // flush with the Mode pop-up below)
  myVolumeSliderLabel = new StaticTextWidget(pane, _font, "Volume");
  myVolumeSlider = new SliderWidget(pane, _font,
                                    0, 0, 4 * fontWidth, "%");
  myVolumeSlider->setMinValue(1); myVolumeSlider->setMaxValue(100);
  myVolumeSlider->setTickmarkIntervals(4);
  myVolumeSlider->setToolTip(Event::VolumeDecrease, Event::VolumeIncrease);
  wid.push_back(myVolumeSlider);

  // Mode
  items.clear();
  VarList::push_back(items, "Low quality, medium lag", static_cast<int>(AudioSettings::Preset::lowQualityMediumLag));
  VarList::push_back(items, "High quality, medium lag", static_cast<int>(AudioSettings::Preset::highQualityMediumLag));
  VarList::push_back(items, "High quality, low lag", static_cast<int>(AudioSettings::Preset::highQualityLowLag));
  VarList::push_back(items, "Ultra quality, minimal lag", static_cast<int>(AudioSettings::Preset::ultraQualityMinimalLag));
  VarList::push_back(items, "Custom", static_cast<int>(AudioSettings::Preset::custom));
  myModePopupLabel = new StaticTextWidget(pane, _font, "Mode");
  myModePopup = new PopUpWidget(pane, _font, items, kModeChanged);
  wid.push_back(myModePopup);

  // Output frequency
  items.clear();
  VarList::push_back(items, "44100 Hz", 44100);
  VarList::push_back(items, "48000 Hz", 48000);
  VarList::push_back(items, "96000 Hz", 96000);
  myFreqPopupLabel = new StaticTextWidget(pane, _font, "Sample rate");
  myFreqPopup = new PopUpWidget(pane, _font, items);
  wid.push_back(myFreqPopup);

  // Resampling quality
  items.clear();
  VarList::push_back(items, "Low", static_cast<int>(AudioSettings::ResamplingQuality::nearestNeighbour));
  VarList::push_back(items, "High", static_cast<int>(AudioSettings::ResamplingQuality::lanczos_2));
  VarList::push_back(items, "Ultra", static_cast<int>(AudioSettings::ResamplingQuality::lanczos_3));
  myResamplingPopupLabel = new StaticTextWidget(pane, _font, "Resampling quality");
  myResamplingPopup = new PopUpWidget(pane, _font, items);
  wid.push_back(myResamplingPopup);

  // Param 1
  myHeadroomSliderLabel = new StaticTextWidget(pane, _font, "Headroom");
  myHeadroomSlider = new SliderWidget(pane, _font, 1,
                                      kHeadroomChanged, 10 * fontWidth);
  myHeadroomSlider->setMinValue(0); myHeadroomSlider->setMaxValue(AudioSettings::MAX_HEADROOM);
  myHeadroomSlider->setTickmarkIntervals(5);
  wid.push_back(myHeadroomSlider);

  // Param 2
  myBufferSizeSliderLabel = new StaticTextWidget(pane, _font, "Buffer size");
  myBufferSizeSlider = new SliderWidget(pane, _font, 1,
                                        kBufferSizeChanged, 10 * fontWidth);
  myBufferSizeSlider->setMinValue(0); myBufferSizeSlider->setMaxValue(AudioSettings::MAX_BUFFER_SIZE);
  myBufferSizeSlider->setTickmarkIntervals(5);
  wid.push_back(myBufferSizeSlider);

  // Stereo sound
  myStereoSoundCheckbox = new CheckboxWidget(pane, _font,
                                             "Stereo for all ROMs");
  wid.push_back(myStereoSoundCheckbox);

  myDpcPitchLabel = new StaticTextWidget(pane, _font, "Pitfall II music pitch");
  myDpcPitch = new SliderWidget(pane, _font, 1,
                                0, 5 * fontWidth);
  myDpcPitch->setMinValue(10000); myDpcPitch->setMaxValue(30000);
  myDpcPitch->setStepValue(100);
  myDpcPitch->setTickmarkIntervals(2);
  wid.push_back(myDpcPitch);

  addToFocusList(wid, myTab, tabID);
  pane->setHelpAnchor("VideoAudioAudio");

  pane->setLayout([this](GUI::BoxLayout& col) {
    using GUI::BoxLayout;
    using GUI::anchoredItem;
    using GUI::labeledRow;
    using Dir = BoxLayout::Dir;
    const int VGAP = Dialog::vGap();
    const int INDENT = CheckboxWidget::prefixSize(_font);

    // Three columns, not one: Volume and Mode read as one, the four controls
    // indented under Mode as another, and the pitch slider stands alone.  Merging
    // them would push Mode's box out to the width of "Resampling quality"
    GUI::alignLabels({{myVolumeSliderLabel}, {myModePopupLabel}});
    GUI::alignLabels({{myFreqPopupLabel}, {myResamplingPopupLabel},
                      {myHeadroomSliderLabel}, {myBufferSizeSliderLabel}});
    GUI::alignLabels({{myDpcPitchLabel}});

    // Everything indented under Mode ends flush with IT -- not with the tab,
    // which is wider (the widest tab in the dialog sets that).  The sliders'
    // tracks reach it; the pop-ups, sitting a level further in, are given a cell
    // that reaches it.  Volume is not one of them: it keeps its own track.
    // Headroom/BufferSize are a SEPARATE alignLabels group from Mode's (shared
    // with Freq/Resampling instead), so naming one label from each group lets
    // alignTracks() cross that gap itself -- same gap flushWidth below crosses
    // by hand for the pop-up rows it sizes
    GUI::alignTracks({myHeadroomSlider, myBufferSizeSlider}, myModePopup, INDENT,
                     myHeadroomSliderLabel, myModePopupLabel);
    // DpcPitch sits at Mode's OWN indent level (not a level further in), so only
    // the label-column-width gap between its lone group and Mode's needs crossing
    GUI::alignTracks({myDpcPitch}, myModePopup, 0, myDpcPitchLabel, myModePopupLabel);

    // They sit a level further in than Mode, so the cell that reaches its
    // right edge is that much narrower
    const int flushWidth = GUI::flushSpan(myModePopup, myModePopupLabel, INDENT);

    col.addAuto(anchoredItem(mySoundEnableCheckbox));
    col.addSpace(VGAP);
    col.addAuto(labeledRow(myVolumeSliderLabel, myVolumeSlider, 0, INDENT));
    col.addSpace(VGAP);
    auto modeRow = std::make_unique<BoxLayout>(Dir::Horizontal);
    modeRow->addSpace(INDENT);
    modeRow->addStretch(labeledRow(myModePopupLabel, myModePopup));
    col.addAuto(std::move(modeRow));
    col.addSpace(VGAP);
    auto freqRow = std::make_unique<BoxLayout>(Dir::Horizontal);
    freqRow->addSpace(INDENT * 2);
    freqRow->addFixed(labeledRow(myFreqPopupLabel, myFreqPopup, 0, 0, true), flushWidth);
    col.addAuto(std::move(freqRow));
    col.addSpace(VGAP);
    auto resamplingRow = std::make_unique<BoxLayout>(Dir::Horizontal);
    resamplingRow->addSpace(INDENT * 2);
    resamplingRow->addFixed(labeledRow(myResamplingPopupLabel, myResamplingPopup, 0, 0, true), flushWidth);
    col.addAuto(std::move(resamplingRow));
    col.addSpace(VGAP);
    col.addAuto(labeledRow(myHeadroomSliderLabel, myHeadroomSlider, 0, INDENT * 2));
    col.addSpace(VGAP);
    col.addAuto(labeledRow(myBufferSizeSliderLabel, myBufferSizeSlider, 0, INDENT * 2));
    col.addSpace(VGAP);
    col.addAuto(anchoredItem(myStereoSoundCheckbox));
    col.addSpace(VGAP);
    col.addAuto(labeledRow(myDpcPitchLabel, myDpcPitch, 0, INDENT));
  });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VideoAudioDialog::loadConfig()
{
  const Settings& settings = instance().settings();

  // Display tab
  // Renderer settings
  myRenderer->setSelected(settings.getString("video"), "default");
  handleRendererChanged();

  // TIA interpolation
  myTIAInterpolate->setState(settings.getBool("tia.inter"));

  // TIA zoom levels
  // These are dynamically loaded, since they depend on the size of
  // the desktop and which renderer we're using
  const double minZoom = instance().frameBuffer().supportedTIAMinZoom(); // or 2 if we allow lower values
  const double maxZoom = instance().frameBuffer().supportedTIAMaxZoom();

  myTIAZoom->setMinValue(minZoom * 100);
  myTIAZoom->setMaxValue(maxZoom * 100);
  myTIAZoom->setTickmarkIntervals((maxZoom - minZoom) * 2); // every ~50%
  myTIAZoom->setValue(settings.getFloat("tia.zoom") * 100);

  // Fullscreen
  myFullscreen->setState(settings.getBool("fullscreen"));
  // Fullscreen stretch setting
  myUseStretch->setState(settings.getBool("tia.fs_stretch"));
#ifdef ADAPTABLE_REFRESH_SUPPORT
  // Adapt refresh rate
  myRefreshAdapt->setState(settings.getBool("tia.fs_refresh"));
#endif
  // Fullscreen overscan setting
  myTVOverscan->setValue(settings.getInt("tia.fs_overscan"));
  handleFullScreenChange();

  // Aspect ratio correction
  myCorrectAspect->setState(settings.getBool("tia.correct_aspect"));

  // Aspect ratio setting (NTSC and PAL)
  myVSizeAdjust->setValue(settings.getInt("tia.vsizeadjust"));

  /////////////////////////////////////////////////////////////////////////////
  // Palettes tab
  // TIA Palette
  myPalette = settings.getString("palette");
  myTIAPalette->setSelected(myPalette, PaletteHandler::SETTING_STANDARD);

  // Palette adjustables
  const bool isPAL = instance().hasConsole()
    && instance().console().timing() == ConsoleTiming::pal;

  instance().frameBuffer().tiaSurface().paletteHandler().getAdjustables(myPaletteAdj);
  if(isPAL)
  {
    myPhaseShift->setLabel("PAL phase");
    myPhaseShift->setMinValue((PaletteHandler::DEF_PAL_SHIFT - PaletteHandler::MAX_PHASE_SHIFT) * 10);
    myPhaseShift->setMaxValue((PaletteHandler::DEF_PAL_SHIFT + PaletteHandler::MAX_PHASE_SHIFT) * 10);
    myPhaseShift->setTickmarkIntervals(4);
    myPhaseShift->setToolTip("Adjust PAL phase shift of 'Custom' palette.");
    myPhaseShift->setValue(myPaletteAdj.phasePal);
  }
  else
  {
    myPhaseShift->setLabel("NTSC phase");
    myPhaseShift->setMinValue((PaletteHandler::DEF_NTSC_SHIFT - PaletteHandler::MAX_PHASE_SHIFT) * 10);
    myPhaseShift->setMaxValue((PaletteHandler::DEF_NTSC_SHIFT + PaletteHandler::MAX_PHASE_SHIFT) * 10);
    myPhaseShift->setTickmarkIntervals(4);
    myPhaseShift->setToolTip("Adjust NTSC phase shift of 'Custom' palette.");
    myPhaseShift->setValue(myPaletteAdj.phaseNtsc);
  }
  myTVRedScale->setValue(myPaletteAdj.redScale);
  myTVRedShift->setValue(myPaletteAdj.redShift);
  myTVGreenScale->setValue(myPaletteAdj.greenScale);
  myTVGreenShift->setValue(myPaletteAdj.greenShift);
  myTVBlueScale->setValue(myPaletteAdj.blueScale);
  myTVBlueShift->setValue(myPaletteAdj.blueShift);
  myTVHue->setValue(myPaletteAdj.hue);
  myTVBright->setValue(myPaletteAdj.brightness);
  myTVContrast->setValue(myPaletteAdj.contrast);
  myTVSatur->setValue(myPaletteAdj.saturation);
  myTVGamma->setValue(myPaletteAdj.gamma);
  handlePaletteChange();
  colorPalette();

  // Autodetection
  myDetectPal60->setState(settings.getBool("detectpal60"));
  myDetectNtsc50->setState(settings.getBool("detectntsc50"));

  /////////////////////////////////////////////////////////////////////////////
  // TV Effects tab
  // TV Mode
  myTVMode->setSelected(
    settings.getString("tv.filter"), "0");
  const int preset = settings.getInt("tv.filter");
  handleTVModeChange(static_cast<NTSCFilter::Preset>(preset));

  // TV Custom adjustables
  loadTVAdjustables(NTSCFilter::Preset::CUSTOM);

  // TV phosphor mode & blend
  myTVPhosphor->setSelected(settings.getString(PhosphorHandler::SETTING_MODE), PhosphorHandler::VALUE_BYROM);
  myTVPhosLevel->setValue(settings.getInt(PhosphorHandler::SETTING_BLEND));
  handlePhosphorChange();

  // TV scanline intensity & mask
  myTVScanIntense->setValue(settings.getInt("tv.scanlines"));
  myTVScanMask->setSelected(settings.getString("tv.scanmask"), TIASurface::SETTING_STANDARD);

  /////////////////////////////////////////////////////////////////////////////
#ifdef IMAGE_SUPPORT
  // Bezel tab
  myBezelEnableCheckbox->setState(settings.getBool("bezel.show"));
  myBezelPath->setText(settings.getString("bezel.dir"));
  myBezelShowWindowed->setState(settings.getBool("bezel.windowed"));
  myManualWindow->setState(!settings.getBool("bezel.win.auto"));
  myWinLeftSlider->setValue(settings.getInt("bezel.win.left"));
  myWinRightSlider->setValue(settings.getInt("bezel.win.right"));
  myWinTopSlider->setValue(settings.getInt("bezel.win.top"));
  myWinBottomSlider->setValue(settings.getInt("bezel.win.bottom"));
  handleBezelChange();
#endif

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

  updateAudioEnabledState();

  myTab->loadConfig();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VideoAudioDialog::updateSettingsWithPreset(AudioSettings& audioSettings)
{
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
  Settings& settings = instance().settings();

  /////////////////////////////////////////////////////////////////////////////
  // Display tab
  // Renderer setting
  settings.setValue("video", myRenderer->getSelectedTag().toString());

  // TIA interpolation
  settings.setValue("tia.inter", myTIAInterpolate->getState());

  // Fullscreen
  settings.setValue("fullscreen", myFullscreen->getState());
  // Fullscreen stretch setting
  settings.setValue("tia.fs_stretch", myUseStretch->getState());
#ifdef ADAPTABLE_REFRESH_SUPPORT
  // Adapt refresh rate
  settings.setValue("tia.fs_refresh", myRefreshAdapt->getState());
#endif
  // Fullscreen overscan
  settings.setValue("tia.fs_overscan", myTVOverscan->getValueLabel());

  // TIA zoom levels
  settings.setValue("tia.zoom", myTIAZoom->getValue() / 100.0);

  // Aspect ratio correction
  settings.setValue("tia.correct_aspect", myCorrectAspect->getState());

  // Aspect ratio setting (NTSC and PAL)
  const int oldAdjust = settings.getInt("tia.vsizeadjust");
  const int newAdjust = myVSizeAdjust->getValue();
  const bool vsizeChanged = oldAdjust != newAdjust;

  settings.setValue("tia.vsizeadjust", newAdjust);

  Logger::debug("Saving palette settings...");
  instance().frameBuffer().tiaSurface().paletteHandler().saveConfig(settings);

  // Autodetection
  settings.setValue("detectpal60", myDetectPal60->getState());
  settings.setValue("detectntsc50", myDetectNtsc50->getState());

  /////////////////////////////////////////////////////////////////////////////
  // TV Effects tab
  // TV Mode
  settings.setValue("tv.filter", myTVMode->getSelectedTag().toString());
  // TV Custom adjustables
  NTSCFilter::Adjustable ntscAdj;
  ntscAdj.sharpness = myTVSharp->getValue();
  ntscAdj.resolution = myTVRes->getValue();
  ntscAdj.artifacts = myTVArtifacts->getValue();
  ntscAdj.fringing = myTVFringe->getValue();
  ntscAdj.bleed = myTVBleed->getValue();
  NTSCFilter::setCustomAdjustables(ntscAdj);

  Logger::debug("Saving TV effects options ...");
  NTSCFilter::saveConfig(settings);

  // TV phosphor mode & blend
  settings.setValue(PhosphorHandler::SETTING_MODE, myTVPhosphor->getSelectedTag());
  settings.setValue(PhosphorHandler::SETTING_BLEND, myTVPhosLevel->getValueLabel() == "Off"
                    ? "0" : myTVPhosLevel->getValueLabel());

  // TV scanline intensity & mask
  settings.setValue("tv.scanlines", myTVScanIntense->getValueLabel());
  settings.setValue("tv.scanmask", myTVScanMask->getSelectedTag());

  /////////////////////////////////////////////////////////////////////////////
#ifdef IMAGE_SUPPORT
  // Bezel tab
  settings.setValue("bezel.show", myBezelEnableCheckbox->getState());
  settings.setValue("bezel.dir", myBezelPath->getText());
  settings.setValue("bezel.windowed", myBezelShowWindowed->getState());
  settings.setValue("bezel.win.auto", !myManualWindow->getState());
  settings.setValue("bezel.win.left", myWinLeftSlider->getValueLabel());
  settings.setValue("bezel.win.right", myWinRightSlider->getValueLabel());
  settings.setValue("bezel.win.top", myWinTopSlider->getValueLabel());
  settings.setValue("bezel.win.bottom", myWinBottomSlider->getValueLabel());
#endif

  // Note: The following has to happen after all video related setting have been saved
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
  if(instance().hasConsole() &&
      instance().console().cartridge().name() == "CartridgeDPC")
  {
    auto& cart = static_cast<CartridgeDPC&>(instance().console().cartridge());
    cart.setDpcPitch(myDpcPitch->getValue());
  }

  const auto preset = static_cast<AudioSettings::Preset>
      (myModePopup->getSelectedTag().toInt());
  audioSettings.setPreset(preset);

  if(preset == AudioSettings::Preset::custom) {
    // Fragsize
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
      myCorrectAspect->setState(true);
      myVSizeAdjust->setValue(0);

      handleFullScreenChange();
      break;
    }

    case 1:  // Palettes
    {
      const bool isPAL = instance().hasConsole()
        && instance().console().timing() == ConsoleTiming::pal;

      myTIAPalette->setSelected(PaletteHandler::SETTING_STANDARD);
      myPhaseShift->setValue(isPAL
                             ? PaletteHandler::DEF_PAL_SHIFT * 10
                             : PaletteHandler::DEF_NTSC_SHIFT * 10);
      myTVRedScale->setValue(50);
      myTVRedShift->setValue(PaletteHandler::DEF_RGB_SHIFT);
      myTVGreenScale->setValue(50);
      myTVGreenShift->setValue(PaletteHandler::DEF_RGB_SHIFT);
      myTVBlueScale->setValue(50);
      myTVBlueShift->setValue(PaletteHandler::DEF_RGB_SHIFT);
      myTVHue->setValue(50);
      myTVSatur->setValue(50);
      myTVContrast->setValue(50);
      myTVBright->setValue(50);
      myTVGamma->setValue(50);
      handlePaletteChange();
      handlePaletteUpdate();
      myDetectPal60->setState(false);
      myDetectNtsc50->setState(false);
      break;
    }

    case 2:  // TV effects
    {
      myTVMode->setSelected("0", "0");

      // TV phosphor mode & blend
      myTVPhosphor->setSelected(PhosphorHandler::VALUE_BYROM);
      myTVPhosLevel->setValue(50);

      // TV scanline intensity & mask
      myTVScanIntense->setValue(0);
      myTVScanMask->setSelected(TIASurface::SETTING_STANDARD);

      // Make sure that mutually-exclusive items are not enabled at the same time
      handleTVModeChange(NTSCFilter::Preset::OFF);
      handlePhosphorChange();
      loadTVAdjustables(NTSCFilter::Preset::CUSTOM);
      break;
    }
    case 3: // Bezels
#ifdef IMAGE_SUPPORT
      myBezelEnableCheckbox->setState(true);
      myBezelPath->setText(instance().userDir().getShortPath());
      myBezelShowWindowed->setState(false);
      myManualWindow->setState(false);
      handleBezelChange();
      break;

    case 4:  // Audio
#endif
      mySoundEnableCheckbox->setState(AudioSettings::DEFAULT_ENABLED);
      myVolumeSlider->setValue(AudioSettings::DEFAULT_VOLUME);
      myStereoSoundCheckbox->setState(AudioSettings::DEFAULT_STEREO);
      myDpcPitch->setValue(AudioSettings::DEFAULT_DPC_PITCH);
      myModePopup->setSelected(static_cast<int>(AudioSettings::DEFAULT_PRESET));

      if constexpr(AudioSettings::DEFAULT_PRESET == AudioSettings::Preset::custom) {
        myResamplingPopup->setSelected(static_cast<int>(AudioSettings::DEFAULT_RESAMPLING_QUALITY));
        myFreqPopup->setSelected(AudioSettings::DEFAULT_SAMPLE_RATE);
        myHeadroomSlider->setValue(AudioSettings::DEFAULT_HEADROOM);
        myBufferSizeSlider->setValue(AudioSettings::DEFAULT_BUFFER_SIZE);
      }
      else updatePreset();

      updateAudioEnabledState();
      break;

    default:  // satisfy compiler
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VideoAudioDialog::handleTVModeChange(NTSCFilter::Preset preset)
{
  const bool enable = preset == NTSCFilter::Preset::CUSTOM;

  myTVSharpLabel->setEnabled(enable);
  myTVSharp->setEnabled(enable);
  myTVResLabel->setEnabled(enable);
  myTVRes->setEnabled(enable);
  myTVArtifactsLabel->setEnabled(enable);
  myTVArtifacts->setEnabled(enable);
  myTVFringeLabel->setEnabled(enable);
  myTVFringe->setEnabled(enable);
  myTVBleedLabel->setEnabled(enable);
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
  NTSCFilter::getAdjustables(adj, preset);
  myTVSharp->setValue(adj.sharpness);
  myTVRes->setValue(adj.resolution);
  myTVArtifacts->setValue(adj.artifacts);
  myTVFringe->setValue(adj.fringing);
  myTVBleed->setValue(adj.bleed);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VideoAudioDialog::handleRendererChanged()
{
  const bool enable = myRenderer->getSelectedTag().toString() != "software";
  myTIAInterpolate->setEnabled(enable);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VideoAudioDialog::handlePaletteChange()
{
  const bool enable = myTIAPalette->getSelectedTag().toString() == "custom";

  myPhaseShiftLabel->setEnabled(enable);
  myPhaseShift->setEnabled(enable);
  myTVRedScaleLabel->setEnabled(enable);
  myTVRedScale->setEnabled(enable);
  myTVRedShift->setEnabled(enable);
  myTVGreenScaleLabel->setEnabled(enable);
  myTVGreenScale->setEnabled(enable);
  myTVGreenShift->setEnabled(enable);
  myTVBlueScaleLabel->setEnabled(enable);
  myTVBlueScale->setEnabled(enable);
  myTVBlueShift->setEnabled(enable);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VideoAudioDialog::handleShiftChanged(SliderWidget* widget)
{
  widget->setValueLabel(std::format("{:4.1f}{}", 0.1 * widget->getValue(), DEGREE));
  handlePaletteUpdate();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VideoAudioDialog::handlePaletteUpdate()
{
  // TIA Palette
  instance().settings().setValue("palette",
                                  myTIAPalette->getSelectedTag().toString());
  // Palette adjustables
  PaletteHandler::Adjustable paletteAdj;
  const bool isPAL = instance().hasConsole()
    && instance().console().timing() == ConsoleTiming::pal;

  if(isPAL)
  {
    paletteAdj.phaseNtsc = myPaletteAdj.phaseNtsc; // unchanged
    paletteAdj.phasePal = myPhaseShift->getValue();
  }
  else
  {
    paletteAdj.phaseNtsc = myPhaseShift->getValue();
    paletteAdj.phasePal = myPaletteAdj.phasePal; // unchanged
  }
  paletteAdj.redScale   = myTVRedScale->getValue();
  paletteAdj.redShift   = myTVRedShift->getValue();
  paletteAdj.greenScale = myTVGreenScale->getValue();
  paletteAdj.greenShift = myTVGreenShift->getValue();
  paletteAdj.blueScale  = myTVBlueScale->getValue();
  paletteAdj.blueShift  = myTVBlueShift->getValue();
  paletteAdj.hue        = myTVHue->getValue();
  paletteAdj.saturation = myTVSatur->getValue();
  paletteAdj.contrast   = myTVContrast->getValue();
  paletteAdj.brightness = myTVBright->getValue();
  paletteAdj.gamma      = myTVGamma->getValue();
  instance().frameBuffer().tiaSurface().paletteHandler().setAdjustables(paletteAdj);

  if(instance().hasConsole())
  {
    instance().frameBuffer().tiaSurface().paletteHandler().setPalette();

    for(auto& row: myColor)
      for(auto* w: row)
        w->setDirty();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VideoAudioDialog::handleFullScreenChange()
{
  const bool enable = myFullscreen->getState();
  myUseStretch->setEnabled(enable);
#ifdef ADAPTABLE_REFRESH_SUPPORT
  myRefreshAdapt->setEnabled(enable);
#endif
  myTVOverscanLabel->setEnabled(enable);
  myTVOverscan->setEnabled(enable);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VideoAudioDialog::handleOverscanChange()
{
  if(myTVOverscan->getValue() == 0)
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
  const bool enable = myTVPhosphor->getSelectedTag() != PhosphorHandler::VALUE_BYROM;
  myTVPhosLevelLabel->setEnabled(enable);
  myTVPhosLevel->setEnabled(enable);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VideoAudioDialog::handleBezelChange()
{
  const bool enable = myBezelEnableCheckbox->getState();
  const bool nonAuto = myManualWindow->getState();

  myOpenBrowserButton->setEnabled(enable);
  myBezelPath->setEnabled(enable);
  myBezelShowWindowed->setEnabled(enable);
  myWinLeftSliderLabel->setEnabled(enable && nonAuto);
  myWinLeftSlider->setEnabled(enable && nonAuto);
  myWinRightSliderLabel->setEnabled(enable && nonAuto);
  myWinRightSlider->setEnabled(enable && nonAuto);
  myWinTopSliderLabel->setEnabled(enable && nonAuto);
  myWinTopSlider->setEnabled(enable && nonAuto);
  myWinBottomSliderLabel->setEnabled(enable && nonAuto);
  myWinBottomSlider->setEnabled(enable && nonAuto);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VideoAudioDialog::handleCommand(CommandSender* sender, int cmd,
                                     int data, int id)
{
  switch(cmd)
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

    case kPhaseShiftChanged:
      handleShiftChanged(myPhaseShift);
      break;

    case kRedShiftChanged:
      handleShiftChanged(myTVRedShift);
      break;

    case kGreenShiftChanged:
      handleShiftChanged(myTVGreenShift);
      break;

    case kBlueShiftChanged:
      handleShiftChanged(myTVBlueShift);
      break;

    case kRendererChanged:
      handleRendererChanged();
      break;

    case kVSizeChanged:
    {
      const int adjust = myVSizeAdjust->getValue();

      if(!adjust)
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
      handleTVModeChange(static_cast<NTSCFilter::Preset>(myTVMode->getSelectedTag().toInt()));
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
      if(myTVScanIntense->getValue() == 0)
      {
        myTVScanIntense->setValueLabel("Off");
        myTVScanIntense->setValueUnit("");
        myTVScanMaskLabel->setEnabled(false);
        myTVScanMask->setEnabled(false);
      }
      else
      {
        myTVScanIntense->setValueUnit("%");
        myTVScanMaskLabel->setEnabled(true);
        myTVScanMask->setEnabled(true);
      }
      break;

    case kPhosphorChanged:
      handlePhosphorChange();
      break;

    case kPhosBlendChanged:
      if(myTVPhosLevel->getValue() == 0)
      {
        myTVPhosLevel->setValueLabel("Off");
        myTVPhosLevel->setValueUnit("");
      }
      else
        myTVPhosLevel->setValueUnit("%");
      break;

    case kBezelEnableChanged:
    case kAutoWindowChanged:
      handleBezelChange();
      break;

    case kChooseBezelDirCmd:
      BrowserDialog::show(this, _font, "Select Bezel Directory",
                          myBezelPath->getText(),
                          BrowserDialog::Mode::Directories,
                          [this](bool OK, const FSNode& node) {
                            if(OK) myBezelPath->setText(node.getShortPath());
                          });
      break;

    case kSoundEnableChanged:
      updateAudioEnabledState();
      break;

    case kModeChanged:
      updatePreset();
      updateAudioEnabledState();
      break;

    case kHeadroomChanged:
      myHeadroomSlider->setValueLabel(
        std::format("{:.1f} frames", 0.5 * myHeadroomSlider->getValue()));
      break;

    case kBufferSizeChanged:
      myBufferSizeSlider->setValueLabel(
        std::format("{:.1f} frames", 0.5 * myBufferSizeSlider->getValue()));
      break;

    default:
      Dialog::handleCommand(sender, cmd, data, 0);
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VideoAudioDialog::createPaletteWidgets(TabPaneWidget* pane)
{
  const GUI::Font& ifont = instance().frameBuffer().infoFont();

  for(int idx = 0; idx < NUM_CHROMA; ++idx)
  {
    // The chroma's hex digit, put in as the palette loads
    myColorLbl[idx] = new StaticTextWidget(pane, ifont, "");
    for(int lum = 0; lum < NUM_LUMA; ++lum)
    {
      myColor[idx][lum] = new ColorWidget(pane, _font, 1, 1, 0, false);
      myColor[idx][lum]->clearFlags(FLAG_CLEARBG | FLAG_RETAIN_FOCUS
                                    | FLAG_MOUSE_FOCUS | FLAG_BORDER);
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
unique_ptr<GUI::Layout> VideoAudioDialog::paletteLayout()
{
  using GUI::GridLayout;
  using GUI::widgetItem;
  using GUI::anchoredItem;

  const GUI::Font& ifont = instance().frameBuffer().infoFont();

  // A chroma per row, a luminance per column, plus a column for the hex digit
  // naming the chroma.  The swatches FILL their cells, so the grid tiles the
  // area it is given exactly -- whatever size that is, and with no gaps
  auto grid = std::make_unique<GridLayout>(1 + NUM_LUMA, NUM_CHROMA);

  grid->columnFixed(0, static_cast<int>(ifont.getMaxCharWidth() * 1.5));
  for(int lum = 0; lum < NUM_LUMA; ++lum)
    grid->columnStretch(1 + lum);
  for(int idx = 0; idx < NUM_CHROMA; ++idx)
    grid->rowStretch(idx);

  for(int idx = 0; idx < NUM_CHROMA; ++idx)
  {
    grid->place(0, idx, anchoredItem(myColorLbl[idx]));
    for(int lum = 0; lum < NUM_LUMA; ++lum)
      grid->place(1 + lum, idx, widgetItem(myColor[idx][lum]));
  }

  return grid;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VideoAudioDialog::colorPalette()
{
  if(instance().hasConsole())
  {
    static constexpr int order[2][NUM_CHROMA] =
    {
      {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15},
      {0, 1, 2, 4, 6, 8, 10, 12, 13, 11, 9, 7, 5, 3, 14, 15}
    };
    const int type = instance().console().timing() == ConsoleTiming::pal ? 1 : 0;

    for(int idx = 0; idx < NUM_CHROMA; ++idx)
    {
      const int color = order[type][idx];

      myColorLbl[idx]->setLabel(std::format("{:1X}", color));
      for(int lum = 0; lum < NUM_LUMA; ++lum)
        myColor[idx][lum]->setColor(color * NUM_CHROMA + lum * 2); // skip grayscale colors
    }
  }
  else
    // disable palette
    for(const auto& row: myColor)
      for(auto* w: row)
        w->setEnabled(false);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VideoAudioDialog::updateAudioEnabledState()
{
  const bool active = mySoundEnableCheckbox->getState();
  const auto preset = static_cast<AudioSettings::Preset>
      (myModePopup->getSelectedTag().toInt());
  const bool userMode = preset == AudioSettings::Preset::custom;

  myVolumeSliderLabel->setEnabled(active);
  myVolumeSlider->setEnabled(active);
  myStereoSoundCheckbox->setEnabled(active);
  myModePopupLabel->setEnabled(active);
  myModePopup->setEnabled(active);
  // enable only for Pitfall II cart
  const bool dpcEnable = active && instance().hasConsole() &&
      instance().console().cartridge().name() == "CartridgeDPC";
  myDpcPitchLabel->setEnabled(dpcEnable);
  myDpcPitch->setEnabled(dpcEnable);

  myFreqPopupLabel->setEnabled(active && userMode);
  myFreqPopup->setEnabled(active && userMode);
  myResamplingPopupLabel->setEnabled(active && userMode);
  myResamplingPopup->setEnabled(active && userMode);
  myHeadroomSliderLabel->setEnabled(active && userMode);
  myHeadroomSlider->setEnabled(active && userMode);
  myBufferSizeSliderLabel->setEnabled(active && userMode);
  myBufferSizeSlider->setEnabled(active && userMode);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VideoAudioDialog::updatePreset()
{
  const auto preset = static_cast<AudioSettings::Preset>
      (myModePopup->getSelectedTag().toInt());

  // Make a copy that does not affect the actual settings...
  AudioSettings audioSettings = instance().audioSettings();
  audioSettings.setPersistent(false);
  // ... and set the requested preset
  audioSettings.setPreset(preset);

  updateSettingsWithPreset(audioSettings);
}
