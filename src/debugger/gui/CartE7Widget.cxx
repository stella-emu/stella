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

#include "CartE7.hxx"
#include "PopUpWidget.hxx"
#include "CartE7Widget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeE7Widget::CartridgeE7Widget(
    GuiObject* boss, const GUI::Font& lfont, const GUI::Font& nfont,
    int x, int y, int w, int h,
    CartridgeE7& cart)
  : CartDebugWidget(boss, lfont, nfont, x, y, w, h),
    myCart{cart}
{
  const string info = std::format(
    "E7 cartridge, {} 2K banks ROM + 2K RAM, \n"
    "  mapped into three segments\n"
    "Lower 2K accessible @ $F000 - $F7FF\n"
    "{}"
    "  1K RAM bank 3 (hotspot $FFE7)\n"
    "    $F400 - $F7FF (R), $F000 - $F3FF (W)\n"
    "256B RAM accessible @ $F800 - $F9FF\n"
    "  RAM banks 0 - 3 (hotspots $FFE8 - $FFEB)\n"
    "    $F900 - $F9FF (R), $F800 - $F8FF (W)\n"
    "Upper 1.5K ROM accessible @ $FA00 - $FFFF\n"
    "  Always points to last 1.5K of ROM\n"
    "Startup segments = 0 / 0 or undetermined\n",
    myCart.romBankCount() == 4 ? "four"
      : myCart.romBankCount() == 6 ? "six" : "eight",
    myCart.romBankCount() == 4
      ? "  ROM banks 0 - 2 (hotspots $FFE4 - $FFE6)\n"
      : myCart.romBankCount() == 6
        ? "  ROM Banks 0 - 4 (hotspots $FFE0/$FFE1\n    and $FFE4 - $FFE6)\n"
        : "  ROM Banks 0 - 6 (hotspots $FFE0 - $FFE6)\n");
#if 0
  // Eventually, we should query this from the debugger/disassembler
  uInt16 start = (cart.myImage[size - 3] << 8) | cart.myImage[size - 4];
  start -= start % 0x1000;
  info << "Bank RORG" << " = $" << HEX4 << start << "\n";
#endif

  initialize(boss, cart, info);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeE7Widget::initialize(GuiObject* boss,
    const CartridgeE7& cart, string_view info)
{
  const uInt32 size = cart.romBankCount() * CartridgeE7::BANK_SIZE;

  constexpr int xpos = 2;
  int ypos = addBaseInformation(size, "M Network", info, 15) + myLineHeight;

  VariantList items0, items1;
  for(int i = 0; std::cmp_less(i, cart.romBankCount()); ++i)
    VarList::push_back(items0, getSpotLower(i, myCart.romBankCount()));
  for(int i = 0; i < 4; ++i)
    VarList::push_back(items1, getSpotUpper(i));

  const int lwidth = _font.getStringWidth("Set bank for upper 256B segment "),
    fwidth = _font.getStringWidth("#3 - RAM ($FFEB)");
  myLower2K =
    new PopUpWidget(boss, _font, xpos, ypos - 2, fwidth, myLineHeight, items0,
                    "Set bank for lower 2K segment", lwidth, kLowerChanged);
  myLower2K->setTarget(this);
  addFocusWidget(myLower2K);
  ypos += myLower2K->getHeight() + 4;

  myUpper256B =
    new PopUpWidget(boss, _font, xpos, ypos - 2, fwidth, myLineHeight, items1,
                    "Set bank for upper 256B segment ", lwidth, kUpperChanged);
  myUpper256B->setTarget(this);
  addFocusWidget(myUpper256B);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeE7Widget::saveOldState()
{
  myOldState.internalram.clear();
  myOldState.internalram.assign(myCart.myRAM.data(),
                                myCart.myRAM.data() + internalRamSize());

  myOldState.lowerBank = myCart.myCurrentBank[0];
  myOldState.upperBank = myCart.myCurrentRAM;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeE7Widget::loadConfig()
{
  myLower2K->setSelectedIndex(myCart.myCurrentBank[0], myCart.myCurrentBank[0] != myOldState.lowerBank);
  myUpper256B->setSelectedIndex(myCart.myCurrentRAM, myCart.myCurrentRAM != myOldState.upperBank);

  CartDebugWidget::loadConfig();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeE7Widget::handleCommand(CommandSender* sender,
                                            int cmd, int data, int id)
{
  myCart.unlockHotspots();

  switch(cmd)
  {
    case kLowerChanged:
      myCart.bank(myLower2K->getSelected());
      break;
    case kUpperChanged:
      myCart.bankRAM(myUpper256B->getSelected());
      break;
    default:
      break;
  }

  myCart.lockHotspots();
  invalidate();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string CartridgeE7Widget::bankState()
{
  return std::format("Segments: {} / {}",
    getSpotLower(myCart.myCurrentBank[0], myCart.romBankCount()),
    getSpotUpper(myCart.myCurrentRAM));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 CartridgeE7Widget::internalRamSize()
{
  return CartridgeE7::RAM_SIZE;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 CartridgeE7Widget::internalRamRPort(int start)
{
  return 0x0000 + start;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string CartridgeE7Widget::internalRamDescription()
{
  return
    "First 1K accessible via:\n"
    "  $F000 - $F3FF used for write access\n"
    "  $F400 - $F7FF used for read access\n"
    "256 bytes of second 1K accessible via:\n"
    "  $F800 - $F8FF used for write access\n"
    "  $F900 - $F9FF used for read access";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const ByteArray& CartridgeE7Widget::internalRamOld(int start, int count)
{
  myRamOld.clear();
  myRamOld.assign(myOldState.internalram.data() + start,
                  myOldState.internalram.data() + start + count);
  return myRamOld;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const ByteArray& CartridgeE7Widget::internalRamCurrent(int start, int count)
{
  myRamCurrent.clear();
  myRamCurrent.assign(myCart.myRAM.data() + start,
                      myCart.myRAM.data() + start + count);
  return myRamCurrent;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeE7Widget::internalRamSetValue(int addr, uInt8 value)
{
  myCart.myRAM[addr] = value;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 CartridgeE7Widget::internalRamGetValue(int addr)
{
  return myCart.myRAM[addr];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string_view CartridgeE7Widget::getSpotLower(int idx, int bankcount)
{
  static constexpr std::array<string_view, 4> spot_lower_8K = {
    "#0 - ROM ($FFE4)", "#1 - ROM ($FFE5)", "#2 - ROM ($FFE6)", "#3 - RAM ($FFE7)"
  };
  static constexpr std::array<string_view, 6> spot_lower_12K = {
    "#0 - ROM ($FFE0)", "#1 - ROM ($FFE1)",
    "#2 - ROM ($FFE4)", "#3 - ROM ($FFE5)", "#4 - ROM ($FFE6)", "#5 - RAM ($FFE7)"
  };
  static constexpr std::array<string_view, 8> spot_lower_16K = {
    "#0 - ROM ($FFE0)", "#1 - ROM ($FFE1)", "#2 - ROM ($FFE2)", "#3 - ROM ($FFE3)",
    "#4 - ROM ($FFE4)", "#5 - ROM ($FFE5)", "#6 - ROM ($FFE6)", "#7 - RAM ($FFE7)"
  };

  return bankcount == 4
    ? spot_lower_8K[idx]
    : bankcount == 6
      ? spot_lower_12K[idx]
      : spot_lower_16K[idx];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string_view CartridgeE7Widget::getSpotUpper(int idx)
{
  static constexpr std::array<string_view, 4> spot_upper = {
    "#0 - RAM ($FFE8)", "#1 - RAM ($FFE9)", "#2 - RAM ($FFEA)", "#3 - RAM ($FFEB)"
  };

  return spot_upper[idx];
}
