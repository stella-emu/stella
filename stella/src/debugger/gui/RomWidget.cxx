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
// $Id: RomWidget.cxx,v 1.13 2005-10-14 13:50:00 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#include <sstream>

#include "Debugger.hxx"
#include "DebuggerParser.hxx"
#include "CpuDebug.hxx"
#include "DataGridWidget.hxx"
#include "PackedBitArray.hxx"
#include "GuiObject.hxx"
#include "InputTextDialog.hxx"
#include "ContextMenu.hxx"
#include "RomListWidget.hxx"
#include "RomWidget.hxx"

enum {
  kRomNameEntered = 'RWrn'
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RomWidget::RomWidget(GuiObject* boss, const GUI::Font& font, int x, int y)
  : Widget(boss, x, y, 16, 16),
    CommandSender(boss),
    myListIsDirty(true),
    mySourceAvailable(false),
    myCurrentBank(-1)
{
  int w = 58 * font.getMaxCharWidth(),
      h = 30 * font.getLineHeight(),
      xpos, ypos;
  StaticTextWidget* t;

  // Create bank editable area
  xpos = x + 40;  ypos = y + 7;
  t = new StaticTextWidget(boss, xpos, ypos,
                           font.getStringWidth("Current bank: "),
                           font.getFontHeight(),
                           "Current bank:", kTextAlignLeft);
  t->setFont(font);

  xpos += t->getWidth() + 10;
  myBank = new DataGridWidget(boss, font, xpos, ypos-2,
                              1, 1, 1, 2, kBASE_16_4);
  myBank->setTarget(this);
  myBank->setRange(0, instance()->debugger().bankCount());
  if(instance()->debugger().bankCount() <= 1)
    myBank->setEditable(false);
  addFocusWidget(myBank);

  // Show number of banks
  xpos += myBank->getWidth() + 45;
  t = new StaticTextWidget(boss, xpos, ypos,
                           font.getStringWidth("Total banks: "),
                           font.getFontHeight(),
                           "Total banks:", kTextAlignLeft);
  t->setFont(font);

  xpos += t->getWidth() + 10;
  myBankCount = new EditTextWidget(boss, xpos, ypos-2,
                                   20, font.getLineHeight(), "");
  myBankCount->setFont(font);
  myBankCount->setEditable(false);

  // Create rom listing
  xpos = x;  ypos += myBank->getHeight() + 4;
  myRomList = new RomListWidget(boss, font, xpos, ypos, w, h);
  myRomList->setTarget(this);
  myRomList->myMenu->setTarget(this);
  myRomList->setStyle(kSolidFill);
  addFocusWidget(myRomList);

  // Calculate real dimensions
  _w = myRomList->getWidth();
  _h = myRomList->getHeight();

  // Create dialog box for save ROM (get name)
  mySaveRom = new InputTextDialog(boss, font, _x + 50, _y + 80);
  mySaveRom->setTarget(this);
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
      {
        mySaveRom->setTitle("");
        mySaveRom->setEmitSignal(kRomNameEntered);
        parent()->addDialog(mySaveRom);
      }
      else if(rmb == "Set PC")
        setPC(myRomList->getSelected());

      break;
    }

    case kRomNameEntered:
    {
      const string& rom = mySaveRom->getResult();
      if(rom == "")
        mySaveRom->setTitle("Invalid name");
      else
      {
        saveROM(rom);
        parent()->removeDialog();
      }
      break;
    }

    case kDGItemDataChangedCmd:
    {
      int bank = myBank->getSelectedValue();
      instance()->debugger().setBank(bank);
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RomWidget::loadConfig()
{
  Debugger& dbg = instance()->debugger();
  bool bankChanged = myCurrentBank != dbg.getBank();

  // Only reload full bank when necessary
  if(myListIsDirty || bankChanged)
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
  // Take mirroring of PC into account
  int pc = dbg.cpuDebug().pc() | 0xe000;
  AddrToLine::iterator iter = myLineList.find(pc);

  // if current PC not found, do an update (we're executing what
  // we thought was an operand)

  // This doesn't help, and seems to actually hurt.
  /*
  if(iter == myLineList.end()) {
    incrementalUpdate(myRomList->currentPos(), myRomList->rows());
    iter = myLineList.find(pc);
  }
  */

  if(iter != myLineList.end())
    myRomList->setHighlighted(iter->second);

  // Set current bank
  IntArray alist;
  IntArray vlist;
  BoolArray changed;

  alist.push_back(-1);
  vlist.push_back(dbg.getBank());
  changed.push_back(bankChanged);
  myBank->setList(alist, vlist, changed);

  // Indicate total number of banks
  int bankCount = dbg.bankCount();
  myBankCount->setEditString(dbg.valueToString(bankCount));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RomWidget::initialUpdate()
{
  Debugger& dbg = instance()->debugger();
  PackedBitArray* bp = dbg.breakpoints();

  // Reading from ROM might trigger a bankswitch, so save the current bank
  myCurrentBank = dbg.getBank();

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

    // Disassemble entire bank (up to 4096 lines) and reset breakpoints
    dbg.disassemble(myAddrList, label, data, disasm, 0xf000, 4096);
    for(unsigned int i = 0; i < data.size(); ++i)
    {
      if(bp && bp->isSet(myAddrList[i]))
        state.push_back(true);
      else
        state.push_back(false);
    }

    // Create a mapping from addresses to line numbers
    myLineList.clear();
    for(unsigned int i = 0; i < myAddrList.size(); ++i)
      myLineList.insert(make_pair(myAddrList[i], i));

    myRomList->setList(label, data, disasm, state);
  }

  // Restore the old bank, in case we inadvertently switched while reading.
  dbg.setBank(myCurrentBank);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RomWidget::incrementalUpdate(int line, int rows)
{
  // TODO - implement this
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RomWidget::setBreak(int data)
{
  bool state = myRomList->getState(data);
  instance()->debugger().setBreakPoint(myAddrList[data], state);
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
void RomWidget::saveROM(const string& rom)
{
  ostringstream command;
  command << "saverom " << rom;
  instance()->debugger().run(command.str());
}
