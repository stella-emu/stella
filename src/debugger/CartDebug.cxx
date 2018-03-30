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
// Copyright (c) 1995-2018 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include <ctime>

#include "bspf.hxx"
#include "System.hxx"
#include "M6502.hxx"
#include "FSNode.hxx"
#include "DiStella.hxx"
#include "Debugger.hxx"
#include "DebuggerParser.hxx"
#include "CpuDebug.hxx"
#include "OSystem.hxx"
#include "Settings.hxx"
#include "Version.hxx"
#include "Cart.hxx"
#include "CartDebug.hxx"
#include "CartDebugWidget.hxx"
#include "CartRamWidget.hxx"
#include "RomWidget.hxx"
#include "Base.hxx"
using Common::Base;
using std::hex;
using std::dec;
using std::setfill;
using std::setw;
using std::left;
using std::right;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartDebug::CartDebug(Debugger& dbg, Console& console, const OSystem& osystem)
  : DebuggerSystem(dbg, console),
    myOSystem(osystem),
    myDebugWidget(nullptr),
    myAddrToLineIsROM(true),
    myRWPortAddress(0),
    myLabelLength(8)   // longest pre-defined label
{
  // Add case sensitive compare for user labels
  // TODO - should user labels be case insensitive too?
  auto usrCmp = [](const string& a, const string& b) { return a < b; };
  myUserAddresses = LabelToAddr(usrCmp);

  // Add case insensitive compare for system labels
  auto sysCmp = [](const string& a, const string& b) {
      return BSPF::compareIgnoreCase(a, b) < 0;
  };
  mySystemAddresses = LabelToAddr(sysCmp);

  // Add Zero-page RAM addresses
  for(uInt16 i = 0x80; i <= 0xFF; ++i)
  {
    myState.rport.push_back(i);
    myState.wport.push_back(i);
    myOldState.rport.push_back(i);
    myOldState.wport.push_back(i);
  }

  // Create bank information for each potential bank, and an extra one for ZP RAM
  // Banksizes greater than 4096 indicate multi-bank ROMs, but we handle only
  // 4K pieces at a time
  // Banksizes less than 4K use the actual value
  uInt32 banksize = 0;
  myConsole.cartridge().getImage(banksize);

  BankInfo info;
  info.size = std::min(banksize, 4096u);
  for(uInt32 i = 0; i < myConsole.cartridge().bankCount(); ++i)
    myBankInfo.push_back(info);

  info.size = 128;  // ZP RAM
  myBankInfo.push_back(info);

  // We know the address for the startup bank right now
  myBankInfo[myConsole.cartridge().startBank()].addressList.push_front(
    myDebugger.dpeek(0xfffc));
  addLabel("Start", myDebugger.dpeek(0xfffc, DATA));

  // Add system equates
  for(uInt16 addr = 0x00; addr <= 0x0F; ++addr)
  {
    if(ourTIAMnemonicR[addr])
      mySystemAddresses.emplace(ourTIAMnemonicR[addr], addr);
    myReserved.TIARead[addr] = false;
  }
  for(uInt16 addr = 0x00; addr <= 0x3F; ++addr)
  {
    if(ourTIAMnemonicW[addr])
      mySystemAddresses.emplace(ourTIAMnemonicW[addr], addr);
    myReserved.TIAWrite[addr] = false;
  }
  for(uInt16 addr = 0x280; addr <= 0x297; ++addr)
  {
    if(ourIOMnemonic[addr-0x280])
      mySystemAddresses.emplace(ourIOMnemonic[addr-0x280], addr);
    myReserved.IOReadWrite[addr-0x280] = false;
  }
  for(uInt16 addr = 0x80; addr <= 0xFF; ++addr)
  {
    mySystemAddresses.emplace(ourZPMnemonic[addr-0x80], addr);
    myReserved.ZPRAM[addr-0x80] = false;
  }

  myReserved.Label.clear();
  myDisassembly.list.reserve(2048);

  // Add settings for Distella
  DiStella::settings.gfxFormat =
    myOSystem.settings().getInt("dis.gfxformat") == 16 ? Base::F_16 : Base::F_2;
  DiStella::settings.resolveCode =
    myOSystem.settings().getBool("dis.resolve");
  DiStella::settings.showAddresses =
    myOSystem.settings().getBool("dis.showaddr");
  DiStella::settings.aFlag = false; // Not currently configurable
  DiStella::settings.fFlag = true;  // Not currently configurable
  DiStella::settings.rFlag = myOSystem.settings().getBool("dis.relocate");
  DiStella::settings.bFlag = true;  // Not currently configurable
  DiStella::settings.bytesWidth = 8+1;  // TODO - configure based on window size
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const DebuggerState& CartDebug::getState()
{
  myState.ram.clear();
  for(uInt32 i = 0; i < myState.rport.size(); ++i)
    myState.ram.push_back(myDebugger.peek(myState.rport[i]));

  if(myDebugWidget)
    myState.bank = myDebugWidget->bankState();

  return myState;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartDebug::saveOldState()
{
  myOldState.ram.clear();
  for(uInt32 i = 0; i < myOldState.rport.size(); ++i)
    myOldState.ram.push_back(myDebugger.peek(myOldState.rport[i]));

  if(myDebugWidget)
  {
    myOldState.bank = myDebugWidget->bankState();
    myDebugWidget->saveOldState();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartDebug::triggerReadFromWritePort(uInt16 addr)
{
  myRWPortAddress = addr;
  mySystem.setDirtyPage(addr);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int CartDebug::readFromWritePort()
{
  uInt16 addr = myRWPortAddress;
  myRWPortAddress = 0;

  // A read from the write port occurs when the read is actually in the write
  // port address space AND the last access was actually a read (the latter
  // differentiates between reads that are normally part of a write cycle vs.
  // ones that are illegal)
  if(mySystem.m6502().lastReadAddress() &&
      (mySystem.getPageAccessType(addr) & System::PA_WRITE) == System::PA_WRITE)
    return addr;
  else
    return 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int CartDebug::lastReadBaseAddress()
{
  return mySystem.m6502().lastReadBaseAddress();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int CartDebug::lastWriteBaseAddress()
{
  return mySystem.m6502().lastWriteBaseAddress();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string CartDebug::toString()
{
  ostringstream buf;
  uInt32 bytesPerLine;

  switch(Base::format())
  {
    case Base::F_16:
    case Base::F_10:
      bytesPerLine = 0x10;
      break;

    case Base::F_2:
      bytesPerLine = 0x04;
      break;

    case Base::F_DEFAULT:
    default:
      return DebuggerParser::red("invalid base, this is a BUG");
  }

  const CartState& state    = static_cast<const CartState&>(getState());
  const CartState& oldstate = static_cast<const CartState&>(getOldState());

  uInt32 curraddr = 0, bytesSoFar = 0;
  for(uInt32 i = 0; i < state.ram.size(); i += bytesPerLine, bytesSoFar += bytesPerLine)
  {
    // We detect different 'pages' of RAM when the addresses jump by
    // more than the number of bytes on the previous line, or when 256
    // bytes have been previously output
    if(state.rport[i] - curraddr > bytesPerLine || bytesSoFar >= 256)
    {
      char port[37];
      std::snprintf(port, 36, "%04x: (rport = %04x, wport = %04x)\n",
              state.rport[i], state.rport[i], state.wport[i]);
      port[2] = port[3] = 'x';
      buf << DebuggerParser::red(port);
      bytesSoFar = 0;
    }
    curraddr = state.rport[i];
    buf << Base::HEX2 << (curraddr & 0x00ff) << ": ";

    for(uInt8 j = 0; j < bytesPerLine; ++j)
    {
      buf << myDebugger.invIfChanged(state.ram[i+j], oldstate.ram[i+j]) << " ";

      if(j == 0x07) buf << " ";
    }
    buf << endl;
  }

  return buf.str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartDebug::disassemble(bool force)
{
  // Test current disassembly; don't re-disassemble if it hasn't changed
  // Also check if the current PC is in the current list
  bool bankChanged = myConsole.cartridge().bankChanged();
  uInt16 PC = myDebugger.cpuDebug().pc();
  int pcline = addressToLine(PC);
  bool pcfound = (pcline != -1) && (uInt32(pcline) < myDisassembly.list.size()) &&
                  (myDisassembly.list[pcline].disasm[0] != '.');
  bool pagedirty = (PC & 0x1000) ? mySystem.isPageDirty(0x1000, 0x1FFF) :
                                   mySystem.isPageDirty(0x80, 0xFF);

  bool changed = !mySystem.autodetectMode() &&
                 (force || bankChanged || !pcfound || pagedirty);
  if(changed)
  {
    // Are we disassembling from ROM or ZP RAM?
    BankInfo& info = (PC & 0x1000) ? myBankInfo[getBank()] :
        myBankInfo[myBankInfo.size()-1];

    // If the offset has changed, all old addresses must be 'converted'
    // For example, if the list contains any $fxxx and the address space is now
    // $bxxx, it must be changed
    uInt16 offset = (PC - (PC % 0x1000));
    AddressList& addresses = info.addressList;
    for(auto& i: addresses)
      i = (i & 0xFFF) + offset;

    // Only add addresses when absolutely necessary, to cut down on the
    // work that Distella has to do
    if(bankChanged || !pcfound)
    {
      AddressList::const_iterator i;
      for(i = addresses.cbegin(); i != addresses.cend(); ++i)
      {
        if (PC == *i)  // already present
          break;
      }
      // Otherwise, add the item at the end
      if (i == addresses.end())
        addresses.push_back(PC);
    }

    // Always attempt to resolve code sections unless it's been
    // specifically disabled
    bool found = fillDisassemblyList(info, PC);
    if(!found && DiStella::settings.resolveCode)
    {
      // Temporarily turn off code resolution
      DiStella::settings.resolveCode = false;
      fillDisassemblyList(info, PC);
      DiStella::settings.resolveCode = true;
    }
  }

  return changed;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartDebug::fillDisassemblyList(BankInfo& info, uInt16 search)
{
  // An empty address list means that DiStella can't do a disassembly
  if(info.addressList.size() == 0)
    return false;

  myDisassembly.list.clear();
  myDisassembly.fieldwidth = 24 + myLabelLength;
  DiStella distella(*this, myDisassembly.list, info, DiStella::settings,
                    myDisLabels, myDisDirectives, myReserved);

  // Parts of the disassembly will be accessed later in different ways
  // We place those parts in separate maps, to speed up access
  bool found = false;
  myAddrToLineList.clear();
  myAddrToLineIsROM = info.offset & 0x1000;
  for(uInt32 i = 0; i < myDisassembly.list.size(); ++i)
  {
    const DisassemblyTag& tag = myDisassembly.list[i];
    const uInt16 address = tag.address & 0xFFF;

    // Exclude 'ROW'; they don't have a valid address
    if(tag.type != CartDebug::ROW)
    {
      // Create a mapping from addresses to line numbers
      myAddrToLineList.emplace(address, i);

      // Did we find the search value?
      if(address == (search & 0xFFF))
        found = true;
    }
  }
  return found;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int CartDebug::addressToLine(uInt16 address) const
{
  // Switching between ZP RAM address space and Cart/ROM address space
  // means the line isn't present
  if(!myAddrToLineIsROM != !(address & 0x1000))
    return -1;

  const auto& iter = myAddrToLineList.find(address & 0xFFF);
  return iter != myAddrToLineList.end() ? iter->second : -1;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string CartDebug::disassemble(uInt16 start, uInt16 lines) const
{
  // Fill the string with disassembled data
  start &= 0xFFF;
  ostringstream buffer;

  // First find the lines in the range, and determine the longest string
  uInt32 list_size = uInt32(myDisassembly.list.size());
  uInt32 begin = list_size, end = 0, length = 0;
  for(end = 0; end < list_size && lines > 0; ++end)
  {
    const CartDebug::DisassemblyTag& tag = myDisassembly.list[end];
    if((tag.address & 0xfff) >= start)
    {
      if(begin == list_size) begin = end;
      if(tag.type != CartDebug::ROW)
        length = std::max(length, uInt32(tag.disasm.length()));

      --lines;
    }
  }

  // Now output the disassembly, using as little space as possible
  for(uInt32 i = begin; i < end; ++i)
  {
    const CartDebug::DisassemblyTag& tag = myDisassembly.list[i];
    if(tag.type == CartDebug::NONE)
      continue;
    else if(tag.address)
      buffer << std::uppercase << std::hex << std::setw(4)
             << std::setfill('0') << tag.address << ":  ";
    else
      buffer << "       ";

    buffer << tag.disasm << std::setw(int(length - tag.disasm.length() + 2))
           << std::setfill(' ') << " "
           << std::setw(4) << std::left << tag.ccount << "   " << tag.bytes << endl;
  }

  return buffer.str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartDebug::addDirective(CartDebug::DisasmType type,
                             uInt16 start, uInt16 end, int bank)
{
  if(end < start || start == 0 || end == 0)
    return false;

  if(bank < 0)  // Do we want the current bank or ZP RAM?
    bank = (myDebugger.cpuDebug().pc() & 0x1000) ? getBank() : int(myBankInfo.size())-1;

  bank = std::min(bank, bankCount());
  BankInfo& info = myBankInfo[bank];
  DirectiveList& list = info.directiveList;

  DirectiveTag tag;
  tag.type = type;
  tag.start = start;
  tag.end = end;

  DirectiveList::iterator i;

  // If the same directive and range is added, consider it a removal instead
  for(i = list.begin(); i != list.end(); ++i)
  {
    if(i->type == tag.type && i->start == tag.start && i->end == tag.end)
    {
      list.erase(i);
      return false;
    }
  }

  // Otherwise, scan the list and make space for a 'smart' merge
  // Note that there are 4 possibilities:
  //  1: a range is completely inside the new range
  //  2: a range is completely outside the new range
  //  3: a range overlaps at the beginning of the new range
  //  4: a range overlaps at the end of the new range
  for(i = list.begin(); i != list.end(); ++i)
  {
    // Case 1: remove range that is completely inside new range
    if(tag.start <= i->start && tag.end >= i->end)
    {
      i = list.erase(i);
    }
    // Case 2: split the old range
    else if(tag.start >= i->start && tag.end <= i->end)
    {
      // Only split when necessary
      if(tag.type == i->type)
        return true;  // node is fine as-is

      // Create new endpoint
      DirectiveTag tag2;
      tag2.type = i->type;
      tag2.start = tag.end + 1;
      tag2.end = i->end;

      // Modify startpoint
      i->end = tag.start - 1;

      // Insert new endpoint
      i++;
      list.insert(i, tag2);
      break;  // no need to go further; this is the insertion point
    }
    // Case 3: truncate end of old range
    else if(tag.start >= i->start && tag.start <= i->end)
    {
      i->end = tag.start - 1;
    }
    // Case 4: truncate start of old range
    else if(tag.end >= i->start && tag.end <= i->end)
    {
      i->start = tag.end + 1;
    }
  }

  // We now know that the new range can be inserted without overlap
  // Where possible, consecutive ranges should be merged rather than
  // new nodes created
  for(i = list.begin(); i != list.end(); ++i)
  {
    if(tag.end < i->start)  // node should be inserted *before* this one
    {
      bool createNode = true;

      // Is the new range ending consecutive with the old range beginning?
      // If so, a merge will suffice
      if(i->type == tag.type && tag.end + 1 == i->start)
      {
        i->start = tag.start;
        createNode = false;  // a merge was done, so a new node isn't needed
      }

      // Can we also merge with the previous range (if any)?
      if(i != list.begin())
      {
        DirectiveList::iterator p = i;
        --p;
        if(p->type == tag.type && p->end + 1 == tag.start)
        {
          if(createNode)  // a merge with right-hand range didn't previously occur
          {
            p->end = tag.end;
            createNode = false;  // a merge was done, so a new node isn't needed
          }
          else  // merge all three ranges
          {
            i->start = p->start;
            i = list.erase(p);
            createNode = false;  // a merge was done, so a new node isn't needed
          }
        }
      }

      // Create the node only when necessary
      if(createNode)
        i = list.insert(i, tag);

      break;
    }
  }
  // Otherwise, add the tag at the end
  if(i == list.end())
    list.push_back(tag);

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int CartDebug::getBank()
{
  return myConsole.cartridge().getBank();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int CartDebug::bankCount() const
{
  return myConsole.cartridge().bankCount();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartDebug::addLabel(const string& label, uInt16 address)
{
  // Only user-defined labels can be added or redefined
  switch(addressType(address))
  {
    case ADDR_TIA:
    case ADDR_IO:
      return false;
    default:
      removeLabel(label);
      myUserAddresses.emplace(label, address);
      myUserLabels.emplace(address, label);
      myLabelLength = std::max(myLabelLength, uInt16(label.size()));
      mySystem.setDirtyPage(address);
      return true;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartDebug::removeLabel(const string& label)
{
  // Only user-defined labels can be removed
  const auto& iter = myUserAddresses.find(label);
  if(iter != myUserAddresses.end())
  {
    // Erase the address assigned to the label
    const auto& iter2 = myUserLabels.find(iter->second);
    if(iter2 != myUserLabels.end())
      myUserLabels.erase(iter2);

    // Erase the label itself
    mySystem.setDirtyPage(iter->second);
    myUserAddresses.erase(iter);

    return true;
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartDebug::getLabel(ostream& buf, uInt16 addr, bool isRead, int places) const
{
  switch(addressType(addr))
  {
    case ADDR_TIA:
    {
      if(isRead)
      {
        uInt16 a = addr & 0x0F, offset = addr & 0xFFF0;
        if(ourTIAMnemonicR[a])
        {
          buf << ourTIAMnemonicR[a];
          if(offset > 0)
            buf << "|$" << Base::HEX2 << offset;
        }
        else
          buf << "$" << Base::HEX2 << addr;
      }
      else
      {
        uInt16 a = addr & 0x3F, offset = addr & 0xFFC0;
        if(ourTIAMnemonicW[a])
        {
          buf << ourTIAMnemonicW[a];
          if(offset > 0)
            buf << "|$" << Base::HEX2 << offset;
        }
        else
          buf << "$" << Base::HEX2 << addr;
      }
      return true;
    }

    case ADDR_IO:
    {
      uInt16 a = addr & 0xFF, offset = addr & 0xFD00;
      if(a <= 0x97)
      {
        if(ourIOMnemonic[a - 0x80])
        {
            buf << ourIOMnemonic[a - 0x80];
            if(offset > 0)
              buf << "|$" << Base::HEX2 << offset;
        }
        else
          buf << "$" << Base::HEX2 << addr;
      }
      else
        buf << "$" << Base::HEX2 << addr;

      return true;
    }

    case ADDR_ZPRAM:
    {
      // RAM can use user-defined labels; otherwise we default to
      // standard mnemonics
      auto iter = myUserLabels.find(addr);
      if(iter != myUserLabels.end())
      {
        buf << iter->second;
      }
      else
      {
        uInt16 a = addr & 0xFF, offset = addr & 0xFF00;
        if((iter = myUserLabels.find(a)) != myUserLabels.end())
          buf << iter->second;
        else
          buf << ourZPMnemonic[a - 0x80];
        if(offset > 0)
          buf << "|$" << Base::HEX2 << offset;
      }

      return true;
    }

    case ADDR_ROM:
    {
      // These addresses can never be in the system labels list
      const auto& iter = myUserLabels.find(addr);
      if(iter != myUserLabels.end())
      {
        buf << iter->second;
        return true;
      }
      break;
    }
  }

  switch(places)
  {
    case 2:
      buf << "$" << Base::HEX2 << addr;
      return true;
    case 4:
      buf << "$" << Base::HEX4 << addr;
      return true;
    case 8:
      buf << "$" << Base::HEX8 << addr;
      return true;
  }

  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string CartDebug::getLabel(uInt16 addr, bool isRead, int places) const
{
  ostringstream buf;
  getLabel(buf, addr, isRead, places);
  return buf.str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int CartDebug::getAddress(const string& label) const
{
  LabelToAddr::const_iterator iter;

  if((iter = mySystemAddresses.find(label)) != mySystemAddresses.end())
    return iter->second;
  else if((iter = myUserAddresses.find(label)) != myUserAddresses.end())
    return iter->second;
  else
    return -1;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string CartDebug::loadListFile()
{
  // The default naming/location for list files is the ROM dir based on the
  // actual ROM filename

  if(myListFile == "")
  {
    FilesystemNode lst(myOSystem.romFile().getPathWithExt("") + ".lst");
    if(lst.isFile() && lst.isReadable())
      myListFile = lst.getPath();
    else
      return DebuggerParser::red("list file not found in:\n  " + lst.getShortPath());
  }

  FilesystemNode node(myListFile);
  ifstream in(node.getPath());
  if(!in.is_open())
    return DebuggerParser::red("list file '" + node.getShortPath() + "' not readable");

  while(!in.eof())
  {
    string line, addr_s;

    getline(in, line);

    if(!in.good() || line == "" || line[0] == '-')
      continue;
    else  // Search for constants
    {
      stringstream buf(line);

      // Swallow first value, then get actual numerical value for address
      // We need to read the address as a string, since it may contain 'U'
      int addr = -1;
      buf >> addr >> addr_s;
      if(addr_s.length() == 0)
        continue;
      const char* p = addr_s[0] == 'U' ? addr_s.c_str() + 1 : addr_s.c_str();
      addr = int(strtoul(p, nullptr, 16));

      // For now, completely ignore ROM addresses
      if(!(addr & 0x1000))
      {
        // Search for pattern 'xx yy  CONSTANT ='
        buf.seekg(20);  // skip potential '????'
        int xx = -1, yy = -1;
        char eq = '\0';
        buf >> hex >> xx >> hex >> yy >> line >> eq;
        if(xx >= 0 && yy >= 0 && eq == '=')
          //myUserCLabels.emplace(xx*256+yy, line);
          addLabel(line, xx * 256 + yy);
      }
    }
  }
  myDebugger.rom().invalidate();

  return "loaded " + node.getShortPath() + " OK";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string CartDebug::loadSymbolFile()
{
  // The default naming/location for symbol files is the ROM dir based on the
  // actual ROM filename

  if(mySymbolFile == "")
  {
    FilesystemNode sym(myOSystem.romFile().getPathWithExt("") + ".sym");
    if(sym.isFile() && sym.isReadable())
      mySymbolFile = sym.getPath();
    else
      return DebuggerParser::red("symbol file not found in:\n  " + sym.getShortPath());
  }

  FilesystemNode node(mySymbolFile);
  ifstream in(node.getPath());
  if(!in.is_open())
    return DebuggerParser::red("symbol file '" + node.getShortPath() + "' not readable");

  myUserAddresses.clear();
  myUserLabels.clear();

  while(!in.eof())
  {
    string label;
    int value = -1;

    getline(in, label);
    if(!in.good())  continue;
    stringstream buf(label);
    buf >> label >> hex >> value;

    if(label.length() > 0 && label[0] != '-' && value >= 0)
    {
      // Make sure the value doesn't represent a constant
      // For now, we simply ignore constants completely
      //const auto& iter = myUserCLabels.find(value);
      //if(iter == myUserCLabels.end() || !BSPF::equalsIgnoreCase(label, iter->second))
      const auto& iter = myUserLabels.find(value);
      if (iter == myUserLabels.end() || !BSPF::equalsIgnoreCase(label, iter->second))
      {
        // Check for period, and strip leading number
        string::size_type pos = label.find_first_of(".", 0);
        if(pos != string::npos)
          addLabel(label.substr(pos), value);
        else
        {
          pos = label.find_last_of("$");
          if (pos == string::npos || pos != label.length() - 1)
            addLabel(label, value);
        }
      }
    }
  }
  myDebugger.rom().invalidate();

  return "loaded " + node.getShortPath() + " OK";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string CartDebug::loadConfigFile()
{
  // There are two possible locations for loading config files
  //   (in order of decreasing relevance):
  // 1) ROM dir based on properties entry name
  // 2) CFG dir based on properties entry name

  if(myCfgFile == "")
  {
    const string& propsname =
      myConsole.properties().get(Cartridge_Name) + ".cfg";

    FilesystemNode case1(myOSystem.romFile().getParent().getPath() + propsname);
    FilesystemNode case2(myOSystem.cfgDir() + propsname);

    if(case1.isFile() && case1.isReadable())
      myCfgFile = case1.getPath();
    else if(case2.isFile() && case2.isReadable())
      myCfgFile = case2.getPath();
    else
      return DebuggerParser::red("config file not found in:\n  " +
          case1.getShortPath() + "\n  " + case2.getShortPath());
  }

  FilesystemNode node(myCfgFile);
  ifstream in(node.getPath());
  if(!in.is_open())
    return "Unable to load directives from " + node.getPath();

  // Erase all previous directives
  for(auto& bi: myBankInfo)
    bi.directiveList.clear();

  int currentbank = 0;
  while(!in.eof())
  {
    // Skip leading space
    int c = in.peek();
    while(c == ' ' || c == '\t')
    {
      in.get();
      c = in.peek();
    }

    string line;
    c = in.peek();
    if(c == '/')  // Comment, swallow line and continue
    {
      getline(in, line);
      continue;
    }
    else if(c == '[')
    {
      in.get();
      getline(in, line, ']');
      stringstream buf(line);
      buf >> currentbank;
    }
    else  // Should be commands from this point on
    {
      getline(in, line);
      stringstream buf;
      buf << line;

      string directive;
      uInt16 start = 0, end = 0;
      buf >> directive;
      if(BSPF::startsWithIgnoreCase(directive, "ORG"))
      {
        // TODO - figure out what to do with this
        buf >> hex >> start;
      }
      else if(BSPF::startsWithIgnoreCase(directive, "CODE"))
      {
        buf >> hex >> start >> hex >> end;
        addDirective(CartDebug::CODE, start, end, currentbank);
      }
      else if(BSPF::startsWithIgnoreCase(directive, "GFX"))
      {
        buf >> hex >> start >> hex >> end;
        addDirective(CartDebug::GFX, start, end, currentbank);
      }
      else if(BSPF::startsWithIgnoreCase(directive, "PGFX"))
      {
        buf >> hex >> start >> hex >> end;
        addDirective(CartDebug::PGFX, start, end, currentbank);
      }
      else if(BSPF::startsWithIgnoreCase(directive, "DATA"))
      {
        buf >> hex >> start >> hex >> end;
        addDirective(CartDebug::DATA, start, end, currentbank);
      }
      else if(BSPF::startsWithIgnoreCase(directive, "ROW"))
      {
        buf >> hex >> start;
        buf >> hex >> end;
        addDirective(CartDebug::ROW, start, end, currentbank);
      }
    }
  }
  myDebugger.rom().invalidate();

  stringstream retVal;
  if(myConsole.cartridge().bankCount() > 1)
    retVal << DebuggerParser::red("config file for multi-bank ROM not fully supported\n");
  retVal << "loaded " << node.getShortPath() << " OK";
  return retVal.str();

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string CartDebug::saveConfigFile()
{
  // While there are two possible locations for loading config files,
  // the main 'config' directory is used whenever possible when saving,
  // unless the rom-specific file already exists

  FilesystemNode node;

  FilesystemNode case0(myCfgFile);
  if(myCfgFile != "" && case0.isFile() && case0.isWritable())
    node = case0;
  else
  {
    const string& propsname =
      myConsole.properties().get(Cartridge_Name) + ".cfg";

    node = FilesystemNode(myOSystem.cfgDir() + propsname);
  }

  const string& name = myConsole.properties().get(Cartridge_Name);
  const string& md5 = myConsole.properties().get(Cartridge_MD5);

  ofstream out(node.getPath());
  if(!out.is_open())
    return "Unable to save directives to " + node.getShortPath();

  // Store all bank information
  out << "//Stella.pro: \"" << name << "\"" << endl
      << "//MD5: " << md5 << endl
      << endl;
  for(uInt32 b = 0; b < myConsole.cartridge().bankCount(); ++b)
  {
    out << "[" << b << "]" << endl;
    getBankDirectives(out, myBankInfo[b]);
  }

  stringstream retVal;
  if(myConsole.cartridge().bankCount() > 1)
    retVal << DebuggerParser::red("config file for multi-bank ROM not fully supported\n");
  retVal << "saved " << node.getShortPath() << " OK";
  return retVal.str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string CartDebug::saveDisassembly()
{
  if(myDisasmFile == "")
  {
    const string& propsname =
      myConsole.properties().get(Cartridge_Name) + ".asm";

    myDisasmFile = FilesystemNode(myOSystem.defaultSaveDir() + propsname).getPath();
  }

  FilesystemNode node(myDisasmFile);
  ofstream out(node.getPath());
  if(!out.is_open())
    return "Unable to save disassembly to " + node.getShortPath();

#define ALIGN(x) setfill(' ') << left << setw(x)

  // We can't print the header to the disassembly until it's actually
  // been processed; therefore buffer output to a string first
  ostringstream buf;
  buf << "\n\n;***********************************************************\n"
      << ";      Bank " << myConsole.cartridge().getBank();
  if (myConsole.cartridge().bankCount() > 1)
    buf << " / 0.." << myConsole.cartridge().bankCount() - 1;
  buf << "\n;***********************************************************\n\n";

  // Use specific settings for disassembly output
  // This will most likely differ from what you see in the debugger
  DiStella::Settings settings;
  settings.gfxFormat = DiStella::settings.gfxFormat;
  settings.resolveCode = true;
  settings.showAddresses = false;
  settings.aFlag = false; // Otherwise DASM gets confused
  settings.fFlag = DiStella::settings.fFlag;
  settings.rFlag = DiStella::settings.rFlag;
  settings.bytesWidth = 8+1;  // same as Stella debugger
  settings.bFlag = DiStella::settings.bFlag;; // process break routine (TODO)

  Disassembly disasm;
  disasm.list.reserve(2048);
  for(int bank = 0; bank < myConsole.cartridge().bankCount(); ++bank)
  {
    BankInfo& info = myBankInfo[bank];
    // An empty address list means that DiStella can't do a disassembly
    if(info.addressList.size() == 0)
      continue;

    // Disassemble bank
    disasm.list.clear();
    DiStella distella(*this, disasm.list, info, settings,
                      myDisLabels, myDisDirectives, myReserved);

    if (myReserved.breakFound)
      addLabel("Break", myDebugger.dpeek(0xfffe));

    buf << "    SEG     CODE\n"
        << "    ORG     $" << Base::HEX4 << info.offset << "\n\n";

    // Format in 'distella' style
    for(uInt32 i = 0; i < disasm.list.size(); ++i)
    {
      const DisassemblyTag& tag = disasm.list[i];

      // Add label (if any)
      if(tag.label != "")
        buf << ALIGN(4) << (tag.label) << "\n";
      buf << "    ";

      switch(tag.type)
      {
        case CartDebug::CODE:
        {
          buf << ALIGN(32) << tag.disasm << tag.ccount.substr(0, 5) << tag.ctotal << tag.ccount.substr(5, 2);
          if (tag.disasm.find("WSYNC") != std::string::npos)
            buf << "\n;---------------------------------------";
          break;
        }
        case CartDebug::ROW:
        {
          buf << ".byte   " << ALIGN(32) << tag.disasm.substr(6, 8*4-1) << "; $" << Base::HEX4 << tag.address << " (*)";
          break;
        }
        case CartDebug::GFX:
        {
          buf << ".byte   " << (settings.gfxFormat == Base::F_2 ? "%" : "$")
              << tag.bytes << " ; |";
          for(int c = 12; c < 20; ++c)
            buf << ((tag.disasm[c] == '\x1e') ? "#" : " ");
          buf << ALIGN(13) << "|" << "$" << Base::HEX4 << tag.address << " (G)";
          break;
        }
        case CartDebug::PGFX:
        {
          buf << ".byte   " << (settings.gfxFormat == Base::F_2 ? "%" : "$")
              << tag.bytes << " ; |";
          for(int c = 12; c < 20; ++c)
            buf << ((tag.disasm[c] == '\x1f') ? "*" : " ");
          buf << ALIGN(13) << "|" << "$" << Base::HEX4 << tag.address << " (P)";
          break;
        }
        case CartDebug::DATA:
        {
          buf << ".byte   " << ALIGN(32) << tag.disasm.substr(6, 8 * 4 - 1) << "; $" << Base::HEX4 << tag.address << " (D)";
          break;
        }
        case CartDebug::NONE:
        default:
        {
          break;
        }
      } // switch
      buf << "\n";
    }
  }

  // Some boilerplate, similar to what DiStella adds
  time_t currtime;
  time(&currtime);
  out << "; Disassembly of " << myOSystem.romFile().getShortPath() << "\n"
      << "; Disassembled " << ctime(&currtime)
      << "; Using Stella " << STELLA_VERSION << "\n;\n"
      << "; ROM properties name : " << myConsole.properties().get(Cartridge_Name) << "\n"
      << "; ROM properties MD5  : " << myConsole.properties().get(Cartridge_MD5) << "\n"
      << "; Bankswitch type     : " << myConsole.cartridge().about() << "\n;\n"
      << "; Legend: * = CODE not yet run (tentative code)\n"
      << ";         D = DATA directive (referenced in some way)\n"
      << ";         G = GFX directive, shown as '#' (stored in player, missile, ball)\n"
      << ";         P = PGFX directive, shown as '*' (stored in playfield)\n"
      << ";         i = indexed accessed only\n"
      << ";         c = used by code executed in RAM\n"
      << ";         s = used by stack\n"
      << ";         ! = page crossed, 1 cycle penalty\n"
      << "\n    processor 6502\n\n";

  bool addrUsed = false;
  for(uInt16 addr = 0x00; addr <= 0x0F; ++addr)
    addrUsed = addrUsed || myReserved.TIARead[addr] || (mySystem.getAccessFlags(addr) & WRITE);
  for(uInt16 addr = 0x00; addr <= 0x3F; ++addr)
    addrUsed = addrUsed || myReserved.TIAWrite[addr] || (mySystem.getAccessFlags(addr) & DATA);
  for(uInt16 addr = 0x00; addr <= 0x17; ++addr)
    addrUsed = addrUsed || myReserved.IOReadWrite[addr];
  if(addrUsed)
  {
    out << "\n;-----------------------------------------------------------\n"
        << ";      TIA and IO constants accessed\n"
        << ";-----------------------------------------------------------\n\n";

    // TIA read access
    for(uInt16 addr = 0x00; addr <= 0x0F; ++addr)
      if(myReserved.TIARead[addr] && ourTIAMnemonicR[addr])
        out << ALIGN(16) << ourTIAMnemonicR[addr] << "= $"
            << Base::HEX2 << right << addr << "  ; (R)\n";
      else if (mySystem.getAccessFlags(addr) & DATA)
        out << ";" << ALIGN(16-1) << ourTIAMnemonicR[addr] << "= $"
        << Base::HEX2 << right << addr << "  ; (Ri)\n";
    out << "\n";

    // TIA write access
    for(uInt16 addr = 0x00; addr <= 0x3F; ++addr)
      if(myReserved.TIAWrite[addr] && ourTIAMnemonicW[addr])
        out << ALIGN(16) << ourTIAMnemonicW[addr] << "= $"
            << Base::HEX2 << right << addr << "  ; (W)\n";
      else if (mySystem.getAccessFlags(addr) & WRITE)
        out << ";" << ALIGN(16-1) << ourTIAMnemonicW[addr] << "= $"
        << Base::HEX2 << right << addr << "  ; (Wi)\n";
    out << "\n";

    // RIOT IO access
    for(uInt16 addr = 0x00; addr <= 0x17; ++addr)
      if(myReserved.IOReadWrite[addr] && ourIOMnemonic[addr])
        out << ALIGN(16) << ourIOMnemonic[addr] << "= $"
            << Base::HEX4 << right << (addr+0x280) << "\n";
  }

  addrUsed = false;
  for(uInt16 addr = 0x80; addr <= 0xFF; ++addr)
    addrUsed = addrUsed || myReserved.ZPRAM[addr-0x80]
      || (mySystem.getAccessFlags(addr) & (DATA | WRITE))
      || (mySystem.getAccessFlags(addr|0x100) & (DATA | WRITE));
  if(addrUsed)
  {
    bool addLine = false;
    out << "\n\n;-----------------------------------------------------------\n"
        << ";      RIOT RAM (zero-page) labels\n"
        << ";-----------------------------------------------------------\n\n";

    for (uInt16 addr = 0x80; addr <= 0xFF; ++addr) {
      bool ramUsed = (mySystem.getAccessFlags(addr) & (DATA | WRITE));
      bool codeUsed = (mySystem.getAccessFlags(addr) & CODE);
      bool stackUsed = (mySystem.getAccessFlags(addr|0x100) & (DATA | WRITE));

      if (myReserved.ZPRAM[addr - 0x80] &&
          myUserLabels.find(addr) == myUserLabels.end()) {
        if (addLine)
          out << "\n";
        out << ALIGN(16) << ourZPMnemonic[addr - 0x80] << "= $"
          << Base::HEX2 << right << (addr)
          << (stackUsed|codeUsed ? "; (" : "")
          << (codeUsed ? "c" : "")
          << (stackUsed ? "s" : "")
          << (stackUsed | codeUsed ? ")" : "")
          << "\n";
        addLine = false;
      } else if (ramUsed|codeUsed|stackUsed) {
        if (addLine)
          out << "\n";
        out << ALIGN(18) << ";" << "$"
          << Base::HEX2 << right << (addr)
          << "  ("
          << (ramUsed ? "i" : "")
          << (codeUsed ? "c" : "")
          << (stackUsed ? "s" : "")
          << ")\n";
        addLine = false;
      } else
        addLine = true;
    }
  }

  if(myReserved.Label.size() > 0)
  {
    out << "\n\n;-----------------------------------------------------------\n"
        << ";      Non Locatable Labels\n"
        << ";-----------------------------------------------------------\n\n";
    for(const auto& iter: myReserved.Label)
        out << ALIGN(16) << iter.second << "= $" << iter.first << "\n";
  }

  if(myUserLabels.size() > 0)
  {
    out << "\n\n;-----------------------------------------------------------\n"
        << ";      User Defined Labels\n"
        << ";-----------------------------------------------------------\n\n";
    int max_len = 16;
    for(const auto& iter: myUserLabels)
      max_len = std::max(max_len, int(iter.second.size()));
    for(const auto& iter: myUserLabels)
      out << ALIGN(max_len) << iter.second << "= $" << iter.first << "\n";
  }

  // And finally, output the disassembly
  out << buf.str();

  stringstream retVal;
  if(myConsole.cartridge().bankCount() > 1)
    retVal << DebuggerParser::red("disassembly for multi-bank ROM not fully supported, only currently enabled banks disassembled\n");
  retVal << "saved " << node.getShortPath() << " OK";
  return retVal.str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string CartDebug::saveRom()
{
  const string& rom = myConsole.properties().get(Cartridge_Name) + ".a26";

  FilesystemNode node(myOSystem.defaultSaveDir() + rom);
  ofstream out(node.getPath(), std::ios::binary);
  if(out && myConsole.cartridge().saveROM(out))
    return "saved ROM as " + node.getShortPath();
  else
    return DebuggerParser::red("failed to save ROM");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string CartDebug::listConfig(int bank)
{
  if(myConsole.cartridge().bankCount() > 1)
    return DebuggerParser::red("config file for multi-bank ROM not yet supported");

  uInt32 startbank = 0, endbank = bankCount();
  if(bank >= 0 && bank < bankCount())
  {
    startbank = bank;
    endbank = startbank + 1;
  }

  ostringstream buf;
  buf << "(items marked '*' are user-defined)" << endl;
  for(uInt32 b = startbank; b < endbank; ++b)
  {
    BankInfo& info = myBankInfo[b];
    buf << "[" << b << "]" << endl;
    for(const auto& i: info.directiveList)
    {
      if(i.type != CartDebug::NONE)
      {
        buf << "(*) ";
        disasmTypeAsString(buf, i.type);
        buf << " " << Base::HEX4 << i.start << " " << Base::HEX4 << i.end << endl;
      }
    }
    getBankDirectives(buf, info);
  }

  return buf.str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string CartDebug::clearConfig(int bank)
{
  uInt32 startbank = 0, endbank = bankCount();
  if(bank >= 0 && bank < bankCount())
  {
    startbank = bank;
    endbank = startbank + 1;
  }

  uInt32 count = 0;
  for(uInt32 b = startbank; b < endbank; ++b)
  {
    count += myBankInfo[b].directiveList.size();
    myBankInfo[b].directiveList.clear();
  }

  ostringstream buf;
  if(count > 0)
    buf << "removed " << dec << count << " directives from "
        << dec << (endbank - startbank) << " banks";
  else
    buf << "no directives present";
  return buf.str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartDebug::getCompletions(const char* in, StringList& completions) const
{
  // First scan system equates
  for(uInt16 addr = 0x00; addr <= 0x0F; ++addr)
    if(ourTIAMnemonicR[addr] && BSPF::matches(ourTIAMnemonicR[addr], in))
      completions.push_back(ourTIAMnemonicR[addr]);
  for(uInt16 addr = 0x00; addr <= 0x3F; ++addr)
    if(ourTIAMnemonicW[addr] && BSPF::matches(ourTIAMnemonicW[addr], in))
      completions.push_back(ourTIAMnemonicW[addr]);
  for(uInt16 addr = 0; addr <= 0x297-0x280; ++addr)
    if(ourIOMnemonic[addr] && BSPF::matches(ourIOMnemonic[addr], in))
      completions.push_back(ourIOMnemonic[addr]);
  for(uInt16 addr = 0; addr <= 0x7F; ++addr)
    if(ourZPMnemonic[addr] && BSPF::matches(ourZPMnemonic[addr], in))
      completions.push_back(ourZPMnemonic[addr]);

  // Now scan user-defined labels
  for(const auto& iter: myUserAddresses)
  {
    const char* l = iter.first.c_str();
    if(BSPF::matches(l, in))
      completions.push_back(l);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartDebug::AddrType CartDebug::addressType(uInt16 addr) const
{
  // Determine the type of address to access the correct list
  // These addresses were based on (and checked against) Kroko's 2600 memory
  // map, found at http://www.qotile.net/minidig/docs/2600_mem_map.txt
  if(addr % 0x2000 < 0x1000)
  {
    if((addr & 0x00ff) < 0x80)
      return ADDR_TIA;
    else
    {
      switch(addr & 0x0f00)
      {
        case 0x000:  case 0x100:  case 0x400:  case 0x500:
        case 0x800:  case 0x900:  case 0xc00:  case 0xd00:
          return ADDR_ZPRAM;
        case 0x200:  case 0x300:  case 0x600:  case 0x700:
        case 0xa00:  case 0xb00:  case 0xe00:  case 0xf00:
          return ADDR_IO;
      }
    }
  }
  return ADDR_ROM;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartDebug::getBankDirectives(ostream& buf, BankInfo& info) const
{
  // Start with the offset for this bank
  buf << "ORG " << Base::HEX4 << info.offset << endl;

  // Now consider each byte
  uInt32 prev = info.offset, addr = prev + 1;
  DisasmType prevType = disasmTypeAbsolute(mySystem.getAccessFlags(prev));
  for( ; addr < info.offset + info.size; ++addr)
  {
    DisasmType currType = disasmTypeAbsolute(mySystem.getAccessFlags(addr));

    // Have we changed to a new type?
    if(currType != prevType)
    {
      disasmTypeAsString(buf, prevType);
      buf << " " << Base::HEX4 << prev << " " << Base::HEX4 << (addr-1) << endl;

      prev = addr;
      prevType = currType;
    }
  }

  // Grab the last directive, making sure it accounts for all remaining space
  if(prev != addr)
  {
    disasmTypeAsString(buf, prevType);
    buf << " " << Base::HEX4 << prev << " " << Base::HEX4 << (addr-1) << endl;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartDebug::addressTypeAsString(ostream& buf, uInt16 addr) const
{
  if(!(addr & 0x1000))
  {
    buf << DebuggerParser::red("type only defined for cart address space");
    return;
  }

  uInt8 directive = myDisDirectives[addr & 0xFFF] & 0xFC,
        debugger  = myDebugger.getAccessFlags(addr) & 0xFC,
        label     = myDisLabels[addr & 0xFFF];

  buf << endl << "directive: " << Base::toString(directive, Base::F_2_8) << " ";
  disasmTypeAsString(buf, directive);
  buf << endl << "emulation: " << Base::toString(debugger, Base::F_2_8) << " ";
  disasmTypeAsString(buf, debugger);
  buf << endl << "tentative: " << Base::toString(label, Base::F_2_8) << " ";
  disasmTypeAsString(buf, label);
  buf << endl;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartDebug::DisasmType CartDebug::disasmTypeAbsolute(uInt8 flags) const
{
  if(flags & CartDebug::CODE)
    return CartDebug::CODE;
  else if(flags & CartDebug::TCODE)
    return CartDebug::CODE;          // TODO - should this be separate??
  else if(flags & CartDebug::GFX)
    return CartDebug::GFX;
  else if(flags & CartDebug::PGFX)
    return CartDebug::PGFX;
  else if(flags & CartDebug::DATA)
    return CartDebug::DATA;
  else if(flags & CartDebug::ROW)
    return CartDebug::ROW;
  else
    return CartDebug::NONE;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartDebug::disasmTypeAsString(ostream& buf, DisasmType type) const
{
  switch(type)
  {
    case CartDebug::CODE:   buf << "CODE";   break;
    case CartDebug::TCODE:  buf << "TCODE";  break;
    case CartDebug::GFX:    buf << "GFX";    break;
    case CartDebug::PGFX:   buf << "PGFX";   break;
    case CartDebug::DATA:   buf << "DATA";   break;
    case CartDebug::ROW:    buf << "ROW";    break;
    case CartDebug::REFERENCED:
    case CartDebug::VALID_ENTRY:
    case CartDebug::NONE:                    break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartDebug::disasmTypeAsString(ostream& buf, uInt8 flags) const
{
  if(flags)
  {
    if(flags & CartDebug::CODE)
      buf << "CODE ";
    if(flags & CartDebug::TCODE)
      buf << "TCODE ";
    if(flags & CartDebug::GFX)
      buf << "GFX ";
    if(flags & CartDebug::PGFX)
      buf << "PGFX ";
    if(flags & CartDebug::DATA)
      buf << "DATA ";
    if(flags & CartDebug::ROW)
      buf << "ROW ";
    if(flags & CartDebug::REFERENCED)
      buf << "*REFERENCED ";
    if(flags & CartDebug::VALID_ENTRY)
      buf << "*VALID_ENTRY ";
  }
  else
    buf << "no flags set";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const char* const CartDebug::ourTIAMnemonicR[16] = {
  "CXM0P", "CXM1P", "CXP0FB", "CXP1FB", "CXM0FB", "CXM1FB", "CXBLPF", "CXPPMM",
  "INPT0", "INPT1", "INPT2", "INPT3", "INPT4", "INPT5", "$1e", "$1f"
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const char* const CartDebug::ourTIAMnemonicW[64] = {
  "VSYNC", "VBLANK", "WSYNC", "RSYNC", "NUSIZ0", "NUSIZ1", "COLUP0", "COLUP1",
  "COLUPF", "COLUBK", "CTRLPF", "REFP0", "REFP1", "PF0", "PF1", "PF2",
  "RESP0", "RESP1", "RESM0", "RESM1", "RESBL", "AUDC0", "AUDC1", "AUDF0",
  "AUDF1", "AUDV0", "AUDV1", "GRP0", "GRP1", "ENAM0", "ENAM1", "ENABL",
  "HMP0", "HMP1", "HMM0", "HMM1", "HMBL", "VDELP0", "VDELP1", "VDELBL",
  "RESMP0", "RESMP1", "HMOVE", "HMCLR", "CXCLR", "$2d", "$2e", "$2f",
  "$30", "$31", "$32", "$33", "$34", "$35", "$36", "$37",
  "$38", "$39", "$3a", "$3b", "$3c", "$3d", "$3e", "$3f"
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const char* const CartDebug::ourIOMnemonic[24] = {
  "SWCHA", "SWACNT", "SWCHB", "SWBCNT", "INTIM", "TIMINT",
  "$286", "$287", "$288", "$289", "$28a", "$28b", "$28c",
  "$28d", "$28e", "$28f", "$290", "$291", "$292", "$293",
  "TIM1T", "TIM8T", "TIM64T", "T1024T"
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const char* const CartDebug::ourZPMnemonic[128] = {
  "ram_80", "ram_81", "ram_82", "ram_83", "ram_84", "ram_85", "ram_86", "ram_87",
  "ram_88", "ram_89", "ram_8A", "ram_8B", "ram_8C", "ram_8D", "ram_8E", "ram_8F",
  "ram_90", "ram_91", "ram_92", "ram_93", "ram_94", "ram_95", "ram_96", "ram_97",
  "ram_98", "ram_99", "ram_9A", "ram_9B", "ram_9C", "ram_9D", "ram_9E", "ram_9F",
  "ram_A0", "ram_A1", "ram_A2", "ram_A3", "ram_A4", "ram_A5", "ram_A6", "ram_A7",
  "ram_A8", "ram_A9", "ram_AA", "ram_AB", "ram_AC", "ram_AD", "ram_AE", "ram_AF",
  "ram_B0", "ram_B1", "ram_B2", "ram_B3", "ram_B4", "ram_B5", "ram_B6", "ram_B7",
  "ram_B8", "ram_B9", "ram_BA", "ram_BB", "ram_BC", "ram_BD", "ram_BE", "ram_BF",
  "ram_C0", "ram_C1", "ram_C2", "ram_C3", "ram_C4", "ram_C5", "ram_C6", "ram_C7",
  "ram_C8", "ram_C9", "ram_CA", "ram_CB", "ram_CC", "ram_CD", "ram_CE", "ram_CF",
  "ram_D0", "ram_D1", "ram_D2", "ram_D3", "ram_D4", "ram_D5", "ram_D6", "ram_D7",
  "ram_D8", "ram_D9", "ram_DA", "ram_DB", "ram_DC", "ram_DD", "ram_DE", "ram_DF",
  "ram_E0", "ram_E1", "ram_E2", "ram_E3", "ram_E4", "ram_E5", "ram_E6", "ram_E7",
  "ram_E8", "ram_E9", "ram_EA", "ram_EB", "ram_EC", "ram_ED", "ram_EE", "ram_EF",
  "ram_F0", "ram_F1", "ram_F2", "ram_F3", "ram_F4", "ram_F5", "ram_F6", "ram_F7",
  "ram_F8", "ram_F9", "ram_FA", "ram_FB", "ram_FC", "ram_FD", "ram_FE", "ram_FF"
};
