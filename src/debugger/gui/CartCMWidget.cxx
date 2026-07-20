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

#include "CartCM.hxx"
#include "OSystem.hxx"
#include "Debugger.hxx"
#include "CartDebug.hxx"
#include "RiotDebug.hxx"
#include "DataGridWidget.hxx"
#include "EditTextWidget.hxx"
#include "PopUpWidget.hxx"
#include "ToggleBitWidget.hxx"
#include "CartCMWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeCMWidget::CartridgeCMWidget(
      GuiObject* boss, const GUI::Font& lfont, const GUI::Font& nfont,
      CartridgeCM& cart)
  : CartDebugWidget(boss, lfont, nfont),
    myCart{cart}
{
  constexpr uInt16 size = 4 * 4096;

  constexpr string_view info =
    "CM cartridge, four 4K banks + 2K RAM\n"
    "2K RAM accessible @ $1800 - $1FFF in read or write-only mode "
    "(no separate ports)\n"
    "All TIA controller registers (INPT0-INPT5) and RIOT SWCHA are "
    "used to control the cart functionality\n"
    "Startup bank = 3 (ROM), RAM disabled\n";

  createBaseInformation(size, "CompuMate", info);

  VariantList items;
  VarList::push_back(items, " 0 ");
  VarList::push_back(items, " 1 ");
  VarList::push_back(items, " 2 ");
  VarList::push_back(items, " 3 ");

  myBank = new PopUpWidget(boss, _font, 0, 0, items, "Set bank", 0, kBankChanged);
  myBank->setTarget(this);
  addFocusWidget(myBank);

  // The selector's box lines up with the info fields above it
  myLabelColumn.emplace_back(myBank);

  // Raw SWCHA value (this will be broken down further in other UI elements)
  mySWCHALabel = new StaticTextWidget(boss, _font, 0, 0, "Current SWCHA");
  mySWCHA = new ToggleBitWidget(boss, _nfont, 8, 1);
  mySWCHA->setTarget(this);
  mySWCHA->setEditable(false);

  // Current column number
  myColumnLabel = new StaticTextWidget(boss, _font, 0, 0, "Current column");
  myColumn = new DataGridWidget(boss, _nfont, 1, 1, 2, 8, Common::Base::Fmt::_16);
  myColumn->setTarget(this);
  myColumn->setEditable(false);

  // The relevant pins of SWCHA, as two columns of read-only checkboxes
  const auto addPin = [&](CheckboxWidget*& box, string_view label) {
    box = new CheckboxWidget(boss, _font, 0, 0, label);
    box->setTarget(this);
    box->setEditable(false);
  };

  addPin(myIncrease, "Increase Column");   // D6 (column part)
  addPin(myReset,    "Reset Column");      // D5 (column part)
  for(size_t i = 0; i < myRow.size(); ++i)
    addPin(myRow[i], std::format("Row {}", i));
  addPin(myFunc,   "FUNC key pressed");
  addPin(myShift,  "Shift key pressed");
  addPin(myAudIn,  "Audio Input");         // D7
  addPin(myAudOut, "Audio Output");        // D6 (audio part)

  // Ram state (combination of several bits in SWCHA)
  myRAMLabel = new StaticTextWidget(boss, _font, 0, 0, "Ram State");
  myRAM = new EditTextWidget(boss, _nfont, 0, 0,
                             EditTextWidget::calcWidth(_nfont, " Write-only "));
  myRAM->setEditable(false, true);

  reflow();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeCMWidget::layoutContent(GUI::BoxLayout& col) const
{
  using GUI::BoxLayout;
  using GUI::anchoredItem;
  using GUI::labeledRow;
  using Dir = BoxLayout::Dir;

  // The two SWCHA rows share a label column of their own, below the info block's.
  // The RAM state label has nothing to line up with, so it is a group of one --
  // which is where its clearance from the field beside it comes from
  GUI::alignLabels({{mySWCHALabel}, {myColumnLabel}});
  GUI::alignLabels({{myRAMLabel}});

  col.addAuto(anchoredItem(myBank));
  col.addSpace(_lineHeight / 2);
  col.addAuto(labeledRow(mySWCHALabel, mySWCHA));
  col.addAuto(labeledRow(myColumnLabel, myColumn));

  // The SWCHA pins, in two columns: the keyboard ones, then the audio ones with
  // the RAM state under them
  auto keys = std::make_unique<BoxLayout>(Dir::Vertical, VGAP);
  keys->addAuto(anchoredItem(myIncrease));
  keys->addAuto(anchoredItem(myReset));
  for(auto* row: myRow)
    keys->addAuto(anchoredItem(row));
  keys->addAuto(anchoredItem(myFunc));
  keys->addAuto(anchoredItem(myShift));

  auto audio = std::make_unique<BoxLayout>(Dir::Vertical, VGAP);
  audio->addAuto(anchoredItem(myAudIn));
  audio->addAuto(anchoredItem(myAudOut));
  audio->addSpace(_lineHeight / 2);
  audio->addAuto(labeledRow(myRAMLabel, myRAM));

  auto pins = std::make_unique<BoxLayout>(Dir::Horizontal, _fontWidth * 2);
  pins->addSpace(_fontWidth * 2);   // the pins sit in from the rows above them
  pins->addAuto(std::move(keys));
  pins->addAuto(std::move(audio));

  col.addSpace(_lineHeight / 2);
  col.addAuto(std::move(pins));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeCMWidget::saveOldState()
{
  myOldState.swcha = myCart.mySWCHA;
  myOldState.column = myCart.column();

  myOldState.internalram.clear();
  myOldState.internalram.assign(myCart.myRAM.data(),
                                myCart.myRAM.data() + internalRamSize());

  myOldState.bank = myCart.getBank();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeCMWidget::loadConfig()
{
  myBank->setSelectedIndex(myCart.getBank(), myCart.getBank() != myOldState.bank);

  RiotDebug& riot = Debugger::debugger().riotDebug();
  const auto& state = static_cast<const RiotState&>(riot.getState());

  const uInt8 swcha = myCart.mySWCHA;

  // SWCHA
  BoolArray oldbits, newbits, changed;
  Debugger::set_bits(myOldState.swcha, oldbits);
  Debugger::set_bits(swcha, newbits);

  for(uInt32 i = 0; i < oldbits.size(); ++i)
    changed.push_back(oldbits[i] != newbits[i]);
  mySWCHA->setState(newbits, changed);

  // Column
  myColumn->setList(0, myCart.column(), myCart.column() != myOldState.column);

  // Various bits from SWCHA and INPTx
  myIncrease->setState(swcha & 0x40);
  myReset->setState(swcha & 0x20);
  myRow[0]->setState(!(state.INPT4 & 0x80));
  myRow[1]->setState(!(swcha & 0x04));
  myRow[2]->setState(!(state.INPT5 & 0x80));
  myRow[3]->setState(!(swcha & 0x08));
  myFunc->setState(state.INPT0 & 0x80);
  myShift->setState(state.INPT3 & 0x80);

  // Audio in and out (used for communicating with the external cassette)
  myAudIn->setState(swcha & 0x80);
  myAudOut->setState(swcha & 0x40);

  // RAM state (several bits from SWCHA)
  const string_view ram = (swcha & 0x10) ? " Inactive" :
                          (swcha & 0x20) ? " Read-only" : " Write-only";
  myRAM->setText(ram, (swcha & 0x30) != (myOldState.swcha & 0x30));

  CartDebugWidget::loadConfig();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeCMWidget::handleCommand(CommandSender* sender,
                                      int cmd, int data, int id)
{
  if(cmd == kBankChanged)
  {
    myCart.unlockHotspots();
    myCart.mySWCHA &= 0xFC;
    myCart.mySWCHA |= myBank->getSelected();
    myCart.bank(myCart.mySWCHA & 0x03);
    myCart.lockHotspots();
    invalidate();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string CartridgeCMWidget::bankState()
{
  return std::format("Bank = {}, RAM is{}",
    myCart.getBank(),
    (myCart.mySWCHA & 0x10) ? " Inactive" :
    (myCart.mySWCHA & 0x20) ? " Read-only" : " Write-only");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 CartridgeCMWidget::internalRamSize()
{
  return 2048;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 CartridgeCMWidget::internalRamRPort(int start)
{
  return 0xF800 + start;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string CartridgeCMWidget::internalRamDescription()
{
  return "$F800 - $FFFF used for Exclusive Read\n"
         "              or Exclusive Write Access";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const ByteArray& CartridgeCMWidget::internalRamOld(int start, int count)
{
  myRamOld.clear();
  myRamOld.assign(myOldState.internalram.data() + start,
                  myOldState.internalram.data() + start + count);
  return myRamOld;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const ByteArray& CartridgeCMWidget::internalRamCurrent(int start, int count)
{
  myRamCurrent.clear();
  myRamCurrent.assign(myCart.myRAM.data() + start,
                      myCart.myRAM.data() + start + count);
  return myRamCurrent;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeCMWidget::internalRamSetValue(int addr, uInt8 value)
{
  myCart.myRAM[addr] = value;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 CartridgeCMWidget::internalRamGetValue(int addr)
{
  return myCart.myRAM[addr];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string CartridgeCMWidget::internalRamLabel(int addr)
{
  const CartDebug& dbg = instance().debugger().cartDebug();
  return dbg.getLabel(addr + 0xF800, false);
}
