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
// Copyright (c) 1995-2011 by Bradford W. Mott, Stephen Anthony
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
  // TODO ...

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
  bool gl = (instance().frameBuffer().type() == kGLBuffer);

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
  myGLStretchCheckbox->setState(instance().settings().getBool("gl_fsmax"));
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

  // Show UI messages
  instance().settings().setBool("uimessages", myUIMessagesCheckbox->getState());

  // Center window
  instance().settings().setBool("center", myCenterCheckbox->getState());

  // Fast loading of Supercharger BIOS
  instance().settings().setBool("fastscbios", myFastSCBiosCheckbox->getState());

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
  myFrameRateLabel->setLabel("Auto");

  myFullscreenPopup->setSelected("0", "");
  myColorLossCheckbox->setState(false);
  myGLStretchCheckbox->setState(false);
  myUseVSyncCheckbox->setState(true);
  myUIMessagesCheckbox->setState(true);
  myCenterCheckbox->setState(false);
  myFastSCBiosCheckbox->setState(false);

  // Make sure that mutually-exclusive items are not enabled at the same time
  handleFullscreenChange(true);
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

    default:
      Dialog::handleCommand(sender, cmd, data, 0);
      break;
  }
}
