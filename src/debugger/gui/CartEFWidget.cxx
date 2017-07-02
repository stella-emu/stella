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

#include "CartEF.hxx"
#include "PopUpWidget.hxx"
#include "CartEFWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeEFWidget::CartridgeEFWidget(
      GuiObject* boss, const GUI::Font& lfont, const GUI::Font& nfont,
      int x, int y, int w, int h, CartridgeEF& cart)
  : CartDebugWidget(boss, lfont, nfont, x, y, w, h),
    myCart(cart)
{
  uInt32 size = 16 * 4096;

  ostringstream info;
  info << "64K H. Runner EF cartridge, 16 4K banks\n"
       << "Startup bank = " << cart.myStartBank << "\n";

  // Eventually, we should query this from the debugger/disassembler
  for(uInt32 i = 0, offset = 0xFFC, spot = 0xFE0; i < 16; ++i, offset += 0x1000)
  {
    uInt16 start = (cart.myImage[offset+1] << 8) | cart.myImage[offset];
    start -= start % 0x1000;
    info << "Bank " << std::dec << i << " @ $" << Common::Base::HEX4 << start << " - "
         << "$" << (start + 0xFFF) << " (hotspot = $F" << (spot+i) << ")\n";
  }

  int xpos = 10,
      ypos = addBaseInformation(size, "Paul Slocum / Homestar Runner",
                                info.str()) + myLineHeight;

  VariantList items;
  VarList::push_back(items, " 0 ($FFE0)");
  VarList::push_back(items, " 1 ($FFE1)");
  VarList::push_back(items, " 2 ($FFE2)");
  VarList::push_back(items, " 3 ($FFE3)");
  VarList::push_back(items, " 4 ($FFE4)");
  VarList::push_back(items, " 5 ($FFE5)");
  VarList::push_back(items, " 6 ($FFE6)");
  VarList::push_back(items, " 7 ($FFE7)");
  VarList::push_back(items, " 8 ($FFE8)");
  VarList::push_back(items, " 9 ($FFE9)");
  VarList::push_back(items, "10 ($FFEA)");
  VarList::push_back(items, "11 ($FFEB)");
  VarList::push_back(items, "12 ($FFEC)");
  VarList::push_back(items, "13 ($FFED)");
  VarList::push_back(items, "14 ($FFEE)");
  VarList::push_back(items, "15 ($FFEF)");
  myBank =
    new PopUpWidget(boss, _font, xpos, ypos-2, _font.getStringWidth("15 ($FFE0) "),
                    myLineHeight, items, "Set bank ",
                    _font.getStringWidth("Set bank "), kBankChanged);
  myBank->setTarget(this);
  addFocusWidget(myBank);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeEFWidget::loadConfig()
{
  myBank->setSelectedIndex(myCart.myCurrentBank);

  CartDebugWidget::loadConfig();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeEFWidget::handleCommand(CommandSender* sender,
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
string CartridgeEFWidget::bankState()
{
  ostringstream& buf = buffer();

  static const char* const spot[] = {
    "$FFE0", "$FFE1", "$FFE2", "$FFE3", "$FFE4", "$FFE5", "$FFE6", "$FFE7",
    "$FFE8", "$FFE9", "$FFEA", "$FFEB", "$FFEC", "$FFED", "$FFEE", "$FFEF"
  };
  buf << "Bank = " << std::dec << myCart.myCurrentBank
      << ", hotspot = " << spot[myCart.myCurrentBank];

  return buf.str();
}
