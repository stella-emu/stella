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
// $Id: RomWidget.cxx,v 1.2 2005-09-01 19:14:09 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#include <sstream>

#include "Debugger.hxx"
#include "CpuDebug.hxx"
#include "GuiObject.hxx"
#include "RomListWidget.hxx"
#include "RomWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RomWidget::RomWidget(GuiObject* boss, const GUI::Font& font, int x, int y)
  : Widget(boss, x, y, 16, 16),
    CommandSender(boss),
    myFirstLoad(true),
    mySourceAvailable(false),
    myCurrentBank(-1)
{
  int w = 58 * font.getMaxCharWidth(),
      h = 31 * font.getLineHeight();

  myRomList = new RomListWidget(boss, font, x, y, w, h);
  myRomList->setTarget(this);
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
      incrementalUpdate();
      break;

    case kListItemChecked:
    {
      // We don't care about state, as breakpoints are turned on
      // and off with the same command
      // FIXME - at some point, we might want to add 'breakon'
      //         and 'breakoff' to DebuggerParser, so the states
      //         don't get out of sync
      ostringstream cmd;
      cmd << "break #" << myAddrList[data];
      instance()->debugger().run(cmd.str());

      break;
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RomWidget::loadConfig()
{
//cerr << "RomWidget::loadConfig()\n";
  // Only reload full bank when necessary
  if(myFirstLoad || myCurrentBank != instance()->debugger().getBank())
  {
    initialUpdate();
    myFirstLoad = false;
  }
  else  // only reload what's in current view
  {
    incrementalUpdate();
  }

  // Update romlist to point to current PC
  int pc = instance()->debugger().cpuDebug().pc();
  AddrToLine::iterator iter = myLineList.find(pc);
  if(iter != myLineList.end())
    myRomList->setHighlighted(iter->second);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RomWidget::initialUpdate()
{
  Debugger& dbg = instance()->debugger();

  myCurrentBank = dbg.getBank();

  // Fill romlist the current bank of source or disassembly
  if(mySourceAvailable)
    ; // FIXME
  else
  {
    // Clear old mappings
    myAddrList.clear();
    myLineList.clear();

    StringList label, data, disasm;
    BoolArray state;

    // Disassemble entire bank (up to 4096 lines)
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
void RomWidget::incrementalUpdate()
{
}
