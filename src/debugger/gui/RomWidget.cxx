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
// Copyright (c) 1995-2010 by Bradford W. Mott and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#include <sstream>

#include "Debugger.hxx"
#include "DebuggerParser.hxx"
#include "CartDebug.hxx"
#include "CpuDebug.hxx"
#include "GuiObject.hxx"
#include "InputTextDialog.hxx"
#include "EditTextWidget.hxx"
#include "PopUpWidget.hxx"
#include "StringList.hxx"
#include "ContextMenu.hxx"
#include "RomListWidget.hxx"
#include "RomWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RomWidget::RomWidget(GuiObject* boss, const GUI::Font& font, int x, int y)
  : Widget(boss, font, x, y, 16, 16),
    CommandSender(boss),
    myListIsDirty(true),
    myCurrentBank(-1)
{
  _type = kRomWidget;

  int xpos, ypos;
  StaticTextWidget* t;
  WidgetArray wid;

  // Show current bank
  xpos = x;  ypos = y + 7;
  t = new StaticTextWidget(boss, font, xpos, ypos,
                           font.getStringWidth("Bank (current/total):"),
                           font.getFontHeight(),
                           "Bank (current/total):", kTextAlignLeft);

  xpos += t->getWidth() + 10;
  myBank = new EditTextWidget(boss, font, xpos, ypos-2,
                              4 * font.getMaxCharWidth(),
                              font.getLineHeight(), "");
  myBank->setEditable(false);

  // Show number of banks
  xpos += myBank->getWidth() + 5;
  myBankCount =
    new EditTextWidget(boss, font, xpos, ypos-2, 4 * font.getMaxCharWidth(),
                       font.getLineHeight(), "");
  myBankCount->setEditable(false);

  // 'Autocode' setting for Distella
  xpos += myBankCount->getWidth() + 20;
  StringMap items;
  items.push_back("Never", "0");
  items.push_back("Always", "1");
  items.push_back("Automatic", "2");
  myAutocode =
    new PopUpWidget(boss, font, xpos, ypos-2, font.getStringWidth("Automatic"),
                    font.getLineHeight(), items,
                    "Determine code: ", font.getStringWidth("Determine code: "),
                    kAutocodeChanged);
  myAutocode->setTarget(this);
  addFocusWidget(myAutocode);

  // Create rom listing
  xpos = x;  ypos += myBank->getHeight() + 4;
  const GUI::Rect& dialog = instance().debugger().getDialogBounds();
  int w = dialog.width() - x - 5, h = dialog.height() - ypos - 3;

  myRomList = new RomListWidget(boss, font, xpos, ypos, w, h);
  myRomList->setTarget(this);
  myRomList->myMenu->setTarget(this);
  addFocusWidget(myRomList);

  // Calculate real dimensions
  _w = myRomList->getWidth();
  _h = myRomList->getHeight();

  // Create dialog box for save ROM (get name)
  StringList label;
  label.push_back("Filename: ");
  mySaveRom = new InputTextDialog(boss, font, label);
  mySaveRom->setTarget(this);

  // By default, we try to automatically determine code vs. data sections
  myAutocode->setSelected(instance().settings().getString("autocode"), "2");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RomWidget::~RomWidget()
{
  delete mySaveRom;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RomWidget::loadConfig()
{
  Debugger& dbg = instance().debugger();
  CartDebug& cart = dbg.cartDebug();
  bool bankChanged = myCurrentBank != cart.getBank();
  myCurrentBank = cart.getBank();

  // Fill romlist the current bank of source or disassembly
  myListIsDirty |= cart.disassemble(myAutocode->getSelectedTag(), myListIsDirty);
  if(myListIsDirty)
  {
cerr << "list is dirty, re-disassembled\n";
    myRomList->setList(cart.disassemblyList(), dbg.breakpoints());
    myListIsDirty = false;
  }

  // Update romlist to point to current PC
  int pcline = cart.addressToLine(dbg.cpuDebug().pc());
cerr << "PC = " << hex <<  dbg.cpuDebug().pc() << ", line = " << dec << pcline << endl;
  if(pcline >= 0)
    myRomList->setHighlighted(pcline);

  // Set current bank and number of banks
  myBank->setEditString(instance().debugger().valueToString(myCurrentBank, kBASE_10), bankChanged);
  myBankCount->setEditString(instance().debugger().valueToString(cart.bankCount(), kBASE_10));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RomWidget::handleCommand(CommandSender* sender, int cmd, int data, int id)
{
  switch(cmd)
  {
    case kRLBreakpointChangedCmd:
      // 'id' is the line in the disassemblylist to be accessed
      // 'data' is the state of the breakpoint at 'id'
      setBreak(id, data);
      // Refresh the romlist, since the breakpoint may not have
      // actually changed
      myRomList->setDirty();
      myRomList->draw();
      break;

    case kRLRomChangedCmd:
      // 'data' is the line in the disassemblylist to be accessed
      patchROM(data, myRomList->getEditString());
      break;

    case kCMenuItemSelectedCmd:
    {
      const string& rmb = myRomList->myMenu->getSelectedTag();

      if(rmb == "saverom")
      {
        mySaveRom->show(_x + 50, _y + 80);
        mySaveRom->setTitle("");
        mySaveRom->setEmitSignal(kRomNameEntered);
      }
      else if(rmb == "setpc")
        setPC(myRomList->getSelected());

      break;
    }

    case kAutocodeChanged:
      instance().settings().setString("autocode", myAutocode->getSelectedTag());
      invalidate();
      loadConfig();
      break;

    case kRomNameEntered:
    {
      const string& rom = mySaveRom->getResult();
      if(rom == "")
        mySaveRom->setTitle("Invalid name");
      else
      {
        saveROM(rom);
        parent().removeDialog();
      }
      break;
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RomWidget::setBreak(int disasm_line, bool state)
{
  const CartDebug::DisassemblyList& list =
      instance().debugger().cartDebug().disassemblyList();
  if(disasm_line >= (int)list.size())  return;

  if(list[disasm_line].address != 0 && list[disasm_line].bytes != "")
    instance().debugger().setBreakPoint(list[disasm_line].address, state);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RomWidget::setPC(int disasm_line)
{
  const CartDebug::DisassemblyList& list =
      instance().debugger().cartDebug().disassemblyList();
  if(disasm_line >= (int)list.size())  return;

  if(list[disasm_line].address != 0)
  {
    ostringstream command;
    command << "pc #" << list[disasm_line].address;
    instance().debugger().run(command.str());
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RomWidget::patchROM(int disasm_line, const string& bytes)
{
  const CartDebug::DisassemblyList& list =
      instance().debugger().cartDebug().disassemblyList();
  if(disasm_line >= (int)list.size())  return;

  if(list[disasm_line].address != 0)
  {
    ostringstream command;

    // Temporarily set to base 16, since that's the format the disassembled
    // byte string is in.  This eliminates the need to prefix each byte with
    // a '$' character
    BaseFormat oldbase = instance().debugger().parser().base();
    instance().debugger().parser().setBase(kBASE_16);

    command << "rom #" << list[disasm_line].address << " " << bytes;
    instance().debugger().run(command.str());

    // Restore previous base
    instance().debugger().parser().setBase(oldbase);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RomWidget::saveROM(const string& rom)
{
  ostringstream command;
  command << "saverom " << rom;
  instance().debugger().run(command.str());
}
