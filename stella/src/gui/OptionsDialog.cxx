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
// $Id: OptionsDialog.cxx,v 1.64 2008-03-12 22:04:53 stephena Exp $
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
#include "UIDialog.hxx"
#include "FileSnapDialog.hxx"
#include "GameInfoDialog.hxx"
#include "HelpDialog.hxx"
#include "AboutDialog.hxx"
#include "OptionsDialog.hxx"
#include "Launcher.hxx"

#ifdef CHEATCODE_SUPPORT
  #include "CheatCodeDialog.hxx"
#endif

#include "bspf.hxx"

#define addBigButton(label, cmd) \
	new ButtonWidget(this, font, xoffset, yoffset, kBigButtonWidth, 18, label, cmd); yoffset += kRowHeight

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
OptionsDialog::OptionsDialog(OSystem* osystem, DialogContainer* parent,
                             GuiObject* boss, bool global)
  : Dialog(osystem, parent, 0, 0, kMainMenuWidth, kMainMenuHeight),
    myVideoDialog(NULL),
    myAudioDialog(NULL),
    myInputDialog(NULL),
    myUIDialog(NULL),
    myFileSnapDialog(NULL),
    myGameInfoDialog(NULL),
    myCheatCodeDialog(NULL),
    myHelpDialog(NULL),
    myAboutDialog(NULL),
    myIsGlobal(global)
{
  const GUI::Font& font = instance()->font();
  int xoffset = 10, yoffset = 10;
  WidgetArray wid;
  ButtonWidget* b = NULL;

  myVideoSettingsButton = addBigButton("Video Settings", kVidCmd);
  wid.push_back(myVideoSettingsButton);

  myAudioSettingsButton = addBigButton("Audio Settings", kAudCmd);
#ifndef SOUND_SUPPORT
  myAudioSettingsButton->clearFlags(WIDGET_ENABLED);
#endif
  wid.push_back(myAudioSettingsButton);

  b = addBigButton("Input Settings", kInptCmd);
  wid.push_back(b);

  myUIButton = addBigButton("UI Settings", kUsrIfaceCmd);
  wid.push_back(myUIButton);

  myFileSnapButton = addBigButton("Config Files", kFileSnapCmd);
  wid.push_back(myFileSnapButton);

  // Move to second column
  xoffset += kBigButtonWidth + 10;  yoffset = 10;

  myGameInfoButton = addBigButton("Game Properties", kInfoCmd);
  wid.push_back(myGameInfoButton);

  myCheatCodeButton = addBigButton("Cheat Code", kCheatCmd);
#ifndef CHEATCODE_SUPPORT
  myCheatCodeButton->clearFlags(WIDGET_ENABLED);
#endif
  wid.push_back(myCheatCodeButton);

  myHelpButton = addBigButton("Help", kHelpCmd);
  wid.push_back(myHelpButton);

  myAboutButton = addBigButton("About", kAboutCmd);
  wid.push_back(myAboutButton);

  b = addBigButton("Exit Menu", kExitCmd);
  wid.push_back(b);
  addCancelWidget(b);

  // Set some sane values for the dialog boxes
  int x = 0, y = 0, w, h;

  // Now create all the dialogs attached to each menu button
  w = 230; h = 185;
  myVideoDialog = new VideoDialog(myOSystem, parent, font, x, y, w, h);

  w = 200; h = 140;
  myAudioDialog = new AudioDialog(myOSystem, parent, font, x, y, w, h);

#ifdef _WIN32_WCE
  // we scale the input dialog down a bit in low res devices.
  // looks only a little ugly, but the functionality is very welcome
  if(myOSystem->desktopWidth() < 320) { w = 220; h = 176; }
  else                                { w = 230; h = 185; }
#else
  w = 230; h = 185;
#endif
  myInputDialog = new InputDialog(myOSystem, parent, font, x, y, w, h);

  w = 200; h = 155;
  myUIDialog = new UIDialog(myOSystem, parent, font, x, y, w, h);

  w = 280; h = 180;
  myFileSnapDialog = new FileSnapDialog(myOSystem, parent, font,
                                        boss, x, y, w, h);

  w = 255; h = 190;
  myGameInfoDialog = new GameInfoDialog(myOSystem, parent, font, this, x, y, w, h);

#ifdef CHEATCODE_SUPPORT
  w = 230; h = 150;
  myCheatCodeDialog = new CheatCodeDialog(myOSystem, parent, font, x, y, w, h);
#endif

  w = 255; h = 150;
  myHelpDialog = new HelpDialog(myOSystem, parent, font, x, y, w, h);

  w = 255; h = 150;
  myAboutDialog = new AboutDialog(myOSystem, parent, font, x, y, w, h);

  addToFocusList(wid);

  // Certain buttons are disabled depending on mode
  if(myIsGlobal)
  {
//    myGameInfoButton->clearFlags(WIDGET_ENABLED);
    myCheatCodeButton->clearFlags(WIDGET_ENABLED);
  }

#ifdef _WIN32_WCE
  myAudioSettingsButton->clearFlags(WIDGET_ENABLED);  // not honored in wince port
#endif
  if(myOSystem->desktopWidth() < 320)
  {
    // These cannot be displayed in low res devices
    myVideoSettingsButton->clearFlags(WIDGET_ENABLED);
    myFileSnapButton->clearFlags(WIDGET_ENABLED);
    myGameInfoButton->clearFlags(WIDGET_ENABLED);
    myHelpButton->clearFlags(WIDGET_ENABLED);
    myAboutButton->clearFlags(WIDGET_ENABLED);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
OptionsDialog::~OptionsDialog()
{
  delete myVideoDialog;
  delete myAudioDialog;
  delete myInputDialog;
  delete myUIDialog;
  delete myFileSnapDialog;
  delete myGameInfoDialog;
#ifdef CHEATCODE_SUPPORT
  delete myCheatCodeDialog;
#endif
  delete myHelpDialog;
  delete myAboutDialog;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OptionsDialog::loadConfig()
{
  // Determine whether we should show the 'Game Information' button
  // We always show it in emulation mode, or if a valid ROM is selected
  // in launcher mode
  switch(instance()->eventHandler().state())
  {
    case EventHandler::S_EMULATE:
      myGameInfoButton->setFlags(WIDGET_ENABLED);
      break;
    case EventHandler::S_LAUNCHER:
      if(instance()->launcher().selectedRomMD5() != "")
        myGameInfoButton->setFlags(WIDGET_ENABLED);
      else
        myGameInfoButton->clearFlags(WIDGET_ENABLED);
      break;
    default:
      break;
  }
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

    case kUsrIfaceCmd:
      parent()->addDialog(myUIDialog);
      break;

    case kFileSnapCmd:
      parent()->addDialog(myFileSnapDialog);
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
      if(myIsGlobal)
        close();
      else
        instance()->eventHandler().leaveMenuMode();
      break;

    default:
      Dialog::handleCommand(sender, cmd, data, 0);
  }
}
