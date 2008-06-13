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
// $Id: CommandDialog.cxx,v 1.20 2008-06-13 13:14:51 stephena Exp $
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

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CommandDialog::CommandDialog(OSystem* osystem, DialogContainer* parent)
  : Dialog(osystem, parent, 0, 0, 16, 16),
    mySelectedItem(0)
{
  const GUI::Font& font = osystem->font();
  int buttonWidth  = 65,
      buttonHeight = 18,
      xoffset = 5,
      yoffset = 5,
      lwidth  = buttonWidth + 5;

  // Set real dimensions
  _w = 4 * (lwidth) + 5;
  _h = 4 * (buttonHeight+3) + 7;

  WidgetArray wid;
  ButtonWidget* b;

  b = new ButtonWidget(this, font, xoffset, yoffset, buttonWidth, buttonHeight,
                       "Select", kSelectCmd);
  wid.push_back(b);
  xoffset += lwidth;
  b = new ButtonWidget(this, font, xoffset, yoffset, buttonWidth, buttonHeight,
                       "Reset", kResetCmd);
  wid.push_back(b);
  xoffset += lwidth;
  b = new ButtonWidget(this, font, xoffset, yoffset, buttonWidth, buttonHeight,
                       "Color TV", kColorCmd);
  wid.push_back(b);
  xoffset += lwidth;
  b = new ButtonWidget(this, font, xoffset, yoffset, buttonWidth, buttonHeight,
                       "B/W TV", kBWCmd);
  wid.push_back(b);

  xoffset = 5;  yoffset += buttonHeight + 3;

  b = new ButtonWidget(this, font, xoffset, yoffset, buttonWidth, buttonHeight,
                       "Left Diff A", kLeftDiffACmd);
  wid.push_back(b);
  xoffset += lwidth;
  b = new ButtonWidget(this, font, xoffset, yoffset, buttonWidth, buttonHeight,
                       "Left Diff B", kLeftDiffBCmd);
  wid.push_back(b);
  xoffset += lwidth;
  b = new ButtonWidget(this, font, xoffset, yoffset, buttonWidth, buttonHeight,
                       "Right Diff A", kRightDiffACmd);
  wid.push_back(b);
  xoffset += lwidth;
  b = new ButtonWidget(this, font, xoffset, yoffset, buttonWidth, buttonHeight,
                       "Right Diff B", kRightDiffBCmd);
  wid.push_back(b);

  xoffset = 5;  yoffset += buttonHeight + 3;

  b = new ButtonWidget(this, font, xoffset, yoffset, buttonWidth, buttonHeight,
                       "Save State", kSaveStateCmd);
  wid.push_back(b);
  xoffset += lwidth;
  b = new ButtonWidget(this, font, xoffset, yoffset, buttonWidth, buttonHeight,
                       "State Slot", kStateSlotCmd);
  wid.push_back(b);
  xoffset += lwidth;
  b = new ButtonWidget(this, font, xoffset, yoffset, buttonWidth, buttonHeight,
                       "Load State", kLoadStateCmd);
  wid.push_back(b);
  xoffset += lwidth;
  b = new ButtonWidget(this, font, xoffset, yoffset, buttonWidth, buttonHeight,
                       "Snapshot", kSnapshotCmd);
  wid.push_back(b);

  xoffset = 5;  yoffset += buttonHeight + 3;

  b = new ButtonWidget(this, font, xoffset, yoffset, buttonWidth, buttonHeight,
                       "NTSC/PAL", kFormatCmd);
  wid.push_back(b);
  xoffset += lwidth;
  b = new ButtonWidget(this, font, xoffset, yoffset, buttonWidth, buttonHeight,
                       "Palette", kPaletteCmd);
  wid.push_back(b);
  xoffset += lwidth;
  b = new ButtonWidget(this, font, xoffset, yoffset, buttonWidth, buttonHeight,
                       "Reload ROM", kReloadRomCmd);
  wid.push_back(b);
  xoffset += lwidth;
  b = new ButtonWidget(this, font, xoffset, yoffset, buttonWidth, buttonHeight,
                       "Exit Game", kExitCmd);
  wid.push_back(b);

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
      stateCmd = true;
      break;

    case kStateSlotCmd:
      event = Event::ChangeState;
      stateCmd = true;
      break;

    case kLoadStateCmd:
      event = Event::LoadState;
      stateCmd = true;
      break;

    case kSnapshotCmd:
      instance().eventHandler().leaveMenuMode();
      instance().eventHandler().refreshDisplay(true);
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
    instance().console().mediaSource().update();
    instance().eventHandler().handleEvent(event, 0);
  }
  else if(stateCmd)
  {
    instance().eventHandler().handleEvent(event, 1);
  }
}
