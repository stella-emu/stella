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
// $Id: OptionsDialog.cxx,v 1.5 2005-03-14 04:08:15 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#include "OSystem.hxx"
#include "Menu.hxx"
#include "Dialog.hxx"
#include "Widget.hxx"
#include "Control.hxx"
#include "VideoDialog.hxx"
#include "OptionsDialog.hxx"

#include "bspf.hxx"

enum {
  kVidCmd   = 'VIDO',
  kAudCmd   = 'AUDO',
  kEMapCmd  = 'EMAP',
  kMiscCmd  = 'MISC',
  kInfoCmd  = 'INFO',
  kHelpCmd  = 'HELP',
};

enum {
  kRowHeight      = 22,
  kBigButtonWidth = 100,
  kMainMenuWidth  = (kBigButtonWidth + 2 * 8),
  kMainMenuHeight = 6 * kRowHeight + 10,
};

#define addBigButton(label, cmd, hotkey) \
	new ButtonWidget(this, xoffset, yoffset, kBigButtonWidth, 18, label, cmd, hotkey); yoffset += kRowHeight

OptionsDialog::OptionsDialog(OSystem* osystem)
    : Dialog(osystem, (osystem->frameBuffer().width() - kMainMenuWidth) / 2,
                      (osystem->frameBuffer().height() - kMainMenuHeight)/2,
                      kMainMenuWidth, kMainMenuHeight),
      myVideoDialog(NULL)
{
  int yoffset = 7;
  const int xoffset = (_w - kBigButtonWidth) / 2;

  addBigButton("Video Settings", kVidCmd, 'V');
  addBigButton("Audio Settings", kAudCmd, 'A');
  addBigButton("Event Remapping", kEMapCmd, 'E');
  addBigButton("Miscellaneous", kMiscCmd, 'M');
  addBigButton("Game Information", kInfoCmd, 'I');
  addBigButton("Help", kHelpCmd, 'H');

  // Set some sane values for the dialog boxes
  uInt32 fbWidth  = osystem->frameBuffer().width();
  uInt32 fbHeight = osystem->frameBuffer().height();
  uInt16 x, y, w, h;

  // Now create all the dialogs attached to each menu button
  w = 250; h = 150;
  if(w > fbWidth) w = fbWidth;
  if(h > fbHeight) h = fbHeight;
  x = (fbWidth - w) / 2;
  y = (fbHeight - h) / 2;

  myVideoDialog        = new VideoDialog(myOSystem, x, y, w, h);



/*
  myAudioDialog        = new AudioDialog(myOSystem);
  myEventMappingDialog = new EventMappingDialog(myOSystem);
  myMiscDialog         = new MiscDialog(myOSystem);
  myGameInfoDialog     = new GameInfoDialog(myOSystem);
  myHelpDialog         = new HelpDialog(myOSystem);
*/
}

OptionsDialog::~OptionsDialog()
{
  delete myVideoDialog;
/* FIXME
  delete myAudioDialog;
  delete myEventMappingDialog;
  delete myMiscDialog;
  delete myGameInfoDialog;
  delete myHelpDialog;
*/
}

void OptionsDialog::handleCommand(CommandSender* sender, uInt32 cmd, uInt32 data)
{
  switch(cmd)
  {
    case kVidCmd:
      instance()->menu().addDialog(myVideoDialog);
      break;

    case kAudCmd:
//      instance()->menu().addDialog(myAudioDialog);
cerr << "push AudioDialog to top of stack\n";
      break;

    case kEMapCmd:
//      instance()->menu().addDialog(myEventMappingDialog);
cerr << "push EventMappingDialog to top of stack\n";
      break;

    case kMiscCmd:
//      instance()->menu().addDialog(myMiscDialog);
cerr << "push MiscDialog to top of stack\n";
      break;

    case kInfoCmd:
//      instance()->menu().addDialog(myGameInfoDialog);
cerr << "push GameInfoDialog to top of stack\n";
      break;

    case kHelpCmd:
//      instance()->menu().addDialog(myHelpDialog);
cerr << "push HelpDialog to top of stack\n";
      break;

    default:
      Dialog::handleCommand(sender, cmd, data);
  }
}
