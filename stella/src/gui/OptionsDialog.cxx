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
// $Id: OptionsDialog.cxx,v 1.3 2005-03-12 01:47:15 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#include "OSystem.hxx"
#include "Menu.hxx"
#include "Dialog.hxx"
#include "Widget.hxx"
#include "Control.hxx"
#include "OptionsDialog.hxx"

#include "bspf.hxx"

/*
using GUI::CommandSender;
using GUI::StaticTextWidget;
using GUI::kButtonWidth;
using GUI::kCloseCmd;
using GUI::kTextAlignCenter;
using GUI::kTextAlignLeft;
using GUI::WIDGET_ENABLED;

typedef GUI::OptionsDialog GUI_OptionsDialog;
typedef GUI::ChooserDialog GUI_ChooserDialog;
*/

enum {
  kVidCmd   = 'VIDO',
  kAudCmd   = 'AUDO',
  kEMapCmd  = 'EMAP',
  kMiscCmd  = 'MISC',
  kInfoCmd  = 'INFO',
  kHelpCmd  = 'HELP',
};

enum {
  kRowHeight      = 20,
  kBigButtonWidth = 140,
  kMainMenuWidth  = (kBigButtonWidth + 2 * 8),
  kMainMenuHeight = 6 * kRowHeight + 10,
};

#define addBigButton(label, cmd, hotkey) \
	new ButtonWidget(this, x, y, kBigButtonWidth, 16, label, cmd, hotkey); y += kRowHeight

OptionsDialog::OptionsDialog(OSystem* osystem)
    : Dialog(osystem, (320 - kMainMenuWidth) / 2, (200 - kMainMenuHeight)/2, kMainMenuWidth, kMainMenuHeight)
{
cerr << "OptionsDialog::OptionsDialog()\n";

  int y = 7;

  const int x = (_w - kBigButtonWidth) / 2;
  addBigButton("Video Settings", kVidCmd, 'V');
  addBigButton("Audio Settings", kAudCmd, 'A');
  addBigButton("Event Remapping", kEMapCmd, 'E');
  addBigButton("Miscellaneous", kMiscCmd, 'M');
  addBigButton("Game Information", kInfoCmd, 'I');
  addBigButton("Help", kHelpCmd, 'H');

/*
  // Now create all the dialogs attached to each menu button
  myVideoDialog        = new VideoDialog(myOSystem);
  myAudioDialog        = new AudioDialog(myOSystem);
  myEventMappingDialog = new EventMappingDialog(myOSystem);
  myMiscDialog         = new MiscDialog(myOSystem);
  myGameInfoDialog     = new GameInfoDialog(myOSystem);
  myHelpDialog         = new HelpDialog(myOSystem);
*/
}

OptionsDialog::~OptionsDialog()
{
/* FIXME
  delete myVideoDialog;
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
//      instance()->menu().addDialog(myVideoDialog);
cerr << "push VideoDialog to top of stack\n";
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
