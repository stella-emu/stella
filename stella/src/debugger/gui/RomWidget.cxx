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
// $Id: RomWidget.cxx,v 1.5 2005-09-15 19:43:36 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#include <sstream>

#include "Debugger.hxx"
#include "DebuggerParser.hxx"
#include "CpuDebug.hxx"
#include "GuiObject.hxx"
#include "ContextMenu.hxx"
#include "RomListWidget.hxx"
#include "RomWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RomWidget::RomWidget(GuiObject* boss, const GUI::Font& font, int x, int y)
  : Widget(boss, x, y, 16, 16),
    CommandSender(boss),
    myListIsDirty(true),
    mySourceAvailable(false),
    myCurrentBank(-1)
{
  int w = 58 * font.getMaxCharWidth(),
      h = 31 * font.getLineHeight();

  myRomList = new RomListWidget(boss, font, x, y, w, h);
  myRomList->setTarget(this);
  myRomList->myMenu->setTarget(this);
  myRomList->setStyle(kSolidFill);
  addFocusWidget(myRomList);

  // Calculate real dimensions
  _w = myRomList->getWidth();
  _h = myRomList->getHeight();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RomWidget::~RomWidget()
{
  myAddrList.clear();
  myLineList.clear();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RomWidget::handleCommand(CommandSender* sender, int cmd, int data, int id)
{
  switch(cmd)
  {
    case kListScrolledCmd:
      incrementalUpdate(data, myRomList->rows());
      break;

    case kListItemChecked:
      setBreak(data);
      break;

    case kListItemDataChangedCmd:
      patchROM(data, myRomList->getSelectedString());
      break;

    case kCMenuItemSelectedCmd:
    {
      const string& rmb = myRomList->myMenu->getSelectedString();

      if(rmb == "Save ROM")
        saveROM();
      else if(rmb == "Set PC")
        setPC(myRomList->getSelected());

      break;
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RomWidget::loadConfig()
{
  Debugger& dbg = instance()->debugger();

  // Only reload full bank when necessary
  if(myListIsDirty || myCurrentBank != dbg.getBank())
  {
    initialUpdate();
    myListIsDirty = false;
  }
  else  // only reload what's in current view
  {
    incrementalUpdate(myRomList->currentPos(), myRomList->rows());
  }

  myCurrentBank = dbg.getBank();

  // Update romlist to point to current PC
  int pc = dbg.cpuDebug().pc();
  AddrToLine::iterator iter = myLineList.find(pc);
  if(iter != myLineList.end())
    myRomList->setHighlighted(iter->second);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RomWidget::initialUpdate()
{
  Debugger& dbg = instance()->debugger();

  // Fill romlist the current bank of source or disassembly
  if(mySourceAvailable)
    ; // TODO - actually implement this
  else
  {
    // Clear old mappings
    myAddrList.clear();
    myLineList.clear();

    StringList label, data, disasm;
    BoolArray state;

    // Disassemble entire bank (up to 4096 lines) and invalidate all lines
    dbg.disassemble(myAddrList, label, data, disasm, 0xf000, 4096);
    for(unsigned int i = 0; i < data.size(); ++i)
      state.push_back(false);

    // Create a mapping from addresses to line numbers
    myLineList.clear();
    for(unsigned int i = 0; i < myAddrList.size(); ++i)
      myLineList.insert(make_pair(myAddrList[i], i));

    myRomList->setList(label, data, disasm, state);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RomWidget::incrementalUpdate(int line, int rows)
{
  // TODO - implement this
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RomWidget::setBreak(int data)
{
  // We don't care about state, as breakpoints are turned on
  // and off with the same command
  // TODO - at some point, we might want to add 'breakon'
  //        and 'breakoff' to DebuggerParser, so the states
  //        don't get out of sync
  ostringstream command;
  command << "break #" << myAddrList[data];
  instance()->debugger().run(command.str());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RomWidget::setPC(int data)
{
  ostringstream command;
  command << "pc #" << myAddrList[data];
  instance()->debugger().run(command.str());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RomWidget::patchROM(int data, const string& bytes)
{
  ostringstream command;

  // Temporarily set to base 16, since that's the format the disassembled
  // byte string is in.  This eliminates the need to prefix each byte with
  // a '$' character
  BaseFormat oldbase = instance()->debugger().parser()->base();
  instance()->debugger().parser()->setBase(kBASE_16);

  command << "rom #" << myAddrList[data] << " " << bytes;
  instance()->debugger().run(command.str());

  // Restore previous base
  instance()->debugger().parser()->setBase(oldbase);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RomWidget::saveROM()
{
cerr << "save ROM\n";
}
