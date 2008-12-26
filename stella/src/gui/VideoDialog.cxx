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
// Copyright (c) 1995-2008 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: VideoDialog.cxx,v 1.56 2008-12-26 20:05:17 stephena Exp $
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
#include "PopUpWidget.hxx"
#include "Console.hxx"
#include "Settings.hxx"
#include "StringList.hxx"
#include "Widget.hxx"

#include "VideoDialog.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
VideoDialog::VideoDialog(OSystem* osystem, DialogContainer* parent,
                         const GUI::Font& font, int x, int y, int w, int h)
  : Dialog(osystem, parent, x, y, w, h)
{
  const int lineHeight   = font.getLineHeight(),
            fontWidth    = font.getMaxCharWidth(),
            fontHeight   = font.getFontHeight(),
            buttonWidth  = font.getStringWidth("Defaults") + 20,
            buttonHeight = font.getLineHeight() + 4;
  int xpos, ypos;
  int lwidth = font.getStringWidth("Dirty Rects: "),
      pwidth = font.getStringWidth("1920x1200");
  WidgetArray wid;
  StringMap items;

  xpos = 5;  ypos = 10;

  // Video renderer
  items.clear();
  items.push_back("Software", "soft");
#ifdef DISPLAY_OPENGL
  items.push_back("OpenGL", "gl");
#endif
  myRendererPopup = new PopUpWidget(this, font, xpos, ypos, pwidth, lineHeight,
                                    items, "Renderer: ", lwidth,
                                    kRendererChanged);
  wid.push_back(myRendererPopup);
  ypos += lineHeight + 4;

  // TIA filters (will be dynamically filled later)
  items.clear();
  myTIAFilterPopup = new PopUpWidget(this, font, xpos, ypos, pwidth,
                                     lineHeight, items, "TIA Filter: ", lwidth);
  wid.push_back(myTIAFilterPopup);
  ypos += lineHeight + 4;

  // TIA Palette
  items.clear();
  items.push_back("Standard", "standard");
  items.push_back("Z26", "z26");
  items.push_back("User", "user");
  myTIAPalettePopup = new PopUpWidget(this, font, xpos, ypos, pwidth,
                                      lineHeight, items, "TIA Palette: ", lwidth);
  wid.push_back(myTIAPalettePopup);
  ypos += lineHeight + 4;

  // Fullscreen resolution
  items.clear();
  items.push_back("Auto", "auto");
  for(uInt32 i = 0; i < instance().supportedResolutions().size(); ++i)
    items.push_back(instance().supportedResolutions()[i].name,
                    instance().supportedResolutions()[i].name);
  myFSResPopup = new PopUpWidget(this, font, xpos, ypos, pwidth,
                                 lineHeight, items, "FS Res: ", lwidth);
  wid.push_back(myFSResPopup);
  ypos += lineHeight + 4;

  // GL Video filter
  items.clear();
  items.push_back("Linear", "linear");
  items.push_back("Nearest", "nearest");
  myGLFilterPopup = new PopUpWidget(this, font, xpos, ypos, pwidth, lineHeight,
                                  items, "GL Filter: ", lwidth);
  wid.push_back(myGLFilterPopup);
  ypos += lineHeight + 4;

  // GL FS stretch
  items.clear();
  items.push_back("Never", "never");
  items.push_back("UI mode", "ui");
  items.push_back("TIA mode", "tia");
  items.push_back("Always", "always");
  myGLStretchPopup = new PopUpWidget(this, font, xpos, ypos, pwidth, lineHeight,
                                     items, "GL Stretch: ", lwidth);
  wid.push_back(myGLStretchPopup);
  ypos += lineHeight + 4;


  // GL aspect ratio
  myAspectRatioSlider =
    new SliderWidget(this, font, xpos, ypos, pwidth, lineHeight,
                     "GL Aspect: ", lwidth, kAspectRatioChanged);
  myAspectRatioSlider->setMinValue(50); myAspectRatioSlider->setMaxValue(100);
  wid.push_back(myAspectRatioSlider);
  myAspectRatioLabel =
    new StaticTextWidget(this, font, xpos + myAspectRatioSlider->getWidth() + 4,
                         ypos + 1, fontWidth * 3, fontHeight, "", kTextAlignLeft);
  myAspectRatioLabel->setFlags(WIDGET_CLEARBG);
  ypos += lineHeight + 4;

  // Framerate
  myFrameRateSlider =
    new SliderWidget(this, font, xpos, ypos, pwidth, lineHeight,
                     "Framerate: ", lwidth, kFrameRateChanged);
  myFrameRateSlider->setMinValue(0); myFrameRateSlider->setMaxValue(300);
  wid.push_back(myFrameRateSlider);
  myFrameRateLabel =
    new StaticTextWidget(this, font, xpos + myFrameRateSlider->getWidth() + 4,
                         ypos + 1, fontWidth * 3, fontHeight, "", kTextAlignLeft);
  myFrameRateLabel->setFlags(WIDGET_CLEARBG);

  // Move over to the next column
  xpos += myAspectRatioSlider->getWidth() + myAspectRatioLabel->getWidth();
  ypos = 10;

  // Fullscreen
  myFullscreenCheckbox = new CheckboxWidget(this, font, xpos, ypos,
                                            "Fullscreen mode", kFullScrChanged);
  wid.push_back(myFullscreenCheckbox);
  ypos += lineHeight + 4;

  // PAL color-loss effect
  myColorLossCheckbox = new CheckboxWidget(this, font, xpos, ypos,
                                           "PAL color-loss");
  wid.push_back(myColorLossCheckbox);
  ypos += lineHeight + 4;

  // Use sync to vblank in OpenGL
  myUseVSyncCheckbox = new CheckboxWidget(this, font, xpos, ypos,
                                          "GL VSync");
  wid.push_back(myUseVSyncCheckbox);
  ypos += lineHeight + 4;

  // Center window (in windowed mode)
  myCenterCheckbox = new CheckboxWidget(this, font, xpos, ypos,
                                        "Center window (*)");
  wid.push_back(myCenterCheckbox);
  ypos += lineHeight + 4;

  // Add message concerning usage
  lwidth = font.getStringWidth("(*) Requires application restart");
  new StaticTextWidget(this, font, 10, _h - 2*buttonHeight - 10, lwidth, fontHeight,
                       "(*) Requires application restart",
                       kTextAlignLeft);

  // Add Defaults, OK and Cancel buttons
  ButtonWidget* b;
  b = new ButtonWidget(this, font, 10, _h - buttonHeight - 10,
                       buttonWidth, buttonHeight, "Defaults", kDefaultsCmd);
  wid.push_back(b);
  addOKCancelBGroup(wid, font);

  addToFocusList(wid);

  // Disable certain functions when we know they aren't present
#ifndef DISPLAY_GL
  myGLFilterPopup->clearFlags(WIDGET_ENABLED);
  myGLStretchPopup->clearFlags(WIDGET_ENABLED);
  myAspectRatioSlider->clearFlags(WIDGET_ENABLED);
  myAspectRatioLabel->clearFlags(WIDGET_ENABLED);
  myUseVSyncCheckbox->clearFlags(WIDGET_ENABLED);
#endif
#ifndef WINDOWED_SUPPORT
  myFullscreenCheckbox->clearFlags(WIDGET_ENABLED);
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
  // Renderer setting
  myRendererPopup->setSelected(
    instance().settings().getString("video"), "soft");

  // TIA Filter
  //  taken care of in ::handleRendererChange()

  // TIA Palette
  myTIAPalettePopup->setSelected(
    instance().settings().getString("palette"), "standard");

  // Fullscreen resolution
  myFSResPopup->setSelected(
    instance().settings().getString("fullres"), "auto");

  // GL Filter setting
  myGLFilterPopup->setSelected(
    instance().settings().getString("gl_filter"), "linear");

  // GL stretch setting
  myGLStretchPopup->setSelected(
    instance().settings().getString("gl_fsmax"), "never");

  // GL aspect ratio setting
  myAspectRatioSlider->setValue(instance().settings().getInt("gl_aspect"));
  myAspectRatioLabel->setLabel(instance().settings().getString("gl_aspect"));

  // Framerate (0 or -1 means disabled)
  int rate = instance().settings().getInt("framerate");
  myFrameRateSlider->setValue(rate < 0 ? 0 : rate);
  myFrameRateLabel->setLabel(rate < 0 ? "0" :
    instance().settings().getString("framerate"));

  // Fullscreen
  bool b = instance().settings().getBool("fullscreen");
  myFullscreenCheckbox->setState(b);
  handleFullscreenChange(b);

  // PAL color-loss effect
  myColorLossCheckbox->setState(instance().settings().getBool("colorloss"));

  // Use sync to vertical blank (GL mode only)
  myUseVSyncCheckbox->setState(instance().settings().getBool("gl_vsync"));

  // Center window
  myCenterCheckbox->setState(instance().settings().getBool("center"));

  // Make sure that mutually-exclusive items are not enabled at the same time
  // Also, this sets the TIA filters, so it cannot be removed from here
  handleRendererChange(myRendererPopup->getSelectedTag());
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

  // GL Filter setting
  instance().settings().setString("gl_filter", myGLFilterPopup->getSelectedTag());

  // GL stretch setting
  instance().settings().setString("gl_fsmax", myGLStretchPopup->getSelectedTag());

  // GL aspect ratio setting
  instance().settings().setString("gl_aspect", myAspectRatioLabel->getLabel());

  // Framerate
  int i = myFrameRateSlider->getValue();
  instance().settings().setInt("framerate", i);
  if(&instance().console())
  {
    // Make sure auto-frame calculation is only enabled when necessary
    instance().console().mediaSource().enableAutoFrame(i <= 0);
    instance().console().setFramerate(float(i));
  }

  // Fullscreen
  instance().settings().setBool("fullscreen", myFullscreenCheckbox->getState());

  // PAL color-loss effect
  instance().settings().setBool("colorloss", myColorLossCheckbox->getState());

  // Use sync to vertical blank (GL mode only)
  instance().settings().setBool("gl_vsync", myUseVSyncCheckbox->getState());

  // Center window
  instance().settings().setBool("center", myCenterCheckbox->getState());

  // Finally, issue a complete framebuffer re-initialization
  instance().createFrameBuffer();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VideoDialog::setDefaults()
{
  myRendererPopup->setSelected("soft", "");
  myTIAFilterPopup->setSelected(
#ifdef SMALL_SCREEN
    "zoom1x", "");
#else
    "zoom2x", "");
#endif
  myTIAPalettePopup->setSelected("standard", "");
  myFSResPopup->setSelected("auto", "");
  myGLFilterPopup->setSelected("linear", "");
  myGLStretchPopup->setSelected("never", "");
  myAspectRatioSlider->setValue(100);
  myAspectRatioLabel->setLabel("100");
  myFrameRateSlider->setValue(0);
  myFrameRateLabel->setLabel("0");

  myFullscreenCheckbox->setState(false);
  myColorLossCheckbox->setState(false);
  myUseVSyncCheckbox->setState(true);
  myCenterCheckbox->setState(true);

  // Make sure that mutually-exclusive items are not enabled at the same time
  handleRendererChange("soft");
  handleFullscreenChange(false);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VideoDialog::handleRendererChange(const string& mode)
{
  // TIA filters are dynamically loaded, since they depend on the size of
  // the desktop and which renderer we're using
  const StringMap& items = instance().frameBuffer().supportedTIAFilters(mode);
  myTIAFilterPopup->addItems(items);
  myTIAFilterPopup->setSelected(instance().settings().getString("tia_filter"),
#ifdef SMALL_SCREEN
    "zoom1x");
#else
    "zoom2x");
#endif

#ifdef DISPLAY_OPENGL
  // When we're in software mode, certain OpenGL-related options are disabled
  bool gl = (mode == "gl");

  myGLFilterPopup->setEnabled(gl);
  myGLStretchPopup->setEnabled(gl);
  myAspectRatioSlider->setEnabled(gl);
  myAspectRatioLabel->setEnabled(gl);
  myUseVSyncCheckbox->setEnabled(gl);
#endif

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

    case kRendererChanged:
      handleRendererChange(myRendererPopup->getSelectedTag());
      break;

    case kAspectRatioChanged:
      myAspectRatioLabel->setValue(myAspectRatioSlider->getValue());
      break;

    case kFrameRateChanged:
      myFrameRateLabel->setValue(myFrameRateSlider->getValue());
      break;

    case kFullScrChanged:
      handleFullscreenChange(myFullscreenCheckbox->getState());
      break;

    default:
      Dialog::handleCommand(sender, cmd, data, 0);
      break;
  }
}
