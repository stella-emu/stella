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
// Copyright (c) 1995-2005 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: VideoDialog.cxx,v 1.25 2005-10-18 18:49:46 stephena Exp $
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

enum {
  kVideoRowHeight = 12,
  kVideoWidth     = 200,
  kVideoHeight    = 100
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
VideoDialog::VideoDialog(OSystem* osystem, DialogContainer* parent,
                         int x, int y, int w, int h)
    : Dialog(osystem, parent, x, y, w, h)
{
  const GUI::Font& font = instance()->font();

  int yoff = 10,
      xoff = 2,
      woff = 110,
      labelWidth = 55;

  // Use dirty rectangle updates
  myDirtyPopup = new PopUpWidget(this, xoff, yoff, woff, kLineHeight,
                                 "Dirty Rects: ", labelWidth);
  myDirtyPopup->appendEntry("Yes", 1);
  myDirtyPopup->appendEntry("No", 2);
  yoff += kVideoRowHeight + 4;

  // Video renderer
  myRendererPopup = new PopUpWidget(this, xoff, yoff, woff, kLineHeight,
                                    "Renderer: ", labelWidth, kRendererChanged);
  myRendererPopup->appendEntry("Software", 1);
#ifdef PSP
  myRendererPopup->appendEntry("Hardware", 2);
#endif
#ifdef DISPLAY_OPENGL
  myRendererPopup->appendEntry("OpenGL", 3);
#endif
  yoff += kVideoRowHeight + 4;

  // Video filter
  myFilterPopup = new PopUpWidget(this, xoff, yoff, woff, kLineHeight,
                                  "GL Filter: ", labelWidth);
  myFilterPopup->appendEntry("Linear", 1);
  myFilterPopup->appendEntry("Nearest", 2);
  yoff += kVideoRowHeight + 4;

  // Aspect ratio
  myAspectRatioSlider = new SliderWidget(this, xoff, yoff, woff - 14, kLineHeight,
                                         "GL Aspect: ", labelWidth, kAspectRatioChanged);
  myAspectRatioSlider->setMinValue(1); myAspectRatioSlider->setMaxValue(100);
  myAspectRatioLabel = new StaticTextWidget(this, xoff + woff - 11, yoff, 15, kLineHeight,
                       "", kTextAlignLeft);
  myAspectRatioLabel->setFlags(WIDGET_CLEARBG);
  yoff += kVideoRowHeight + 4;

  // Palette
  myPalettePopup = new PopUpWidget(this, xoff, yoff, woff, kLineHeight, "Palette: ", labelWidth);
  myPalettePopup->appendEntry("Standard", 1);
  myPalettePopup->appendEntry("Original", 2);
  myPalettePopup->appendEntry("Z26", 3);
  yoff += kVideoRowHeight + 4;

  // Move over to the next column
  yoff = 10;
  xoff = xoff + 115;

  // Framerate
  myFrameRateSlider = new SliderWidget(this, xoff, yoff, woff - 25, kLineHeight,
                                       "Framerate: ", labelWidth, kFrameRateChanged);
  myFrameRateSlider->setMinValue(1); myFrameRateSlider->setMaxValue(300);
  myFrameRateLabel = new StaticTextWidget(this, xoff + woff - 22, yoff, 20, kLineHeight,
                                          "", kTextAlignLeft);
  myFrameRateLabel->setFlags(WIDGET_CLEARBG);
  yoff += kVideoRowHeight + 4;

  // Zoom level
  myZoomSlider = new SliderWidget(this, xoff, yoff, woff - 25, kLineHeight,
                                  "Zoom: ", labelWidth, kZoomChanged);
  myZoomSlider->setMinValue(0); myZoomSlider->setMaxValue(50);
  myZoomLabel = new StaticTextWidget(this, xoff + woff - 22, yoff, 20, kLineHeight,
                                     "", kTextAlignLeft);
  myZoomLabel->setFlags(WIDGET_CLEARBG);
  yoff += kVideoRowHeight + 10;

  myFullscreenCheckbox = new CheckboxWidget(this, font, xoff + 5, yoff,
                                            "Fullscreen mode");
  yoff += kVideoRowHeight + 4;

  myUseDeskResCheckbox = new CheckboxWidget(this, font, xoff + 5, yoff,
                                            "Desktop Res in FS");
  yoff += kVideoRowHeight + 20;

  // Add Defaults, OK and Cancel buttons
  addButton( 10, _h - 24, "Defaults", kDefaultsCmd, 0);
#ifndef MAC_OSX
  addButton(_w - 2 * (kButtonWidth + 7), _h - 24, "OK", kOKCmd, 0);
  addButton(_w - (kButtonWidth + 10), _h - 24, "Cancel", kCloseCmd, 0);
#else
  addButton(_w - 2 * (kButtonWidth + 7), _h - 24, "Cancel", kCloseCmd, 0);
  addButton(_w - (kButtonWidth + 10), _h - 24, "OK", kOKCmd, 0);
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
  double f;

  // Driver setting
  b = instance()->settings().getBool("dirtyrects");
  i = b ? 1 : 2;
  myDirtyPopup->setSelectedTag(i);

  // Renderer setting
  s = instance()->settings().getString("video");
  if(s == "soft")
    myRendererPopup->setSelectedTag(1);
  else if(s == "hard")
    myRendererPopup->setSelectedTag(2);
  else if(s == "gl")
    myRendererPopup->setSelectedTag(3);
  else
    myRendererPopup->setSelectedTag(1);

  // Filter setting
  s = instance()->settings().getString("gl_filter");
  if(s == "linear")
    myFilterPopup->setSelectedTag(1);
  else if(s == "nearest")
    myFilterPopup->setSelectedTag(2);

  // Aspect ratio - another huge hack
  s = instance()->settings().getString("gl_aspect");
  f = instance()->settings().getFloat("gl_aspect");
  if(f < 1.1)
  {
    f = 1.1;
    s = "1.1";
  }
  else if(f > 2.0)
  {
    f = 2.0;
    s = "2.0";
  }
  i = (int)((f * 10) - 10) * 10;
  myAspectRatioSlider->setValue(i);
  myAspectRatioLabel->setLabel(s);

  // Palette
  s = instance()->settings().getString("palette");
  if(s == "standard")
    myPalettePopup->setSelectedTag(1);
  else if(s == "original")
    myPalettePopup->setSelectedTag(2);
  else if(s == "z26")
    myPalettePopup->setSelectedTag(3);

  // Zoom
  i = (instance()->settings().getInt("zoom") - 1) * 10;
  myZoomSlider->setValue(i);
  myZoomLabel->setLabel(instance()->settings().getString("zoom"));

  // Fullscreen
  b = instance()->settings().getBool("fullscreen");
  myFullscreenCheckbox->setState(b);

  // Use desktop resolution in fullscreen mode
  b = instance()->settings().getBool("gl_fsmax");
  myUseDeskResCheckbox->setState(b);

  // Make sure that mutually-exclusive items are not enabled at the same time
  i = myRendererPopup->getSelectedTag() - 1;
  handleRendererChange(i);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VideoDialog::saveConfig()
{
  string s;
  int i;
  bool b, restart = false;

  // Dirty rectangle updates
  i = myDirtyPopup->getSelectedTag();
  b = (i == 1) ? 1 : 0;
  if(b != instance()->settings().getBool("dirtyrects"))
  {
    instance()->settings().setBool("dirtyrects", b);
    restart = true;
  }

  // Renderer setting
  i = myRendererPopup->getSelectedTag();
  if(i == 1)
    s = "soft";
  else if(i == 2)
    s = "hard";
  else if(i == 3)
    s = "gl";
  if(s != instance()->settings().getString("video"))
  {
    instance()->settings().setString("video", s);
    restart = true;
  }

  // Filter setting
  i = myFilterPopup->getSelectedTag();
  if(i == 1)
    s = "linear";
  else if(i == 2)
    s = "nearest";
  if(s != instance()->settings().getString("gl_filter"))
  {
    instance()->settings().setString("gl_filter", s);
    restart = true;
  }

  // Aspect ratio
  s = myAspectRatioLabel->getLabel();
  if(s != instance()->settings().getString("gl_aspect"))
  {
    instance()->settings().setString("gl_aspect", s);
    restart = true;
  }

  // Palette
  i = myPalettePopup->getSelectedTag();
  if(i == 1)
    instance()->settings().setString("palette", "standard");
  else if(i == 2)
    instance()->settings().setString("palette", "original");
  else if(i == 3)
    instance()->settings().setString("palette", "z26");
  s = myPalettePopup->getSelectedString();
  instance()->settings().setString("palette", s);
  instance()->console().togglePalette(s);

  // Framerate
  i = myFrameRateSlider->getValue();
  if(i > 0)
    instance()->setFramerate(i);

  // Zoom
  i = (myZoomSlider->getValue() / 10) + 1;
  instance()->settings().setInt("zoom", i);
  instance()->frameBuffer().resize(GivenSize, i);

  // Fullscreen (the setFullscreen method takes care of updating settings)
  b = myFullscreenCheckbox->getState();
  instance()->frameBuffer().setFullscreen(b);

  // Use desktop resolution in fullscreen mode
  b = myUseDeskResCheckbox->getState();
  if(b != instance()->settings().getBool("gl_fsmax"))
  {
    instance()->settings().setBool("gl_fsmax", b);
    restart = true;
  }

  // Finally, issue a complete framebuffer re-initialization
  // Not all options may require a full re-initialization, so we only
  // do it when necessary
  if(restart)
    instance()->createFrameBuffer();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VideoDialog::setDefaults()
{
  myDirtyPopup->setSelectedTag(1);
  myRendererPopup->setSelectedTag(1);
  myFilterPopup->setSelectedTag(1);
  myPalettePopup->setSelectedTag(1);
  myFrameRateSlider->setValue(0);
  myFrameRateLabel->setLabel("0");

  // For some unknown reason (ie, a bug), slider widgets can only
  // take certain ranges of numbers.  So we have to fudge things ...
  myZoomSlider->setValue(10);
  myZoomLabel->setLabel("2");
  myAspectRatioSlider->setValue(100);
  myAspectRatioLabel->setLabel("2.0");

  myFullscreenCheckbox->setState(false);
  myUseDeskResCheckbox->setState(true);

  // Make sure that mutually-exclusive items are not enabled at the same time
  handleRendererChange(0);  // 0 indicates software mode
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VideoDialog::handleRendererChange(int item)
{
  // When we're in software mode, certain OpenGL-related options are disabled
  bool active = (item == 0 || item == 1) ? false : true;

  myFilterPopup->setEnabled(active);
  myAspectRatioSlider->setEnabled(active);
  myAspectRatioLabel->setEnabled(active);
  myUseDeskResCheckbox->setEnabled(active);

  // Also, in OpenGL mode, certain software related items are disabled
  myDirtyPopup->setEnabled(!active);
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

    case kZoomChanged:
      myZoomLabel->setValue((myZoomSlider->getValue() + 10) / 10);
      break;

    default:
      Dialog::handleCommand(sender, cmd, data, 0);
      break;
  }
}
