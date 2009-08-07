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
// Copyright (c) 1995-2009 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
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
#include "Settings.hxx"
#include "StringList.hxx"
#include "Widget.hxx"
#include "TabWidget.hxx"
#include "FrameBufferGL.hxx"

#include "VideoDialog.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
VideoDialog::VideoDialog(OSystem* osystem, DialogContainer* parent,
                         const GUI::Font& font)
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
  _w = 52 * fontWidth + 10;
  _h = 15 * (lineHeight + 4) + 10;

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
  myRenderer = new EditTextWidget(myTab, font, xpos+lwidth, ypos,
                                  pwidth, fontHeight, "");
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
                                 lineHeight, items, "FS Res: ", lwidth);
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
  wid.push_back(myFrameRateSlider);
  myFrameRateLabel =
    new StaticTextWidget(myTab, font, xpos + myFrameRateSlider->getWidth() + 4,
                         ypos + 1, fontWidth * 3, fontHeight, "", kTextAlignLeft);
  myFrameRateLabel->setFlags(WIDGET_CLEARBG);

  // Add message concerning usage
  ypos += (lineHeight + 4) * 2;
  new StaticTextWidget(myTab, font, 10, ypos,
        font.getStringWidth("(*) Requires application restart"), fontHeight,
        "(*) Requires application restart", kTextAlignLeft);

  // Move over to the next column
  xpos += myNAspectRatioSlider->getWidth() + myNAspectRatioLabel->getWidth() + 10;
  ypos = 10;

  // Fullscreen
  items.clear();
  items.push_back("On", "1");
  items.push_back("Off", "0");
  items.push_back("Disabled", "-1");
  lwidth = font.getStringWidth("Fullscreen: ");
  myFullscreenPopup =
    new PopUpWidget(myTab, font, xpos, ypos, pwidth, lineHeight,
                    items, "Fullscreen: ", lwidth, kFullScrChanged);
  wid.push_back(myFullscreenPopup);
  ypos += lineHeight + 4;

  // PAL color-loss effect
  myColorLossCheckbox = new CheckboxWidget(myTab, font, xpos, ypos,
                                           "PAL color-loss");
  wid.push_back(myColorLossCheckbox);
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

  // Grab mouse (in windowed mode)
  myGrabmouseCheckbox = new CheckboxWidget(myTab, font, xpos, ypos,
                                           "Grab mouse");
  wid.push_back(myGrabmouseCheckbox);
  ypos += lineHeight + 4;

  // Center window (in windowed mode)
  myCenterCheckbox = new CheckboxWidget(myTab, font, xpos, ypos,
                                        "Center window (*)");
  wid.push_back(myCenterCheckbox);
  ypos += lineHeight + 4;

  // Skip progress load bars for SuperCharger ROMs
  // Doesn't really belong here, but I couldn't find a better place for it
  myFastSCBiosCheckbox = new CheckboxWidget(myTab, font, xpos, ypos,
                                            "Fast SC/AR BIOS");
  wid.push_back(myFastSCBiosCheckbox);
  ypos += lineHeight + 4;

  // Add items for tab 0
  addToFocusList(wid, tabID);

  //////////////////////////////////////////////////////////
  // 2) TV effects options
  wid.clear();
  tabID = myTab->addTab(" TV Effects ");
  xpos = ypos = 8;
  lwidth = font.getStringWidth("TV Color Texture: ");
  pwidth = font.getStringWidth("Staggered");

  // Use TV color texture effect
  items.clear();
  items.push_back("Off", "off");
  items.push_back("Normal", "normal");
  items.push_back("Staggered", "stag");
  myTexturePopup =
    new PopUpWidget(myTab, font, xpos, ypos, pwidth, lineHeight, items,
                    "TV Color Texture: ", lwidth);
  wid.push_back(myTexturePopup);
  ypos += lineHeight + 4;

  // Use color bleed effect
  items.clear();
  items.push_back("Off", "off");
  items.push_back("Low", "low");
  items.push_back("Medium", "medium");
  items.push_back("High", "high");
  myBleedPopup =
    new PopUpWidget(myTab, font, xpos, ypos, pwidth, lineHeight, items,
                    "TV Color Bleed: ", lwidth);
  wid.push_back(myBleedPopup);
  ypos += lineHeight + 4;

  // Use image noise effect
  items.clear();
  items.push_back("Off", "off");
  items.push_back("Low", "low");
  items.push_back("Medium", "medium");
  items.push_back("High", "high");
  myNoisePopup =
    new PopUpWidget(myTab, font, xpos, ypos, pwidth, lineHeight, items,
                    "TV Image Noise: ", lwidth);
  wid.push_back(myNoisePopup);
  ypos += lineHeight + 4;

  // Use phosphor burn-off effect
  ypos += 4;
  myPhosphorCheckbox =
    new CheckboxWidget(myTab, font, xpos, ypos, "TV Phosphor Burn-off");
  wid.push_back(myPhosphorCheckbox);
  ypos += lineHeight + 4;

  // OpenGL information
  // Add message concerning GLSL requirement
  ypos += lineHeight + 4;
  lwidth = font.getStringWidth("(*) TV effects require OpenGL 2.0+ & GLSL");
  new StaticTextWidget(myTab, font, 10, ypos, lwidth, fontHeight,
                       "(*) TV effects require OpenGL 2.0+ & GLSL",
                       kTextAlignLeft);
  ypos += lineHeight + 4;
  new StaticTextWidget(myTab, font, 10+font.getStringWidth("(*) "), ypos,
                       lwidth, fontHeight, "\'gl_texrect\' must be disabled",
                       kTextAlignLeft);
  ypos += lineHeight + 10;

  myGLVersionInfo =
    new StaticTextWidget(myTab, font, 10+font.getStringWidth("(*) "), ypos,
                         lwidth, fontHeight, "", kTextAlignLeft);
  ypos += lineHeight + 4;
  myGLTexRectInfo =
    new StaticTextWidget(myTab, font, 10+font.getStringWidth("(*) "), ypos,
                         lwidth, fontHeight, "", kTextAlignLeft);

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

  myTexturePopup->clearFlags(WIDGET_ENABLED);
  myBleedPopup->clearFlags(WIDGET_ENABLED);
  myNoisePopup->clearFlags(WIDGET_ENABLED);
  myPhosphorCheckbox->clearFlags(WIDGET_ENABLED);
