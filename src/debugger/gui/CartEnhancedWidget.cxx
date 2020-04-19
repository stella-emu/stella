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
// Copyright (c) 1995-2020 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================


#include "PopUpWidget.hxx"
#include "CartEnhanced.hxx"
#include "CartEnhancedWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartEnhancedWidget::CartEnhancedWidget(GuiObject* boss, const GUI::Font& lfont,
                                       const GUI::Font& nfont,
                                       int x, int y, int w, int h,
                                       CartridgeEnhanced& cart)
  : CartDebugWidget(boss, lfont, nfont, x, y, w, h),
    myCart(cart)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartEnhancedWidget::initialize()
{
  int ypos = addBaseInformation(size(), manufacturer(), description(), descriptionLines())
    + myLineHeight;

  bankSelect(ypos);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
size_t CartEnhancedWidget::size()
{
  size_t size;

  myCart.getImage(size);

  return size;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string CartEnhancedWidget::description()
{
  ostringstream info;

  if (myCart.myRamSize > 0)
    info << ramDescription();
  info << romDescription();

  return info.str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int CartEnhancedWidget::descriptionLines()
{
  return 20; // should be enough for almost all types
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string CartEnhancedWidget::ramDescription()
{
  ostringstream info;

  info << myCart.myRamSize << " bytes RAM @ "
    << "$" << Common::Base::HEX4 << ADDR_BASE << " - "
    << "$" << (ADDR_BASE | (myCart.myRamSize * 2 - 1)) << "\n"
    << "  $" << (ADDR_BASE | myCart.myReadOffset)
    << " - $" << (ADDR_BASE | (myCart.myReadOffset + myCart.myRamSize - 1)) << " (R)"
    << ", $" << (ADDR_BASE | myCart.myWriteOffset)
    << " - $" << (ADDR_BASE | (myCart.myWriteOffset + myCart.myRamSize - 1)) << " (W)\n";

  return info.str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string CartEnhancedWidget::romDescription()
{
  ostringstream info;
  size_t size;
  const uInt8* image = myCart.getImage(size);

  if(myCart.romBankCount() > 1)
  {
    for(int bank = 0, offset = 0xFFC; bank < myCart.romBankCount(); ++bank, offset += 0x1000)
    {
      uInt16 start = (image[offset + 1] << 8) | image[offset];
      start -= start % 0x1000;

      info << "Bank #" << std::dec << bank << " @ $"
        << Common::Base::HEX4 << (start + myCart.myRomOffset) << " - $" << (start + 0xFFF)
        << " (hotspot " << hotspotStr(bank) << ")\n";
    }
    info << "Startup bank = #" << std::dec << myCart.startBank() << " or undetermined\n";
  }
  else
  {
    uInt16 start = (image[myCart.mySize - 3] << 8) | image[myCart.mySize - 4];

    start -= start % std::min(int(size), 0x1000);
    // special check for ROMs where the extra RAM is not included in the image (e.g. CV).
    if((start & 0xFFF) < size)
    {
      start += myCart.myRomOffset;
    }
    info << "ROM accessible @ $"
      << Common::Base::HEX4 << start << " - $"
      << Common::Base::HEX4 << (start + myCart.mySize - 1);
  }

  return info.str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartEnhancedWidget::bankSelect(int& ypos)
{
  if(myCart.romBankCount() > 1)
  {
    int xpos = 2;

    myBankWidgets = make_unique<PopUpWidget* []>(bankSegs());

    for(int seg = 0; seg < bankSegs(); ++seg)
    {
      // fill bank and hotspot list
      uInt16 hotspot = myCart.hotspot();
      VariantList items;
      int pw = 0;

      for(int bank = 0; bank < myCart.romBankCount(); ++bank)
      {
        ostringstream buf;

        buf << std::setw(bank < 10 ? 2 : 1) << "#" << std::dec << bank;
        if(hotspot >= 0x100 && myHotspotDelta > 0)
          buf << " (" << hotspotStr(bank, seg) << ")";
        VarList::push_back(items, buf.str());
        pw = std::max(pw, _font.getStringWidth(buf.str()));
      }

      // create widgets
      ostringstream buf;

      buf << "Set bank";
      if(bankSegs() > 1)
        buf << " for segment #" << seg << " ";
      else
        buf << "     "; // align with info

      myBankWidgets[seg] = new PopUpWidget(_boss, _font, xpos, ypos - 2,
                                           pw, myLineHeight, items, buf.str(),
                                           0, kBankChanged);
      myBankWidgets[seg]->setTarget(this);
      myBankWidgets[seg]->setID(seg);
      addFocusWidget(myBankWidgets[seg]);

      ypos += myBankWidgets[seg]->getHeight() + 4;
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string CartEnhancedWidget::bankState()
{
  if(myCart.romBankCount() > 1)
  {
    ostringstream& buf = buffer();
    uInt16 hotspot = myCart.hotspot();
    bool hasRamBanks = myCart.myRamBankCount > 0;

    if(bankSegs() > 1)
    {
      buf << "Segments: ";

      for(int seg = 0; seg < bankSegs(); ++seg)
      {
        int bank = myCart.getSegmentBank(seg);
        bool isRamBank = (bank >= myCart.romBankCount());


        if(seg > 0)
          buf << " / ";

        buf << "#" << std::dec << (bank - (isRamBank ? myCart.romBankCount() : 0));

        if(isRamBank) // was RAM mapped here?
          buf << " RAM";
        else if (hasRamBanks)
          buf << " ROM";

        if(hotspot >= 0x100)
          buf << " (" << (bankSegs() < 3 ? "hotspot " : "") << hotspotStr(bank) << ")";
      }
    }
    else
    {
      buf << "Bank #" << std::dec << myCart.getBank();

      //if(hotspot >= 0x100)
        buf << " (hotspot " << hotspotStr(myCart.getSegmentBank()) << ")";
    }
    return buf.str();
  }
  return "0 (non-bankswitched)";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string CartEnhancedWidget::hotspotStr(int bank, int segment)
{
  ostringstream info;
  uInt16 hotspot = myCart.hotspot();

  if(hotspot & 0x1000)
    hotspot |= ADDR_BASE;

  info << "$" << Common::Base::HEX1 << (hotspot + bank * myHotspotDelta);

  return info.str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int CartEnhancedWidget::bankSegs()
{
  return myCart.myBankSegs;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartEnhancedWidget::saveOldState()
{
  myOldState.internalRam.clear();
  for(uInt32 i = 0; i < myCart.myRamSize; ++i)
    myOldState.internalRam.push_back(myCart.myRAM[i]);

  myOldState.banks.clear();
  for(int seg = 0; seg < bankSegs(); ++seg)
    myOldState.banks.push_back(myCart.getSegmentBank(seg));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartEnhancedWidget::loadConfig()
{
  if(myBankWidgets != nullptr)
  {
    for(int seg = 0; seg < bankSegs(); ++seg)
      myBankWidgets[seg]->setSelectedIndex(myCart.getSegmentBank(seg),
                                           myCart.getSegmentBank(seg) != myOldState.banks[seg]);
  }
  CartDebugWidget::loadConfig();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartEnhancedWidget::handleCommand(CommandSender* sender,
                                       int cmd, int data, int id)
{
  if(cmd == kBankChanged)
  {
    myCart.unlockBank();
    myCart.bank(myBankWidgets[id]->getSelected(), id);
    myCart.lockBank();
    invalidate();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 CartEnhancedWidget::internalRamSize()
{
  return myCart.myRamSize;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 CartEnhancedWidget::internalRamRPort(int start)
{
  return ADDR_BASE + myCart.myReadOffset + start;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string CartEnhancedWidget::internalRamDescription()
{
  ostringstream desc;

  // order RW by addresses
  if(myCart.myReadOffset <= myCart.myWriteOffset)
  {
    desc << "$" << Common::Base::HEX4 << (ADDR_BASE | myCart.myReadOffset)
      << " - $" << (ADDR_BASE | (myCart.myReadOffset + myCart.myRamSize - 1))
      << " used for Read Access\n";
  }

  desc
    << "$" << Common::Base::HEX4 << (ADDR_BASE | myCart.myWriteOffset)
    << " - $" << (ADDR_BASE | (myCart.myWriteOffset + myCart.myRamSize - 1))
    << " used for Write Access";

  if(myCart.myReadOffset > myCart.myWriteOffset)
  {
    desc << "\n$" << Common::Base::HEX4 << (ADDR_BASE | myCart.myReadOffset)
      << " - $" << (ADDR_BASE | (myCart.myReadOffset + myCart.myRamSize - 1))
      << " used for Read Access";
  }

  return desc.str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const ByteArray& CartEnhancedWidget::internalRamOld(int start, int count)
{
  myRamOld.clear();
  for(int i = 0; i < count; i++)
    myRamOld.push_back(myOldState.internalRam[start + i]);
  return myRamOld;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const ByteArray& CartEnhancedWidget::internalRamCurrent(int start, int count)
{
  myRamCurrent.clear();
  for(int i = 0; i < count; i++)
    myRamCurrent.push_back(myCart.myRAM[start + i]);
  return myRamCurrent;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartEnhancedWidget::internalRamSetValue(int addr, uInt8 value)
{
  myCart.myRAM[addr] = value;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 CartEnhancedWidget::internalRamGetValue(int addr)
{
  return myCart.myRAM[addr];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string CartEnhancedWidget::internalRamLabel(int addr)
{
  CartDebug& dbg = instance().debugger().cartDebug();
  return dbg.getLabel(addr + ADDR_BASE + myCart.myReadOffset, false);
}
