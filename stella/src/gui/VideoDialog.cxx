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
// Copyright (c) 1995-2005 by Bradford W. Mott
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: VideoDialog.cxx,v 1.1 2005-03-14 04:08:15 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

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
VideoDialog::VideoDialog(OSystem* osystem, uInt16 x, uInt16 y, uInt16 w, uInt16 h)
    : Dialog(osystem, x, y, w, h),
      myDriver(NULL)
{
  int yoff = 10;
  const int xoff = 2;
  const int woff = _w - 120;
  const int labelWidth = 65;

  // Video driver (query FrameBuffer for what's supported)
  myDriver = new PopUpWidget(this, xoff, yoff, woff, kLineHeight, "(*)Driver: ", labelWidth);
  myDriver->appendEntry("<default>", 0);
  myDriver->appendEntry("");
  myDriver->appendEntry("First one", 1);
  myDriver->appendEntry("Another one", 2);
  yoff += kVideoRowHeight + 4;

  // Video renderer
  myRenderer = new PopUpWidget(this, xoff, yoff, woff, kLineHeight, "(*)Renderer: ", labelWidth);
  myRenderer->appendEntry("<default>", 0);
  myRenderer->appendEntry("");
  myRenderer->appendEntry("Software", 1);
  myRenderer->appendEntry("OpenGL", 2);
  yoff += kVideoRowHeight + 4;

  // Video filter
  myFilter = new PopUpWidget(this, xoff, yoff, woff, kLineHeight, "Filter: ", labelWidth);
  myFilter->appendEntry("<default>", 0);
  myFilter->appendEntry("");
  myFilter->appendEntry("Linear", 1);
  myFilter->appendEntry("Nearest", 2);
  yoff += kVideoRowHeight + 4;

  // Aspect ratio   FIXME - maybe this should be a slider ??
  myAspectRatio = new PopUpWidget(this, xoff, yoff, woff, kLineHeight, "(*)Aspect: ", labelWidth);
  myAspectRatio->appendEntry("<default>", 0);
  myAspectRatio->appendEntry("");
  myAspectRatio->appendEntry("1.0", 1);
  myAspectRatio->appendEntry("1.1", 2);
  myAspectRatio->appendEntry("1.2", 3);
  myAspectRatio->appendEntry("1.3", 4);
  myAspectRatio->appendEntry("1.4", 5);
/*
  myAspectRatio->appendEntry("1.5", 6);
  myAspectRatio->appendEntry("1.6", 7);
  myAspectRatio->appendEntry("1.7", 8);
  myAspectRatio->appendEntry("1.8", 9);
  myAspectRatio->appendEntry("1.9", 10);
  myAspectRatio->appendEntry("2.0", 11);
*/
  yoff += kVideoRowHeight + 4;

  // Palette
  myPalette = new PopUpWidget(this, xoff, yoff, woff, kLineHeight, "Palette: ", labelWidth);
  myPalette->appendEntry("<default>", 0);
  myPalette->appendEntry("");
  myPalette->appendEntry("Standard", 1);
  myPalette->appendEntry("Original", 2);
  myPalette->appendEntry("Z26", 3);
  yoff += kVideoRowHeight + 4;

  // Add OK & Cancel buttons
#ifndef MAC_OSX
  addButton(_w - 2 * (kButtonWidth + 10), _h - 24, "OK", kOKCmd, 0);
  addButton(_w - (kButtonWidth + 10), _h - 24, "Cancel", kCloseCmd, 0);
#else
  addButton(_w - 2 * (kButtonWidth + 10), _h - 24, "OK", kOKCmd, 0);
  addButton(_w - (kButtonWidth + 10), _h - 24, "Cancel", kCloseCmd, 0);
#endif

  // Set the items according to current settings
  loadConfig();

// FIXME - get list of video drivers from OSystem
//  const Common::LanguageDescription *l = Common::g_languages;
//  for (; l->code; ++l) {
//    _langPopUp->appendEntry(l->description, l->id);
//  }
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
  uInt32 i;

  // Driver setting
  myDriver->setSelectedTag(0); // FIXME

  // Renderer setting
  s = instance()->settings().getString("video");
  if(s == "soft")
    myRenderer->setSelectedTag(1);
  else if(s == "gl")
    myRenderer->setSelectedTag(2);
  else
    myRenderer->setSelectedTag(0);

  // Filter setting
  s = instance()->settings().getString("gl_filter");
  if(s == "linear")
    myFilter->setSelectedTag(1);
  else if(s == "nearest")
    myFilter->setSelectedTag(2);
  else
    myFilter->setSelectedTag(0);

  // Aspect ratio
  s = instance()->settings().getString("gl_aspect");
  // TODO

  // Palette
  // Filter setting
  s = instance()->settings().getString("palette");
  if(s == "standard")
    myPalette->setSelectedTag(1);
  else if(s == "original")
    myPalette->setSelectedTag(2);
  else if(s == "z26")
    myPalette->setSelectedTag(3);
  else
    myPalette->setSelectedTag(0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VideoDialog::saveConfig()
{
cerr << "VideoDialog::saveConfig()\n";
  string s;

  // Palette
  s = myPalette->getSelectedString();
  instance()->settings().setString("palette", s);
  instance()->console().togglePalette(s);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VideoDialog::handleCommand(CommandSender* sender, uInt32 cmd, uInt32 data)
{
  switch(cmd)
  {
    case kOKCmd:
      saveConfig();
      close();
      break;

    default:
      Dialog::handleCommand(sender, cmd, data);
      break;
  }
}
