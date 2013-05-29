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
// Copyright (c) 1995-2013 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
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
#include "DiStella.hxx"
#include "CpuDebug.hxx"
#include "GuiObject.hxx"
#include "DataGridWidget.hxx"
#include "EditTextWidget.hxx"
#include "PopUpWidget.hxx"
#include "StringList.hxx"
#include "ContextMenu.hxx"
#include "RomListWidget.hxx"
#include "RomWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RomWidget::RomWidget(GuiObject* boss, const GUI::Font& font,
                     int x, int y, int w, int h)
  : Widget(boss, font, x, y, w, h),
    CommandSender(boss),
    myListIsDirty(true)
{
  _type = kRomWidget;

  int xpos, ypos;
  StaticTextWidget* t;
  WidgetArray wid;

  // Show current bank state
  xpos = x;  ypos = y + 7;
  t = new StaticTextWidget(boss, font, xpos, ypos,
                           font.getStringWidth("Bank state: "),
                           font.getFontHeight(),
                           "Bank state: ", kTextAlignLeft);

  xpos += t->getWidth() + 5;
  myBank = new EditTextWidget(boss, font, xpos, ypos,
                              _w - 2 - xpos, font.getFontHeight());

  // Create rom listing
  xpos = x;  ypos += myBank->getHeight() + 4;

  myRomList = new RomListWidget(boss, font, xpos, ypos, _w - 4, _h - ypos - 2);
  addFocusWidget(myRomList);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RomWidget::~RomWidget()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RomWidget::loadConfig()
{
  Debugger& dbg = instance().debugger();
  CartDebug& cart = dbg.cartDebug();
  const CartState& state = (CartState&) cart.getState();
  const CartState& oldstate = (CartState&) cart.getOldState();

  // Fill romlist the current bank of source or disassembly
  myListIsDirty |= cart.disassemble("always", /*FIXME myResolveData->getSelectedTag().toString(),*/
                                    myListIsDirty);
  if(myListIsDirty)
  {
    myRomList->setList(cart.disassembly(), dbg.breakpoints());
    myListIsDirty = false;
  }

  // Update romlist to point to current PC (if it has changed)
  int pcline = cart.addressToLine(dbg.cpuDebug().pc());
  if(pcline >= 0 && pcline != myRomList->getHighlighted())
    myRomList->setHighlighted(pcline);

  // Set current bank state
  myBank->setText(state.bank, state.bank != oldstate.bank);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RomWidget::handleCommand(CommandSender* sender, int cmd, int data, int id)
{
cerr << cmd << " " << data << " " << id << endl;
  switch(cmd)
  {
    case RomListWidget::kBreakpointChangedCmd:
      // 'id' is the line in the disassemblylist to be accessed
      // 'data' is the state of the breakpoint at 'id'
      setBreak(id, data);
      // Refresh the romlist, since the breakpoint may not have
      // actually changed
      myRomList->setDirty();
      myRomList->draw();
      break;

    case RomListWidget::kRomChangedCmd:
      // 'data' is the line in the disassemblylist to be accessed
      patchROM(data, myRomList->getEditString());
      break;

#if 0
    case ContextMenu::kItemSelectedCmd:
    {
      const string& rmb = myRomList->myMenu->getSelectedTag().toString();

      if(rmb == "setpc")
        setPC(myRomList->getSelected());
      else if(rmb == "runtopc")
        runtoPC(myRomList->getSelected());
      else if(rmb == "disasm")
        invalidate();
      else if(rmb == "pcaddr")
      {
        DiStella::settings.show_addresses = !DiStella::settings.show_addresses;
        instance().settings().setValue("dis.showaddr",
            DiStella::settings.show_addresses);
        invalidate();
      }
      else if(rmb == "gfx")
      {
        if(DiStella::settings.gfx_format == kBASE_16)
        {
          DiStella::settings.gfx_format = kBASE_2;
          instance().settings().setValue("dis.gfxformat", "2");
        }
        else
        {
          DiStella::settings.gfx_format = kBASE_16;
          instance().settings().setValue("dis.gfxformat", "16");
        }
        invalidate();
      }
      else if(rmb == "relocate")
      {
        DiStella::settings.rflag = !DiStella::settings.rflag;
        instance().settings().setValue("dis.relocate",
            DiStella::settings.rflag);
        invalidate();
      }
      break;  // kCMenuItemSelectedCmd
    }

    case kResolveDataChanged:
      instance().settings().setValue("dis.resolvedata", myResolveData->getSelectedTag());
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
        dialog().close();
      }
      break;
    }
#endif
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RomWidget::setBreak(int disasm_line, bool state)
{
  const CartDebug::DisassemblyList& list =
      instance().debugger().cartDebug().disassembly().list;
  if(disasm_line >= (int)list.size())  return;

  if(list[disasm_line].address != 0 && list[disasm_line].bytes != "")
    instance().debugger().setBreakPoint(list[disasm_line].address, state);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RomWidget::setPC(int disasm_line)
{
  const CartDebug::DisassemblyList& list =
      instance().debugger().cartDebug().disassembly().list;
  if(disasm_line >= (int)list.size())  return;

  if(list[disasm_line].address != 0)
  {
    ostringstream command;
    command << "pc #" << list[disasm_line].address;
    instance().debugger().run(command.str());
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RomWidget::runtoPC(int disasm_line)
{
  const CartDebug::DisassemblyList& list =
      instance().debugger().cartDebug().disassembly().list;
  if(disasm_line >= (int)list.size())  return;

  if(list[disasm_line].address != 0)
  {
    ostringstream command;
    command << "runtopc #" << list[disasm_line].address;
    instance().debugger().run(command.str());
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RomWidget::patchROM(int disasm_line, const string& bytes)
{
  const CartDebug::DisassemblyList& list =
      instance().debugger().cartDebug().disassembly().list;
  if(disasm_line >= (int)list.size())  return;

  if(list[disasm_line].address != 0)
  {
    ostringstream command;

    // Temporarily set to correct base, so we don't have to prefix each byte
    // with the type of data
    BaseFormat oldbase = instance().debugger().parser().base();
    if(list[disasm_line].type == CartDebug::GFX)
      instance().debugger().parser().setBase(DiStella::settings.gfx_format);
    else
      instance().debugger().parser().setBase(kBASE_16);

    command << "rom #" << list[disasm_line].address << " " << bytes;
    instance().debugger().run(command.str());

    // Restore previous base
    instance().debugger().parser().setBase(oldbase);
  }
}
