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
// Copyright (c) 1995-2020 by Bradford W. Mott, Stephen Anthony
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
#include "RomAuditDialog.hxx"
#include "GameInfoDialog.hxx"
#include "LoggerDialog.hxx"
#include "DeveloperDialog.hxx"
#include "HelpDialog.hxx"
#include "AboutDialog.hxx"
#include "OptionsDialog.hxx"
#include "Launcher.hxx"
#include "Settings.hxx"
#include "Menu.hxx"

#ifdef CHEATCODE_SUPPORT
  #include "CheatCodeDialog.hxx"
#endif

#include "bspf.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
OptionsDialog::OptionsDialog(OSystem& osystem, DialogContainer& parent,
                             GuiObject* boss, int max_w, int max_h, Menu::AppMode mode)
  : Dialog(osystem, parent, osystem.frameBuffer().font(), "Options"),
    myBoss(boss),
    myMode(mode)
{
  // do not show basic settings options in debugger
  bool minSettings = osystem.settings().getBool("minimal_ui") && mode != Menu::AppMode::debugger;
  const int
    fontWidth    = _font.getMaxCharWidth(),
    fontHeight   = _font.getFontHeight(),
    buttonHeight = _font.getLineHeight() * 1.25,
    VGAP = fontHeight / 4,
    HGAP = fontWidth,
    rowHeight = buttonHeight + VGAP;
  const int VBORDER = fontHeight / 2;
  const int HBORDER = fontWidth * 1.25;
  int buttonWidth = _font.getStringWidth("Game Properties" + ELLIPSIS) + fontWidth * 2.5;

  _w = 2 * buttonWidth + HBORDER * 2 + HGAP;
  _h = 7 * rowHeight + VBORDER * 2 - VGAP + _th;

  int xoffset = HBORDER, yoffset = VBORDER + _th;
  WidgetArray wid;
  ButtonWidget* b = nullptr;

  if (minSettings)
  {
    ButtonWidget* bw = new ButtonWidget(this, _font, xoffset, yoffset,
      _w - HBORDER * 2, buttonHeight, "Use Basic Settings", kBasSetCmd);
    wid.push_back(bw);
    yoffset += rowHeight + VGAP * 2;
    _h += rowHeight + VGAP * 2;
  }

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
  b->clearFlags(Widget::FLAG_ENABLED);
#endif
  wid.push_back(b);

  b = ADD_OD_BUTTON("Input" + ELLIPSIS, kInptCmd);
  wid.push_back(b);

  b = ADD_OD_BUTTON("User Interface" + ELLIPSIS, kUsrIfaceCmd);
  wid.push_back(b);

  b = ADD_OD_BUTTON("Snapshots" + ELLIPSIS, kSnapCmd);
  wid.push_back(b);

  //yoffset += rowHeight;
  b = ADD_OD_BUTTON("Developer" + ELLIPSIS, kDevelopCmd);
  wid.push_back(b);

  // Move to second column
  xoffset += buttonWidth + HGAP;
  yoffset = minSettings ? VBORDER + _th + rowHeight + VGAP * 2 : VBORDER + _th;

  myGameInfoButton = ADD_OD_BUTTON("Game Properties" + ELLIPSIS, kInfoCmd);
  wid.push_back(myGameInfoButton);

  myCheatCodeButton = ADD_OD_BUTTON("Cheat Codes" + ELLIPSIS, kCheatCmd);
#ifndef CHEATCODE_SUPPORT
  myCheatCodeButton->clearFlags(Widget::FLAG_ENABLED);
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

  buttonWidth = _font.getStringWidth("   Close   ") + fontWidth * 2.5;
  xoffset -= (buttonWidth + HGAP) / 2;
  b = ADD_OD_BUTTON("Close", kExitCmd);
  wid.push_back(b);
  addCancelWidget(b);

  // Now create all the dialogs attached to each menu button
  myVideoDialog    = make_unique<VideoDialog>(osystem, parent, _font, max_w, max_h);
  myAudioDialog    = make_unique<AudioDialog>(osystem, parent, _font);
  myInputDialog    = make_unique<InputDialog>(osystem, parent, _font, max_w, max_h);
  myUIDialog       = make_unique<UIDialog>(osystem, parent, _font, boss, max_w, max_h);
  mySnapshotDialog = make_unique<SnapshotDialog>(osystem, parent, _font, max_w, max_h);
  myDeveloperDialog = make_unique<DeveloperDialog>(osystem, parent, _font, max_w, max_h);
  myGameInfoDialog = make_unique<GameInfoDialog>(osystem, parent, _font, this, max_w, max_h);
#ifdef CHEATCODE_SUPPORT
  myCheatCodeDialog = make_unique<CheatCodeDialog>(osystem, parent, _font);
#endif
  myRomAuditDialog = make_unique<RomAuditDialog>(osystem, parent, _font, max_w, max_h);
  myHelpDialog      = make_unique<HelpDialog>(osystem, parent, _font);
  myAboutDialog     = make_unique<AboutDialog>(osystem, parent, _font);

  addToFocusList(wid);

  // Certain buttons are disabled depending on mode
  if(myMode == Menu::AppMode::launcher)
  {
    myCheatCodeButton->clearFlags(Widget::FLAG_ENABLED);
  }
  else
  {
    myRomAuditButton->clearFlags(Widget::FLAG_ENABLED);
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
      myGameInfoButton->setFlags(Widget::FLAG_ENABLED);
      break;
    case EventHandlerState::LAUNCHER:
      if(instance().launcher().selectedRomMD5() != "")
        myGameInfoButton->setFlags(Widget::FLAG_ENABLED);
      else
        myGameInfoButton->clearFlags(Widget::FLAG_ENABLED);
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
    case kBasSetCmd:
      // enable basic settings
      instance().settings().setValue("basic_settings", true);
      if (myMode != Menu::AppMode::emulator)
        close();
      else
        instance().eventHandler().leaveMenuMode();
      break;

    case kVidCmd:
    {
      // This dialog is resizable under certain conditions, so we need
      // to re-create it as necessary
      uInt32 w = 0, h = 0;

      if(myVideoDialog == nullptr || myVideoDialog->shouldResize(w, h))
      {
        myVideoDialog = make_unique<VideoDialog>(instance(), parent(),
                                                 instance().frameBuffer().font(), w, h);
      }
      myVideoDialog->open();
      break;
    }

    case kAudCmd:
      myAudioDialog->open();
      break;

    case kInptCmd:
    {
      // This dialog is resizable under certain conditions, so we need
      // to re-create it as necessary
      uInt32 w = 0, h = 0;

      if(myInputDialog == nullptr || myInputDialog->shouldResize(w, h))
      {
        myInputDialog = make_unique<InputDialog>(instance(), parent(),
                                                 instance().frameBuffer().font(), w, h);
      }

      myInputDialog->open();
      break;
    }

    case kUsrIfaceCmd:
    {
      // This dialog is resizable under certain conditions, so we need
      // to re-create it as necessary
      uInt32 w = 0, h = 0;

      if(myUIDialog == nullptr || myUIDialog->shouldResize(w, h))
      {
        myUIDialog = make_unique<UIDialog>(instance(), parent(),
                                           instance().frameBuffer().font(), myBoss, w, h);
      }

      myUIDialog->open();
      break;
    }

    case kSnapCmd:
    {
      // This dialog is resizable under certain conditions, so we need
      // to re-create it as necessary
      uInt32 w = 0, h = 0;

      if(mySnapshotDialog == nullptr || mySnapshotDialog->shouldResize(w, h))
      {
        mySnapshotDialog = make_unique<SnapshotDialog>(instance(), parent(),
                                                       instance().frameBuffer().font(), w, h);
      }
      mySnapshotDialog->open();
      break;
    }

    case kDevelopCmd:
    {
      // This dialog is resizable under certain conditions, so we need
      // to re-create it as necessary
      uInt32 w = 0, h = 0;

      if(myDeveloperDialog == nullptr || myDeveloperDialog->shouldResize(w, h))
      {
        myDeveloperDialog = make_unique<DeveloperDialog>(instance(), parent(),
                                                         instance().frameBuffer().font(), w, h);
      }
      myDeveloperDialog->open();
      break;
    }

    case kInfoCmd:
    {
      // This dialog is resizable under certain conditions, so we need
      // to re-create it as necessary
      uInt32 w = 0, h = 0;

      if(myGameInfoDialog == nullptr || myGameInfoDialog->shouldResize(w, h))
      {
        myGameInfoDialog = make_unique<GameInfoDialog>(instance(), parent(),
                                                       instance().frameBuffer().font(), this, w, h);
      }
      myGameInfoDialog->open();
      break;
    }

#ifdef CHEATCODE_SUPPORT
    case kCheatCmd:
      myCheatCodeDialog->open();
      break;
#endif

    case kAuditCmd:
      myRomAuditDialog->open();
      break;

    case kLoggerCmd:
    {
      // This dialog is resizable under certain conditions, so we need
      // to re-create it as necessary
      uInt32 w = 0, h = 0;
      bool uselargefont = getDynamicBounds(w, h);

      if(myLoggerDialog == nullptr || myLoggerDialog->shouldResize(w, h))
      {
        myLoggerDialog = make_unique<LoggerDialog>(instance(), parent(),
            instance().frameBuffer().font(), w, h, uselargefont);
      }
      myLoggerDialog->open();
      break;
    }

    case kHelpCmd:
      myHelpDialog->open();
      break;

    case kAboutCmd:
      myAboutDialog->open();
      break;

    case kExitCmd:
      if(myMode != Menu::AppMode::emulator)
        close();
      else
        instance().eventHandler().leaveMenuMode();
      break;

    default:
      Dialog::handleCommand(sender, cmd, data, 0);
  }
}