#endif
#ifndef WINDOWED_SUPPORT
  myFullscreenCheckbox->clearFlags(WIDGET_ENABLED);
  myGrabmouseCheckbox->clearFlags(WIDGET_ENABLED);
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
  bool gl = (instance().frameBuffer().type() == kGLBuffer);

  // Renderer settings
  myRenderer->setEditString(gl ? "OpenGL" : "Software");
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
  myGLFilterPopup->setSelected(
    instance().settings().getString("gl_filter"), "nearest");
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

  // Framerate (0 or -1 means disabled)
  int rate = instance().settings().getInt("framerate");
  myFrameRateSlider->setValue(rate < 0 ? 0 : rate);
  myFrameRateLabel->setLabel(rate < 0 ? "0" :
    instance().settings().getString("framerate"));

  // Fullscreen
  const string& fullscreen = instance().settings().getString("fullscreen");
  myFullscreenPopup->setSelected(fullscreen, "0");
  handleFullscreenChange(fullscreen == "1");

  // PAL color-loss effect
  myColorLossCheckbox->setState(instance().settings().getBool("colorloss"));

  // GL stretch setting (item is enabled/disabled in ::handleFullscreenChange)
  myGLStretchCheckbox->setState(instance().settings().getBool("gl_fsmax"));

  // Use sync to vertical blank (GL mode only)
  myUseVSyncCheckbox->setState(instance().settings().getBool("gl_vsync"));
  myUseVSyncCheckbox->setEnabled(gl);

  // Grab mouse
  myGrabmouseCheckbox->setState(instance().settings().getBool("grabmouse"));

  // Center window
  myCenterCheckbox->setState(instance().settings().getBool("center"));

  // Fast loading of Supercharger BIOS
  myFastSCBiosCheckbox->setState(instance().settings().getBool("fastscbios"));

