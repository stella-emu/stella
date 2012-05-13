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

#include <sstream>

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
#include "StringList.hxx"
#include "Widget.hxx"
#include "TabWidget.hxx"
#include "FrameBufferGL.hxx"
#include "NTSCFilter.hxx"

#include "VideoDialog.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
VideoDialog::VideoDialog(OSystem* osystem, DialogContainer* parent,
                         const GUI::Font& font, int max_w, int max_h)
  : Dialog(osystem, parent, 0, 0, 0, 0)
{
  const int lineHeight   = font.getLineHeight(),
            fontWidth    = font.getMaxCharWidth(),
            fontHeight   = font.getFontHeight(),
            buttonWidth  = font.getStringWidth("Defaults") + 20,
            buttonHeight = font.getLineHeight() + 4;
  int xpos, ypos, tabID;
  int lwidth = font.getStringWidth("GL Aspect (P): "),
      pwidth = font.getStringWidth("1920x1200"),
      fwidth = font.getStringWidth("Renderer: ");
  WidgetArray wid;
  StringMap items;

  // Set real dimensions
  _w = BSPF_min(52 * fontWidth + 10, max_w);
  _h = BSPF_min(14 * (lineHeight + 4) + 10, max_h);

  // The tab widget
  xpos = ypos = 5;
  myTab = new TabWidget(this, font, xpos, ypos, _w - 2*xpos, _h - buttonHeight - 20);
  addTabWidget(myTab);
  addFocusWidget(myTab);

  //////////////////////////////////////////////////////////
  // 1) General options
  wid.clear();
  tabID = myTab->addTab(" General ");

  // Video renderer
  new StaticTextWidget(myTab, font, xpos + (lwidth-fwidth), ypos, fwidth,
                       fontHeight, "Renderer:", kTextAlignLeft);
  myRenderer = new StaticTextWidget(myTab, font, xpos+lwidth, ypos,
                                  fwidth, fontHeight, "", kTextAlignLeft);
  ypos += lineHeight + 4;

  items.clear();
  items.push_back("Software", "soft");
#ifdef DISPLAY_OPENGL
  items.push_back("OpenGL", "gl");
#endif
  myRendererPopup = new PopUpWidget(myTab, font, xpos, ypos, pwidth, lineHeight,
                                    items, "(*) ", lwidth);
  wid.push_back(myRendererPopup);
  ypos += lineHeight + 4;

  // TIA filters (will be dynamically filled later)
  items.clear();
  myTIAFilterPopup = new PopUpWidget(myTab, font, xpos, ypos, pwidth,
                                     lineHeight, items, "TIA Filter: ", lwidth);
  wid.push_back(myTIAFilterPopup);
  ypos += lineHeight + 4;

  // TIA Palette
  items.clear();
  items.push_back("Standard", "standard");
  items.push_back("Z26", "z26");
  items.push_back("User", "user");
  myTIAPalettePopup = new PopUpWidget(myTab, font, xpos, ypos, pwidth,
                                      lineHeight, items, "TIA Palette: ", lwidth);
  wid.push_back(myTIAPalettePopup);
  ypos += lineHeight + 4;

  // Fullscreen resolution
  items.clear();
  items.push_back("Auto", "auto");
  for(uInt32 i = 0; i < instance().supportedResolutions().size(); ++i)
    items.push_back(instance().supportedResolutions()[i].name,
                    instance().supportedResolutions()[i].name);
  myFSResPopup = new PopUpWidget(myTab, font, xpos, ypos, pwidth,
                                 lineHeight, items, "Fullscrn Res: ", lwidth);
  wid.push_back(myFSResPopup);
  ypos += lineHeight + 4;

  // Timing to use between frames
  items.clear();
  items.push_back("Sleep", "sleep");
  items.push_back("Busy-wait", "busy");
  myFrameTimingPopup = new PopUpWidget(myTab, font, xpos, ypos, pwidth, lineHeight,
                                       items, "Timing (*): ", lwidth);
  wid.push_back(myFrameTimingPopup);
  ypos += lineHeight + 4;

  // GL Video filter
  items.clear();
  items.push_back("Linear", "linear");
  items.push_back("Nearest", "nearest");
  myGLFilterPopup = new PopUpWidget(myTab, font, xpos, ypos, pwidth, lineHeight,
                                  items, "GL Filter: ", lwidth);
  wid.push_back(myGLFilterPopup);
  ypos += lineHeight + 4;

  // GL aspect ratio (NTSC mode)
  myNAspectRatioSlider =
    new SliderWidget(myTab, font, xpos, ypos, pwidth, lineHeight,
                     "GL Aspect (N): ", lwidth, kNAspectRatioChanged);
  myNAspectRatioSlider->setMinValue(80); myNAspectRatioSlider->setMaxValue(120);
  wid.push_back(myNAspectRatioSlider);
  myNAspectRatioLabel =
    new StaticTextWidget(myTab, font, xpos + myNAspectRatioSlider->getWidth() + 4,
                         ypos + 1, fontWidth * 3, fontHeight, "", kTextAlignLeft);
  myNAspectRatioLabel->setFlags(WIDGET_CLEARBG);
  ypos += lineHeight + 4;

  // GL aspect ratio (PAL mode)
  myPAspectRatioSlider =
    new SliderWidget(myTab, font, xpos, ypos, pwidth, lineHeight,
                     "GL Aspect (P): ", lwidth, kPAspectRatioChanged);
  myPAspectRatioSlider->setMinValue(80); myPAspectRatioSlider->setMaxValue(120);
  wid.push_back(myPAspectRatioSlider);
  myPAspectRatioLabel =
    new StaticTextWidget(myTab, font, xpos + myPAspectRatioSlider->getWidth() + 4,
                         ypos + 1, fontWidth * 3, fontHeight, "", kTextAlignLeft);
  myPAspectRatioLabel->setFlags(WIDGET_CLEARBG);
  ypos += lineHeight + 4;

  // Framerate
  myFrameRateSlider =
    new SliderWidget(myTab, font, xpos, ypos, pwidth, lineHeight,
                     "Framerate: ", lwidth, kFrameRateChanged);
  myFrameRateSlider->setMinValue(0); myFrameRateSlider->setMaxValue(300);
  myFrameRateSlider->setStepValue(10);
  wid.push_back(myFrameRateSlider);
  myFrameRateLabel =
    new StaticTextWidget(myTab, font, xpos + myFrameRateSlider->getWidth() + 4,
                         ypos + 1, fontWidth * 4, fontHeight, "", kTextAlignLeft);
  myFrameRateLabel->setFlags(WIDGET_CLEARBG);

  // Add message concerning usage
  const GUI::Font& infofont = instance().infoFont();
  ypos = myTab->getHeight() - 5 - fontHeight - infofont.getFontHeight()- 10;
  new StaticTextWidget(myTab, infofont, 10, ypos,
        font.getStringWidth("(*) Requires application restart"), fontHeight,
        "(*) Requires application restart", kTextAlignLeft);

  // Move over to the next column
  xpos += myNAspectRatioSlider->getWidth() + myNAspectRatioLabel->getWidth() + 14;
  ypos = 10;

  // Fullscreen
  items.clear();
  items.push_back("On", "1");
  items.push_back("Off", "0");
  items.push_back("Never", "-1");
  lwidth = font.getStringWidth("Fullscreen: ");
  pwidth = font.getStringWidth("Never"),
  myFullscreenPopup =
    new PopUpWidget(myTab, font, xpos, ypos, pwidth, lineHeight,
                    items, "Fullscreen: ", lwidth, kFullScrChanged);
  wid.push_back(myFullscreenPopup);
  ypos += lineHeight + 4;

  // GL FS stretch
  myGLStretchCheckbox = new CheckboxWidget(myTab, font, xpos, ypos,
                                           "GL FS Stretch");
  wid.push_back(myGLStretchCheckbox);
  ypos += lineHeight + 4;

  // Use sync to vblank in OpenGL
  myUseVSyncCheckbox = new CheckboxWidget(myTab, font, xpos, ypos,
                                          "GL VSync");
  wid.push_back(myUseVSyncCheckbox);
  ypos += lineHeight + 4;

  ypos += lineHeight;

  // PAL color-loss effect
  myColorLossCheckbox = new CheckboxWidget(myTab, font, xpos, ypos,
                                           "PAL color-loss");
  wid.push_back(myColorLossCheckbox);
  ypos += lineHeight + 4;

  // Skip progress load bars for SuperCharger ROMs
  // Doesn't really belong here, but I couldn't find a better place for it
  myFastSCBiosCheckbox = new CheckboxWidget(myTab, font, xpos, ypos,
                                            "Fast SC/AR BIOS");
  wid.push_back(myFastSCBiosCheckbox);
  ypos += lineHeight + 4;

  // Show UI messages onscreen
  myUIMessagesCheckbox = new CheckboxWidget(myTab, font, xpos, ypos,
                                            "Show UI messages");
  wid.push_back(myUIMessagesCheckbox);
  ypos += lineHeight + 4;

  // Center window (in windowed mode)
  myCenterCheckbox = new CheckboxWidget(myTab, font, xpos, ypos,
                                        "Center window");
  wid.push_back(myCenterCheckbox);
  ypos += lineHeight + 4;

  // Add items for tab 0
  addToFocusList(wid, tabID);

  //////////////////////////////////////////////////////////
  // 2) TV effects options
  wid.clear();
  tabID = myTab->addTab(" TV Effects ");
  xpos = ypos = 8;

  // TV Mode
  items.clear();
  items.push_back("Disabled", BSPF_toString(NTSCFilter::PRESET_OFF));
  items.push_back("Composite", BSPF_toString(NTSCFilter::PRESET_COMPOSITE));
  items.push_back("S-Video", BSPF_toString(NTSCFilter::PRESET_SVIDEO));
  items.push_back("RGB", BSPF_toString(NTSCFilter::PRESET_RGB));
  items.push_back("Bad adjust", BSPF_toString(NTSCFilter::PRESET_BAD));
  items.push_back("Custom", BSPF_toString(NTSCFilter::PRESET_CUSTOM));
  lwidth = font.getStringWidth("TV Mode: ");
  pwidth = font.getStringWidth("Bad adjust"),
  myTVMode =
    new PopUpWidget(myTab, font, xpos, ypos, pwidth, lineHeight,
                    items, "TV Mode: ", lwidth, kTVModeChanged);
  wid.push_back(myTVMode);
  ypos += lineHeight + 4;

  // Custom adjustables (using macro voodoo)
  xpos += 8; ypos += 4;
  int orig_ypos = ypos;
  pwidth = lwidth;
  lwidth = font.getStringWidth("Saturation: ");

#define CREATE_CUSTOM_SLIDERS(obj, desc)                                 \
  myTV ## obj =                                                          \
    new SliderWidget(myTab, font, xpos, ypos, pwidth, lineHeight,        \
                         desc, lwidth, kTV ## obj ##Changed);            \
  myTV ## obj->setMinValue(0); myTV ## obj->setMaxValue(100);            \
  wid.push_back(myTV ## obj);                                            \
  myTV ## obj ## Label =                                                 \
    new StaticTextWidget(myTab, font, xpos+myTV ## obj->getWidth()+4,    \
                    ypos+1, fontWidth*3, fontHeight, "", kTextAlignLeft);\
  myTV ## obj->setFlags(WIDGET_CLEARBG);                                 \
  ypos += lineHeight + 4

  CREATE_CUSTOM_SLIDERS(Contrast, "Contrast: ");
  CREATE_CUSTOM_SLIDERS(Bright, "Brightness: ");
  CREATE_CUSTOM_SLIDERS(Hue, "Hue: ");
  CREATE_CUSTOM_SLIDERS(Satur, "Saturation: ");
  CREATE_CUSTOM_SLIDERS(Gamma, "Gamma: ");
  CREATE_CUSTOM_SLIDERS(Sharp, "Sharpness: ");
  CREATE_CUSTOM_SLIDERS(Res, "Resolution: ");
  CREATE_CUSTOM_SLIDERS(Artifacts, "Artifacts: ");
  CREATE_CUSTOM_SLIDERS(Fringe, "Fringing: ");
  CREATE_CUSTOM_SLIDERS(Bleed, "Bleeding: ");

  xpos += myTVContrast->getWidth() + myTVContrastLabel->getWidth() + 20;
  ypos = orig_ypos;

  // Scanline intensity and interpolation
  myTVScanLabel = 
    new StaticTextWidget(myTab, font, xpos, ypos, font.getStringWidth("Scanline settings:"),
                         fontHeight, "Scanline settings:", kTextAlignLeft);
  ypos += lineHeight;

  xpos += 20;
  lwidth = font.getStringWidth("Intensity: ");
  pwidth = font.getMaxCharWidth() * 6;
  CREATE_CUSTOM_SLIDERS(ScanIntense, "Intensity: ");

  myTVScanInterpolate = new CheckboxWidget(myTab, font, xpos, ypos,
                                           "Interpolation");
  wid.push_back(myTVScanInterpolate);
  ypos += lineHeight + 4;

  // Adjustable presets
  xpos -= 20;
  int cloneWidth = font.getStringWidth("Clone Bad Adjust") + 20;
#define CREATE_CLONE_BUTTON(obj, desc)                                 \
  myClone ## obj =                                                     \
    new ButtonWidget(myTab, font, xpos, ypos, cloneWidth, buttonHeight,\
                     desc, kClone ## obj ##Cmd);                       \
  wid.push_back(myClone ## obj);                                       \
  ypos += lineHeight + 10

  ypos += lineHeight;
  CREATE_CLONE_BUTTON(Composite, "Clone Composite");
  CREATE_CLONE_BUTTON(Svideo, "Clone S-Video");
  CREATE_CLONE_BUTTON(RGB, "Clone RGB");
  CREATE_CLONE_BUTTON(Bad, "Clone Bad Adjust");
  CREATE_CLONE_BUTTON(Custom, "Revert");

  // Add items for tab 2
  addToFocusList(wid, tabID);

  // Activate the first tab
  myTab->setActiveTab(0);

  // Add Defaults, OK and Cancel buttons
  wid.clear();
  ButtonWidget* b;
  b = new ButtonWidget(this, font, 10, _h - buttonHeight - 10,
                       buttonWidth, buttonHeight, "Defaults", kDefaultsCmd);
  wid.push_back(b);
  addOKCancelBGroup(wid, font);
  addBGroupToFocusList(wid);

  // Disable certain functions when we know they aren't present
#ifndef DISPLAY_OPENGL
  myGLFilterPopup->clearFlags(WIDGET_ENABLED);
  myNAspectRatioSlider->clearFlags(WIDGET_ENABLED);
  myNAspectRatioLabel->clearFlags(WIDGET_ENABLED);
  myPAspectRatioSlider->clearFlags(WIDGET_ENABLED);
  myPAspectRatioLabel->clearFlags(WIDGET_ENABLED);
  myGLStretchCheckbox->clearFlags(WIDGET_ENABLED);
  myUseVSyncCheckbox->clearFlags(WIDGET_ENABLED);

  myTVMode->clearFlags(WIDGET_ENABLED);
  myTVSharp->clearFlags(WIDGET_ENABLED);
  myTVSharpLabel->clearFlags(WIDGET_ENABLED);
  myTVHue->clearFlags(WIDGET_ENABLED);
  myTVHueLabel->clearFlags(WIDGET_ENABLED);
  myTVRes->clearFlags(WIDGET_ENABLED);
  myTVResLabel->clearFlags(WIDGET_ENABLED);
  myTVArtifacts->clearFlags(WIDGET_ENABLED);
  myTVArtifactsLabel->clearFlags(WIDGET_ENABLED);
  myTVFringe->clearFlags(WIDGET_ENABLED);
  myTVFringeLabel->clearFlags(WIDGET_ENABLED);
  myTVBleed->clearFlags(WIDGET_ENABLED);
  myTVBleedLabel->clearFlags(WIDGET_ENABLED);
  myTVBright->clearFlags(WIDGET_ENABLED);
  myTVBrightLabel->clearFlags(WIDGET_ENABLED);
  myTVContrast->clearFlags(WIDGET_ENABLED);
  myTVContrastLabel->clearFlags(WIDGET_ENABLED);
  myTVSatur->clearFlags(WIDGET_ENABLED);
  myTVSaturLabel->clearFlags(WIDGET_ENABLED);
  myTVGamma->clearFlags(WIDGET_ENABLED);
  myTVGammaLabel->clearFlags(WIDGET_ENABLED);

  myTVScanLabel->clearFlags(WIDGET_ENABLED);
  myTVScanIntense->clearFlags(WIDGET_ENABLED);
  myTVScanIntenseLabel->clearFlags(WIDGET_ENABLED);
  myTVScanInterpolate->clearFlags(WIDGET_ENABLED);

  myCloneComposite->clearFlags(WIDGET_ENABLED);
  myCloneSvideo->clearFlags(WIDGET_ENABLED);
  myCloneRGB->clearFlags(WIDGET_ENABLED);
  myCloneBad->clearFlags(WIDGET_ENABLED);
  myCloneCustom->clearFlags(WIDGET_ENABLED);
#endif
#ifndef WINDOWED_SUPPORT
  myFullscreenCheckbox->clearFlags(WIDGET_ENABLED);
  myCenterCheckbox->clearFlags(WIDGET_ENABLED);
#endif
#if !(defined(BSPF_WIN32) || (defined(BSPF_UNIX) && defined(HAVE_X11)))
  myCenterCheckbox->clearFlags(WIDGET_ENABLED);
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
VideoDialog::~VideoDialog()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VideoDialog::loadConfig()
{
  bool gl = (instance().frameBuffer().type() == kDoubleBuffer);

  // Renderer settings
  myRenderer->setLabel(gl ? "OpenGL" : "Software");
  myRendererPopup->setSelected(
    instance().settings().getString("video"), "soft");

  // TIA Filter
  // These are dynamically loaded, since they depend on the size of
  // the desktop and which renderer we're using
  const StringMap& items =
    instance().frameBuffer().supportedTIAFilters(gl ? "gl" : "soft");
  myTIAFilterPopup->addItems(items);
  myTIAFilterPopup->setSelected(instance().settings().getString("tia_filter"),
    instance().desktopWidth() < 640 ? "zoom1x" : "zoom2x");

  // TIA Palette
  myTIAPalettePopup->setSelected(
    instance().settings().getString("palette"), "standard");

  // Fullscreen resolution
  myFSResPopup->setSelected(
    instance().settings().getString("fullres"), "auto");

  // Wait between frames
  myFrameTimingPopup->setSelected(
    instance().settings().getString("timing"), "sleep");

  // GL Filter setting
  const string& gl_inter = instance().settings().getBool("gl_inter") ?
                           "linear" : "nearest";
  myGLFilterPopup->setSelected(gl_inter, "nearest");
  myGLFilterPopup->setEnabled(gl);

  // GL aspect ratio setting (NTSC and PAL)
  myNAspectRatioSlider->setValue(instance().settings().getInt("gl_aspectn"));
  myNAspectRatioSlider->setEnabled(gl);
  myNAspectRatioLabel->setLabel(instance().settings().getString("gl_aspectn"));
  myNAspectRatioLabel->setEnabled(gl);
  myPAspectRatioSlider->setValue(instance().settings().getInt("gl_aspectp"));
  myPAspectRatioSlider->setEnabled(gl);
  myPAspectRatioLabel->setLabel(instance().settings().getString("gl_aspectp"));
  myPAspectRatioLabel->setEnabled(gl);

  // Framerate (0 or -1 means automatic framerate calculation)
  int rate = instance().settings().getInt("framerate");
  myFrameRateSlider->setValue(rate < 0 ? 0 : rate);
  myFrameRateLabel->setLabel(rate <= 0 ? "Auto" :
    instance().settings().getString("framerate"));

  // Fullscreen
  const string& fullscreen = instance().settings().getString("fullscreen");
  myFullscreenPopup->setSelected(fullscreen, "0");
  handleFullscreenChange(fullscreen == "0" || fullscreen == "1");

  // PAL color-loss effect
  myColorLossCheckbox->setState(instance().settings().getBool("colorloss"));

  // GL stretch setting (GL mode only)
  myGLStretchCheckbox->setState(instance().settings().getBool("gl_fsscale"));
  myGLStretchCheckbox->setEnabled(gl);

  // Use sync to vertical blank (GL mode only)
  myUseVSyncCheckbox->setState(instance().settings().getBool("gl_vsync"));
  myUseVSyncCheckbox->setEnabled(gl);

  // Show UI messages
  myUIMessagesCheckbox->setState(instance().settings().getBool("uimessages"));

  // Center window
  myCenterCheckbox->setState(instance().settings().getBool("center"));

  // Fast loading of Supercharger BIOS
  myFastSCBiosCheckbox->setState(instance().settings().getBool("fastscbios"));

  // TV Mode
  myTVMode->setSelected(
    instance().settings().getString("tv_filter"), "0");
  int preset = instance().settings().getInt("tv_filter");
  handleTVModeChange((NTSCFilter::Preset)preset);

  // TV Custom adjustables
  loadTVAdjustables(NTSCFilter::PRESET_CUSTOM);

  // TV scanline intensity and interpolation
  myTVScanIntense->setValue(instance().settings().getInt("tv_scanlines"));
  myTVScanIntenseLabel->setLabel(instance().settings().getString("tv_scanlines"));
  myTVScanInterpolate->setState(instance().settings().getBool("tv_scaninter"));

  myTab->loadConfig();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VideoDialog::saveConfig()
{
  // Renderer setting
  instance().settings().setString("video", myRendererPopup->getSelectedTag());

  // TIA Filter
  instance().settings().setString("tia_filter", myTIAFilterPopup->getSelectedTag());

  // TIA Palette
  instance().settings().setString("palette", myTIAPalettePopup->getSelectedTag());

  // Fullscreen resolution
  instance().settings().setString("fullres", myFSResPopup->getSelectedTag());

  // Wait between frames
  instance().settings().setString("timing", myFrameTimingPopup->getSelectedTag());

  // GL Filter setting
  instance().settings().setBool("gl_inter",
    myGLFilterPopup->getSelectedTag() == "linear" ? true : false);

  // GL aspect ratio setting (NTSC and PAL)
  instance().settings().setString("gl_aspectn", myNAspectRatioLabel->getLabel());
  instance().settings().setString("gl_aspectp", myPAspectRatioLabel->getLabel());

  // Framerate
  int i = myFrameRateSlider->getValue();
  instance().settings().setInt("framerate", i);
  if(&instance().console())
  {
    // Make sure auto-frame calculation is only enabled when necessary
    instance().console().tia().enableAutoFrame(i <= 0);
    instance().console().setFramerate(float(i));
  }

  // Fullscreen
  instance().settings().setString("fullscreen", myFullscreenPopup->getSelectedTag());

  // PAL color-loss effect
  instance().settings().setBool("colorloss", myColorLossCheckbox->getState());
  if(&instance().console())
    instance().console().toggleColorLoss(myColorLossCheckbox->getState());

  // GL stretch setting
  instance().settings().setBool("gl_fsscale", myGLStretchCheckbox->getState());

  // Use sync to vertical blank (GL mode only)
  instance().settings().setBool("gl_vsync", myUseVSyncCheckbox->getState());

  // Show UI messages
  instance().settings().setBool("uimessages", myUIMessagesCheckbox->getState());

  // Center window
  instance().settings().setBool("center", myCenterCheckbox->getState());

  // Fast loading of Supercharger BIOS
  instance().settings().setBool("fastscbios", myFastSCBiosCheckbox->getState());

  // TV Mode
  instance().settings().setString("tv_filter", myTVMode->getSelectedTag());

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
  instance().frameBuffer().ntsc().setCustomAdjustables(adj);

  // TV scanline intensity and interpolation
  instance().settings().setString("tv_scanlines", myTVScanIntenseLabel->getLabel());
  instance().settings().setBool("tv_scaninter", myTVScanInterpolate->getState());

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
      myRendererPopup->setSelected("soft", "");
      myTIAFilterPopup->setSelected(
        instance().desktopWidth() < 640 ? "zoom1x" : "zoom2x", "");
      myTIAPalettePopup->setSelected("standard", "");
      myFSResPopup->setSelected("auto", "");
      myFrameTimingPopup->setSelected("sleep", "");
      myGLFilterPopup->setSelected("nearest", "");
      myNAspectRatioSlider->setValue(90);
      myNAspectRatioLabel->setLabel("90");
      myPAspectRatioSlider->setValue(100);
      myPAspectRatioLabel->setLabel("100");
      myFrameRateSlider->setValue(0);
      myFrameRateLabel->setLabel("Auto");

      myFullscreenPopup->setSelected("0", "");
      myColorLossCheckbox->setState(true);
      myGLStretchCheckbox->setState(true);
      myUseVSyncCheckbox->setState(true);
      myUIMessagesCheckbox->setState(true);
      myCenterCheckbox->setState(false);
      myFastSCBiosCheckbox->setState(false);
      break;
    }

    case 1:  // TV effects
    {
      myTVMode->setSelected("0", "0");

      // TV scanline intensity and interpolation
      myTVScanIntense->setValue(40);
      myTVScanIntenseLabel->setLabel("40");
      myTVScanInterpolate->setState(true);

      // Make sure that mutually-exclusive items are not enabled at the same time
      handleFullscreenChange(true);
      handleTVModeChange(NTSCFilter::PRESET_OFF);
      loadTVAdjustables(NTSCFilter::PRESET_CUSTOM);
      break;
    }
  }

  _dirty = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VideoDialog::handleFullscreenChange(bool enable)
{
#ifdef WINDOWED_SUPPORT
  myFSResPopup->setEnabled(enable);
  _dirty = true;
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VideoDialog::handleTVModeChange(NTSCFilter::Preset preset)
{
  bool enable = true, scanenable = true;
  if(!instance().frameBuffer().type() == kDoubleBuffer)
  {
    enable = scanenable = false;
    myTVMode->setEnabled(enable);
  }
  else
  {
    enable = preset == NTSCFilter::PRESET_CUSTOM;
    scanenable = preset != NTSCFilter::PRESET_OFF;
  }

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
  instance().frameBuffer().ntsc().getAdjustables(adj, (NTSCFilter::Preset)preset);
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
    case kOKCmd:
      saveConfig();
      close();
      break;

    case kDefaultsCmd:
      setDefaults();
      break;

    case kNAspectRatioChanged:
      myNAspectRatioLabel->setValue(myNAspectRatioSlider->getValue());
      break;

    case kPAspectRatioChanged:
      myPAspectRatioLabel->setValue(myPAspectRatioSlider->getValue());
      break;

    case kFrameRateChanged:
      if(myFrameRateSlider->getValue() == 0)
        myFrameRateLabel->setLabel("Auto");
      else
        myFrameRateLabel->setValue(myFrameRateSlider->getValue());
      break;

    case kFullScrChanged:
      handleFullscreenChange(myFullscreenPopup->getSelectedTag() != "-1");
      break;

    case kTVModeChanged:
      handleTVModeChange((NTSCFilter::Preset)atoi(myTVMode->getSelectedTag().c_str()));

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
