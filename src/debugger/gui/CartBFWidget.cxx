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
// Copyright (c) 1995-2017 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include "CartBF.hxx"
#include "PopUpWidget.hxx"
#include "CartBFWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeBFWidget::CartridgeBFWidget(
      GuiObject* boss, const GUI::Font& lfont, const GUI::Font& nfont,
      int x, int y, int w, int h, CartridgeBF& cart)
  : CartDebugWidget(boss, lfont, nfont, x, y, w, h),
    myCart(cart)
{
  uInt32 size = 64 * 4096;

  ostringstream info;
  info << "BF cartridge, 64 4K banks\n"
       << "Startup bank = " << cart.myStartBank << "\n";

  // Eventually, we should query this from the debugger/disassembler
  for(uInt32 i = 0, offset = 0xFFC, spot = 0xF80; i < 64; ++i, offset += 0x1000)
  {
    uInt16 start = (cart.myImage[offset+1] << 8) | cart.myImage[offset];
    start -= start % 0x1000;
    info << "Bank " << std::dec << i << " @ $" << Common::Base::HEX4 << start << " - "
         << "$" << (start + 0xFFF) << " (hotspot = $F" << (spot+i) << ")\n";
  }

  int xpos = 10,
      ypos = addBaseInformation(size, "CPUWIZ", info.str()) + myLineHeight;

  VariantList items;
  VarList::push_back(items, " 0 ($FF80)");
  VarList::push_back(items, " 1 ($FF81)");
  VarList::push_back(items, " 2 ($FF82)");
  VarList::push_back(items, " 3 ($FF83)");
  VarList::push_back(items, " 4 ($FF84)");
  VarList::push_back(items, " 5 ($FF85)");
  VarList::push_back(items, " 6 ($FF86)");
  VarList::push_back(items, " 7 ($FF87)");
  VarList::push_back(items, " 8 ($FF88)");
  VarList::push_back(items, " 9 ($FF89)");
  VarList::push_back(items, "10 ($FF8A)");
  VarList::push_back(items, "11 ($FF8B)");
  VarList::push_back(items, "12 ($FF8C)");
  VarList::push_back(items, "13 ($FF8D)");
  VarList::push_back(items, "14 ($FF8E)");
  VarList::push_back(items, "15 ($FF8F)");
  VarList::push_back(items, "16 ($FF90)");
  VarList::push_back(items, "17 ($FF91)");
  VarList::push_back(items, "18 ($FF92)");
  VarList::push_back(items, "19 ($FF93)");
  VarList::push_back(items, "20 ($FF94)");
  VarList::push_back(items, "21 ($FF95)");
  VarList::push_back(items, "22 ($FF96)");
  VarList::push_back(items, "23 ($FF97)");
  VarList::push_back(items, "24 ($FF98)");
  VarList::push_back(items, "25 ($FF99)");
  VarList::push_back(items, "26 ($FF9A)");
  VarList::push_back(items, "27 ($FF9B)");
  VarList::push_back(items, "28 ($FF9C)");
  VarList::push_back(items, "29 ($FF9D)");
  VarList::push_back(items, "30 ($FF9E)");
  VarList::push_back(items, "31 ($FF9F)");
  VarList::push_back(items, "32 ($FFA0)");
  VarList::push_back(items, "33 ($FFA1)");
  VarList::push_back(items, "34 ($FFA2)");
  VarList::push_back(items, "35 ($FFA3)");
  VarList::push_back(items, "36 ($FFA4)");
  VarList::push_back(items, "37 ($FFA5)");
  VarList::push_back(items, "38 ($FFA6)");
  VarList::push_back(items, "39 ($FFA7)");
  VarList::push_back(items, "40 ($FFA8)");
  VarList::push_back(items, "41 ($FFA9)");
  VarList::push_back(items, "42 ($FFAA)");
  VarList::push_back(items, "43 ($FFAB)");
  VarList::push_back(items, "44 ($FFAC)");
  VarList::push_back(items, "45 ($FFAD)");
  VarList::push_back(items, "46 ($FFAE)");
  VarList::push_back(items, "47 ($FFAF)");
  VarList::push_back(items, "48 ($FFB0)");
  VarList::push_back(items, "49 ($FFB1)");
  VarList::push_back(items, "50 ($FFB2)");
  VarList::push_back(items, "51 ($FFB3)");
  VarList::push_back(items, "52 ($FFB4)");
  VarList::push_back(items, "53 ($FFB5)");
  VarList::push_back(items, "54 ($FFB6)");
  VarList::push_back(items, "55 ($FFB7)");
  VarList::push_back(items, "56 ($FFB8)");
  VarList::push_back(items, "57 ($FFB9)");
  VarList::push_back(items, "58 ($FFBA)");
  VarList::push_back(items, "59 ($FFBB)");
  VarList::push_back(items, "60 ($FFBC)");
  VarList::push_back(items, "61 ($FFBD)");
  VarList::push_back(items, "62 ($FFBE)");
  VarList::push_back(items, "63 ($FFBF)");

  myBank =
    new PopUpWidget(boss, _font, xpos, ypos-2, _font.getStringWidth("64 ($FFBF) "),
                    myLineHeight, items, "Set bank ",
                    _font.getStringWidth("Set bank "), kBankChanged);
  myBank->setTarget(this);
  addFocusWidget(myBank);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeBFWidget::loadConfig()
{
  myBank->setSelectedIndex(myCart.myCurrentBank);

  CartDebugWidget::loadConfig();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeBFWidget::handleCommand(CommandSender* sender,
                                      int cmd, int data, int id)
{
  if(cmd == kBankChanged)
  {
    myCart.unlockBank();
    myCart.bank(myBank->getSelected());
    myCart.lockBank();
    invalidate();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string CartridgeBFWidget::bankState()
{
  ostringstream& buf = buffer();

  static const char* const spot[] = {
    "$FF80", "$FF81", "$FF82", "$FF83", "$FF84", "$FF85", "$FF86", "$FF87",
    "$FF88", "$FF89", "$FF8A", "$FF8B", "$FF8C", "$FF8D", "$FF8E", "$FF8F",
    "$FF90", "$FF91", "$FF92", "$FF93", "$FF94", "$FF95", "$FF96", "$FF97",
    "$FF98", "$FF99", "$FF9A", "$FF9B", "$FF9C", "$FF9D", "$FF9E", "$FF9F",
    "$FFA0", "$FFA1", "$FFA2", "$FFA3", "$FFA4", "$FFA5", "$FFA6", "$FFA7",
    "$FFA8", "$FFA9", "$FFAA", "$FFAB", "$FFAC", "$FFAD", "$FFAE", "$FFAF",
    "$FFB0", "$FFB1", "$FFB2", "$FFB3", "$FFB4", "$FFB5", "$FFB6", "$FFB7",
    "$FFB8", "$FFB9", "$FFBA", "$FFBB", "$FFBC", "$FFBD", "$FFBE", "$FFBF"
  };
  buf << "Bank = " << std::dec << myCart.myCurrentBank
      << ", hotspot = " << spot[myCart.myCurrentBank];

  return buf.str();
}
