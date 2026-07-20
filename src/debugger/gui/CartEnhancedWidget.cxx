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

#include "Font.hxx"
#include "EditTextWidget.hxx"
#include "PopUpWidget.hxx"
#include "OSystem.hxx"
#include "Debugger.hxx"
#include "CartDebug.hxx"
#include "CartEnhanced.hxx"
#include "Layout.hxx"
#include "CartEnhancedWidget.hxx"

using Common::Base;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeEnhancedWidget::CartridgeEnhancedWidget(GuiObject* boss, const GUI::Font& lfont,
                                       const GUI::Font& nfont,
                                       CartridgeEnhanced& cart)
  : CartDebugWidget(boss, lfont, nfont),
    myCart{cart}
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeEnhancedWidget::initialize()
{
  createWidgets();
  reflow();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeEnhancedWidget::createWidgets()
{
  createBaseInformation(size(), manufacturer(), description(), descriptionLines());
  createPlusROM();
  createBankWidgets();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeEnhancedWidget::layoutContent(GUI::BoxLayout& col) const
{
  layoutPlusROM(col);
  layoutBankSelect(col);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
size_t CartridgeEnhancedWidget::size()
{
  return myCart.getImage().size();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string CartridgeEnhancedWidget::description()
{
  string info;
  if(myCart.myRamSize > 0)
    info = ramDescription();
  info += romDescription();
  return info;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int CartridgeEnhancedWidget::descriptionLines()
{
  return 18; // should be enough for almost all types
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string CartridgeEnhancedWidget::ramDescription()
{
  string info;
  if(myCart.ramBankCount() == 0)
    info = std::format("{} bytes RAM @ ${} - ${}\n",
      myCart.myRamSize,
      Base::hex4(ADDR_BASE),
      Base::hex4(static_cast<int>(ADDR_BASE | (myCart.myRamSize * 2 - 1))));

  info += std::format("  ${} - ${} (R), ${} - ${} (W)\n",
    Base::hex4(ADDR_BASE | myCart.myReadOffset),
    Base::hex4(ADDR_BASE | (myCart.myReadOffset + myCart.myRamMask)),
    Base::hex4(ADDR_BASE | myCart.myWriteOffset),
    Base::hex4(ADDR_BASE | (myCart.myWriteOffset + myCart.myRamMask)));
  return info;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string CartridgeEnhancedWidget::romDescription()
{
  string info;
  const ByteSpan image = myCart.getImage();
  if(myCart.romBankCount() > 1)
  {
    for(int bank = 0, offset = 0xFFC; std::cmp_less(bank, myCart.romBankCount());
        ++bank, offset += 0x1000)
    {
      const uInt16 start = (((static_cast<uInt16>(image[offset + 1]) << 8) | image[offset]) / 0x1000) * 0x1000;
      const string_view hash = myCart.romBankCount() > 10 && bank < 10 ? " #" : "#";
      info += std::format("Bank {}{} @ ${} - ${}",
        hash, bank, Base::hex4(start + myCart.myRomOffset), Base::hex4(start + 0xFFF));
      if(myCart.hotspot() != 0)
      {
        const string hs = hotspotStr(bank, 0, true);
        if(hs.length() > 22)
          info += "\n ";
        info += " " + hs;
      }
      info += "\n";
    }
    info += std::format("Startup bank = #{} or undetermined\n", myCart.startBank());
  }
  else
  {
    const auto* end = image.data() + image.size();
    uInt16 start = (((static_cast<uInt16>(end[-3]) << 8) | end[-4]) / 0x1000) * 0x1000;
    const uInt16 last = start + static_cast<uInt16>(image.size()) - 1;
    // special check for ROMs where the extra RAM is not included in the image (e.g. CV).
    if((start & 0xFFFU) < image.size())
      start += myCart.myRomOffset;
    info += std::format("ROM accessible @ ${} - ${}", Base::hex4(start), Base::hex4(last));
  }
  return info;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeEnhancedWidget::createPlusROM()
{
  if(!myCart.isPlusROM())
    return;

  myPlusROMLabel = new StaticTextWidget(_boss, _font, "PlusROM:");

  myPlusROMHostLabel = new StaticTextWidget(_boss, _font, "Host");
  myPlusROMHostWidget = new EditTextWidget(_boss, _font, 1, _lineHeight,
                                           myCart.myPlusROM->getHost());
  myPlusROMHostWidget->setEditable(false);

  myPlusROMPathLabel = new StaticTextWidget(_boss, _font, "Path");
  myPlusROMPathWidget = new EditTextWidget(_boss, _font, 1, _lineHeight,
                                           myCart.myPlusROM->getPath());
  myPlusROMPathWidget->setEditable(false);

  myPlusROMSendLabel = new StaticTextWidget(_boss, _font, "Send");
  myPlusROMSendWidget = new EditTextWidget(_boss, _nfont, 1);
  myPlusROMSendWidget->setEditable(false);

  myPlusROMReceiveLabel = new StaticTextWidget(_boss, _font, "Receive");
  myPlusROMReceiveWidget = new EditTextWidget(_boss, _nfont, 1);
  myPlusROMReceiveWidget->setEditable(false);

  // The fields line up with the ROM info fields above them, their labels indented
  // under the "PlusROM:" header (alignLabels narrows their column by exactly the
  // indent, so the fields still meet)
  const int indent = _fontWidth * 2;

  myLabelColumn.insert(myLabelColumn.end(),
                       {{myPlusROMHostLabel, indent}, {myPlusROMPathLabel, indent},
                        {myPlusROMSendLabel, indent}, {myPlusROMReceiveLabel, indent}});
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeEnhancedWidget::layoutPlusROM(GUI::BoxLayout& col) const
{
  using GUI::anchoredItem;
  using GUI::labeledRow;

  if(!myCart.isPlusROM())
    return;

  // The labels were given their (indented) column when they joined myLabelColumn
  const int indent = _fontWidth * 2;

  col.addAuto(anchoredItem(myPlusROMLabel));
  col.addAuto(labeledRow(myPlusROMHostLabel, myPlusROMHostWidget, 0, indent, true));
  col.addAuto(labeledRow(myPlusROMPathLabel, myPlusROMPathWidget, 0, indent, true));
  col.addAuto(labeledRow(myPlusROMSendLabel, myPlusROMSendWidget, 0, indent, true));
  col.addAuto(labeledRow(myPlusROMReceiveLabel, myPlusROMReceiveWidget, 0, indent, true));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeEnhancedWidget::bankList(uInt16 bankCount, int seg,
                                       VariantList& items)
{
  const bool hasRamBanks = myCart.myRamBankCount > 0;

  for(int bank = 0; std::cmp_less(bank, bankCount); ++bank)
  {
    const bool isRamBank = std::cmp_greater_equal(bank, myCart.romBankCount());
    const int bankNum = bank - (isRamBank ? myCart.romBankCount() : 0);

    string label = std::format("{}{}", bankNum < 10 ? " #" : "#", bankNum);
    if(isRamBank)        label += " RAM"; // was RAM mapped here?
    else if(hasRamBanks) label += " ROM";
    if(myCart.hotspot() != 0 && myHotspotDelta > 0)
      label += " " + hotspotStr(bank, seg);

    VarList::push_back(items, label);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeEnhancedWidget::createBankWidgets()
{
  if(myCart.romBankCount() + myCart.ramBankCount() <= 1)
    return;

  myBankWidgets.resize(bankSegs());

  for(int seg = 0; std::cmp_less(seg, bankSegs()); ++seg)
  {
    // fill bank and hotspot list
    VariantList items;

    bankList(myCart.romBankCount() + myCart.ramBankCount(), seg, items);

    // create widgets; the pop-up sizes its box to the widest entry
    const string label = bankSegs() > 1
      ? std::format("Set bank for segment #{}", seg)
      : "Set bank";

    myBankWidgets[seg] = new PopUpWidget(_boss, _font, items, label,
                                         0, kBankChanged);
    myBankWidgets[seg]->setTarget(this);
    myBankWidgets[seg]->setID(seg);
    addFocusWidget(myBankWidgets[seg]);

    // The selector's box lines up with the info fields above it
    myLabelColumn.emplace_back(myBankWidgets[seg]);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeEnhancedWidget::layoutBankSelect(GUI::BoxLayout& col) const
{
  using GUI::anchoredItem;

  for(auto* popup: myBankWidgets)
    col.addAuto(anchoredItem(popup));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string CartridgeEnhancedWidget::bankState()
{
  if(myCart.romBankCount() <= 1)
    return "non-bankswitched";

  const uInt16 hotspot = myCart.hotspot();
  const bool hasRamBanks = myCart.myRamBankCount > 0;
  string result;

  if(bankSegs() > 1)
  {
    result = "Segments: ";
    for(int seg = 0; std::cmp_less(seg, bankSegs()); ++seg)
    {
      const int bank = myCart.getSegmentBank(seg);
      const bool isRamBank = std::cmp_greater_equal(bank, myCart.romBankCount());
      if(seg > 0) result += " / ";
      result += std::format("#{}", bank - (isRamBank ? myCart.romBankCount() : 0));
      if(isRamBank)        result += " RAM"; // was RAM mapped here?
      else if(hasRamBanks) result += " ROM";
      if(hotspot != 0 && myHotspotDelta > 0)
        result += " " + hotspotStr(bank, seg, bankSegs() < 3);
    }
  }
  else
  {
    result = std::format("Bank #{}", myCart.getBank());
    if(hotspot != 0 && myHotspotDelta > 0)
      result += " " + hotspotStr(myCart.getBank(), 0, true);
  }
  return result;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string CartridgeEnhancedWidget::hotspotStr(int bank, int segment, bool prefix)
{
  uInt16 hotspot = myCart.hotspot();
  if(hotspot & 0x1000)
    hotspot |= ADDR_BASE;
  return std::format("({}${})",
    prefix ? "hotspot " : "",
    Base::hex4(hotspot + bank * myHotspotDelta));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt16 CartridgeEnhancedWidget::bankSegs() const
{
  return myCart.myBankSegs;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeEnhancedWidget::saveOldState()
{
  myOldState.internalRam.assign(myCart.myRAM.begin(), myCart.myRAM.end());

  if(myCart.isPlusROM())
  {
    myOldState.send = myCart.myPlusROM->getSend();
    myOldState.receive = myCart.myPlusROM->getReceive();
  }

  myOldState.banks.clear();
  if(bankSegs() > 1)
    for(int seg = 0; std::cmp_less(seg, bankSegs()); ++seg)
      myOldState.banks.push_back(myCart.getSegmentBank(seg));
  else
    myOldState.banks.push_back(myCart.getBank());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeEnhancedWidget::loadConfig()
{
  if(myCart.isPlusROM())
  {
    const auto formatBytes = [](const ByteArray& arr) {
      string result;
      result.reserve(arr.size() * 3);
      for(auto i : arr)
        result += std::format("{} ", Base::hex2(static_cast<int>(i)));
      return result;
    };

    const ByteArray& sendArr = myCart.myPlusROM->getSend();
    myPlusROMSendWidget->setText(formatBytes(sendArr), sendArr != myOldState.send);

    const ByteArray& recvArr = myCart.myPlusROM->getReceive();
    myPlusROMReceiveWidget->setText(formatBytes(recvArr), recvArr != myOldState.receive);
  }

  if(!myBankWidgets.empty())
  {
    if(bankSegs() > 1)
      for(int seg = 0; std::cmp_less(seg, bankSegs()); ++seg)
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
    myCart.unlockHotspots();
    myCart.bank(myBankWidgets[id]->getSelected(), id);
    myCart.lockHotspots();
    invalidate();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 CartridgeEnhancedWidget::internalRamSize()
{
  return static_cast<uInt32>(myCart.myRamSize);
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
  string desc;
  string_view indent;

  if(myCart.ramBankCount())
  {
    const int halfBank = myCart.bankSize() >> 1;
    desc = halfBank >= 1024
      ? std::format("Accessible {}K at a time via:\n", halfBank / 1024)
      : std::format("Accessible {} bytes at a time via:\n", halfBank);
    indent = "  ";
  }

  // order RW by addresses
  if(myCart.myReadOffset <= myCart.myWriteOffset)
    desc += std::format("{}${} - ${} used for read access\n",
      indent,
      Base::hex4(ADDR_BASE | myCart.myReadOffset),
      Base::hex4(ADDR_BASE | (myCart.myReadOffset + myCart.myRamMask)));

  desc += std::format("{}${} - ${} used for write access",
    indent,
    Base::hex4(ADDR_BASE | myCart.myWriteOffset),
    Base::hex4(ADDR_BASE | (myCart.myWriteOffset + myCart.myRamMask)));

  if(myCart.myReadOffset > myCart.myWriteOffset)
    desc += std::format("\n{}${} - ${} used for read access",
      indent,
      Base::hex4(ADDR_BASE | myCart.myReadOffset),
      Base::hex4(ADDR_BASE | (myCart.myReadOffset + myCart.myRamMask)));

  return desc;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const ByteArray& CartridgeEnhancedWidget::internalRamOld(int start, int count)
{
  myRamOld.clear();
  myRamOld.assign(myOldState.internalRam.data() + start,
                  myOldState.internalRam.data() + start + count);
  return myRamOld;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const ByteArray& CartridgeEnhancedWidget::internalRamCurrent(int start, int count)
{
  myRamCurrent.assign(myCart.myRAM.begin() + start,
                      myCart.myRAM.begin() + start + count);

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
  const CartDebug& dbg = instance().debugger().cartDebug();
  return dbg.getLabel(addr + ADDR_BASE + myCart.myReadOffset, false, -1, true);
}
