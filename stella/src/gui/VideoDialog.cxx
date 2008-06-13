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
// $Id: VideoDialog.cxx,v 1.51 2008-06-13 13:14:52 stephena Exp $
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
  StringList items;

  // Set real dimensions
//  _w = 46 * fontWidth + 10;
//  _h = 11 * (lineHeight + 4) + 10;

  xpos = 5;  ypos = 10;

  // Video renderer
  items.clear();
  items.push_back("Software");
#ifdef DISPLAY_OPENGL
  items.push_back("OpenGL");
#endif
  myRendererPopup = new PopUpWidget(this, font, xpos, ypos, pwidth, lineHeight,
                                    items, "Renderer: ", lwidth,
                                    kRendererChanged);
  wid.push_back(myRendererPopup);
  ypos += lineHeight + 4;

  // Video filter
  items.clear();
  items.push_back("Linear");
  items.push_back("Nearest");
  myFilterPopup = new PopUpWidget(this, font, xpos, ypos, pwidth, lineHeight,
                                  items, "GL Filter: ", lwidth);
  wid.push_back(myFilterPopup);
  ypos += lineHeight + 4;

  // GL FS stretch
  items.clear();
  items.push_back("Never");
  items.push_back("UI mode");
  items.push_back("TIA mode");
  items.push_back("Always");
  myFSStretchPopup = new PopUpWidget(this, font, xpos, ypos, pwidth, lineHeight,
                                     items, "GL Stretch: ", lwidth);
  wid.push_back(myFSStretchPopup);
  ypos += lineHeight + 4;

  // Palette
  items.clear();
  items.push_back("Standard");
  items.push_back("Z26");
  items.push_back("User");
  myPalettePopup = new PopUpWidget(this, font, xpos, ypos, pwidth,
                                   lineHeight, items, "Palette: ", lwidth);
  wid.push_back(myPalettePopup);
  ypos += lineHeight + 4;

  // Fullscreen resolution
  items.clear();
  for(uInt32 i = 0; i < instance().supportedResolutions().size(); ++i)
    items.push_back(instance().supportedResolutions()[i].name);
  myFSResPopup = new PopUpWidget(this, font, xpos, ypos, pwidth,
                                 lineHeight, items, "FS Res: ", lwidth);
  wid.push_back(myFSResPopup);
  ypos += lineHeight + 4;

  // Available UI zoom levels
  myUIZoomSlider = new SliderWidget(this, font, xpos, ypos, pwidth, lineHeight,
                                    "UI Zoom: ", lwidth, kUIZoomChanged);
  myUIZoomSlider->setMinValue(1); myUIZoomSlider->setMaxValue(10);
  wid.push_back(myUIZoomSlider);
  myUIZoomLabel =
    new StaticTextWidget(this, font, xpos + myUIZoomSlider->getWidth() + 4,
                         ypos + 1, fontWidth * 2, fontHeight, "", kTextAlignLeft);
  myUIZoomLabel->setFlags(WIDGET_CLEARBG);
  ypos += lineHeight + 4;

  // Available TIA zoom levels
  myTIAZoomSlider = new SliderWidget(this, font, xpos, ypos, pwidth, lineHeight,
                                     "TIA Zoom: ", lwidth, kTIAZoomChanged);
  myTIAZoomSlider->setMinValue(1); myTIAZoomSlider->setMaxValue(10);
  wid.push_back(myTIAZoomSlider);
  myTIAZoomLabel =
    new StaticTextWidget(this, font, xpos + myTIAZoomSlider->getWidth() + 4,
                         ypos + 1, fontWidth * 2, fontHeight, "", kTextAlignLeft);
  myTIAZoomLabel->setFlags(WIDGET_CLEARBG);
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

  // Move over to the next column
  xpos += myAspectRatioSlider->getWidth() + myAspectRatioLabel->getWidth();
  ypos = 10;

  // Framerate
  myFrameRateSlider = new SliderWidget(this, font, xpos, ypos, 30, lineHeight,
                                       "Framerate: ", lwidth, kFrameRateChanged);
  myFrameRateSlider->setMinValue(0); myFrameRateSlider->setMaxValue(300);
  wid.push_back(myFrameRateSlider);
  myFrameRateLabel = new StaticTextWidget(this, font,
                                          xpos + myFrameRateSlider->getWidth() + 4,
                                          ypos + 1,
                                          15, fontHeight, "", kTextAlignLeft);
  myFrameRateLabel->setFlags(WIDGET_CLEARBG);
  ypos += lineHeight + 4;

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
  myFilterPopup->clearFlags(WIDGET_ENABLED);
  myAspectRatioSlider->clearFlags(WIDGET_ENABLED);
  myAspectRatioLabel->clearFlags(WIDGET_ENABLED);
  myFSStretchPopup->clearFlags(WIDGET_ENABLED);
  myUseVSyncCheckbox->clearFlags(WIDGET_ENABLED);
