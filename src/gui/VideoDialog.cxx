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

#include "bspf.hxx"
#include "Control.hxx"
#include "Dialog.hxx"
#include "Menu.hxx"
#include "OSystem.hxx"
#include "EditTextWidget.hxx"
#include "PopUpWidget.hxx"
#include "Console.hxx"
#include "TIA.hxx"
#include "Settings.hxx"
#include "Widget.hxx"
#include "Font.hxx"
#include "TabWidget.hxx"
#include "NTSCFilter.hxx"
#include "TIASurface.hxx"
#include "VideoDialog.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
VideoDialog::VideoDialog(OSystem& osystem, DialogContainer& parent,
                         const GUI::Font& font, int max_w, int max_h, bool isGlobal)
  : Dialog(osystem, parent)
{
  const int VGAP = 4;
  const int VBORDER = 8;
  const int HBORDER = 8;
  const int lineHeight   = font.getLineHeight(),
            fontWidth    = font.getMaxCharWidth(),
            fontHeight   = font.getFontHeight(),
            buttonWidth  = font.getStringWidth("Defaults") + 20,
            buttonHeight = font.getLineHeight() + 4;
  int xpos, ypos, tabID;
  int lwidth = font.getStringWidth("NTSC Aspect "),
    pwidth = font.getStringWidth("XXXXxXXXX"),
    swidth = 69;

  WidgetArray wid;
  VariantList items;

  // Set real dimensions
  _w = std::min(52 * fontWidth + 10, max_w);
  _h = std::min(16 * (lineHeight + 4) + 14, max_h);

  // The tab widget
  xpos = HBORDER;  ypos = VBORDER;
  myTab = new TabWidget(this, font, xpos, ypos, _w - 2*xpos, _h - buttonHeight - 20);
  addTabWidget(myTab);

  //////////////////////////////////////////////////////////
  // 1) General options
  tabID = myTab->addTab(" General ");

  // Video renderer
  myRenderer = new PopUpWidget(myTab, font, xpos, ypos, pwidth, lineHeight,
                               instance().frameBuffer().supportedRenderers(),
                               "Renderer ", lwidth);
  wid.push_back(myRenderer);
  ypos += lineHeight + VGAP;

  // TIA filters (will be dynamically filled later)
  myTIAZoom = new PopUpWidget(myTab, font, xpos, ypos, pwidth,
                              lineHeight, items, "TIA Zoom ", lwidth);
  wid.push_back(myTIAZoom);
  ypos += lineHeight + VGAP;

  // TIA Palette
  items.clear();
  VarList::push_back(items, "Standard", "standard");
  VarList::push_back(items, "Z26", "z26");
  VarList::push_back(items, "User", "user");
  myTIAPalette = new PopUpWidget(myTab, font, xpos, ypos, pwidth,
                                 lineHeight, items, "TIA Palette ", lwidth);
  wid.push_back(myTIAPalette);
  ypos += lineHeight + VGAP;

  // TIA interpolation
  items.clear();
  VarList::push_back(items, "Linear", "linear");
  VarList::push_back(items, "Nearest", "nearest");
  myTIAInterpolate = new PopUpWidget(myTab, font, xpos, ypos, pwidth, lineHeight,
                                     items, "TIA Inter ", lwidth);
  wid.push_back(myTIAInterpolate);
  ypos += lineHeight + VGAP;

  // Timing to use between frames
  items.clear();
  VarList::push_back(items, "Sleep", "sleep");
  VarList::push_back(items, "Busy-wait", "busy");
  myFrameTiming = new PopUpWidget(myTab, font, xpos, ypos, pwidth, lineHeight,
                                  items, "Timing (*) ", lwidth);
  wid.push_back(myFrameTiming);
  ypos += lineHeight + VGAP;

  // Aspect ratio (NTSC mode)
  myNAspectRatio =
    new SliderWidget(myTab, font, xpos, ypos-1, pwidth, lineHeight,
                     "NTSC Aspect ", lwidth, kNAspectRatioChanged);
  myNAspectRatio->setMinValue(80); myNAspectRatio->setMaxValue(120);
  wid.push_back(myNAspectRatio);
  myNAspectRatioLabel =
    new StaticTextWidget(myTab, font, xpos + myNAspectRatio->getWidth() + 4,
                         ypos + 1, fontWidth * 3, fontHeight, "", kTextAlignLeft);
  myNAspectRatioLabel->setFlags(WIDGET_CLEARBG);
  ypos += lineHeight + VGAP;

  // Aspect ratio (PAL mode)
  myPAspectRatio =
    new SliderWidget(myTab, font, xpos, ypos-1, pwidth, lineHeight,
                     "PAL Aspect ", lwidth, kPAspectRatioChanged);
  myPAspectRatio->setMinValue(80); myPAspectRatio->setMaxValue(120);
  wid.push_back(myPAspectRatio);
  myPAspectRatioLabel =
    new StaticTextWidget(myTab, font, xpos + myPAspectRatio->getWidth() + 4,
                         ypos + 1, fontWidth * 3, fontHeight, "", kTextAlignLeft);
  myPAspectRatioLabel->setFlags(WIDGET_CLEARBG);
  ypos += lineHeight + VGAP;

  // Framerate
  myFrameRate =
    new SliderWidget(myTab, font, xpos, ypos-1, pwidth, lineHeight,
                     "Framerate ", lwidth, kFrameRateChanged);
  myFrameRate->setMinValue(0); myFrameRate->setMaxValue(300);
  myFrameRate->setStepValue(10);
  wid.push_back(myFrameRate);
  myFrameRateLabel =
    new StaticTextWidget(myTab, font, xpos + myFrameRate->getWidth() + 4,
                         ypos + 1, fontWidth * 4, fontHeight, "", kTextAlignLeft);
  myFrameRateLabel->setFlags(WIDGET_CLEARBG);

  // Add message concerning usage
  const GUI::Font& infofont = instance().frameBuffer().infoFont();
  ypos = myTab->getHeight() - 5 - fontHeight - infofont.getFontHeight() - 10;
  new StaticTextWidget(myTab, infofont, 10, ypos,
        font.getStringWidth("(*) Requires application restart"), fontHeight,
        "(*) Requires application restart", kTextAlignLeft);

  // Move over to the next column
  xpos += myNAspectRatio->getWidth() + myNAspectRatioLabel->getWidth() + 30;
  ypos = VBORDER;

  // Fullscreen
  myFullscreen = new CheckboxWidget(myTab, font, xpos, ypos, "Fullscreen");
  wid.push_back(myFullscreen);
  ypos += lineHeight + VGAP;

  // FS stretch
  myUseStretch = new CheckboxWidget(myTab, font, xpos, ypos, "Fullscreen Fill");
  wid.push_back(myUseStretch);
  ypos += lineHeight + VGAP;

  // Use sync to vblank
  myUseVSync = new CheckboxWidget(myTab, font, xpos, ypos, "VSync");
  wid.push_back(myUseVSync);
  ypos += (lineHeight + VGAP) * 2;

  // Skip progress load bars for SuperCharger ROMs
  // Doesn't really belong here, but I couldn't find a better place for it
  myFastSCBios = new CheckboxWidget(myTab, font, xpos, ypos, "Fast SC/AR BIOS");
  wid.push_back(myFastSCBios);
  ypos += lineHeight + VGAP;

  // Show UI messages onscreen
  myUIMessages = new CheckboxWidget(myTab, font, xpos, ypos, "Show UI messages");
  wid.push_back(myUIMessages);
  ypos += lineHeight + VGAP;

  // Center window (in windowed mode)
  myCenter = new CheckboxWidget(myTab, font, xpos, ypos, "Center window");
  wid.push_back(myCenter);
  ypos += lineHeight + VGAP;

  // Use multi-threading
  myUseThreads = new CheckboxWidget(myTab, font, xpos, ypos, "Use multi-threading");
  wid.push_back(myUseThreads);

  // Add items for tab 0
  addToFocusList(wid, myTab, tabID);

  //////////////////////////////////////////////////////////
  // 2) TV effects options
  wid.clear();
  tabID = myTab->addTab(" TV Effects ");
  xpos = HBORDER;
  ypos = VBORDER;

  // TV Mode
  items.clear();
  VarList::push_back(items, "Disabled", NTSCFilter::PRESET_OFF);
  VarList::push_back(items, "Composite", NTSCFilter::PRESET_COMPOSITE);
  VarList::push_back(items, "S-Video", NTSCFilter::PRESET_SVIDEO);
  VarList::push_back(items, "RGB", NTSCFilter::PRESET_RGB);
  VarList::push_back(items, "Bad adjust", NTSCFilter::PRESET_BAD);
  VarList::push_back(items, "Custom", NTSCFilter::PRESET_CUSTOM);
  lwidth = font.getStringWidth("TV Mode ");
  pwidth = font.getStringWidth("Bad adjust");
  myTVMode =
    new PopUpWidget(myTab, font, xpos, ypos, pwidth, lineHeight,
                    items, "TV Mode ", lwidth, kTVModeChanged);
  wid.push_back(myTVMode);
  ypos += lineHeight + VGAP;

  // Custom adjustables (using macro voodoo)
  xpos += 8+1; ypos += 0;
  pwidth = lwidth;
  lwidth = font.getStringWidth("Saturation ");

#define CREATE_CUSTOM_SLIDERS(obj, desc)                                 \
  myTV ## obj =                                                          \
    new SliderWidget(myTab, font, xpos, ypos-1, pwidth, lineHeight,      \
                         desc, lwidth, kTV ## obj ##Changed);            \
  myTV ## obj->setMinValue(0); myTV ## obj->setMaxValue(100);            \
  wid.push_back(myTV ## obj);                                            \
  myTV ## obj ## Label =                                                 \
    new StaticTextWidget(myTab, font, xpos+myTV ## obj->getWidth()+4,    \
                    ypos+1, fontWidth*3, fontHeight, "", kTextAlignLeft);\
  myTV ## obj->setFlags(WIDGET_CLEARBG);                                 \
  ypos += lineHeight + VGAP;

  pwidth = swidth;
  CREATE_CUSTOM_SLIDERS(Contrast, "Contrast ");
  CREATE_CUSTOM_SLIDERS(Bright, "Brightness ");
  CREATE_CUSTOM_SLIDERS(Hue, "Hue ");
  CREATE_CUSTOM_SLIDERS(Satur, "Saturation ");
  CREATE_CUSTOM_SLIDERS(Gamma, "Gamma ");
  CREATE_CUSTOM_SLIDERS(Sharp, "Sharpness ");
  CREATE_CUSTOM_SLIDERS(Res, "Resolution ");
  CREATE_CUSTOM_SLIDERS(Artifacts, "Artifacts ");
  CREATE_CUSTOM_SLIDERS(Fringe, "Fringing ");
  CREATE_CUSTOM_SLIDERS(Bleed, "Bleeding ");

  xpos += myTVContrast->getWidth() + myTVContrastLabel->getWidth() + 36;
  ypos = VBORDER;

  lwidth = font.getStringWidth("Intensity ");
  pwidth = font.getMaxCharWidth() * 6;

  // TV Phosphor effect
  items.clear();
  VarList::push_back(items, "Always", "always");
  VarList::push_back(items, "Per-ROM", "byrom");
  myTVPhosphor = new PopUpWidget(myTab, font, xpos, ypos,
      font.getStringWidth("Per-ROM"), lineHeight, items,
      "TV Phosphor ", font.getStringWidth("TV Phosphor "));
  ypos += lineHeight + VGAP;

  // TV Phosphor default level
  xpos += 8-8+16;
  pwidth = swidth;
  CREATE_CUSTOM_SLIDERS(PhosLevel, "Default   ");
  ypos += 6;

  // Scanline intensity and interpolation
  xpos -= 8+8;
  myTVScanLabel =
    new StaticTextWidget(myTab, font, xpos, ypos, font.getStringWidth("Scanline settings"),
                         fontHeight, "Scanline settings", kTextAlignLeft);
  ypos += lineHeight;

  xpos += 8+8;
  CREATE_CUSTOM_SLIDERS(ScanIntense, "Intensity ");

  myTVScanInterpolate = new CheckboxWidget(myTab, font, xpos, ypos,
                                           "Interpolation");
  wid.push_back(myTVScanInterpolate);
  ypos += lineHeight + 6;

  // Adjustable presets
  xpos -= 8+8;
  int cloneWidth = font.getStringWidth("Clone Bad Adjust") + 20;
#define CREATE_CLONE_BUTTON(obj, desc)                                 \
  myClone ## obj =                                                     \
    new ButtonWidget(myTab, font, xpos, ypos, cloneWidth, buttonHeight,\
                     desc, kClone ## obj ##Cmd);                       \
  wid.push_back(myClone ## obj);                                       \
  ypos += lineHeight + 4 + VGAP

  ypos += VGAP;
  CREATE_CLONE_BUTTON(Composite, "Clone Composite");
  CREATE_CLONE_BUTTON(Svideo, "Clone S-Video");
  CREATE_CLONE_BUTTON(RGB, "Clone RGB");
  CREATE_CLONE_BUTTON(Bad, "Clone Bad Adjust");
  CREATE_CLONE_BUTTON(Custom, "Revert");

  // Add items for tab 2
  addToFocusList(wid, myTab, tabID);

  // Activate the first tab
  myTab->setActiveTab(0);

  // Add Defaults, OK and Cancel buttons
  wid.clear();
  ButtonWidget* b;
  b = new ButtonWidget(this, font, 10, _h - buttonHeight - 10,
                       buttonWidth, buttonHeight, "Defaults", GuiObject::kDefaultsCmd);
  wid.push_back(b);
  addOKCancelBGroup(wid, font);
  addBGroupToFocusList(wid);

  // Disable certain functions when we know they aren't present
#ifndef WINDOWED_SUPPORT
  myFullscreen->clearFlags(WIDGET_ENABLED);
  myCenter->clearFlags(WIDGET_ENABLED);
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VideoDialog::loadConfig()
{
  // Renderer settings
  myRenderer->setSelected(instance().settings().getString("video"), "default");

  // TIA Filter
  // These are dynamically loaded, since they depend on the size of
  // the desktop and which renderer we're using
  const VariantList& items = instance().frameBuffer().supportedTIAZoomLevels();
  myTIAZoom->addItems(items);
  myTIAZoom->setSelected(instance().settings().getString("tia.zoom"), "3");

  // TIA Palette
  myTIAPalette->setSelected(
    instance().settings().getString("palette"), "standard");

  // TIA interpolation
  const string& tia_inter = instance().settings().getBool("tia.inter") ?
                           "linear" : "nearest";
  myTIAInterpolate->setSelected(tia_inter, "nearest");

  // Wait between frames
  myFrameTiming->setSelected(
    instance().settings().getString("timing"), "sleep");

  // Aspect ratio setting (NTSC and PAL)
  myNAspectRatio->setValue(instance().settings().getInt("tia.aspectn"));
  myNAspectRatioLabel->setLabel(instance().settings().getString("tia.aspectn"));
  myPAspectRatio->setValue(instance().settings().getInt("tia.aspectp"));
  myPAspectRatioLabel->setLabel(instance().settings().getString("tia.aspectp"));

  // Framerate (0 or -1 means automatic framerate calculation)
  int rate = instance().settings().getInt("framerate");
  myFrameRate->setValue(rate < 0 ? 0 : rate);
  myFrameRateLabel->setLabel(rate <= 0 ? "Auto" :
    instance().settings().getString("framerate"));

  // Fullscreen
  myFullscreen->setState(instance().settings().getBool("fullscreen"));

  // Fullscreen stretch setting
  myUseStretch->setState(instance().settings().getBool("tia.fsfill"));

  // Use sync to vertical blank
  myUseVSync->setState(instance().settings().getBool("vsync"));

  // Show UI messages
  myUIMessages->setState(instance().settings().getBool("uimessages"));

  // Center window
  myCenter->setState(instance().settings().getBool("center"));

  // Fast loading of Supercharger BIOS
  myFastSCBios->setState(instance().settings().getBool("fastscbios"));

  // Multi-threaded rendering
  myUseThreads->setState(instance().settings().getBool("threads"));

  // TV Mode
  myTVMode->setSelected(
    instance().settings().getString("tv.filter"), "0");
  int preset = instance().settings().getInt("tv.filter");
  handleTVModeChange(NTSCFilter::Preset(preset));

  // TV Custom adjustables
  loadTVAdjustables(NTSCFilter::PRESET_CUSTOM);

  // TV phosphor mode
  myTVPhosphor->setSelected(
      instance().settings().getString("tv.phosphor"), "byrom");

  // TV phosphor blend
  myTVPhosLevel->setValue(instance().settings().getInt("tv.phosblend"));
  myTVPhosLevelLabel->setLabel(instance().settings().getString("tv.phosblend"));

  // TV scanline intensity and interpolation
  myTVScanIntense->setValue(instance().settings().getInt("tv.scanlines"));
  myTVScanIntenseLabel->setLabel(instance().settings().getString("tv.scanlines"));
  myTVScanInterpolate->setState(instance().settings().getBool("tv.scaninter"));

  myTab->loadConfig();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VideoDialog::saveConfig()
{
  // Renderer setting
  instance().settings().setValue("video",
    myRenderer->getSelectedTag().toString());

  // TIA Filter
  instance().settings().setValue("tia.zoom",
    myTIAZoom->getSelectedTag().toString());
  instance().frameBuffer().setZoomMode(instance().settings().getInt("tia.zoom"));

  // TIA Palette
  instance().settings().setValue("palette",
    myTIAPalette->getSelectedTag().toString());

  // Wait between frames
  instance().settings().setValue("timing",
    myFrameTiming->getSelectedTag().toString());

  // TIA interpolation
  instance().settings().setValue("tia.inter",
    myTIAInterpolate->getSelectedTag().toString() == "linear" ? true : false);

  // Aspect ratio setting (NTSC and PAL)
  instance().settings().setValue("tia.aspectn", myNAspectRatioLabel->getLabel());
  instance().settings().setValue("tia.aspectp", myPAspectRatioLabel->getLabel());

  // Framerate
  int f = myFrameRate->getValue();
  instance().settings().setValue("framerate", f);
  if(instance().hasConsole())
  {
    // Make sure auto-frame calculation is only enabled when necessary
    instance().console().tia().enableAutoFrame(f <= 0);
    instance().console().setFramerate(float(f));
  }

  // Fullscreen
  instance().settings().setValue("fullscreen", myFullscreen->getState());

  // Fullscreen stretch setting
  instance().settings().setValue("tia.fsfill", myUseStretch->getState());

  // Use sync to vertical blank
  instance().settings().setValue("vsync", myUseVSync->getState());

  // Show UI messages
  instance().settings().setValue("uimessages", myUIMessages->getState());

  // Center window
  instance().settings().setValue("center", myCenter->getState());

  // Fast loading of Supercharger BIOS
  instance().settings().setValue("fastscbios", myFastSCBios->getState());

  // Multi-threaded rendering
  instance().settings().setValue("threads", myUseThreads->getState());
  if(instance().hasConsole())
    instance().frameBuffer().tiaSurface().ntsc().enableThreading(myUseThreads->getState());

  // TV Mode
  instance().settings().setValue("tv.filter",
    myTVMode->getSelectedTag().toString());

  // TV Custom adjustables
  NTSCFilter::Adjustable adj;
  adj.hue         = myTVHue->getValue();
  adj.saturation  = myTVSatur->getValue();
  adj.contrast    = myTVContrast->getValue();
  adj.brightness  = myTVBright->getValue();
  adj.sharpness   = myTVSharp->getValue();
  adj.gamma       = myTVGamma->getValue();
  adj.resolution  = myTVRes->getValue();
  adj.artifacts   = myTVArtifacts->getValue();
  adj.fringing    = myTVFringe->getValue();
  adj.bleed       = myTVBleed->getValue();
  instance().frameBuffer().tiaSurface().ntsc().setCustomAdjustables(adj);

  // TV phosphor mode
  instance().settings().setValue("tv.phosphor",
      myTVPhosphor->getSelectedTag().toString());

  // TV phosphor blend
  instance().settings().setValue("tv.phosblend", myTVPhosLevelLabel->getLabel());
  Properties::setDefault(Display_PPBlend, myTVPhosLevelLabel->getLabel());

  // TV scanline intensity and interpolation
  instance().settings().setValue("tv.scanlines", myTVScanIntenseLabel->getLabel());
  instance().settings().setValue("tv.scaninter", myTVScanInterpolate->getState());

  // Finally, issue a complete framebuffer re-initialization
  instance().createFrameBuffer();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VideoDialog::setDefaults()
{
  switch(myTab->getActiveTab())
  {
    case 0:  // General
    {
      myRenderer->setSelectedIndex(0);
      myTIAZoom->setSelected("3", "");
      myTIAPalette->setSelected("standard", "");
      myFrameTiming->setSelected("sleep", "");
      myTIAInterpolate->setSelected("nearest", "");
      myNAspectRatio->setValue(90);
      myNAspectRatioLabel->setLabel("90");
      myPAspectRatio->setValue(100);
      myPAspectRatioLabel->setLabel("100");
      myFrameRate->setValue(0);
      myFrameRateLabel->setLabel("Auto");

      myFullscreen->setState(false);
      myUseStretch->setState(true);
      myUseVSync->setState(true);
      myUIMessages->setState(true);
      myCenter->setState(false);
      myFastSCBios->setState(true);
      myUseThreads->setState(false);
      break;
    }

    case 1:  // TV effects
    {
      myTVMode->setSelected("0", "0");

      // TV phosphor mode
      myTVPhosphor->setSelected("byrom", "byrom");

      // TV phosphor blend
      myTVPhosLevel->setValue(50);
      myTVPhosLevelLabel->setLabel("50");

      // TV scanline intensity and interpolation
      myTVScanIntense->setValue(25);
      myTVScanIntenseLabel->setLabel("25");
      myTVScanInterpolate->setState(true);

      // Make sure that mutually-exclusive items are not enabled at the same time
      handleTVModeChange(NTSCFilter::PRESET_OFF);
      loadTVAdjustables(NTSCFilter::PRESET_CUSTOM);
      break;
    }
  }

  _dirty = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VideoDialog::handleTVModeChange(NTSCFilter::Preset preset)
{
  bool enable = preset == NTSCFilter::PRESET_CUSTOM;
  bool scanenable = preset != NTSCFilter::PRESET_OFF;

  myTVSharp->setEnabled(enable);
  myTVSharpLabel->setEnabled(enable);
  myTVHue->setEnabled(enable);
  myTVHueLabel->setEnabled(enable);
  myTVRes->setEnabled(enable);
  myTVResLabel->setEnabled(enable);
  myTVArtifacts->setEnabled(enable);
  myTVArtifactsLabel->setEnabled(enable);
  myTVFringe->setEnabled(enable);
  myTVFringeLabel->setEnabled(enable);
  myTVBleed->setEnabled(enable);
  myTVBleedLabel->setEnabled(enable);
  myTVBright->setEnabled(enable);
  myTVBrightLabel->setEnabled(enable);
  myTVContrast->setEnabled(enable);
  myTVContrastLabel->setEnabled(enable);
  myTVSatur->setEnabled(enable);
  myTVSaturLabel->setEnabled(enable);
  myTVGamma->setEnabled(enable);
  myTVGammaLabel->setEnabled(enable);
  myCloneComposite->setEnabled(enable);
  myCloneSvideo->setEnabled(enable);
  myCloneRGB->setEnabled(enable);
  myCloneBad->setEnabled(enable);
  myCloneCustom->setEnabled(enable);

  myTVScanLabel->setEnabled(scanenable);
  myTVScanIntense->setEnabled(scanenable);
  myTVScanIntenseLabel->setEnabled(scanenable);
  myTVScanInterpolate->setEnabled(scanenable);

  _dirty = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VideoDialog::loadTVAdjustables(NTSCFilter::Preset preset)
{
  NTSCFilter::Adjustable adj;
  instance().frameBuffer().tiaSurface().ntsc().getAdjustables(
      adj, NTSCFilter::Preset(preset));
  myTVSharp->setValue(adj.sharpness);
  myTVSharpLabel->setValue(adj.sharpness);
  myTVHue->setValue(adj.hue);
  myTVHueLabel->setValue(adj.hue);
  myTVRes->setValue(adj.resolution);
  myTVResLabel->setValue(adj.resolution);
  myTVArtifacts->setValue(adj.artifacts);
  myTVArtifactsLabel->setValue(adj.artifacts);
  myTVFringe->setValue(adj.fringing);
  myTVFringeLabel->setValue(adj.fringing);
  myTVBleed->setValue(adj.bleed);
  myTVBleedLabel->setValue(adj.bleed);
  myTVBright->setValue(adj.brightness);
  myTVBrightLabel->setValue(adj.brightness);
  myTVContrast->setValue(adj.contrast);
  myTVContrastLabel->setValue(adj.contrast);
  myTVSatur->setValue(adj.saturation);
  myTVSaturLabel->setValue(adj.saturation);
  myTVGamma->setValue(adj.gamma);
  myTVGammaLabel->setValue(adj.gamma);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VideoDialog::handleCommand(CommandSender* sender, int cmd,
                                int data, int id)
{
  switch(cmd)
  {
    case GuiObject::kOKCmd:
      saveConfig();
      close();
      break;

    case GuiObject::kDefaultsCmd:
      setDefaults();
      break;

    case kNAspectRatioChanged:
      myNAspectRatioLabel->setValue(myNAspectRatio->getValue());
      break;

    case kPAspectRatioChanged:
      myPAspectRatioLabel->setValue(myPAspectRatio->getValue());
      break;

    case kFrameRateChanged:
      if(myFrameRate->getValue() == 0)
        myFrameRateLabel->setLabel("Auto");
      else
        myFrameRateLabel->setValue(myFrameRate->getValue());
      break;

    case kTVModeChanged:
      handleTVModeChange(NTSCFilter::Preset(myTVMode->getSelectedTag().toInt()));
      break;
    case kTVSharpChanged:  myTVSharpLabel->setValue(myTVSharp->getValue());
      break;
    case kTVHueChanged:  myTVHueLabel->setValue(myTVHue->getValue());
      break;
    case kTVResChanged:  myTVResLabel->setValue(myTVRes->getValue());
      break;
    case kTVArtifactsChanged:  myTVArtifactsLabel->setValue(myTVArtifacts->getValue());
      break;
    case kTVFringeChanged:  myTVFringeLabel->setValue(myTVFringe->getValue());
      break;
    case kTVBleedChanged:  myTVBleedLabel->setValue(myTVBleed->getValue());
      break;
    case kTVBrightChanged:  myTVBrightLabel->setValue(myTVBright->getValue());
      break;
    case kTVContrastChanged:  myTVContrastLabel->setValue(myTVContrast->getValue());
      break;
    case kTVSaturChanged:  myTVSaturLabel->setValue(myTVSatur->getValue());
      break;
    case kTVGammaChanged:  myTVGammaLabel->setValue(myTVGamma->getValue());
      break;
    case kTVPhosLevelChanged:  myTVPhosLevelLabel->setValue(myTVPhosLevel->getValue());
      break;
    case kTVScanIntenseChanged:  myTVScanIntenseLabel->setValue(myTVScanIntense->getValue());
      break;
    case kCloneCompositeCmd: loadTVAdjustables(NTSCFilter::PRESET_COMPOSITE);
      break;
    case kCloneSvideoCmd: loadTVAdjustables(NTSCFilter::PRESET_SVIDEO);
      break;
    case kCloneRGBCmd: loadTVAdjustables(NTSCFilter::PRESET_RGB);
      break;
    case kCloneBadCmd: loadTVAdjustables(NTSCFilter::PRESET_BAD);
      break;
    case kCloneCustomCmd: loadTVAdjustables(NTSCFilter::PRESET_CUSTOM);
      break;

    default:
      Dialog::handleCommand(sender, cmd, data, 0);
      break;
  }
}
