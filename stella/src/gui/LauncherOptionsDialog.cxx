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
// $Id: LauncherOptionsDialog.cxx,v 1.2 2005-05-13 18:28:05 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#include "DialogContainer.hxx"
#include "BrowserDialog.hxx"
#include "PopUpWidget.hxx"
#include "TabWidget.hxx"
#include "FSNode.hxx"
#include "bspf.hxx"
#include "LauncherOptionsDialog.hxx"

enum {
  kChooseRomDirCmd  = 'roms',  // rom select
  kChooseSnapDirCmd = 'snps',  // snap select
  kRomDirChosenCmd  = 'romc',  // rom chosen
  kSnapDirChosenCmd = 'snpc'   // snap chosen
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
LauncherOptionsDialog::LauncherOptionsDialog(
      OSystem* osystem, DialogContainer* parent,
      int x, int y, int w, int h)
  : Dialog(osystem, parent, x, y, w, h),
    myBrowser(NULL)
{
  const int vBorder = 4;
  int yoffset;

  // The tab widget
  TabWidget* tab = new TabWidget(this, 0, vBorder, _w, _h - 24 - 2 * vBorder);

  // 1) The ROM locations tab
  tab->addTab("ROM Settings");
  yoffset = vBorder;

  // ROM path
  new ButtonWidget(tab, 5, yoffset, kButtonWidth + 14, 16, "Path", kChooseRomDirCmd, 0);
  myRomPath = new StaticTextWidget(tab, 5 + kButtonWidth + 20, yoffset + 3, _w - (5 + kButtonWidth + 20) - 10, kLineHeight, "xxx", kTextAlignLeft);

  // 2) The snapshot settings tab
  tab->addTab("Snapshot Settings");
  yoffset = vBorder;

  // Save game path
  new ButtonWidget(tab, 5, yoffset, kButtonWidth + 14, 16, "Path", kChooseSnapDirCmd, 0);
  mySnapPath = new StaticTextWidget(tab, 5 + kButtonWidth + 20, yoffset + 3, _w - (5 + kButtonWidth + 20) - 10, kLineHeight, "yyy", kTextAlignLeft);
  yoffset += 18;

// FIXME - add other snapshot stuff
	

  // Activate the first tab
  tab->setActiveTab(0);

  // Add OK & Cancel buttons
  addButton(_w - 2 *(kButtonWidth + 10), _h - 24, "OK", kOKCmd, 0);
  addButton(_w - (kButtonWidth + 10), _h - 24, "Cancel", kCloseCmd, 0);

  // Create file browser dialog
  myBrowser = new BrowserDialog(this, 20, 20, _w - 40, _h - 40);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
LauncherOptionsDialog::~LauncherOptionsDialog()
{
  delete myBrowser;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherOptionsDialog::loadConfig()
{
  string romdir = instance()->settings().getString("romdir");
  string snapdir = instance()->settings().getString("ssdir");

  myRomPath->setLabel(romdir);
  mySnapPath->setLabel(snapdir);
/*
  string s;
  bool b;
  int i;
  float f;

  // Driver setting
  myDriverPopup->setSelectedTag(0); // FIXME

  // Renderer setting
  s = instance()->settings().getString("video");
  if(s == "soft")
    myRendererPopup->setSelectedTag(1);
  else if(s == "gl")
    myRendererPopup->setSelectedTag(2);

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

  // Framerate
  myFrameRateSlider->setValue(instance()->settings().getInt("framerate"));
  myFrameRateLabel->setLabel(instance()->settings().getString("framerate"));

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
*/
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherOptionsDialog::saveConfig()
{
cerr << "LauncherOptionsDialog::saveConfig()\n";
/*
  string s;
  int i;
  bool b, restart = false;

  // Driver setting
  s = myDriverPopup->getSelectedString();
  if(s != instance()->settings().getString("video_driver"))
  {
    instance()->settings().setString("video_driver", s);
    restart = true;
  }

  // Renderer setting
  i = myRendererPopup->getSelectedTag();
  if(i == 1)
    s = "soft";
  else if(i == 2)
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
*/
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherOptionsDialog::openRomBrowser()
{
  myBrowser->setTitle("Select ROM directory:");
  myBrowser->setEmitSignal(kRomDirChosenCmd);
  myBrowser->setStartPath(myRomPath->getLabel());

  parent()->addDialog(myBrowser);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherOptionsDialog::openSnapBrowser()
{
  myBrowser->setTitle("Select snapshot directory:");
  myBrowser->setEmitSignal(kSnapDirChosenCmd);
  myBrowser->setStartPath(mySnapPath->getLabel());

  parent()->addDialog(myBrowser);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherOptionsDialog::handleCommand(CommandSender* sender, int cmd, int data)
{
  switch (cmd)
  {
    case kOKCmd:
      saveConfig();
      close();
      break;

    case kChooseRomDirCmd:
      openRomBrowser();
      break;

    case kChooseSnapDirCmd:
      openSnapBrowser();
      break;

    case kRomDirChosenCmd:
    {
      FilesystemNode dir(myBrowser->getResult());
      myRomPath->setLabel(dir.path());
      break;
    }

    case kSnapDirChosenCmd:
    {
      FilesystemNode dir(myBrowser->getResult());
      mySnapPath->setLabel(dir.path());
      break;
    }

    default:
      Dialog::handleCommand(sender, cmd, data);
      break;
  }
}