#endif
#ifndef WINDOWED_SUPPORT
  myUIZoomSlider->clearFlags(WIDGET_ENABLED);
  myUIZoomLabel->clearFlags(WIDGET_ENABLED);
  myTIAZoomSlider->clearFlags(WIDGET_ENABLED);
  myTIAZoomLabel->clearFlags(WIDGET_ENABLED);
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
  string s;
  bool b;
  int i;

  // Renderer setting
  s = instance().settings().getString("video");
  myRendererPopup->clearSelection();
  if(s == "soft")    myRendererPopup->setSelected(0);
  else if(s == "gl") myRendererPopup->setSelected(1);

  // Filter setting
  s = instance().settings().getString("gl_filter");
  myFilterPopup->clearSelection();
  if(s == "linear")       myFilterPopup->setSelected(0);
  else if(s == "nearest") myFilterPopup->setSelected(1);

  // GL stretch setting
  s = instance().settings().getString("gl_fsmax");
  myFSStretchPopup->clearSelection();
  if(s == "never")        myFSStretchPopup->setSelected(0);
  else if(s == "ui")      myFSStretchPopup->setSelected(1);
  else if(s == "tia")     myFSStretchPopup->setSelected(2);
  else if(s == "always")  myFSStretchPopup->setSelected(3);

  // Palette
  s = instance().settings().getString("palette");
  myPalettePopup->clearSelection();
  if(s == "standard")     myPalettePopup->setSelected(0);
  else if(s == "z26")     myPalettePopup->setSelected(1);
  else if(s == "user")    myPalettePopup->setSelected(2);

  // Fullscreen resolution
  s = instance().settings().getString("fullres");
  myFSResPopup->clearSelection();
  myFSResPopup->setSelected(s);
  if(myFSResPopup->getSelected() < 0)
    myFSResPopup->setSelectedMax();

  // UI zoom level
  s = instance().settings().getString("zoom_ui");
  i = instance().settings().getInt("zoom_ui");
  myUIZoomSlider->setValue(i);
  myUIZoomLabel->setLabel(s);

  // TIA zoom level
  s = instance().settings().getString("zoom_tia");
  i = instance().settings().getInt("zoom_tia");
  myTIAZoomSlider->setValue(i);
  myTIAZoomLabel->setLabel(s);

  // GL aspect ratio setting
  s = instance().settings().getString("gl_aspect");
  i = instance().settings().getInt("gl_aspect");
  myAspectRatioSlider->setValue(i);
  myAspectRatioLabel->setLabel(s);

  // Framerate (0 or -1 means disabled)
  s = instance().settings().getString("framerate");
  i = instance().settings().getInt("framerate");
  myFrameRateSlider->setValue(i < 0 ? 0 : i);
  myFrameRateLabel->setLabel(i < 0 ? "0" : s);

  // Fullscreen
  b = instance().settings().getBool("fullscreen");
  myFullscreenCheckbox->setState(b);
  handleFullscreenChange(b);

  // PAL color-loss effect
  b = instance().settings().getBool("colorloss");
  myColorLossCheckbox->setState(b);

  // Use sync to vertical blank (GL mode only)
  b = instance().settings().getBool("gl_vsync");
  myUseVSyncCheckbox->setState(b);

  // Center window
  b = instance().settings().getBool("center");
  myCenterCheckbox->setState(b);

  // Make sure that mutually-exclusive items are not enabled at the same time
  i = myRendererPopup->getSelected();
  handleRendererChange(i);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VideoDialog::saveConfig()
{
  string s;
  int i;
  bool b;

  // Renderer setting
  i = myRendererPopup->getSelected();
  if(i == 0)       s = "soft";
  else if(i == 1)  s = "gl";
  instance().settings().setString("video", s);

  // Filter setting
  i = myFilterPopup->getSelected();
  if(i == 0)      s = "linear";
  else if(i == 1) s = "nearest";
  instance().settings().setString("gl_filter", s);

  // GL stretch setting
  i = myFSStretchPopup->getSelected();
  if(i == 0)       s = "never";
  else if(i == 1)  s = "ui";
  else if(i == 2)  s = "tia";
  else if(i == 3)  s = "always";
  instance().settings().setString("gl_fsmax", s);

  // Palette
  i = myPalettePopup->getSelected();
  if(i == 0)       s = "standard";
  else if(i == 1)  s = "z26";
  else if(i == 2)  s = "user";
  instance().settings().setString("palette", s);

  // Fullscreen resolution
  s = myFSResPopup->getSelectedString();
  instance().settings().setString("fullres", s);

  // UI Scaler
  s = myUIZoomLabel->getLabel();
  instance().settings().setString("zoom_ui", s);

  // TIA Scaler
  s = myTIAZoomLabel->getLabel();
  instance().settings().setString("zoom_tia", s);

  // GL aspect ratio setting
  s = myAspectRatioLabel->getLabel();
  instance().settings().setString("gl_aspect", s);

  // Framerate
  i = myFrameRateSlider->getValue();
  instance().settings().setInt("framerate", i);
  if(&instance().console())
  {
    // Make sure auto-frame calculation is only enabled when necessary
    instance().console().mediaSource().enableAutoFrame(i <= 0);
    instance().console().setFramerate(i);
  }

  // Fullscreen
  b = myFullscreenCheckbox->getState();
  instance().settings().setBool("fullscreen", b);

  // PAL color-loss effect
  b = myColorLossCheckbox->getState();
  instance().settings().setBool("colorloss", b);

  // Use sync to vertical blank (GL mode only)
  b = myUseVSyncCheckbox->getState();
  instance().settings().setBool("gl_vsync", b);

  // Center window
  b = myCenterCheckbox->getState();
  instance().settings().setBool("center", b);

  // Finally, issue a complete framebuffer re-initialization
  instance().createFrameBuffer(false);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VideoDialog::setDefaults()
{
  myRendererPopup->setSelected(0);
  myFilterPopup->setSelected(0);
  myFSStretchPopup->setSelected(0);
  myPalettePopup->setSelected(0);
  myFSResPopup->setSelectedMax();
  myUIZoomSlider->setValue(2);
  myUIZoomLabel->setLabel("2");
  myTIAZoomSlider->setValue(2);
  myTIAZoomLabel->setLabel("2");
  myAspectRatioSlider->setValue(100);
  myAspectRatioLabel->setLabel("100");
  myFrameRateSlider->setValue(0);
  myFrameRateLabel->setLabel("0");

  myFullscreenCheckbox->setState(false);
  myColorLossCheckbox->setState(false);
  myUseVSyncCheckbox->setState(true);
  myCenterCheckbox->setState(true);

  // Make sure that mutually-exclusive items are not enabled at the same time
  handleRendererChange(0);        // 0 indicates software mode
  handleFullscreenChange(false);  // indicates fullscreen deactivated
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VideoDialog::handleRendererChange(int item)
{
#ifdef DISPLAY_OPENGL
  // When we're in software mode, certain OpenGL-related options are disabled
  bool gl = (item > 0) ? true : false;

  myFilterPopup->setEnabled(gl);
  myFSStretchPopup->setEnabled(gl);
  myAspectRatioSlider->setEnabled(gl);
  myAspectRatioLabel->setEnabled(gl);
  myUseVSyncCheckbox->setEnabled(gl);

  _dirty = true;
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VideoDialog::handleFullscreenChange(bool enable)
{
#ifdef WINDOWED_SUPPORT
  myFSResPopup->setEnabled(enable);

  myUIZoomSlider->setEnabled(!enable);
  myUIZoomLabel->setEnabled(!enable);
  myTIAZoomSlider->setEnabled(!enable);
  myTIAZoomLabel->setEnabled(!enable);

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
      handleRendererChange(data);
      break;

    case kUIZoomChanged:
      myUIZoomLabel->setValue(myUIZoomSlider->getValue());
      break;

    case kTIAZoomChanged:
      myTIAZoomLabel->setValue(myTIAZoomSlider->getValue());
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
