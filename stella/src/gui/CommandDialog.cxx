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
// $Id: CommandDialog.cxx,v 1.8 2006-02-22 17:38:04 stephena Exp $
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

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CommandDialog::CommandDialog(OSystem* osystem, DialogContainer* parent)
  : Dialog(osystem, parent, 0, 0, 16, 16),
    mySelectedItem(0)
{
  const GUI::Font& font = osystem->font();
  int lineHeight   = font.getLineHeight(),
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

  WidgetArray wid;
  ButtonWidget* b;

  b = new ButtonWidget(this, font, xoffset, yoffset, buttonWidth, buttonHeight,
                       "Select", kSelectCmd, 0);
  b->setEditable(true);
  wid.push_back(b);
  xoffset += lwidth;
  b = new ButtonWidget(this, font, xoffset, yoffset, buttonWidth, buttonHeight,
                       "Reset", kResetCmd, 0);
  b->setEditable(true);
  wid.push_back(b);
  xoffset += lwidth;
  b = new ButtonWidget(this, font, xoffset, yoffset, buttonWidth, buttonHeight,
                       "Color TV", kColorCmd, 0);
  b->setEditable(true);
  wid.push_back(b);
  xoffset += lwidth;
  b = new ButtonWidget(this, font, xoffset, yoffset, buttonWidth, buttonHeight,
                       "B/W TV", kBWCmd, 0);
  b->setEditable(true);
  wid.push_back(b);

  xoffset = 5;  yoffset += buttonHeight + 5;

  b = new ButtonWidget(this, font, xoffset, yoffset, buttonWidth, buttonHeight,
                       "Left Diff A", kLeftDiffACmd, 0);
  b->setEditable(true);
  wid.push_back(b);
  xoffset += lwidth;
  b = new ButtonWidget(this, font, xoffset, yoffset, buttonWidth, buttonHeight,
                       "Left Diff B", kLeftDiffBCmd, 0);
  b->setEditable(true);
  wid.push_back(b);
  xoffset += lwidth;
  b = new ButtonWidget(this, font, xoffset, yoffset, buttonWidth, buttonHeight,
                       "Right Diff A", kRightDiffACmd, 0);
  b->setEditable(true);
  wid.push_back(b);
  xoffset += lwidth;
  b = new ButtonWidget(this, font, xoffset, yoffset, buttonWidth, buttonHeight,
                       "Right Diff B", kRightDiffBCmd, 0);
  b->setEditable(true);
  wid.push_back(b);

  xoffset = 5;  yoffset += buttonHeight + 5;

  b = new ButtonWidget(this, font, xoffset, yoffset, buttonWidth, buttonHeight,
                       "Save State", kSaveStateCmd, 0);
  b->setEditable(true);
  wid.push_back(b);
  xoffset += lwidth;
  b = new ButtonWidget(this, font, xoffset, yoffset, buttonWidth, buttonHeight,
                       "State Slot", kStateSlotCmd, 0);
  b->setEditable(true);
  wid.push_back(b);
  xoffset += lwidth;
  b = new ButtonWidget(this, font, xoffset, yoffset, buttonWidth, buttonHeight,
                       "Load State", kLoadStateCmd, 0);
  b->setEditable(true);
  wid.push_back(b);
  xoffset += lwidth;
  b = new ButtonWidget(this, font, xoffset, yoffset, buttonWidth, buttonHeight,
                       "Snapshot", kSnapshotCmd, 0);
  b->setEditable(true);
  wid.push_back(b);

  xoffset = 5;  yoffset += buttonHeight + 5;

  b = new ButtonWidget(this, font, xoffset, yoffset, buttonWidth, buttonHeight,
                       "NTSC/PAL", kFormatCmd, 0);
  b->setEditable(true);
  wid.push_back(b);
  xoffset += lwidth;
  b = new ButtonWidget(this, font, xoffset, yoffset, buttonWidth, buttonHeight,
                       "Palette", kPaletteCmd, 0);
  b->setEditable(true);
  wid.push_back(b);
  xoffset += lwidth;
  b = new ButtonWidget(this, font, xoffset, yoffset, buttonWidth, buttonHeight,
                       "Reload ROM", kReloadRomCmd, 0);
  b->setEditable(true);
  wid.push_back(b);
  xoffset += lwidth;
  b = new ButtonWidget(this, font, xoffset, yoffset, buttonWidth, buttonHeight,
                       "Exit Game", kExitCmd, 0);
  b->setEditable(true);
  wid.push_back(b);

  addToFocusList(wid);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CommandDialog::~CommandDialog()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CommandDialog::handleKeyDown(int ascii, int keycode, int modifiers)
{
  bool handled = true;
  int row = mySelectedItem / kNumRows,
      col = mySelectedItem - (row * kNumCols);

  // Only detect the cursor keys, otherwise pass to base class
  switch(ascii)
  {
    case kCursorUp:
      if (row > 0)
        --row;
      else if(col > 0)
      {
        row = kNumRows - 1;
        --col;
      }
      break;

    case kCursorDown:
      if (row < kNumRows - 1)
        ++row;
      else if(col < kNumCols - 1)
      {
        row = 0;
        ++col;
      }
      break;

    case kCursorLeft:
      if (col > 0)
        --col;
      else if(row > 0)
      {
        col = kNumCols - 1;
        --row;
      }
      break;

    case kCursorRight:
      if (col < kNumCols - 1)
        ++col;
      else if(row < kNumRows - 1)
      {
        col = 0;
        ++row;
      }
      break;

    default:
      handled = false;
      break;
  }

  if(handled)
  {
    mySelectedItem = row * kNumCols + col;
    Dialog::setFocus(getFocusList()[mySelectedItem]);
  }
  else
    Dialog::handleKeyDown(ascii, keycode, modifiers);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CommandDialog::handleJoyAxis(int stick, int axis, int value)
{
  if(!parent()->joymouse())
    return;

  // We make the (hopefully) valid assumption that all joysticks
  // treat axis the same way.  Eventually, we may need to remap
  // these actions of this assumption is invalid.
  // TODO - make this work better with analog axes ...
  int key = -1;
  if(axis % 2 == 0)  // x-direction
  {
    if(value < 0)
      key = kCursorLeft;
    else if(value > 0)
      key = kCursorRight;
  }
  else   // y-direction
  {
    if(value < 0)
      key = kCursorUp;
    else if(value > 0)
      key = kCursorDown;
  }

  if(key != -1)
    handleKeyDown(key, 0, 0);
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
      instance()->eventHandler().leaveMenuMode();
      instance()->console().toggleFormat();
      return;
      break;

    case kPaletteCmd:
      instance()->eventHandler().leaveMenuMode();
      instance()->console().togglePalette();
      return;
      break;

    case kReloadRomCmd:
      instance()->eventHandler().leaveMenuMode();
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
    instance()->eventHandler().leaveMenuMode();
    instance()->eventHandler().handleEvent(event, 1);
    instance()->console().mediaSource().update();
    instance()->eventHandler().handleEvent(event, 0);
  }
}
