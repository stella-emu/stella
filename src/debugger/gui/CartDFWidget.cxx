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

#include "CartDF.hxx"
#include "PopUpWidget.hxx"
#include "CartDFWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeDFWidget::CartridgeDFWidget(
      GuiObject* boss, const GUI::Font& lfont, const GUI::Font& nfont,
      int x, int y, int w, int h, CartridgeDF& cart)
  : CartDebugWidget(boss, lfont, nfont, x, y, w, h),
    myCart(cart)
{
  uInt32 size = 32 * 4096;

  ostringstream info;
  info << "EF 2 cartridge, 32 4K banks\n"
       << "Startup bank = " << cart.myStartBank << "\n";

  // Eventually, we should query this from the debugger/disassembler
  for(uInt32 i = 0, offset = 0xFFC, spot = 0xFD0; i < 32; ++i, offset += 0x1000)
  {
    uInt16 start = (cart.myImage[offset+1] << 8) | cart.myImage[offset];
    start -= start % 0x1000;
    info << "Bank " << std::dec << i << " @ $" << Common::Base::HEX4 << start << " - "
         << "$" << (start + 0xFFF) << " (hotspot = $F" << (spot+i) << ")\n";
  }

  int xpos = 10,
      ypos = addBaseInformation(size, "CPUWIZ", info.str()) + myLineHeight;

  VariantList items;
  VarList::push_back(items, " 0 ($FFC0)");
  VarList::push_back(items, " 1 ($FFC1)");
  VarList::push_back(items, " 2 ($FFC2)");
  VarList::push_back(items, " 3 ($FFC3)");
  VarList::push_back(items, " 4 ($FFC4)");
  VarList::push_back(items, " 5 ($FFC5)");
  VarList::push_back(items, " 6 ($FFC6)");
  VarList::push_back(items, " 7 ($FFC7)");
  VarList::push_back(items, " 8 ($FFC8)");
  VarList::push_back(items, " 9 ($FFC9)");
  VarList::push_back(items, "10 ($FFCA)");
  VarList::push_back(items, "11 ($FFCB)");
  VarList::push_back(items, "12 ($FFCC)");
  VarList::push_back(items, "13 ($FFCD)");
  VarList::push_back(items, "14 ($FFCE)");
  VarList::push_back(items, "15 ($FFCF)");
  VarList::push_back(items, "16 ($FFD0)");
  VarList::push_back(items, "17 ($FFD1)");
  VarList::push_back(items, "18 ($FFD2)");
  VarList::push_back(items, "19 ($FFD3)");
  VarList::push_back(items, "20 ($FFD4)");
  VarList::push_back(items, "21 ($FFD5)");
  VarList::push_back(items, "22 ($FFD6)");
  VarList::push_back(items, "23 ($FFD7)");
  VarList::push_back(items, "24 ($FFD8)");
  VarList::push_back(items, "25 ($FFD9)");
  VarList::push_back(items, "26 ($FFDA)");
  VarList::push_back(items, "27 ($FFDB)");
  VarList::push_back(items, "28 ($FFDC)");
  VarList::push_back(items, "29 ($FFDD)");
  VarList::push_back(items, "30 ($FFDE)");
  VarList::push_back(items, "31 ($FFDF)");

  myBank =
    new PopUpWidget(boss, _font, xpos, ypos-2, _font.getStringWidth("31 ($FFDF) "),
                    myLineHeight, items, "Set bank ",
                    _font.getStringWidth("Set bank "), kBankChanged);
  myBank->setTarget(this);
  addFocusWidget(myBank);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeDFWidget::loadConfig()
{
  myBank->setSelectedIndex(myCart.myCurrentBank);

  CartDebugWidget::loadConfig();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeDFWidget::handleCommand(CommandSender* sender,
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
string CartridgeDFWidget::bankState()
{
  ostringstream& buf = buffer();

  static const char* const spot[] = {
    "$FFC0", "$FFC1", "$FFC2", "$FFC3", "$FFC4", "$FFC5", "$FFC6", "$FFC7",
    "$FFC8", "$FFC9", "$FFCA", "$FFCB", "$FFCC", "$FFCD", "$FFCE", "$FFCF",
    "$FFD0", "$FFD1", "$FFD2", "$FFD3", "$FFD4", "$FFD5", "$FFD6", "$FFD7",
    "$FFD8", "$FFD9", "$FFDA", "$FFDB", "$FFDC", "$FFDD", "$FFDE", "$FFDF"
  };
  buf << "Bank = " << std::dec << myCart.myCurrentBank
      << ", hotspot = " << spot[myCart.myCurrentBank];

  return buf.str();
}
