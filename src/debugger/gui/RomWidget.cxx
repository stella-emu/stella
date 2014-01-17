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
// Copyright (c) 1995-2014 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

#include <sstream>

#include "Debugger.hxx"
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
RomWidget::RomWidget(GuiObject* boss, const GUI::Font& lfont, const GUI::Font& nfont,
                     int x, int y, int w, int h)
  : Widget(boss, lfont, x, y, w, h),
    CommandSender(boss),
    myListIsDirty(true)
{
  int xpos, ypos;
  StaticTextWidget* t;
  WidgetArray wid;

  // Show current bank state
  xpos = x;  ypos = y + 7;
  t = new StaticTextWidget(boss, lfont, xpos, ypos,
                           lfont.getStringWidth("Bank state: "),
                           lfont.getFontHeight(),
                           "Bank state: ", kTextAlignLeft);

  xpos += t->getWidth() + 5;
  myBank = new EditTextWidget(boss, nfont, xpos, ypos-1,
                              _w - 2 - xpos, nfont.getLineHeight());

  // Create rom listing
  xpos = x;  ypos += myBank->getHeight() + 4;

  myRomList = new RomListWidget(boss, lfont, nfont, xpos, ypos, _w - 4, _h - ypos - 2);
  myRomList->setTarget(this);
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
  myListIsDirty |= cart.disassemble(myListIsDirty);
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
  switch(cmd)
  {
    case RomListWidget::kBPointChangedCmd:
      // 'data' is the line in the disassemblylist to be accessed
      // 'id' is the state of the breakpoint at 'data'
      setBreak(data, id);
      // Refresh the romlist, since the breakpoint may not have
      // actually changed
      myRomList->setDirty();
      myRomList->draw();
      break;

    case RomListWidget::kRomChangedCmd:
      // 'data' is the line in the disassemblylist to be accessed
      // 'id' is the base to use for the data to be changed
      patchROM(data, myRomList->getText(), (Common::Base::Format)id);
      break;

    case RomListWidget::kSetPCCmd:
      // 'data' is the line in the disassemblylist to be accessed
      setPC(data);
      break;

    case RomListWidget::kRuntoPCCmd:
      // 'data' is the line in the disassemblylist to be accessed
      runtoPC(data);
      break;

    case RomListWidget::kDisassembleCmd:
      invalidate();
      break;

    case RomListWidget::kTentativeCodeCmd:
    {
      // 'data' is the boolean value
      DiStella::settings.resolve_code = data;
      instance().settings().setValue("dis.resolve",
          DiStella::settings.resolve_code);
      invalidate();
      break;
    }

    case RomListWidget::kPCAddressesCmd:
      // 'data' is the boolean value
      DiStella::settings.show_addresses = data;
      instance().settings().setValue("dis.showaddr",
          DiStella::settings.show_addresses);
      invalidate();
      break;

    case RomListWidget::kGfxAsBinaryCmd:
      // 'data' is the boolean value
      if(data)
      {
        DiStella::settings.gfx_format = Common::Base::F_2;
        instance().settings().setValue("dis.gfxformat", "2");
      }
      else
      {
        DiStella::settings.gfx_format = Common::Base::F_16;
        instance().settings().setValue("dis.gfxformat", "16");
      }
      invalidate();
      break;

    case RomListWidget::kAddrRelocationCmd:
      // 'data' is the boolean value
      DiStella::settings.rflag = data;
      instance().settings().setValue("dis.relocate",
          DiStella::settings.rflag);
      invalidate();
      break;
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
void RomWidget::patchROM(int disasm_line, const string& bytes,
                         Common::Base::Format base)
{
  const CartDebug::DisassemblyList& list =
      instance().debugger().cartDebug().disassembly().list;
  if(disasm_line >= (int)list.size())  return;

  if(list[disasm_line].address != 0)
  {
    ostringstream command;

    // Temporarily set to correct base, so we don't have to prefix each byte
    // with the type of data
    Common::Base::Format oldbase = Common::Base::format();

    Common::Base::setFormat(base);
    command << "rom #" << list[disasm_line].address << " " << bytes;
    instance().debugger().run(command.str());

    // Restore previous base
    Common::Base::setFormat(oldbase);
  }
}
