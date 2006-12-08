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
// $Id: OptionsDialog.cxx,v 1.45 2006-12-08 16:49:36 stephena Exp $
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
#include "InputDialog.hxx"
#include "GameInfoDialog.hxx"
#include "HelpDialog.hxx"
#include "AboutDialog.hxx"
#include "OptionsDialog.hxx"

#ifdef CHEATCODE_SUPPORT
  #include "CheatCodeDialog.hxx"
#endif

#include "bspf.hxx"

enum {
  kVidCmd   = 'VIDO',
  kAudCmd   = 'AUDO',
  kInptCmd  = 'INPT',
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

#define addBigButton(label, cmd) \
	new ButtonWidget(this, font, xoffset, yoffset, kBigButtonWidth, 18, label, cmd); yoffset += kRowHeight

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
OptionsDialog::OptionsDialog(OSystem* osystem, DialogContainer* parent)
  : Dialog(osystem, parent, 0, 0, kMainMenuWidth, kMainMenuHeight),
    myVideoDialog(NULL),
    myAudioDialog(NULL),
    myInputDialog(NULL),
    myGameInfoDialog(NULL),
    myCheatCodeDialog(NULL),
    myHelpDialog(NULL),
    myAboutDialog(NULL)
{
  int yoffset = 7;
  const int xoffset = (_w - kBigButtonWidth) / 2;
  const int fbWidth  = osystem->frameBuffer().baseWidth(),
            fbHeight = osystem->frameBuffer().baseHeight();
  const GUI::Font& font = instance()->font(); // FIXME - change reference to optionsFont()
  WidgetArray wid;
  ButtonWidget* b = NULL;

  // Set actual dialog dimensions
  _x = (fbWidth - kMainMenuWidth) / 2;
  _y = (fbHeight - kMainMenuHeight) / 2;

  b = addBigButton("Video Settings", kVidCmd);
  wid.push_back(b);

  b = addBigButton("Audio Settings", kAudCmd);
#ifndef SOUND_SUPPORT
  b->clearFlags(WIDGET_ENABLED);
#endif
  wid.push_back(b);

  b = addBigButton("Input Settings", kInptCmd);
  wid.push_back(b);

  b = addBigButton("Game Properties", kInfoCmd);
  wid.push_back(b);

  b = addBigButton("Cheat Code", kCheatCmd);
#ifndef CHEATCODE_SUPPORT
  b->clearFlags(WIDGET_ENABLED);
#endif
  wid.push_back(b);

  b = addBigButton("Help", kHelpCmd);
  wid.push_back(b);

  b = addBigButton("About", kAboutCmd);
  wid.push_back(b);

  b = addBigButton("Exit Menu", kExitCmd);
  wid.push_back(b);
  addCancelWidget(b);

  // Set some sane values for the dialog boxes
  int x, y, w, h;

  // Now create all the dialogs attached to each menu button
  w = 230; h = 135;
  checkBounds(fbWidth, fbHeight, &x, &y, &w, &h);
  myVideoDialog = new VideoDialog(myOSystem, parent, font, x, y, w, h);

  w = 200; h = 140;
  checkBounds(fbWidth, fbHeight, &x, &y, &w, &h);
  myAudioDialog = new AudioDialog(myOSystem, parent, font, x, y, w, h);

  w = 230; h = 185;
  checkBounds(fbWidth, fbHeight, &x, &y, &w, &h);
  myInputDialog = new InputDialog(myOSystem, parent, font, x, y, w, h);

  w = 255; h = 190;
  checkBounds(fbWidth, fbHeight, &x, &y, &w, &h);
  myGameInfoDialog = new GameInfoDialog(myOSystem, parent, font, this, x, y, w, h);

#ifdef CHEATCODE_SUPPORT
  w = 230; h = 150;
  checkBounds(fbWidth, fbHeight, &x, &y, &w, &h);
  myCheatCodeDialog = new CheatCodeDialog(myOSystem, parent, font, x, y, w, h);
#endif

  w = 255; h = 150;
  checkBounds(fbWidth, fbHeight, &x, &y, &w, &h);
  myHelpDialog = new HelpDialog(myOSystem, parent, font, x, y, w, h);

  w = 255; h = 150;
  checkBounds(fbWidth, fbHeight, &x, &y, &w, &h);
  myAboutDialog = new AboutDialog(myOSystem, parent, font, x, y, w, h);

  addToFocusList(wid);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
OptionsDialog::~OptionsDialog()
{
  delete myVideoDialog;
  delete myAudioDialog;
  delete myInputDialog;
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

    case kInptCmd:
      parent()->addDialog(myInputDialog);
      break;

    case kInfoCmd:
      parent()->addDialog(myGameInfoDialog);
      break;

#ifdef CHEATCODE_SUPPORT
    case kCheatCmd:
      parent()->addDialog(myCheatCodeDialog);
      break;
#endif

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
