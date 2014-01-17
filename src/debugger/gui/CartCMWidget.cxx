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

#include "CartCM.hxx"
#include "RiotDebug.hxx"
#include "DataGridWidget.hxx"
#include "EditTextWidget.hxx"
#include "PopUpWidget.hxx"
#include "ToggleBitWidget.hxx"
#include "CartCMWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeCMWidget::CartridgeCMWidget(
      GuiObject* boss, const GUI::Font& lfont, const GUI::Font& nfont,
      int x, int y, int w, int h, CartridgeCM& cart)
  : CartDebugWidget(boss, lfont, nfont, x, y, w, h),
    myCart(cart)
{
  uInt16 size = 4 * 4096;

  string info =
    "CM cartridge, four 4K banks + 2K RAM\n"
    "2K RAM accessible @ $1800 - $1FFF in read or write-only mode "
    "(no separate ports)\n"
    "All TIA controller registers (INPT0-INPT5) and RIOT SWCHA are "
    "used to control the cart functionality\n"
    "Startup bank = 3 (ROM), RAM disabled\n";

  int xpos = 10,
      ypos = addBaseInformation(size, "CompuMate", info) + myLineHeight;

  VariantList items;
  items.push_back(" 0 ");
  items.push_back(" 1 ");
  items.push_back(" 2 ");
  items.push_back(" 3 ");
  myBank =
    new PopUpWidget(boss, _font, xpos, ypos-2, _font.getStringWidth(" 0 "),
                    myLineHeight, items, "Set bank: ",
                    _font.getStringWidth("Set bank: "), kBankChanged);
  myBank->setTarget(this);
  addFocusWidget(myBank);

  // Raw SWCHA value (this will be broken down further in other UI elements)
  int lwidth = _font.getStringWidth("Current column: ");
  ypos += myLineHeight + 8;
  new StaticTextWidget(boss, _font, xpos, ypos+2, lwidth, myFontHeight,
                       "Current SWCHA: ", kTextAlignLeft);
  xpos += lwidth;
  mySWCHA = new ToggleBitWidget(boss, _nfont, xpos, ypos, 8, 1);
  mySWCHA->setTarget(this);
  mySWCHA->setEditable(false);

  // Current column number
  xpos = 10;  ypos += myLineHeight + 5;
  new StaticTextWidget(boss, _font, xpos, ypos, lwidth,
        myFontHeight, "Current column: ", kTextAlignLeft);
  xpos += lwidth;

  myColumn = new DataGridWidget(boss, _nfont, xpos, ypos-2, 1, 1, 2, 8, Common::Base::F_16);
  myColumn->setTarget(this);
  myColumn->setEditable(false);

  // Relevant pins of SWCHA
  xpos = 30;

  // D6 (column part)
  ypos += myLineHeight + 8;
  myIncrease = new CheckboxWidget(boss, _font, xpos, ypos, "Increase Column");
  myIncrease->setTarget(this);
  myIncrease->setEditable(false);

  int orig_ypos = ypos;  // save for when we go to the next column

  // D5 (column part)
  ypos += myLineHeight + 4;
  myReset = new CheckboxWidget(boss, _font, xpos, ypos, "Reset Column");
  myReset->setTarget(this);
  myReset->setEditable(false);

  // Row inputs
  ypos += myLineHeight + 4;
  myRow[0] = new CheckboxWidget(boss, _font, xpos, ypos, "Row 0");
  myRow[0]->setTarget(this);
  myRow[0]->setEditable(false);
  ypos += myLineHeight + 4;
  myRow[1] = new CheckboxWidget(boss, _font, xpos, ypos, "Row 1");
  myRow[1]->setTarget(this);
  myRow[1]->setEditable(false);
  ypos += myLineHeight + 4;
  myRow[2] = new CheckboxWidget(boss, _font, xpos, ypos, "Row 2");
  myRow[2]->setTarget(this);
  myRow[2]->setEditable(false);
  ypos += myLineHeight + 4;
  myRow[3] = new CheckboxWidget(boss, _font, xpos, ypos, "Row 3");
  myRow[3]->setTarget(this);
  myRow[3]->setEditable(false);

  // Func and Shift keys
  ypos += myLineHeight + 4;
  myFunc = new CheckboxWidget(boss, _font, xpos, ypos, "FUNC key pressed");
  myFunc->setTarget(this);
  myFunc->setEditable(false);
  ypos += myLineHeight + 4;
  myShift = new CheckboxWidget(boss, _font, xpos, ypos, "Shift key pressed");
  myShift->setTarget(this);
  myShift->setEditable(false);

  // Move to next column
  xpos += myShift->getWidth() + 20;  ypos = orig_ypos;

  // D7
  myAudIn = new CheckboxWidget(boss, _font, xpos, ypos, "Audio Input");
  myAudIn->setTarget(this);
  myAudIn->setEditable(false);

  // D6 (audio part)
  ypos += myLineHeight + 4;
  myAudOut = new CheckboxWidget(boss, _font, xpos, ypos, "Audio Output");
  myAudOut->setTarget(this);
  myAudOut->setEditable(false);

  // Ram state (combination of several bits in SWCHA)
  ypos += myLineHeight + 8;
  lwidth = _font.getStringWidth("Ram State: ");
  new StaticTextWidget(boss, _font, xpos, ypos, lwidth,
        myFontHeight, "Ram State: ", kTextAlignLeft);
  myRAM = new EditTextWidget(boss, _nfont, xpos+lwidth, ypos-1,
              _nfont.getStringWidth(" Write-only "), myLineHeight, "");
  myRAM->setEditable(false);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeCMWidget::saveOldState()
{
  myOldState.swcha = myCart.mySWCHA;
  myOldState.column = myCart.myColumn;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeCMWidget::loadConfig()
{
  myBank->setSelectedIndex(myCart.myCurrentBank);

  RiotDebug& riot = Debugger::debugger().riotDebug();
  const RiotState& state = (RiotState&) riot.getState();

  uInt8 swcha = myCart.mySWCHA;

  // SWCHA
  BoolArray oldbits, newbits, changed;
  Debugger::set_bits(myOldState.swcha, oldbits);
  Debugger::set_bits(swcha, newbits);

  for(uInt32 i = 0; i < oldbits.size(); ++i)
    changed.push_back(oldbits[i] != newbits[i]);
  mySWCHA->setState(newbits, changed);

  // Column
  myColumn->setList(0, myCart.myColumn, myCart.myColumn != myOldState.column);

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
  const string& ram = swcha & 0x10 ? " Inactive" :
                        swcha & 0x20 ? " Read-only" : " Write-only";
  myRAM->setText(ram, (swcha & 0x30) != (myOldState.swcha & 0x30));

  CartDebugWidget::loadConfig();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeCMWidget::handleCommand(CommandSender* sender,
                                      int cmd, int data, int id)
{
  if(cmd == kBankChanged)
  {
    myCart.unlockBank();
    myCart.mySWCHA &= 0xFC;
    myCart.mySWCHA |= myBank->getSelected();
    myCart.bank(myCart.mySWCHA & 0x03);
    myCart.lockBank();
    invalidate();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string CartridgeCMWidget::bankState()
{
  ostringstream& buf = buffer();

  buf << "Bank = " << dec << myCart.myCurrentBank
      << ", RAM is" << (myCart.mySWCHA & 0x10 ? " Inactive" :
         myCart.mySWCHA & 0x20 ? " Read-only" : " Write-only");

  return buf.str();
}
