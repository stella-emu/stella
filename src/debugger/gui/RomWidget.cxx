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
// Copyright (c) 1995-2026 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include "OSystem.hxx"
#include "Settings.hxx"
#include "Debugger.hxx"
#include "CartDebug.hxx"
#include "DiStella.hxx"
#include "CpuDebug.hxx"
#include "GuiObject.hxx"
#include "Font.hxx"
#include "DataGridWidget.hxx"
#include "EditTextWidget.hxx"
#include "RomListWidget.hxx"
#include "Layout.hxx"
#include "RomWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RomWidget::RomWidget(GuiObject* boss, const GUI::Font& lfont, const GUI::Font& nfont)
  : Widget(boss, lfont, 0, 0),
    CommandSender(boss)
{
  // Create the bank display and the listing at a placeholder position/size;
  // reflow() positions and sizes them for the area the widget occupies
  // NOLINTBEGIN(cppcoreguidelines-prefer-member-initializer)
  myInfoLabel = new StaticTextWidget(boss, lfont, "Info ");

  myBank = new EditTextWidget(boss, nfont, 1);
  myBank->setEditable(false);

  myRomList = new RomListWidget(boss, lfont, nfont);
  // NOLINTEND(cppcoreguidelines-prefer-member-initializer)
  myRomList->setTarget(this);
  addFocusWidget(myRomList);

  reflow();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RomWidget::setArea(int x, int y, int w, int h)
{
  Widget::setArea(x, y, w, h);
  reflow();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RomWidget::reflow()
{
  using GUI::BoxLayout;
  using GUI::anchoredItem;
  using GUI::alignedItem;
  using GUI::HAlign;
  using GUI::VAlign;
  using GUI::widgetItem;
  using Dir = BoxLayout::Dir;

  // This tab insets itself by the same small border on every side -- the listing
  // fills it, and its frame is meant to sit close to the tab's own, as it did
  // before the engine laid this widget out.  VGAP is the gap BETWEEN the two
  // rows, and was standing in for the vertical border as well, which left the
  // listing twice as far off the bottom of the tab as it should be
  constexpr int HBORDER = 2, VBORDER = 2, VGAP = 4;

  // The label and the bank display are sibling widgets parented to the boss,
  // not children of this widget, so they are positioned explicitly here
  BoxLayout root(Dir::Vertical, VGAP, HBORDER, VBORDER);

  // The bank info row: a label, then the bank display filling the rest
  auto infoRow = std::make_unique<BoxLayout>(Dir::Horizontal);
  infoRow->addAuto(anchoredItem(myInfoLabel));
  infoRow->addStretch(alignedItem(myBank, HAlign::Fill, VAlign::Center));
  root.addFixed(std::move(infoRow), std::max(_lineHeight, myBank->getHeight()));

  // The disassembly fills whatever is left
  root.addStretch(widgetItem(myRomList));

  root.doLayout(_x, _y, _w, _h);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RomWidget::loadConfig()
{
  const Debugger& dbg = instance().debugger();
  CartDebug& cart = dbg.cartDebug();
  const auto& state = static_cast<const CartState&>(cart.getState());
  const auto& oldstate = static_cast<const CartState&>(cart.getOldState());

  // Fill romlist the current bank of source or disassembly
  myListIsDirty |= cart.disassemblePC(myListIsDirty);
  if(myListIsDirty)
  {
    myRomList->setList(cart.disassembly());
    myListIsDirty = false;
  }

  // Update romlist to point to current PC (if it has changed)
  const int pcline = cart.addressToLine(dbg.cpuDebug().pc());

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
      toggleBreak(data);
      break;

    case RomListWidget::kRomChangedCmd:
      // 'data' is the line in the disassemblylist to be accessed
      // 'id' is the base to use for the data to be changed
      patchROM(data, myRomList->getText(), static_cast<Common::Base::Fmt>(id));
      break;

    case RomListWidget::kSetPCCmd:
      // 'data' is the line in the disassemblylist to be accessed
      setPC(data);
      break;

    case RomListWidget::kRuntoPCCmd:
      // 'data' is the line in the disassemblylist to be accessed
      runtoPC(data);
      break;

    case RomListWidget::kSetTimerCmd:
      // 'data' is the line in the disassemblylist to be accessed
      setTimer(data);
      break;

    case RomListWidget::kDisassembleCmd:
      // 'data' is the line in the disassemblylist to be accessed
      disassemble(data);
      break;

    case RomListWidget::kTentativeCodeCmd:
    {
      // 'data' is the boolean value
      DiStella::settings.resolveCode = data;
      instance().settings().setValue("dis.resolve",
          DiStella::settings.resolveCode);
      invalidate();
      break;
    }

    case RomListWidget::kPCAddressesCmd:
      // 'data' is the boolean value
      DiStella::settings.showAddresses = data;
      instance().settings().setValue("dis.showaddr",
          DiStella::settings.showAddresses);
      invalidate();
      break;

    case RomListWidget::kGfxAsBinaryCmd:
      // 'data' is the boolean value
      if(data)
      {
        DiStella::settings.gfxFormat = Common::Base::Fmt::_2;
        instance().settings().setValue("dis.gfxformat", "2");
      }
      else
      {
        DiStella::settings.gfxFormat = Common::Base::Fmt::_16;
        instance().settings().setValue("dis.gfxformat", "16");
      }
      invalidate();
      break;

    case RomListWidget::kAddrRelocationCmd:
      // 'data' is the boolean value
      DiStella::settings.rFlag = data;
      instance().settings().setValue("dis.relocate",
          DiStella::settings.rFlag);
      invalidate();
      break;

    default:
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RomWidget::toggleBreak(int disasm_line)
{
  const uInt16 address = getAddress(disasm_line);

  if(address != 0)
  {
    const Debugger& debugger = instance().debugger();

    debugger.toggleBreakPoint(address, debugger.cartDebug().getBank(address));
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RomWidget::setPC(int disasm_line)
{
  const uInt16 address = getAddress(disasm_line);

  if(address != 0)
    instance().debugger().run(std::format("pc #{}", address));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RomWidget::runtoPC(int disasm_line)
{
  const uInt16 address = getAddress(disasm_line);

  if(address != 0)
    instance().frameBuffer().showTextMessage(
      instance().debugger().run(std::format("runtopc #{}", address))
    );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RomWidget::setTimer(int disasm_line)
{
  const uInt16 address = getAddress(disasm_line);

  if(address != 0)
  {
    Debugger& dbg = instance().debugger();
    const string& msg = dbg.run(std::format("timer #{} {}",
      address, dbg.cartDebug().getBank(address)));
    instance().frameBuffer().showTextMessage(msg);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RomWidget::disassemble(int disasm_line)
{
  const uInt16 address = getAddress(disasm_line);

  if(address != 0)
  {
    CartDebug& cart = instance().debugger().cartDebug();

    cart.disassembleAddr(address, true);
    invalidate();
    scrollTo(cart.addressToLine(address)); // the line might have been changed
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RomWidget::patchROM(int disasm_line, string_view bytes,
                         Common::Base::Fmt base)
{
  const uInt16 address = getAddress(disasm_line);

  if(address != 0)
  {
    // Temporarily set to correct base, so we don't have to prefix each byte
    // with the type of data
    const Common::Base::Fmt oldbase = Common::Base::format();

    Common::Base::setFormat(base);
    instance().debugger().run(std::format("rom #{} {}", address, bytes));

    // Restore previous base
    Common::Base::setFormat(oldbase);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt16 RomWidget::getAddress(int disasm_line)
{
  const CartDebug::DisassemblyList& list =
    instance().debugger().cartDebug().disassembly().list;

  if(std::cmp_less(disasm_line, list.size()) && list[disasm_line].address != 0)
    return list[disasm_line].address;
  else
    return 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RomWidget::scrollTo(int line)
{
  myRomList->setSelected(line);
}
