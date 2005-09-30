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
// $Id: OptionsDialog.cxx,v 1.31 2005-09-30 00:40:34 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#include "OSystem.hxx"
#include "Dialog.hxx"
#include "DialogContainer.hxx"
#include "Widget.hxx"
#include "Control.hxx"
#include "VideoDialog.hxx"
#include "AudioDialog.hxx"
#include "EventMappingDialog.hxx"
#include "GameInfoDialog.hxx"
#include "HelpDialog.hxx"
#include "AboutDialog.hxx"
#include "OptionsDialog.hxx"

#include "bspf.hxx"

enum {
  kVidCmd   = 'VIDO',
  kAudCmd   = 'AUDO',
  kEMapCmd  = 'EMAP',
  kInfoCmd  = 'INFO',
  kHelpCmd  = 'HELP',
  kAboutCmd = 'ABOU',
  kExitCmd  = 'EXIM',
  kCheatCmd = 'CHET'
};

enum {
  kRowHeight      = 22,
  kBigButtonWidth = 90,
  kMainMenuWidth  = (kBigButtonWidth + 2 * 8),
  kMainMenuHeight = 8 * kRowHeight + 10,
};

#define addBigButton(label, cmd, hotkey) \
	new ButtonWidget(this, xoffset, yoffset, kBigButtonWidth, 18, label, cmd, hotkey); yoffset += kRowHeight

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
OptionsDialog::OptionsDialog(OSystem* osystem, DialogContainer* parent)
    : Dialog(osystem, parent,
            (osystem->frameBuffer().baseWidth() - kMainMenuWidth) / 2,
            (osystem->frameBuffer().baseHeight() - kMainMenuHeight)/2,
            kMainMenuWidth, kMainMenuHeight),
      myVideoDialog(NULL),
      myAudioDialog(NULL),
      myEventMappingDialog(NULL),
      myGameInfoDialog(NULL),
      myCheatCodeDialog(NULL),
      myHelpDialog(NULL),
      myAboutDialog(NULL)
{
  int yoffset = 7;
  const int xoffset = (_w - kBigButtonWidth) / 2;

  addBigButton("Video Settings", kVidCmd, 0);
#ifdef SOUND_SUPPORT
  addBigButton("Audio Settings", kAudCmd, 0);
#else
  ButtonWidget* b = addBigButton("Audio Settings", kAudCmd, 0);
  b->clearFlags(WIDGET_ENABLED);
#endif
  addBigButton("Event Mapping", kEMapCmd, 0);
  addBigButton("Game Properties", kInfoCmd, 0);
  addBigButton("Cheat Code", kCheatCmd, 0);
  addBigButton("Help", kHelpCmd, 0);
  addBigButton("About", kAboutCmd, 0);
  addBigButton("Exit Menu", kExitCmd, 0);

  // Set some sane values for the dialog boxes
  int fbWidth  = osystem->frameBuffer().baseWidth();
  int fbHeight = osystem->frameBuffer().baseHeight();
  int x, y, w, h;

  // Now create all the dialogs attached to each menu button
  w = 230; h = 130;
  checkBounds(fbWidth, fbHeight, &x, &y, &w, &h);
  myVideoDialog = new VideoDialog(myOSystem, parent, x, y, w, h);

  w = 200; h = 110;
  checkBounds(fbWidth, fbHeight, &x, &y, &w, &h);
  myAudioDialog = new AudioDialog(myOSystem, parent, x, y, w, h);

  w = 230; h = 170;
  checkBounds(fbWidth, fbHeight, &x, &y, &w, &h);
  myEventMappingDialog = new EventMappingDialog(myOSystem, parent, x, y, w, h);

  w = 255; h = 175;
  checkBounds(fbWidth, fbHeight, &x, &y, &w, &h);
  myGameInfoDialog = new GameInfoDialog(myOSystem, parent, this, x, y, w, h);

  w = 140; h = 40;
  checkBounds(fbWidth, fbHeight, &x, &y, &w, &h);
  myCheatCodeDialog = new CheatCodeDialog(myOSystem, parent, x, y, w, h);

  w = 255; h = 150;
  checkBounds(fbWidth, fbHeight, &x, &y, &w, &h);
  myHelpDialog = new HelpDialog(myOSystem, parent, x, y, w, h);

  w = 255; h = 150;
  checkBounds(fbWidth, fbHeight, &x, &y, &w, &h);
  myAboutDialog = new AboutDialog(myOSystem, parent, x, y, w, h);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
OptionsDialog::~OptionsDialog()
{
  delete myVideoDialog;
  delete myAudioDialog;
  delete myEventMappingDialog;
  delete myGameInfoDialog;
  delete myCheatCodeDialog;
  delete myHelpDialog;
  delete myAboutDialog;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OptionsDialog::checkBounds(int width, int height,
                                int* x, int* y, int* w, int* h)
{
  if(*w > width) *w = width;
  if(*h > height) *h = height;
  *x = (width - *w) / 2;
  *y = (height - *h) / 2;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OptionsDialog::handleCommand(CommandSender* sender, int cmd,
                                  int data, int id)
{
  switch(cmd)
  {
    case kVidCmd:
      parent()->addDialog(myVideoDialog);
      break;

    case kAudCmd:
      parent()->addDialog(myAudioDialog);
      break;

    case kEMapCmd:
      parent()->addDialog(myEventMappingDialog);
      break;

    case kInfoCmd:
      parent()->addDialog(myGameInfoDialog);
      break;

    case kCheatCmd:
      parent()->addDialog(myCheatCodeDialog);
      break;

    case kHelpCmd:
      parent()->addDialog(myHelpDialog);
      break;

    case kAboutCmd:
      parent()->addDialog(myAboutDialog);
      break;

    case kExitCmd:
      instance()->eventHandler().leaveMenuMode();
      break;

    default:
      Dialog::handleCommand(sender, cmd, data, 0);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OptionsDialog::enterCheatMode()
{
  parent()->addDialog(myCheatCodeDialog);
}
