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
// Copyright (c) 1995-2009 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: CommandDialog.cxx,v 1.25 2009-01-19 16:52:32 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#include "Console.hxx"
#include "Switches.hxx"
#include "DialogContainer.hxx"
#include "Dialog.hxx"
#include "EventHandler.hxx"
#include "OSystem.hxx"
#include "Widget.hxx"
#include "CommandDialog.hxx"

#define addCDButton(label, cmd) \
  new ButtonWidget(this, font, xoffset, yoffset, buttonWidth, buttonHeight, label, cmd); xoffset += buttonWidth + 6

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CommandDialog::CommandDialog(OSystem* osystem, DialogContainer* parent)
  : Dialog(osystem, parent, 0, 0, 16, 16),
    mySelectedItem(0)
{
  const GUI::Font& font = instance().font();
  const int buttonWidth = font.getStringWidth("Right Diff B") + 20,
            buttonHeight = font.getLineHeight() + 6,
            rowHeight = font.getLineHeight() + 10;

  // Set real dimensions
  _w = 3 * (buttonWidth + 5) + 20;
  _h = 6 * rowHeight + 15;

  WidgetArray wid;
  ButtonWidget* b[16];

  // Row 1
  int xoffset = 10, yoffset = 10;
  b[0] = addCDButton("Select", kSelectCmd);
  b[4] = addCDButton("Left Diff A", kLeftDiffACmd);
  b[8] = addCDButton("Save State", kSaveStateCmd);

  // Row 2
  xoffset = 10;  yoffset += buttonHeight + 3;
  b[1] = addCDButton("Reset", kResetCmd);
  b[5] = addCDButton("Left Diff B", kLeftDiffBCmd);
  b[9] = addCDButton("State Slot", kStateSlotCmd);

  // Row 3
  xoffset = 10;  yoffset += buttonHeight + 3;
  b[2]  = addCDButton("Color TV", kColorCmd);
  b[6]  = addCDButton("Right Diff A", kRightDiffACmd);
  b[10] = addCDButton("Load State", kLoadStateCmd);

  // Row 4
  xoffset = 10;  yoffset += buttonHeight + 3;
  b[3]  = addCDButton("B/W TV", kBWCmd);
  b[7]  = addCDButton("Right Diff B", kRightDiffBCmd);
  b[11] = addCDButton("Snapshot", kSnapshotCmd);

  // Row 5
  xoffset = 10;  yoffset += buttonHeight + 3;
  b[12] = addCDButton("NTSC/PAL", kFormatCmd);
  b[13] = addCDButton("Palette", kPaletteCmd);
  b[14] = addCDButton("Reload ROM", kReloadRomCmd);

  // Row 6
  xoffset = 10 + buttonWidth + 6;  yoffset += buttonHeight + 3;
  b[15] = addCDButton("Exit Game", kExitCmd);

  for(uInt8 i = 0; i < 16; ++i)
    wid.push_back(b[i]);

  addToFocusList(wid);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CommandDialog::~CommandDialog()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CommandDialog::handleCommand(CommandSender* sender, int cmd,
                                  int data, int id)
{
  // TODO - fix the logic here; it doesn't seem it needs to be this complex
  bool consoleCmd = false, stateCmd = false;
  Event::Type event = Event::NoType;

  switch(cmd)
  {
    case kSelectCmd:
      event = Event::ConsoleSelect;
      consoleCmd = true;
      break;

    case kResetCmd:
      event = Event::ConsoleReset;
      consoleCmd = true;
      break;

    case kColorCmd:
      event = Event::ConsoleColor;
      consoleCmd = true;
      break;

    case kBWCmd:
      event = Event::ConsoleBlackWhite;
      consoleCmd = true;
      break;

    case kLeftDiffACmd:
      event = Event::ConsoleLeftDifficultyA;
      consoleCmd = true;
      break;

    case kLeftDiffBCmd:
      event = Event::ConsoleLeftDifficultyB;
      consoleCmd = true;
      break;

    case kRightDiffACmd:
      event = Event::ConsoleRightDifficultyA;
      consoleCmd = true;
      break;

    case kRightDiffBCmd:
      event = Event::ConsoleRightDifficultyB;
      consoleCmd = true;
      break;

    case kSaveStateCmd:
      event = Event::SaveState;
      consoleCmd = true;
      break;

    case kStateSlotCmd:
      event = Event::ChangeState;
      stateCmd = true;
      break;

    case kLoadStateCmd:
      event = Event::LoadState;
      consoleCmd = true;
      break;

    case kSnapshotCmd:
      instance().eventHandler().leaveMenuMode();
      instance().frameBuffer().refresh();
      instance().eventHandler().handleEvent(Event::TakeSnapshot, 1);
      break;

    case kFormatCmd:
      instance().eventHandler().leaveMenuMode();
      instance().console().toggleFormat();
      break;

    case kPaletteCmd:
      instance().eventHandler().leaveMenuMode();
      instance().console().togglePalette();
      break;

    case kReloadRomCmd:
      instance().eventHandler().leaveMenuMode();
      instance().deleteConsole();
      instance().createConsole();
      break;

    case kExitCmd:
      instance().eventHandler().handleEvent(Event::LauncherMode, 1);
      break;
  }

  // Console commands show be performed right away, after leaving the menu
  // State commands require you to exit the menu manually
  if(consoleCmd)
  {
    instance().eventHandler().leaveMenuMode();
    instance().eventHandler().handleEvent(event, 1);
    instance().console().switches().update();
    instance().console().tia().update();
    instance().eventHandler().handleEvent(event, 0);
    instance().frameBuffer().refresh();
  }
  else if(stateCmd)
  {
    instance().eventHandler().handleEvent(event, 1);
  }
}
