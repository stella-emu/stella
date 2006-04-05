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
// $Id: OptionsDialog.cxx,v 1.38 2006-04-05 12:28:39 stephena Exp $
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

#define addBigButton(label, cmd, hotkey) \
	new ButtonWidget(this, font, xoffset, yoffset, kBigButtonWidth, 18, label, cmd, hotkey); yoffset += kRowHeight

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
  // Set actual dialog dimensions
  _x = (osystem->frameBuffer().baseWidth() - kMainMenuWidth) / 2;
  _y = (osystem->frameBuffer().baseHeight() - kMainMenuHeight) / 2;

  int yoffset = 7;
  const int xoffset = (_w - kBigButtonWidth) / 2;
  const GUI::Font& font = instance()->font(); // FIXME - change reference to optionsFont()
  ButtonWidget* b = NULL;

  b = addBigButton("Video Settings", kVidCmd, 0);
#ifdef SOUND_SUPPORT
  addBigButton("Audio Settings", kAudCmd, 0);
#else
  b = addBigButton("Audio Settings", kAudCmd, 0);
  b->clearFlags(WIDGET_ENABLED);
#endif
  addBigButton("Input Settings", kInptCmd, 0);
  addBigButton("Game Properties", kInfoCmd, 0);
#ifdef CHEATCODE_SUPPORT
  addBigButton("Cheat Code", kCheatCmd, 0);
#else
  b = addBigButton("Cheat Code", kCheatCmd, 0);
  b->clearFlags(WIDGET_ENABLED);
#endif
  addBigButton("Help", kHelpCmd, 0);
  addBigButton("About", kAboutCmd, 0);
  addBigButton("Exit Menu", kExitCmd, 0);

  // Set some sane values for the dialog boxes
  int fbWidth  = osystem->frameBuffer().baseWidth();
  int fbHeight = osystem->frameBuffer().baseHeight();
  int x, y, w, h;

  // Now create all the dialogs attached to each menu button
  w = 230; h = 135;
  checkBounds(fbWidth, fbHeight, &x, &y, &w, &h);
  myVideoDialog = new VideoDialog(myOSystem, parent, font, x, y, w, h);

  w = 200; h = 110;
  checkBounds(fbWidth, fbHeight, &x, &y, &w, &h);
  myAudioDialog = new AudioDialog(myOSystem, parent, font, x, y, w, h);

  w = 230; h = 185;
  checkBounds(fbWidth, fbHeight, &x, &y, &w, &h);
  myInputDialog = new InputDialog(myOSystem, parent, font, x, y, w, h);

  w = 255; h = 175;
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
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
OptionsDialog::~OptionsDialog()
{
  delete myVideoDialog;
  delete myAudioDialog;
  delete myInputDialog;
  delete myGameInfoDialog;
#ifdef CHEATCODE_SUPPORT
  delete myCheatCodeDialog;
#endif
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
