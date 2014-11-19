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
    info << "Bank " << dec << i << " @ $" << Common::Base::HEX4 << start << " - "
         << "$" << (start + 0xFFF) << " (hotspot = $" << (spot+i) << ")\n";
  }

  int xpos = 10,
      ypos = addBaseInformation(size, "CPUWIZ", info.str()) + myLineHeight;

  VariantList items;
  VarList::push_back(items, " 0 ($FC0)");
  VarList::push_back(items, " 1 ($FC1)");
  VarList::push_back(items, " 2 ($FC2)");
  VarList::push_back(items, " 3 ($FC3)");
  VarList::push_back(items, " 4 ($FC4)");
  VarList::push_back(items, " 5 ($FC5)");
  VarList::push_back(items, " 6 ($FC6)");
  VarList::push_back(items, " 7 ($FC7)");
  VarList::push_back(items, " 8 ($FC8)");
  VarList::push_back(items, " 9 ($FC9)");
  VarList::push_back(items, "10 ($FCA)");
  VarList::push_back(items, "11 ($FCB)");
  VarList::push_back(items, "12 ($FCC)");
  VarList::push_back(items, "13 ($FCD)");
  VarList::push_back(items, "14 ($FCE)");
  VarList::push_back(items, "15 ($FCF)");
  VarList::push_back(items, "16 ($FD0)");
  VarList::push_back(items, "17 ($FD1)");
  VarList::push_back(items, "18 ($FD2)");
  VarList::push_back(items, "19 ($FD3)");
  VarList::push_back(items, "20 ($FD4)");
  VarList::push_back(items, "21 ($FD5)");
  VarList::push_back(items, "22 ($FD6)");
  VarList::push_back(items, "23 ($FD7)");
  VarList::push_back(items, "24 ($FD8)");
  VarList::push_back(items, "25 ($FD9)");
  VarList::push_back(items, "26 ($FDA)");
  VarList::push_back(items, "27 ($FDB)");
  VarList::push_back(items, "28 ($FDC)");
  VarList::push_back(items, "29 ($FDD)");
  VarList::push_back(items, "30 ($FDE)");
  VarList::push_back(items, "31 ($FDF)");
 
  myBank =
    new PopUpWidget(boss, _font, xpos, ypos-2, _font.getStringWidth("31 ($FDF) "),
                    myLineHeight, items, "Set bank: ",
                    _font.getStringWidth("Set bank: "), kBankChanged);
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

  static const char* spot[] = {
    "$FC0", "$FC1", "$FC2", "$FC3", "$FC4", "$FC5", "$FC6", "$FC7",
    "$FC8", "$FC9", "$FCA", "$FCB", "$FCC", "$FCD", "$FCE", "$FCF",
    "$FD0", "$FD1", "$FD2", "$FD3", "$FD4", "$FD5", "$FD6", "$FD7",
    "$FD8", "$FD9", "$FDA", "$FDB", "$FDC", "$FDD", "$FDE", "$FDF"
  };
  buf << "Bank = " << dec << myCart.myCurrentBank
      << ", hotspot = " << spot[myCart.myCurrentBank];

  return buf.str();
}