#ifdef DISPLAY_OPENGL
  //////////////////////////////////////////////////////////////////////
  // TV effects are only enabled in OpenGL mode, and only if GLSL is
  // available; for now, 'gl_texrect' must also be disabled
  bool tv = gl && FrameBufferGL::isGLSLAvailable() &&
            !instance().settings().getBool("gl_texrect");
  //////////////////////////////////////////////////////////////////////

  // TV color texture effect
  myTexturePopup->setSelected(instance().settings().getString("tv_tex"), "off");
  myTexturePopup->setEnabled(tv);

  // TV color bleed effect
  myBleedPopup->setSelected(instance().settings().getString("tv_bleed"), "off");
  myBleedPopup->setEnabled(tv);

  // TV random noise effect
  myNoisePopup->setSelected(instance().settings().getString("tv_noise"), "off");
  myNoisePopup->setEnabled(tv);

  // TV phosphor burn-off effect
  myPhosphorCheckbox->setState(instance().settings().getBool("tv_phos"));
  myPhosphorCheckbox->setEnabled(tv);

  char buf[30];
  if(gl) sprintf(buf, "OpenGL version detected: %3.1f", FrameBufferGL::glVersion());
  else   sprintf(buf, "OpenGL version detected: None");
  myGLVersionInfo->setLabel(buf);
  sprintf(buf, "OpenGL texrect enabled: %s",
          instance().settings().getBool("gl_texrect") ? "Yes" : "No");
  myGLTexRectInfo->setLabel(buf);
#else
  myGLVersionInfo->setLabel("OpenGL mode not supported");
#endif

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
  instance().settings().setString("gl_filter", myGLFilterPopup->getSelectedTag());

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

  // GL stretch setting
  instance().settings().setBool("gl_fsmax", myGLStretchCheckbox->getState());

  // Use sync to vertical blank (GL mode only)
  instance().settings().setBool("gl_vsync", myUseVSyncCheckbox->getState());

  // Grab mouse
  instance().settings().setBool("grabmouse", myGrabmouseCheckbox->getState());
  instance().frameBuffer().setCursorState();

  // Center window
  instance().settings().setBool("center", myCenterCheckbox->getState());

  // Fast loading of Supercharger BIOS
  instance().settings().setBool("fastscbios", myFastSCBiosCheckbox->getState());

  // TV color texture effect
  instance().settings().setString("tv_tex", myTexturePopup->getSelectedTag());

  // TV color bleed effect
  instance().settings().setString("tv_bleed", myBleedPopup->getSelectedTag());

  // TV image noise effect
  instance().settings().setString("tv_noise", myNoisePopup->getSelectedTag());

  // TV phosphor burn-off effect
  instance().settings().setBool("tv_phos", myPhosphorCheckbox->getState());

  // Finally, issue a complete framebuffer re-initialization
  instance().createFrameBuffer();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VideoDialog::setDefaults()
{
  myRendererPopup->setSelected("soft", "");
  myTIAFilterPopup->setSelected(
    instance().desktopWidth() < 640 ? "zoom1x" : "zoom2x", "");
  myTIAPalettePopup->setSelected("standard", "");
  myFSResPopup->setSelected("auto", "");
  myFrameTimingPopup->setSelected("sleep", "");
  myGLFilterPopup->setSelected("nearest", "");
  myNAspectRatioSlider->setValue(100);
  myNAspectRatioLabel->setLabel("100");
  myPAspectRatioSlider->setValue(100);
  myPAspectRatioLabel->setLabel("100");
  myFrameRateSlider->setValue(0);
  myFrameRateLabel->setLabel("0");

  myFullscreenPopup->setSelected("0", "");
  myColorLossCheckbox->setState(false);
  myGLStretchCheckbox->setState(false);
  myUseVSyncCheckbox->setState(true);
  myGrabmouseCheckbox->setState(false);
  myCenterCheckbox->setState(true);
  myFastSCBiosCheckbox->setState(false);

  myTexturePopup->setSelected("off", "");
  myBleedPopup->setSelected("off", "");
  myNoisePopup->setSelected("off", "");
  myPhosphorCheckbox->setState(false);

  // Make sure that mutually-exclusive items are not enabled at the same time
  handleFullscreenChange(false);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VideoDialog::handleFullscreenChange(bool enable)
{
#ifdef WINDOWED_SUPPORT
  myFSResPopup->setEnabled(enable);

  // GL stretch is only enabled in OpenGL mode
  myGLStretchCheckbox->setEnabled(
    enable && instance().frameBuffer().type() == kGLBuffer);

  _dirty = true;
#endif
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
      myFrameRateLabel->setValue(myFrameRateSlider->getValue());
      break;

    case kFullScrChanged:
      handleFullscreenChange(myFullscreenPopup->getSelectedTag() == "1");
      break;

    default:
      Dialog::handleCommand(sender, cmd, data, 0);
      break;
  }
}
