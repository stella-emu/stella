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
// Copyright (c) 1995-2019 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include "Console.hxx"
#include "TIA.hxx"
#include "Switches.hxx"
#include "Dialog.hxx"
#include "Font.hxx"
#include "EventHandler.hxx"
#include "StateManager.hxx"
#include "RewindManager.hxx"
#include "OSystem.hxx"
#include "Widget.hxx"
#include "StellaSettingsDialog.hxx"
#include "OptionsDialog.hxx"
#include "TIASurface.hxx"
#include "MinUICommandDialog.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
MinUICommandDialog::MinUICommandDialog(OSystem& osystem, DialogContainer& parent)
  : Dialog(osystem, parent, osystem.frameBuffer().font(), "Commands"),
    myStellaSettingsDialog(nullptr),
    myOptionsDialog(nullptr)
{
  const int HBORDER = 10;
  const int VBORDER = 10;
  const int HGAP = 8;
  const int VGAP = 5;
  const int buttonWidth = _font.getStringWidth(" Load State 0") + 20,
    buttonHeight = _font.getLineHeight() + 8,
    rowHeight = buttonHeight + VGAP;

  // Set real dimensions
  _w = 3 * (buttonWidth + 5) + HBORDER * 2;
  _h = 6 * rowHeight - VGAP + VBORDER * 2 + _th;
  ButtonWidget* bw = nullptr;
  WidgetArray wid;
  int xoffset = HBORDER, yoffset = VBORDER + _th;

  auto ADD_CD_BUTTON = [&](const string& label, int cmd)
  {
    ButtonWidget* b = new ButtonWidget(this, _font, xoffset, yoffset,
                                       buttonWidth, buttonHeight, label, cmd);
    yoffset += buttonHeight + VGAP;
    return b;
  };

  // Column 1
  bw = ADD_CD_BUTTON("Select", kSelectCmd);
  wid.push_back(bw);
  bw = ADD_CD_BUTTON("Reset", kResetCmd);
  wid.push_back(bw);
  myColorButton = ADD_CD_BUTTON("", kColorCmd);
  wid.push_back(myColorButton);
  myLeftDiffButton = ADD_CD_BUTTON("", kLeftDiffCmd);
  wid.push_back(myLeftDiffButton);
  myRightDiffButton = ADD_CD_BUTTON("", kLeftDiffCmd);
  wid.push_back(myRightDiffButton);

  // Column 2
  xoffset += buttonWidth + HGAP;
  yoffset = VBORDER + _th;

  mySaveStateButton = ADD_CD_BUTTON("", kSaveStateCmd);
  wid.push_back(mySaveStateButton);
  myStateSlotButton = ADD_CD_BUTTON("", kStateSlotCmd);
  wid.push_back(myStateSlotButton);
  myLoadStateButton = ADD_CD_BUTTON("", kLoadStateCmd);
  wid.push_back(myLoadStateButton);
  myRewindButton = ADD_CD_BUTTON("Rewind", kRewindCmd);
  wid.push_back(myRewindButton);
  myUnwindButton = ADD_CD_BUTTON("Unwind", kUnwindCmd);
  wid.push_back(myUnwindButton);
  bw = ADD_CD_BUTTON("Close", GuiObject::kCloseCmd);
  wid.push_back(bw);

  // Column 3
  xoffset += buttonWidth + HGAP;
  yoffset = VBORDER + _th;

  myTVFormatButton = ADD_CD_BUTTON("", kFormatCmd);
  wid.push_back(myTVFormatButton);
  myStretchButton = ADD_CD_BUTTON("", kStretchCmd);
  wid.push_back(myStretchButton);
  myPhosphorButton = ADD_CD_BUTTON("", kPhosphorCmd);
  wid.push_back(myPhosphorButton);
  bw = ADD_CD_BUTTON("Settings"+ ELLIPSIS, kSettings);
  wid.push_back(bw);
  bw = ADD_CD_BUTTON("Exit Game", kExitGameCmd);
  wid.push_back(bw);

  addToFocusList(wid);

  // We don't have a close/cancel button, but we still want the cancel
  // event to be processed
  processCancelWithoutWidget(true);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MinUICommandDialog::loadConfig()
{
  // Column 1
  myColorButton->setLabel(instance().console().switches().tvColor() ? "Color Mode" : "B/W Mode");
  myLeftDiffButton->setLabel(instance().console().switches().leftDifficultyA() ? "P1 Skill A" : "P1 Skill B");
  myRightDiffButton->setLabel(instance().console().switches().rightDifficultyA() ? "P2 Skill A" : "P2 Skill B");
  // Column 2
  updateSlot(instance().state().currentSlot());
  updateWinds();

  // Column 3
  updateTVFormat();
  myStretchButton->setLabel(instance().settings().getBool("tia.fs_stretch") ? "Stretched" : "4:3 Format");
  myPhosphorButton->setLabel(instance().frameBuffer().tiaSurface().phosphorEnabled() ? "Phosphor On" : "Phosphor Off");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MinUICommandDialog::handleKeyDown(StellaKey key, StellaMod mod)
{
  switch (key)
  {
    case KBDK_F8: // front  ("Skill P2")
      instance().eventHandler().leaveMenuMode();
      break;

    default:
      Dialog::handleKeyDown(key, mod);
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MinUICommandDialog::handleCommand(CommandSender* sender, int cmd,
                                  int data, int id)
{
  bool consoleCmd = false, stateCmd = false;
  Event::Type event = Event::NoType;

  switch(cmd)
  {
    // Column 1
    case kSelectCmd:
      event = Event::ConsoleSelect;
      consoleCmd = true;
      break;

    case kResetCmd:
      event = Event::ConsoleReset;
      consoleCmd = true;
      break;

    case kColorCmd:
      event = Event::ConsoleColorToggle;
      consoleCmd = true;
      break;

    case kLeftDiffCmd:
      event = Event::ConsoleLeftDiffToggle;
      consoleCmd = true;
      break;

    case kRightDiffCmd:
      event = Event::ConsoleRightDiffToggle;
      consoleCmd = true;
      break;

      // Column 2
    case kSaveStateCmd:
      event = Event::SaveState;
      consoleCmd = true;
      break;

    case kStateSlotCmd:
    {
      event = Event::ChangeState;
      stateCmd = true;
      int slot = (instance().state().currentSlot() + 1) % 10;
      updateSlot(slot);
      break;
    }

    case kLoadStateCmd:
      event = Event::LoadState;
      consoleCmd = true;
      break;

    case kRewindCmd:
      // rewind 5s
      instance().state().rewindStates(5);
      updateWinds();
	    break;

    case kUnwindCmd:
      // unwind 5s
		  instance().state().unwindStates(5);
      updateWinds();
      break;

    case GuiObject::kCloseCmd:
      instance().eventHandler().leaveMenuMode();
      break;

      // Column 3
    case kFormatCmd:
      instance().console().toggleFormat();
      updateTVFormat();
      break;

    case kStretchCmd:
      instance().eventHandler().leaveMenuMode();
      instance().settings().setValue("tia.fs_stretch", !instance().settings().getBool("tia.fs_stretch"));
      break;

    case kPhosphorCmd:
      instance().eventHandler().leaveMenuMode();
      instance().console().togglePhosphor();
      break;

    case kSettings:
      openSettings();
      break;

    case kExitGameCmd:
      instance().eventHandler().handleEvent(Event::LauncherMode);
      break;
  }

  // Console commands should be performed right away, after leaving the menu
  // State commands require you to exit the menu manually
  if(consoleCmd)
  {
    instance().eventHandler().leaveMenuMode();
    instance().eventHandler().handleEvent(event);
    instance().console().switches().update();
    instance().console().tia().update();
    instance().eventHandler().handleEvent(event, false);
  }
  else if(stateCmd)
    instance().eventHandler().handleEvent(event);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MinUICommandDialog::processCancel()
{
  instance().eventHandler().leaveMenuMode();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MinUICommandDialog::updateSlot(int slot)
{
  ostringstream buf;
  buf << " " << slot;

  mySaveStateButton->setLabel("Save State" + buf.str());
  myStateSlotButton->setLabel("State Slot" + buf.str());
  myLoadStateButton->setLabel("Load State" + buf.str());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MinUICommandDialog::updateTVFormat()
{
  myTVFormatButton->setLabel(instance().console().getFormatString() + " Mode");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MinUICommandDialog::updateWinds()
{
  RewindManager& r = instance().state().rewindManager();

  myRewindButton->setEnabled(!r.atFirst());
  myUnwindButton->setEnabled(!r.atLast());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MinUICommandDialog::openSettings()
{
  // Create an options dialog, similar to the in-game one
  if (instance().settings().getBool("basic_settings"))
  {
    if (myStellaSettingsDialog == nullptr)
      myStellaSettingsDialog = make_unique<StellaSettingsDialog>(instance(), parent(),
        instance().frameBuffer().launcherFont(), FBMinimum::Width, FBMinimum::Height, Menu::AppMode::launcher);
    myStellaSettingsDialog->open();
  }
  else
  {
    if (myOptionsDialog == nullptr)
      myOptionsDialog = make_unique<OptionsDialog>(instance(), parent(), this,
        FBMinimum::Width, FBMinimum::Height, Menu::AppMode::launcher);
    myOptionsDialog->open();
  }
}
