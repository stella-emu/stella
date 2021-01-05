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
// Copyright (c) 1995-2021 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

//#include "CartE7.hxx"
#include "CartMNetwork.hxx"
#include "PopUpWidget.hxx"
#include "CartMNetworkWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeMNetworkWidget::CartridgeMNetworkWidget(
    GuiObject* boss, const GUI::Font& lfont, const GUI::Font& nfont,
    int x, int y, int w, int h,
    CartridgeMNetwork& cart)
  : CartDebugWidget(boss, lfont, nfont, x, y, w, h),
    myCart{cart}
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeMNetworkWidget::initialize(GuiObject* boss,
    CartridgeMNetwork& cart, ostringstream& info)
{
  uInt32 size = cart.romBankCount() * cart.BANK_SIZE;

  int xpos = 2,
    ypos = addBaseInformation(size, "M Network", info.str(), 15) +
    myLineHeight;

  VariantList items0, items1;
  for(int i = 0; i < cart.romBankCount(); ++i)
    VarList::push_back(items0, getSpotLower(i));
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
void CartridgeMNetworkWidget::saveOldState()
{
  myOldState.internalram.clear();

  for(uInt32 i = 0; i < internalRamSize(); ++i)
    myOldState.internalram.push_back(myCart.myRAM[i]);

  myOldState.lowerBank = myCart.myCurrentBank[0];
  myOldState.upperBank = myCart.myCurrentRAM;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeMNetworkWidget::loadConfig()
{
  myLower2K->setSelectedIndex(myCart.myCurrentBank[0], myCart.myCurrentBank[0] != myOldState.lowerBank);
  myUpper256B->setSelectedIndex(myCart.myCurrentRAM, myCart.myCurrentRAM != myOldState.upperBank);

  CartDebugWidget::loadConfig();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeMNetworkWidget::handleCommand(CommandSender* sender,
                                            int cmd, int data, int id)
{
  myCart.unlockBank();

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

  myCart.lockBank();
  invalidate();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string CartridgeMNetworkWidget::bankState()
{
  ostringstream& buf = buffer();

  buf << "Segments: " << std::dec
    << getSpotLower(myCart.myCurrentBank[0]) << " / "
    << getSpotUpper(myCart.myCurrentRAM);

  return buf.str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 CartridgeMNetworkWidget::internalRamSize()
{
  return 2048;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 CartridgeMNetworkWidget::internalRamRPort(int start)
{
  return 0x0000 + start;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string CartridgeMNetworkWidget::internalRamDescription()
{
  ostringstream desc;
  desc << "First 1K accessible via:\n"
    << "  $F000 - $F3FF used for write access\n"
    << "  $F400 - $F7FF used for read access\n"
    << "256 bytes of second 1K accessible via:\n"
    << "  $F800 - $F8FF used for write access\n"
    << "  $F900 - $F9FF used for read access";

  return desc.str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const ByteArray& CartridgeMNetworkWidget::internalRamOld(int start, int count)
{
  myRamOld.clear();
  for(int i = 0; i < count; i++)
    myRamOld.push_back(myOldState.internalram[start + i]);
  return myRamOld;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const ByteArray& CartridgeMNetworkWidget::internalRamCurrent(int start, int count)
{
  myRamCurrent.clear();
  for(int i = 0; i < count; i++)
    myRamCurrent.push_back(myCart.myRAM[start + i]);
  return myRamCurrent;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeMNetworkWidget::internalRamSetValue(int addr, uInt8 value)
{
  myCart.myRAM[addr] = value;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 CartridgeMNetworkWidget::internalRamGetValue(int addr)
{
  return myCart.myRAM[addr];
}
