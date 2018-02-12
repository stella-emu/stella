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
// Copyright (c) 1995-2018 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include "OSystem.hxx"
#include "FrameBuffer.hxx"
#include "EventHandler.hxx"
#include "Dialog.hxx"
#include "DialogContainer.hxx"
#include "Widget.hxx"
#include "Font.hxx"
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
#include "DeveloperDialog.hxx"
#include "HelpDialog.hxx"
#include "AboutDialog.hxx"
#include "OptionsDialog.hxx"
#include "Launcher.hxx"
#include "Settings.hxx"

#ifdef CHEATCODE_SUPPORT
  #include "CheatCodeDialog.hxx"
#endif

#include "bspf.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
OptionsDialog::OptionsDialog(OSystem& osystem, DialogContainer& parent,
                             GuiObject* boss, int max_w, int max_h, stellaMode mode)
  : Dialog(osystem, parent, osystem.frameBuffer().font(), "Options"),
    myMode(mode),
    _boss(boss)
{
  const int buttonWidth = _font.getStringWidth("Game Properties" + ELLIPSIS) + 20,
            buttonHeight = _font.getLineHeight() + 6,
            rowHeight = _font.getLineHeight() + 10;
  const int VBORDER = 10 + _th;

  _w = 2 * buttonWidth + 30;
  _h = 7 * rowHeight + 15 + _th;

  int xoffset = 10, yoffset = VBORDER;
  WidgetArray wid;
  ButtonWidget* b = nullptr;

  auto ADD_OD_BUTTON = [&](const string& label, int cmd)
  {
    ButtonWidget* bw = new ButtonWidget(this, _font, xoffset, yoffset,
            buttonWidth, buttonHeight, label, cmd);
    yoffset += rowHeight;
    return bw;
  };

  b = ADD_OD_BUTTON("Video" + ELLIPSIS, kVidCmd);
  wid.push_back(b);

  b = ADD_OD_BUTTON("Audio" + ELLIPSIS, kAudCmd);
#ifndef SOUND_SUPPORT
  b->clearFlags(WIDGET_ENABLED);
#endif
  wid.push_back(b);

  b = ADD_OD_BUTTON("Input" + ELLIPSIS, kInptCmd);
  wid.push_back(b);

  b = ADD_OD_BUTTON("User Interface" + ELLIPSIS, kUsrIfaceCmd);
  wid.push_back(b);

  b = ADD_OD_BUTTON("Snapshots" + ELLIPSIS, kSnapCmd);
  wid.push_back(b);

  b = ADD_OD_BUTTON("Paths" + ELLIPSIS, kCfgPathsCmd);
  wid.push_back(b);

  b = ADD_OD_BUTTON("Developer" + ELLIPSIS, kDevelopCmd);
  wid.push_back(b);

  // Move to second column
  xoffset += buttonWidth + 10;  yoffset = VBORDER;

  myGameInfoButton = ADD_OD_BUTTON("Game Properties" + ELLIPSIS, kInfoCmd);
  wid.push_back(myGameInfoButton);

  myCheatCodeButton = ADD_OD_BUTTON("Cheat Codes" + ELLIPSIS, kCheatCmd);
#ifndef CHEATCODE_SUPPORT
  myCheatCodeButton->clearFlags(WIDGET_ENABLED);
#endif
  wid.push_back(myCheatCodeButton);

  myRomAuditButton = ADD_OD_BUTTON("Audit ROMs" + ELLIPSIS, kAuditCmd);
  wid.push_back(myRomAuditButton);

  b = ADD_OD_BUTTON("System Logs" + ELLIPSIS, kLoggerCmd);
  wid.push_back(b);

  b = ADD_OD_BUTTON("Help" + ELLIPSIS, kHelpCmd);
  wid.push_back(b);

  b = ADD_OD_BUTTON("About" + ELLIPSIS, kAboutCmd);
  wid.push_back(b);

  b = ADD_OD_BUTTON("Close", kExitCmd);
  wid.push_back(b);
  addCancelWidget(b);

  // Now create all the dialogs attached to each menu button
  myVideoDialog    = make_unique<VideoDialog>(osystem, parent, _font, max_w, max_h);
  myAudioDialog    = make_unique<AudioDialog>(osystem, parent, _font);
  myInputDialog    = make_unique<InputDialog>(osystem, parent, _font, max_w, max_h);
  myUIDialog       = make_unique<UIDialog>(osystem, parent, _font);
  mySnapshotDialog = make_unique<SnapshotDialog>(osystem, parent, _font, max_w, max_h);
  myConfigPathDialog = make_unique<ConfigPathDialog>(osystem, parent, _font, boss, max_w, max_h);
  myRomAuditDialog = make_unique<RomAuditDialog>(osystem, parent, _font, max_w, max_h);
  myGameInfoDialog = make_unique<GameInfoDialog>(osystem, parent, _font, this);
#ifdef CHEATCODE_SUPPORT
  myCheatCodeDialog = make_unique<CheatCodeDialog>(osystem, parent, _font);
#endif
  myLoggerDialog    = make_unique<LoggerDialog>(osystem, parent, _font, max_w, max_h);
  myDeveloperDialog = make_unique<DeveloperDialog>(osystem, parent, _font, max_w, max_h);
  myHelpDialog      = make_unique<HelpDialog>(osystem, parent, _font);
  myAboutDialog     = make_unique<AboutDialog>(osystem, parent, _font);

  addToFocusList(wid);

  // Certain buttons are disabled depending on mode
  if(myMode == launcher)
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
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OptionsDialog::loadConfig()
{
  // Determine whether we should show the 'Game Information' button
  // We always show it in emulation mode, or if a valid ROM is selected
  // in launcher mode
  switch(instance().eventHandler().state())
  {
    case EventHandlerState::EMULATION:
      myGameInfoButton->setFlags(WIDGET_ENABLED);
      break;
    case EventHandlerState::LAUNCHER:
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
      // This dialog is resizable under certain conditions, so we need
      // to re-create it as necessary
      if(myMode != launcher)
      {
        uInt32 w = 0, h = 0;

        getResizableBounds(w, h);
        myVideoDialog = make_unique<VideoDialog>(instance(), parent(), instance().frameBuffer().font(), w, h);
      }
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
      // This dialog is resizable under certain conditions, so we need
      // to re-create it as necessary
      if(myMode != launcher)
      {
        uInt32 w = 0, h = 0;

        getResizableBounds(w, h);
        mySnapshotDialog = make_unique<SnapshotDialog>(instance(), parent(), instance().frameBuffer().font(), w, h);
      }
      mySnapshotDialog->open();
      break;

    case kCfgPathsCmd:
      // This dialog is resizable under certain conditions, so we need
      // to re-create it as necessary
      if(myMode != launcher)
      {
        uInt32 w = 0, h = 0;

        getResizableBounds(w, h);
        myConfigPathDialog = make_unique<ConfigPathDialog>(instance(), parent(),
                                                         instance().frameBuffer().font(), _boss, w, h);
      }
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
      // This dialog is resizable under certain conditions, so we need
      // to re-create it as necessary
      if(myMode != launcher)
      {
        uInt32 w = 0, h = 0;
        bool uselargefont = getResizableBounds(w, h);

        myLoggerDialog = make_unique<LoggerDialog>(instance(), parent(),
            instance().frameBuffer().font(), w, h, uselargefont);
      }
      myLoggerDialog->open();
      break;

    case kDevelopCmd:
      myDeveloperDialog->open();
      break;

    case kHelpCmd:
      myHelpDialog->open();
      break;

    case kAboutCmd:
      myAboutDialog->open();
      break;

    case kExitCmd:
      if(myMode != emulator)
        close();
      else
        instance().eventHandler().leaveMenuMode();
      break;

    default:
      Dialog::handleCommand(sender, cmd, data, 0);
  }
}
