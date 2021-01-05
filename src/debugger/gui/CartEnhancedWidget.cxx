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

#include "PopUpWidget.hxx"
#include "OSystem.hxx"
#include "Debugger.hxx"
#include "CartDebug.hxx"
#include "CartEnhanced.hxx"
#include "CartEnhancedWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeEnhancedWidget::CartridgeEnhancedWidget(GuiObject* boss, const GUI::Font& lfont,
                                       const GUI::Font& nfont,
                                       int x, int y, int w, int h,
                                       CartridgeEnhanced& cart)
  : CartDebugWidget(boss, lfont, nfont, x, y, w, h),
    myCart{cart}
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int CartridgeEnhancedWidget::initialize()
{
  int ypos = addBaseInformation(size(), manufacturer(), description(), descriptionLines())
    + myLineHeight;

  bankSelect(ypos);

  return ypos;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
size_t CartridgeEnhancedWidget::size()
{
  size_t size;

  myCart.getImage(size);

  return size;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string CartridgeEnhancedWidget::description()
{
  ostringstream info;

  if (myCart.myRamSize > 0)
    info << ramDescription();
  info << romDescription();

  return info.str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int CartridgeEnhancedWidget::descriptionLines()
{
  return 18; // should be enough for almost all types
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string CartridgeEnhancedWidget::ramDescription()
{
  ostringstream info;

  if(myCart.ramBankCount() == 0)
    info << myCart.myRamSize << " bytes RAM @ "
      << "$" << Common::Base::HEX4 << ADDR_BASE << " - "
      << "$" << (ADDR_BASE | (myCart.myRamSize * 2 - 1)) << "\n";

  info << "  $" << Common::Base::HEX4 << (ADDR_BASE | myCart.myReadOffset)
    << " - $" << (ADDR_BASE | (myCart.myReadOffset + myCart.myRamMask)) << " (R)"
    << ", $" << (ADDR_BASE | myCart.myWriteOffset)
    << " - $" << (ADDR_BASE | (myCart.myWriteOffset + myCart.myRamMask)) << " (W)\n";

  return info.str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string CartridgeEnhancedWidget::romDescription()
{
  ostringstream info;
  size_t size;
  const ByteBuffer& image = myCart.getImage(size);

  if(myCart.romBankCount() > 1)
  {
    for(int bank = 0, offset = 0xFFC; bank < myCart.romBankCount(); ++bank, offset += 0x1000)
    {
      uInt16 start = (image[offset + 1] << 8) | image[offset];
      start -= start % 0x1000;
      string hash = myCart.romBankCount() > 10 && bank < 10 ? " #" : "#";

      info << "Bank " << hash << std::dec << bank << " @ $"
        << Common::Base::HEX4 << (start + myCart.myRomOffset) << " - $" << (start + 0xFFF);
      if(myCart.hotspot() != 0)
      {
        string hs = hotspotStr(bank, 0, true);
        if(hs.length() > 22)
          info << "\n ";
        info << " " << hs;
      }
      info << "\n";
    }
    info << "Startup bank = #" << std::dec << myCart.startBank() << " or undetermined\n";
  }
  else
  {
    uInt16 start = (image[myCart.mySize - 3] << 8) | image[myCart.mySize - 4];
    uInt16 end;

    start -= start % std::min(int(size), 0x1000);
    end = start + uInt16(myCart.mySize) - 1;
    // special check for ROMs where the extra RAM is not included in the image (e.g. CV).
    if((start & 0xFFFU) < size)
    {
      start += myCart.myRomOffset;
    }
    info << "ROM accessible @ $"
      << Common::Base::HEX4 << start << " - $"
      << Common::Base::HEX4 << end;
  }

  return info.str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeEnhancedWidget::bankList(uInt16 bankCount, int seg, VariantList& items, int& width)
{
  width = 0;

  for(int bank = 0; bank < bankCount; ++bank)
  {
    ostringstream buf;

    buf << std::setw(bank < 10 ? 2 : 1) << "#" << std::dec << bank;
    if(myCart.hotspot() != 0 && myHotspotDelta > 0)
      buf << " " << hotspotStr(bank, seg);
    VarList::push_back(items, buf.str());
    width = std::max(width, _font.getStringWidth(buf.str()));
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeEnhancedWidget::bankSelect(int& ypos)
{
  if(myCart.romBankCount() > 1)
  {
    int xpos = 2;

    myBankWidgets = make_unique<PopUpWidget* []>(bankSegs());

    for(int seg = 0; seg < bankSegs(); ++seg)
    {
      // fill bank and hotspot list
      VariantList items;
      int pw = 0;

      bankList(myCart.romBankCount(), seg, items, pw);

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
string CartridgeEnhancedWidget::bankState()
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

        //if(hotspot >= 0x100)
        if(hotspot != 0 && myHotspotDelta > 0)
          buf << " " << hotspotStr(bank, 0, bankSegs() < 3);
      }
    }
    else
    {
      buf << "Bank #" << std::dec << myCart.getBank();

      if(hotspot != 0 && myHotspotDelta > 0)
        buf << " " << hotspotStr(myCart.getBank(), 0, true);
    }
    return buf.str();
  }
  return "non-bankswitched";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string CartridgeEnhancedWidget::hotspotStr(int bank, int segment, bool prefix)
{
  ostringstream info;
  uInt16 hotspot = myCart.hotspot();

  if(hotspot & 0x1000)
    hotspot |= ADDR_BASE;

  info << "(" << (prefix ? "hotspot " : "");
  info << "$" << Common::Base::HEX1 << (hotspot + bank * myHotspotDelta);
  info << ")";

  return info.str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt16 CartridgeEnhancedWidget::bankSegs()
{
  return myCart.myBankSegs;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeEnhancedWidget::saveOldState()
{
  myOldState.internalRam.clear();
  for(uInt32 i = 0; i < myCart.myRamSize; ++i)
    myOldState.internalRam.push_back(myCart.myRAM[i]);

  myOldState.banks.clear();
  if (bankSegs() > 1)
    for(int seg = 0; seg < bankSegs(); ++seg)
      myOldState.banks.push_back(myCart.getSegmentBank(seg));
  else
    myOldState.banks.push_back(myCart.getBank());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeEnhancedWidget::loadConfig()
{
  if(myBankWidgets != nullptr)
  {
    if (bankSegs() > 1)
      for(int seg = 0; seg < bankSegs(); ++seg)
        myBankWidgets[seg]->setSelectedIndex(myCart.getSegmentBank(seg),
                                             myCart.getSegmentBank(seg) != myOldState.banks[seg]);
    else
      myBankWidgets[0]->setSelectedIndex(myCart.getBank(),
                                         myCart.getBank() != myOldState.banks[0]);
  }
  CartDebugWidget::loadConfig();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeEnhancedWidget::handleCommand(CommandSender* sender,
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
uInt32 CartridgeEnhancedWidget::internalRamSize()
{
  return uInt32(myCart.myRamSize);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 CartridgeEnhancedWidget::internalRamRPort(int start)
{
  if(myCart.ramBankCount() == 0)
    return ADDR_BASE + myCart.myReadOffset + start;
  else
    return start;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string CartridgeEnhancedWidget::internalRamDescription()
{
  ostringstream desc;
  string indent = "";

  if(myCart.ramBankCount())
  {
    desc << "Accessible ";
    if (myCart.bankSize() >> 1 >= 1024)
      desc << ((myCart.bankSize() >> 1) / 1024) << "K";
    else
      desc << (myCart.bankSize() >> 1) << " bytes";
    desc << " at a time via:\n";
    indent = "  ";
  }

  // order RW by addresses
  if(myCart.myReadOffset <= myCart.myWriteOffset)
  {
    desc << indent << "$" << Common::Base::HEX4 << (ADDR_BASE | myCart.myReadOffset)
      << " - $" << (ADDR_BASE | (myCart.myReadOffset + myCart.myRamMask))
      << " used for read access\n";
  }

  desc << indent << "$" << Common::Base::HEX4 << (ADDR_BASE | myCart.myWriteOffset)
    << " - $" << (ADDR_BASE | (myCart.myWriteOffset + myCart.myRamMask))
    << " used for write access";

  if(myCart.myReadOffset > myCart.myWriteOffset)
  {
    desc << indent << "\n$" << Common::Base::HEX4 << (ADDR_BASE | myCart.myReadOffset)
      << " - $" << (ADDR_BASE | (myCart.myReadOffset + myCart.myRamMask))
      << " used for read access";
  }

  return desc.str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const ByteArray& CartridgeEnhancedWidget::internalRamOld(int start, int count)
{
  myRamOld.clear();
  for(int i = 0; i < count; i++)
    myRamOld.push_back(myOldState.internalRam[start + i]);
  return myRamOld;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const ByteArray& CartridgeEnhancedWidget::internalRamCurrent(int start, int count)
{
  myRamCurrent.clear();
  for(int i = 0; i < count; i++)
    myRamCurrent.push_back(myCart.myRAM[start + i]);
  return myRamCurrent;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeEnhancedWidget::internalRamSetValue(int addr, uInt8 value)
{
  myCart.myRAM[addr] = value;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 CartridgeEnhancedWidget::internalRamGetValue(int addr)
{
  return myCart.myRAM[addr];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string CartridgeEnhancedWidget::internalRamLabel(int addr)
{
  CartDebug& dbg = instance().debugger().cartDebug();
  return dbg.getLabel(addr + ADDR_BASE + myCart.myReadOffset, false, -1, true);
}
