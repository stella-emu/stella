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
// $Id: CommandDialog.cxx,v 1.3 2005-09-16 18:15:44 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#include "OSystem.hxx"
#include "Dialog.hxx"
#include "DialogContainer.hxx"
#include "Widget.hxx"
#include "CommandDialog.hxx"
#include "EventHandler.hxx"

enum {
  kSelectCmd     = 'Csel',
  kResetCmd      = 'Cres',
  kColorCmd      = 'Ccol',
  kBWCmd         = 'Cbwt',
  kLeftDiffACmd  = 'Clda',
  kLeftDiffBCmd  = 'Cldb',
  kRightDiffACmd = 'Crda',
  kRightDiffBCmd = 'Crdb',
  kSaveStateCmd  = 'Csst',
  kStateSlotCmd  = 'Ccst',
  kLoadStateCmd  = 'Clst',
  kSnapshotCmd   = 'Csnp',
  kFormatCmd     = 'Cfmt',
  kPaletteCmd    = 'Cpal',
  kReloadRomCmd  = 'Crom',
  kExitCmd       = 'Clex'
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CommandDialog::CommandDialog(OSystem* osystem, DialogContainer* parent)
  : Dialog(osystem, parent, 0, 0, 16, 16)
{
  int lineHeight   = osystem->font().getLineHeight(),
      buttonWidth  = 60,
      buttonHeight = lineHeight + 2,
      xoffset = 5,
      yoffset = 5,
      lwidth  = buttonWidth + 5;

  // Set real dimensions
  _w = 4 * (lwidth) + 5;
  _h = 4 * (buttonHeight+5) + 5;
  _x = (osystem->frameBuffer().baseWidth()  - _w) / 2;
  _y = (osystem->frameBuffer().baseHeight() - _h) / 2;


  new ButtonWidget(this, xoffset, yoffset, buttonWidth, buttonHeight,
                   "Select", kSelectCmd, 0);
  xoffset += lwidth;
  new ButtonWidget(this, xoffset, yoffset, buttonWidth, buttonHeight,
                   "Reset", kResetCmd, 0);
  xoffset += lwidth;
  new ButtonWidget(this, xoffset, yoffset, buttonWidth, buttonHeight,
                   "Color TV", kColorCmd, 0);
  xoffset += lwidth;
  new ButtonWidget(this, xoffset, yoffset, buttonWidth, buttonHeight,
                   "B/W TV", kBWCmd, 0);

  xoffset = 5;  yoffset += buttonHeight + 5;

  new ButtonWidget(this, xoffset, yoffset, buttonWidth, buttonHeight,
                   "Left Diff A", kLeftDiffACmd, 0);
  xoffset += lwidth;
  new ButtonWidget(this, xoffset, yoffset, buttonWidth, buttonHeight,
                   "Left Diff B", kLeftDiffBCmd, 0);
  xoffset += lwidth;
  new ButtonWidget(this, xoffset, yoffset, buttonWidth, buttonHeight,
                   "Right Diff A", kRightDiffACmd, 0);
  xoffset += lwidth;
  new ButtonWidget(this, xoffset, yoffset, buttonWidth, buttonHeight,
                   "Right Diff B", kRightDiffBCmd, 0);

  xoffset = 5;  yoffset += buttonHeight + 5;

  new ButtonWidget(this, xoffset, yoffset, buttonWidth, buttonHeight,
                   "Save State", kSaveStateCmd, 0);
  xoffset += lwidth;
  new ButtonWidget(this, xoffset, yoffset, buttonWidth, buttonHeight,
                   "State Slot", kStateSlotCmd, 0);
  xoffset += lwidth;
  new ButtonWidget(this, xoffset, yoffset, buttonWidth, buttonHeight,
                   "Load State", kLoadStateCmd, 0);
  xoffset += lwidth;
  new ButtonWidget(this, xoffset, yoffset, buttonWidth, buttonHeight,
                   "Snapshot", kSnapshotCmd, 0);

  xoffset = 5;  yoffset += buttonHeight + 5;

  new ButtonWidget(this, xoffset, yoffset, buttonWidth, buttonHeight,
                   "NTSC/PAL", kFormatCmd, 0);
  xoffset += lwidth;
  new ButtonWidget(this, xoffset, yoffset, buttonWidth, buttonHeight,
                   "Palette", kPaletteCmd, 0);
  xoffset += lwidth;
  new ButtonWidget(this, xoffset, yoffset, buttonWidth, buttonHeight,
                   "Reload ROM", kReloadRomCmd, 0);
  xoffset += lwidth;
  new ButtonWidget(this, xoffset, yoffset, buttonWidth, buttonHeight,
                   "Exit Game", kExitCmd, 0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CommandDialog::~CommandDialog()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CommandDialog::handleCommand(CommandSender* sender, int cmd,
                                  int data, int id)
{
  bool execute = true;
  Event::Type event = Event::NoType;

  switch(cmd)
  {
    case kSelectCmd:
      event = Event::ConsoleSelect;
      break;

    case kResetCmd:
      event = Event::ConsoleReset;
      break;

    case kColorCmd:
      event = Event::ConsoleColor;
      break;

    case kBWCmd:
      event = Event::ConsoleBlackWhite;
      break;

    case kLeftDiffACmd:
      event = Event::ConsoleLeftDifficultyA;
      break;

    case kLeftDiffBCmd:
      event = Event::ConsoleLeftDifficultyB;
      break;

    case kRightDiffACmd:
      event = Event::ConsoleRightDifficultyA;
      break;

    case kRightDiffBCmd:
      event = Event::ConsoleRightDifficultyB;
      break;

    case kSaveStateCmd:
      event = Event::SaveState;
      break;

    case kStateSlotCmd:
      event = Event::ChangeState;
      break;

    case kLoadStateCmd:
      event = Event::LoadState;
      break;

    case kSnapshotCmd:
      event = Event::TakeSnapshot;
      break;

    case kFormatCmd:
      instance()->eventHandler().leaveCmdMenuMode();
      instance()->console().toggleFormat();
      return;
      break;

    case kPaletteCmd:
      instance()->eventHandler().leaveCmdMenuMode();
      instance()->console().togglePalette();
      return;
      break;

    case kReloadRomCmd:
      instance()->eventHandler().leaveCmdMenuMode();
      instance()->createConsole();
      return;
      break;

    case kExitCmd:
      if(instance()->eventHandler().useLauncher())
        event = Event::LauncherMode;
      else
        event = Event::Quit;
      break;

    default:
      execute = false;
  }

  // Show changes onscreen
  if(execute)
  {
    instance()->eventHandler().leaveCmdMenuMode();
    instance()->eventHandler().handleEvent(event, 1);
    instance()->console().mediaSource().update();
    instance()->eventHandler().handleEvent(event, 0);
  }
}
