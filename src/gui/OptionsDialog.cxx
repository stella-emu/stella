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
// Copyright (c) 1995-2014 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
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
#include "SnapshotDialog.hxx"
#include "ConfigPathDialog.hxx"
#include "RomAuditDialog.hxx"
#include "GameInfoDialog.hxx"
#include "LoggerDialog.hxx"
#include "HelpDialog.hxx"
#include "AboutDialog.hxx"
#include "OptionsDialog.hxx"
#include "Launcher.hxx"

#ifdef CHEATCODE_SUPPORT
  #include "CheatCodeDialog.hxx"
#endif

#include "bspf.hxx"

#define addODButton(label, cmd) \
  new ButtonWidget(this, font, xoffset, yoffset, buttonWidth, buttonHeight, label, cmd); yoffset += rowHeight

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
OptionsDialog::OptionsDialog(OSystem* osystem, DialogContainer* parent,
                             GuiObject* boss, int max_w, int max_h, bool global)
  : Dialog(osystem, parent, 0, 0, 0, 0),
    myVideoDialog(NULL),
    myAudioDialog(NULL),
    myInputDialog(NULL),
    myUIDialog(NULL),
    mySnapshotDialog(NULL),
    myConfigPathDialog(NULL),
    myGameInfoDialog(NULL),
    myCheatCodeDialog(NULL),
    myLoggerDialog(NULL),
    myHelpDialog(NULL),
    myAboutDialog(NULL),
    myIsGlobal(global)
{
  const GUI::Font& font = instance().font();
  const int buttonWidth = font.getStringWidth("Snapshot Settings") + 20,
            buttonHeight = font.getLineHeight() + 6,
            rowHeight = font.getLineHeight() + 10;

  _w = 2 * buttonWidth + 30;
  _h = 7 * rowHeight + 15;

  int xoffset = 10, yoffset = 10;
  WidgetArray wid;
  ButtonWidget* b = NULL;

  b = addODButton("Video Settings", kVidCmd);
  wid.push_back(b);

  b = addODButton("Audio Settings", kAudCmd);
#ifndef SOUND_SUPPORT
  b->clearFlags(WIDGET_ENABLED);
#endif
  wid.push_back(b);

  b = addODButton("Input Settings", kInptCmd);
  wid.push_back(b);

  b = addODButton("UI Settings", kUsrIfaceCmd);
  wid.push_back(b);

  b = addODButton("Snapshot Settings", kSnapCmd);
  wid.push_back(b);

  b = addODButton("Config Paths", kCfgPathsCmd);
  wid.push_back(b);

  myRomAuditButton = addODButton("Audit ROMs", kAuditCmd);
  wid.push_back(myRomAuditButton);

  // Move to second column
  xoffset += buttonWidth + 10;  yoffset = 10;

  myGameInfoButton = addODButton("Game Properties", kInfoCmd);
  wid.push_back(myGameInfoButton);

  myCheatCodeButton = addODButton("Cheat Code", kCheatCmd);
#ifndef CHEATCODE_SUPPORT
  myCheatCodeButton->clearFlags(WIDGET_ENABLED);
#endif
  wid.push_back(myCheatCodeButton);

  b = addODButton("System Logs", kLoggerCmd);
  wid.push_back(b);

  b = addODButton("Help", kHelpCmd);
  wid.push_back(b);

  b = addODButton("About", kAboutCmd);
  wid.push_back(b);

  b = addODButton("Exit Menu", kExitCmd);
  wid.push_back(b);
  addCancelWidget(b);

  // Now create all the dialogs attached to each menu button
  myVideoDialog    = new VideoDialog(osystem, parent, font, max_w, max_h);
  myAudioDialog    = new AudioDialog(osystem, parent, font);
  myInputDialog    = new InputDialog(osystem, parent, font, max_w, max_h);
  myUIDialog       = new UIDialog(osystem, parent, font);
  mySnapshotDialog = new SnapshotDialog(osystem, parent, font, boss, max_w, max_h);
  myConfigPathDialog = new ConfigPathDialog(osystem, parent, font, boss, max_w, max_h);
  myRomAuditDialog = new RomAuditDialog(osystem, parent, font, max_w, max_h);
  myGameInfoDialog = new GameInfoDialog(osystem, parent, font, this);
#ifdef CHEATCODE_SUPPORT
  myCheatCodeDialog = new CheatCodeDialog(osystem, parent, font);
#endif
  myLoggerDialog    = new LoggerDialog(osystem, parent, font, max_w, max_h);
  myHelpDialog      = new HelpDialog(osystem, parent, font);
  myAboutDialog     = new AboutDialog(osystem, parent, font);

  addToFocusList(wid);

  // Certain buttons are disabled depending on mode
  if(myIsGlobal)
  {
    myCheatCodeButton->clearFlags(WIDGET_ENABLED);
  }
  else
  {
    myRomAuditButton->clearFlags(WIDGET_ENABLED);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
OptionsDialog::~OptionsDialog()
{
  delete myVideoDialog;
  delete myAudioDialog;
  delete myInputDialog;
  delete myUIDialog;
  delete mySnapshotDialog;
  delete myConfigPathDialog;
  delete myRomAuditDialog;
  delete myGameInfoDialog;
#ifdef CHEATCODE_SUPPORT
  delete myCheatCodeDialog;
#endif
  delete myLoggerDialog;
  delete myHelpDialog;
  delete myAboutDialog;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OptionsDialog::loadConfig()
{
  // Determine whether we should show the 'Game Information' button
  // We always show it in emulation mode, or if a valid ROM is selected
  // in launcher mode
  switch(instance().eventHandler().state())
  {
    case EventHandler::S_EMULATE:
      myGameInfoButton->setFlags(WIDGET_ENABLED);
      break;
    case EventHandler::S_LAUNCHER:
      if(instance().launcher().selectedRomMD5() != "")
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
      myVideoDialog->open();
      break;

    case kAudCmd:
      myAudioDialog->open();
      break;

    case kInptCmd:
      myInputDialog->open();
      break;

    case kUsrIfaceCmd:
      myUIDialog->open();
      break;

    case kSnapCmd:
      mySnapshotDialog->open();
      break;

    case kCfgPathsCmd:
      myConfigPathDialog->open();
      break;

    case kAuditCmd:
      myRomAuditDialog->open();
      break;

    case kInfoCmd:
      myGameInfoDialog->open();
      break;

#ifdef CHEATCODE_SUPPORT
    case kCheatCmd:
      myCheatCodeDialog->open();
      break;
#endif

    case kLoggerCmd:
      myLoggerDialog->open();
      break;

    case kHelpCmd:
      myHelpDialog->open();
      break;

    case kAboutCmd:
      myAboutDialog->open();
      break;

    case kExitCmd:
      if(myIsGlobal)
        close();
      else
        instance().eventHandler().leaveMenuMode();
      break;

    default:
      Dialog::handleCommand(sender, cmd, data, 0);
  }
}
