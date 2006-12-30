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
// Copyright (c) 1995-2006 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: VideoDialog.cxx,v 1.40 2006-12-30 22:26:29 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#include <sstream>

#include "OSystem.hxx"
#include "Settings.hxx"
#include "Menu.hxx"
#include "Control.hxx"
#include "Widget.hxx"
#include "PopUpWidget.hxx"
#include "Dialog.hxx"
#include "VideoDialog.hxx"
#include "GuiUtils.hxx"

#include "bspf.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
VideoDialog::VideoDialog(OSystem* osystem, DialogContainer* parent,
                         const GUI::Font& font, int x, int y, int w, int h)
  : Dialog(osystem, parent, x, y, w, h)
{
  const int lineHeight = font.getLineHeight(),
            fontHeight = font.getFontHeight();
  int xpos, ypos;
  int lwidth = font.getStringWidth("Dirty Rects: "),
      pwidth = font.getStringWidth("Software");
  WidgetArray wid;

  xpos = 5;  ypos = 10;

  // Video renderer
  myRendererPopup = new PopUpWidget(this, font, xpos, ypos,
                                    pwidth, lineHeight, "Renderer: ", lwidth,
                                    kRendererChanged);
  myRendererPopup->appendEntry("Software", 1);
#ifdef DISPLAY_OPENGL
  myRendererPopup->appendEntry("OpenGL", 2);
#endif
  wid.push_back(myRendererPopup);
  ypos += lineHeight + 4;

  // Video filter
  myFilterPopup = new PopUpWidget(this, font, xpos, ypos,
                                  pwidth, lineHeight, "GL Filter: ", lwidth);
  myFilterPopup->appendEntry("Linear", 1);
  myFilterPopup->appendEntry("Nearest", 2);
  wid.push_back(myFilterPopup);
  ypos += lineHeight + 4;

  // Aspect ratio
  myAspectRatioSlider = new SliderWidget(this, font, xpos, ypos, pwidth, lineHeight,
                                         "GL Aspect: ", lwidth, kAspectRatioChanged);
  myAspectRatioSlider->setMinValue(1); myAspectRatioSlider->setMaxValue(100);
  wid.push_back(myAspectRatioSlider);
  myAspectRatioLabel = new StaticTextWidget(this, font,
                                            xpos + myAspectRatioSlider->getWidth() + 4,
                                            ypos + 1,
                                            15, fontHeight, "", kTextAlignLeft);
  myAspectRatioLabel->setFlags(WIDGET_CLEARBG);
  ypos += lineHeight + 4;

  // Palette
  myPalettePopup = new PopUpWidget(this, font, xpos, ypos, pwidth,
                                   lineHeight, "Palette: ", lwidth);
  myPalettePopup->appendEntry("Standard", 1);
  myPalettePopup->appendEntry("Original", 2);
  myPalettePopup->appendEntry("Z26", 3);
  myPalettePopup->appendEntry("User", 4);
  wid.push_back(myPalettePopup);
  ypos += lineHeight + 4;

  // Available TIA scalers
  myTIAScalerPopup = new PopUpWidget(this, font, xpos, ypos, pwidth,
                                  lineHeight, "TIA Scaler: ", lwidth);
  myTIAScalerPopup->appendEntry("Zoom1x", 1);
  myTIAScalerPopup->appendEntry("Zoom2x", 2);
  myTIAScalerPopup->appendEntry("Zoom3x", 3);
  myTIAScalerPopup->appendEntry("Zoom4x", 4);
  myTIAScalerPopup->appendEntry("Zoom5x", 5);
  myTIAScalerPopup->appendEntry("Zoom6x", 6);
  wid.push_back(myTIAScalerPopup);

  ypos += lineHeight + 4;
  myUIScalerPopup = new PopUpWidget(this, font, xpos, ypos, pwidth,
                                    lineHeight, "UI Scaler: ", lwidth);
  myUIScalerPopup->appendEntry("Zoom1x", 1);
  myUIScalerPopup->appendEntry("Zoom2x", 2);
  myUIScalerPopup->appendEntry("Zoom3x", 3);
  myUIScalerPopup->appendEntry("Zoom4x", 4);
  myUIScalerPopup->appendEntry("Zoom5x", 5);
  myUIScalerPopup->appendEntry("Zoom6x", 6);
  wid.push_back(myUIScalerPopup);

  // Move over to the next column
  xpos += 115;  ypos = 10;

  // Framerate
  myFrameRateSlider = new SliderWidget(this, font, xpos, ypos, 30, lineHeight,
                                       "Framerate: ", lwidth, kFrameRateChanged);
  myFrameRateSlider->setMinValue(1); myFrameRateSlider->setMaxValue(300);
  wid.push_back(myFrameRateSlider);
  myFrameRateLabel = new StaticTextWidget(this, font,
                                          xpos + myFrameRateSlider->getWidth() + 4,
                                          ypos + 1,
                                          15, fontHeight, "", kTextAlignLeft);
  myFrameRateLabel->setFlags(WIDGET_CLEARBG);
  ypos += lineHeight + 4;

  // Fullscreen
  myFullscreenCheckbox = new CheckboxWidget(this, font, xpos + 5, ypos,
                                            "Fullscreen mode");
  wid.push_back(myFullscreenCheckbox);
  ypos += lineHeight + 4;

  // PAL color-loss effect
  myColorLossCheckbox = new CheckboxWidget(this, font, xpos + 5, ypos,
                                           "PAL color-loss");
  wid.push_back(myColorLossCheckbox);
  ypos += lineHeight + 4;

  // Use dirty rectangle merging
  myDirtyRectCheckbox = new CheckboxWidget(this, font, xpos + 5, ypos,
                                           "Dirty-rect merging");
  wid.push_back(myDirtyRectCheckbox);
  ypos += lineHeight + 4;

  // Use desktop res in OpenGL
  myUseDeskResCheckbox = new CheckboxWidget(this, font, xpos + 5, ypos,
                                            "Desktop Res in FS");
  wid.push_back(myUseDeskResCheckbox);
  ypos += lineHeight + 4;

  // Use sync to vblank in OpenGL
  myUseVSyncCheckbox = new CheckboxWidget(this, font, xpos + 5, ypos,
                                          "GL VSync");
  wid.push_back(myUseVSyncCheckbox);
  ypos += lineHeight + 20;

  // Add Defaults, OK and Cancel buttons
  ButtonWidget* b;
  b = addButton(font, 10, _h - 24, "Defaults", kDefaultsCmd);
  wid.push_back(b);
#ifndef MAC_OSX
  b = addButton(font, _w - 2 * (kButtonWidth + 7), _h - 24, "OK", kOKCmd);
  wid.push_back(b);
  addOKWidget(b);
  b = addButton(font, _w - (kButtonWidth + 10), _h - 24, "Cancel", kCloseCmd);
  wid.push_back(b);
  addCancelWidget(b);
#else
  b = addButton(font, _w - 2 * (kButtonWidth + 7), _h - 24, "Cancel", kCloseCmd);
  wid.push_back(b);
  addCancelWidget(b);
  b = addButton(font, _w - (kButtonWidth + 10), _h - 24, "OK", kOKCmd);
  wid.push_back(b);
  addOKWidget(b);
#endif

  addToFocusList(wid);
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
  double f;

  // Renderer setting
  s = instance()->settings().getString("video");
  if(s == "soft")    myRendererPopup->setSelectedTag(1);
  else if(s == "gl") myRendererPopup->setSelectedTag(2);

  // Filter setting
  s = instance()->settings().getString("gl_filter");
  if(s == "linear")       myFilterPopup->setSelectedTag(1);
  else if(s == "nearest") myFilterPopup->setSelectedTag(2);

  // Aspect ratio - another huge hack
  s = instance()->settings().getString("gl_aspect");
  f = instance()->settings().getFloat("gl_aspect");
  if(f < 1.1)      { f = 1.1;  s = "1.1"; }
  else if(f > 2.0) { f = 2.0;  s = "2.0"; }
  i = (int)((f * 10) - 10) * 10;
  myAspectRatioSlider->setValue(i);
  myAspectRatioLabel->setLabel(s);

  // Palette
  s = instance()->settings().getString("palette");
  if(s == "standard")      myPalettePopup->setSelectedTag(1);
  else if(s == "original") myPalettePopup->setSelectedTag(2);
  else if(s == "z26")      myPalettePopup->setSelectedTag(3);
  else if(s == "user")     myPalettePopup->setSelectedTag(4);

  // TIA Scaler
  s = instance()->settings().getString("scale_tia");
  if(s == "zoom1x")       myTIAScalerPopup->setSelectedTag(1);
  else if(s == "zoom2x")  myTIAScalerPopup->setSelectedTag(2);
  else if(s == "zoom3x")  myTIAScalerPopup->setSelectedTag(3);
  else if(s == "zoom4x")  myTIAScalerPopup->setSelectedTag(4);
  else if(s == "zoom5x")  myTIAScalerPopup->setSelectedTag(5);
  else if(s == "zoom6x")  myTIAScalerPopup->setSelectedTag(6);
  else                    myTIAScalerPopup->setSelectedTag(0);

  // UI Scaler
  s = instance()->settings().getString("scale_ui");
  if(s == "zoom1x")       myUIScalerPopup->setSelectedTag(1);
  else if(s == "zoom2x")  myUIScalerPopup->setSelectedTag(2);
  else if(s == "zoom3x")  myUIScalerPopup->setSelectedTag(3);
  else if(s == "zoom4x")  myUIScalerPopup->setSelectedTag(4);
  else if(s == "zoom5x")  myUIScalerPopup->setSelectedTag(5);
  else if(s == "zoom6x")  myUIScalerPopup->setSelectedTag(6);
  else                    myUIScalerPopup->setSelectedTag(0);

  // FIXME - what to do with this??
  myFrameRateSlider->setEnabled(false);

  // Fullscreen
  b = instance()->settings().getBool("fullscreen");
  myFullscreenCheckbox->setState(b);

  // PAL color-loss effect
  b = instance()->settings().getBool("colorloss");
  myColorLossCheckbox->setState(b);

  // Dirty-rect merging (software mode only)
  b = instance()->settings().getBool("dirtyrects");
  myDirtyRectCheckbox->setState(b);

  // Use desktop resolution in fullscreen mode (GL mode only)
  b = instance()->settings().getBool("gl_fsmax");
  myUseDeskResCheckbox->setState(b);

  // Use sync to vertical blank (GL mode only)
  b = instance()->settings().getBool("gl_vsync");
  myUseVSyncCheckbox->setState(b);

  // Make sure that mutually-exclusive items are not enabled at the same time
  i = myRendererPopup->getSelectedTag();
  handleRendererChange(i);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VideoDialog::saveConfig()
{
  string s;
  int i;
  bool b;

  // Renderer setting
  i = myRendererPopup->getSelectedTag();
  if(i == 1)       s = "soft";
  else if(i == 2)  s = "gl";
  instance()->settings().setString("video", s);

  // Filter setting
  i = myFilterPopup->getSelectedTag();
  if(i == 1)      s = "linear";
  else if(i == 2) s = "nearest";
  instance()->settings().setString("gl_filter", s);

  // Aspect ratio
  s = myAspectRatioLabel->getLabel();
  instance()->settings().setString("gl_aspect", s);

  // Palette
  i = myPalettePopup->getSelectedTag();
  if(i == 1)       s = "standard";
  else if(i == 2)  s = "original";
  else if(i == 3)  s = "z26";
  else if(i == 4)  s = "user";
  instance()->settings().setString("palette", s);

  // TIA Scaler
  i = myTIAScalerPopup->getSelectedTag();
  if(i == 1)       s = "zoom1x";
  else if(i == 2)  s = "zoom2x";
  else if(i == 3)  s = "zoom3x";
  else if(i == 4)  s = "zoom4x";
  else if(i == 5)  s = "zoom5x";
  else if(i == 6)  s = "zoom6x";
  instance()->settings().setString("scale_tia", s);

  // UI Scaler
  i = myUIScalerPopup->getSelectedTag();
  if(i == 1)       s = "zoom1x";
  else if(i == 2)  s = "zoom2x";
  else if(i == 3)  s = "zoom3x";
  else if(i == 4)  s = "zoom4x";
  else if(i == 5)  s = "zoom5x";
  else if(i == 6)  s = "zoom6x";
  instance()->settings().setString("scale_ui", s);

  // Framerate   FIXME - I haven't figured out what to do with this yet
/*
  i = myFrameRateSlider->getValue();
  if(i > 0)
    instance()->setFramerate(i);
*/

  // Fullscreen
  b = myFullscreenCheckbox->getState();
  instance()->settings().setBool("fullscreen", b);

  // PAL color-loss effect
  b = myColorLossCheckbox->getState();
  instance()->settings().setBool("colorloss", b);

  // Dirty rectangle merging (software mode only)
  b = myDirtyRectCheckbox->getState();
  instance()->settings().setBool("dirtyrects", b);

  // Use desktop resolution in fullscreen mode (GL mode only)
  b = myUseDeskResCheckbox->getState();
  instance()->settings().setBool("gl_fsmax", b);

  // Use sync to vertical blank (GL mode only)
  b = myUseVSyncCheckbox->getState();
  instance()->settings().setBool("gl_vsync", b);

  // Finally, issue a complete framebuffer re-initialization
  instance()->createFrameBuffer(false);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VideoDialog::setDefaults()
{
  myRendererPopup->setSelectedTag(1);
  myFilterPopup->setSelectedTag(1);
  myPalettePopup->setSelectedTag(1);
  myTIAScalerPopup->setSelectedTag(2);
  myUIScalerPopup->setSelectedTag(2);
//  myFrameRateSlider->setValue(0);
//  myFrameRateLabel->setLabel("0");

  // For some unknown reason (ie, a bug), slider widgets can only
  // take certain ranges of numbers.  So we have to fudge things ...
  myAspectRatioSlider->setValue(100);
  myAspectRatioLabel->setLabel("2.0");

  myFullscreenCheckbox->setState(false);
  myColorLossCheckbox->setState(false);
  myDirtyRectCheckbox->setState(false);
  myUseDeskResCheckbox->setState(true);
  myUseVSyncCheckbox->setState(true);

  // Make sure that mutually-exclusive items are not enabled at the same time
  handleRendererChange(1);  // 1 indicates software mode
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VideoDialog::handleRendererChange(int item)
{
  // When we're in software mode, certain OpenGL-related options are disabled
  bool gl = (item > 1) ? true : false;

  myFilterPopup->setEnabled(gl);
  myAspectRatioSlider->setEnabled(gl);
  myAspectRatioLabel->setEnabled(gl);
  myUseDeskResCheckbox->setEnabled(gl);
  myUseVSyncCheckbox->setEnabled(gl);

  // Also, in OpenGL mode, certain software related items are disabled
  myDirtyRectCheckbox->setEnabled(!gl);

  _dirty = true;
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

    case kAspectRatioChanged:
    {
      // This is terribly dirty, but what can we do?
      float ratio = (((myAspectRatioSlider->getValue() + 9) / 10) / 10.0) + 1.0;
      ostringstream r;
      if(ratio == 2.0)
        r << ratio << ".0";
      else
        r << ratio;
      myAspectRatioLabel->setLabel(r.str());
      break;
    }

    case kFrameRateChanged:
      myFrameRateLabel->setValue(myFrameRateSlider->getValue());
      break;

    default:
      Dialog::handleCommand(sender, cmd, data, 0);
      break;
  }
}
