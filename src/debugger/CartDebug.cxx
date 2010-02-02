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
// Copyright (c) 1995-2009 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

#include "bspf.hxx"
#include "Array.hxx"
#include "System.hxx"
#include "DiStella.hxx"
#include "CartDebug.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartDebug::CartDebug(Debugger& dbg, Console& console, const RamAreaList& areas)
  : DebuggerSystem(dbg, console),
    myRWPortAddress(0)
{
  // Zero-page RAM is always present
  addRamArea(0x80, 128, 0, 0);

  // Add extended RAM
  myRamAreas = areas;
  for(RamAreaList::const_iterator i = areas.begin(); i != areas.end(); ++i)
    addRamArea(i->start, i->size, i->roffset, i->woffset);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartDebug::addRamArea(uInt16 start, uInt16 size,
                           uInt16 roffset, uInt16 woffset)
{
  // First make sure this area isn't already present
  for(uInt32 i = 0; i < myState.rport.size(); ++i)
    if(myState.rport[i] == start + roffset ||
       myState.wport[i] == start + woffset)
      return;

  // Otherwise, add a new area
  for(uInt32 i = 0; i < size; ++i)
  {
    myState.rport.push_back(i + start + roffset);
    myState.wport.push_back(i + start + woffset);

    myOldState.rport.push_back(i + start + roffset);
    myOldState.wport.push_back(i + start + woffset);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const DebuggerState& CartDebug::getState()
{
  myState.ram.clear();
  for(uInt32 i = 0; i < myState.rport.size(); ++i)
    myState.ram.push_back(read(myState.rport[i]));

  return myState;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartDebug::saveOldState()
{
  myOldState.ram.clear();
  for(uInt32 i = 0; i < myOldState.rport.size(); ++i)
    myOldState.ram.push_back(read(myOldState.rport[i]));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 CartDebug::read(uInt16 addr)
{
  return mySystem.peek(addr);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartDebug::write(uInt16 addr, uInt8 value)
{
  mySystem.poke(addr, value);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int CartDebug::readFromWritePort()
{
  uInt16 peekAddress = myRWPortAddress;
  myRWPortAddress = 0;

  // A read from the write port occurs when the read is actually in the write
  // port address space AND the last access was actually a read (the latter
  // differentiates between reads that are normally part of a write cycle vs.
  // ones that are illegal
  if(mySystem.m6502().lastReadAddress() && peekAddress & 0x1000)
  {
    uInt16 addr = peekAddress & 0x0FFF;
    for(RamAreaList::const_iterator i = myRamAreas.begin(); i != myRamAreas.end(); ++i)
    {
      uInt16 start = (i->start + i->woffset) & 0x0FFF;
      uInt16 end = (i->start + i->woffset + i->size) & 0x0FFF;
      if(addr >= start && addr < end)
        return peekAddress;
    }
  }
  return 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string CartDebug::toString()
{
  string result;
  char buf[128];
  uInt32 bytesPerLine;

  switch(myDebugger.parser().base())
  {
    case kBASE_16:
    case kBASE_10:
      bytesPerLine = 0x10;
      break;

    case kBASE_2:
      bytesPerLine = 0x04;
      break;

    case kBASE_DEFAULT:
    default:
      return DebuggerParser::red("invalid base, this is a BUG");
  }

  const CartState& state    = (CartState&) getState();
  const CartState& oldstate = (CartState&) getOldState();

  uInt32 curraddr = 0, bytesSoFar = 0;
  for(uInt32 i = 0; i < state.ram.size(); i += bytesPerLine, bytesSoFar += bytesPerLine)
  {
    // We detect different 'pages' of RAM when the addresses jump by
    // more than the number of bytes on the previous line, or when 256
    // bytes have been previously output
    if(state.rport[i] - curraddr > bytesPerLine || bytesSoFar >= 256)
    {
      sprintf(buf, "%04x: (rport = %04x, wport = %04x)\n",
              state.rport[i], state.rport[i], state.wport[i]);
      buf[2] = buf[3] = 'x';
      result += DebuggerParser::red(buf);
      bytesSoFar = 0;
    }
    curraddr = state.rport[i];
    sprintf(buf, "%.2x: ", curraddr & 0x00ff);
    result += buf;

    for(uInt8 j = 0; j < bytesPerLine; ++j)
    {
      result += myDebugger.invIfChanged(state.ram[i+j], oldstate.ram[i+j]);
      result += " ";

      if(j == 0x07) result += " ";
    }
    result += "\n";
  }

  return result;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const string& CartDebug::disassemble(int start, int lines)
{
  static string result= "";
  ostringstream buffer;
  string cpubuf;

  if(start < 0x80 || start > 0xffff)
    return result;

  do {
    buffer << getLabel(start, true, 4) << ": ";

    int count = disassemble(start, cpubuf);
    for(int i = 0; i < count; i++)
      buffer << hex << setw(2) << setfill('0') << myDebugger.peek(start++) << " " << dec;

    if(count < 3) buffer << "   ";
    if(count < 2) buffer << "   ";

    buffer << " " << cpubuf << "\n";
  } while(--lines > 0 && start <= 0xffff);

  result = buffer.str();
  return result;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartDebug::disassemble(IntArray& addr, StringList& addrLabel,
                            StringList& bytes, StringList& data,
                            int start, int end)
{
  if(start < 0x80 || end > 0xffff)
    return;

  string cpubuf, tmp;
  char buf[255];

  do {
    addr.push_back(start);
    addrLabel.push_back(getLabel(start, true, 4) + ":");

    cpubuf = "";
    int count = disassemble(start, cpubuf);

    tmp = "";
    for(int i = 0; i < count; i++)
    {
      sprintf(buf, "%02x ", myDebugger.peek(start++));
      tmp += buf;
    }
    bytes.push_back(tmp);

    data.push_back(cpubuf);
  }
  while(start <= end);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartDebug::disassemble(DisassemblyList& list, uInt16 start, bool autocode)
{
  DiStella distella;
  distella.disassemble(list, start, autocode);

  // TODO - look at list, extract address to label mappings
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int CartDebug::disassemble(int address, string& result)
{
  ostringstream buf;
  int count = 0;
  int opcode = mySystem.peek(address);

  // Are we looking at a read or write operation?
  // It will determine what type of label to use
  bool isRead = (CartDebug::AccessModeTable[opcode] == CartDebug::Read);

  switch(CartDebug::AddressModeTable[opcode])
  {
    case CartDebug::Absolute:
      buf << CartDebug::InstructionMnemonicTable[opcode] << " "
          << getLabel(myDebugger.dpeek(address + 1), isRead, 4) << " ; "
          << M6502::ourInstructionCycleTable[opcode];
      count = 3;
      break;

    case CartDebug::AbsoluteX:
      buf << CartDebug::InstructionMnemonicTable[opcode] << " "
          << getLabel(myDebugger.dpeek(address + 1), isRead, 4) << ",x ; "
          << M6502::ourInstructionCycleTable[opcode];
      count = 3;
      break;

    case CartDebug::AbsoluteY:
      buf << CartDebug::InstructionMnemonicTable[opcode] << " "
          << getLabel(myDebugger.dpeek(address + 1), isRead, 4) << ",y ; "
          << M6502::ourInstructionCycleTable[opcode];
      count = 3;
      break;

    case CartDebug::Immediate:
      buf << CartDebug::InstructionMnemonicTable[opcode] << " #$"
          << hex << setw(2) << setfill('0') << (int) mySystem.peek(address + 1) << " ; "
          << dec << M6502::ourInstructionCycleTable[opcode];
      count = 2;
      break;

    case CartDebug::Implied:
      buf << CartDebug::InstructionMnemonicTable[opcode] << " ; "
          << M6502::ourInstructionCycleTable[opcode];
      count = 1;
      break;

    case CartDebug::Indirect:
      buf << CartDebug::InstructionMnemonicTable[opcode] << " ("
          << getLabel(myDebugger.dpeek(address + 1), isRead, 4) << ") ; "
          << M6502::ourInstructionCycleTable[opcode];
      count = 3;
      break;

    case CartDebug::IndirectX:
      buf << CartDebug::InstructionMnemonicTable[opcode] << " ("
          << getLabel(mySystem.peek(address + 1), isRead, 2) << ",x) ; "
          << M6502::ourInstructionCycleTable[opcode];
      count = 2;
      break;

    case CartDebug::IndirectY:
      buf << CartDebug::InstructionMnemonicTable[opcode] << " ("
          << getLabel(mySystem.peek(address + 1), isRead, 2) << "),y ; "
          << M6502::ourInstructionCycleTable[opcode];
      count = 2;
      break;

    case CartDebug::Relative:
      buf << CartDebug::InstructionMnemonicTable[opcode] << " "
          << getLabel(address + 2 + ((Int16)(Int8)mySystem.peek(address + 1)), isRead, 4)
          << " ; " << M6502::ourInstructionCycleTable[opcode];
      count = 2;
      break;

    case CartDebug::Zero:
      buf << CartDebug::InstructionMnemonicTable[opcode] << " "
          << getLabel(mySystem.peek(address + 1), isRead, 2) << " ; "
          << M6502::ourInstructionCycleTable[opcode];
      count = 2;
      break;

    case CartDebug::ZeroX:
      buf << CartDebug::InstructionMnemonicTable[opcode] << " "
          << getLabel(mySystem.peek(address + 1), isRead, 2) << ",x ; "
          << M6502::ourInstructionCycleTable[opcode];
      count = 2;
      break;

    case CartDebug::ZeroY:
      buf << CartDebug::InstructionMnemonicTable[opcode] << " "
          << getLabel(mySystem.peek(address + 1), isRead, 2) << ",y ; "
          << M6502::ourInstructionCycleTable[opcode];
      count = 2;
      break;

    default:
      buf << "dc  $" << hex << setw(2) << setfill('0') << (int) opcode << " ; "
          << dec << M6502::ourInstructionCycleTable[opcode];
      count = 1;
      break;
  }

  result = buf.str();
  return count;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int CartDebug::getBank()
{
  return myConsole.cartridge().bank();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int CartDebug::bankCount()
{
  return myConsole.cartridge().bankCount();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string CartDebug::getCartType()
{
  return myConsole.cartridge().name();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartDebug::addLabel(const string& label, uInt16 address)
{
  // First check if this already exists as a hard-coded equate
  LabelToAddr::const_iterator iter = mySystemAddresses.find(label);
  if(iter != mySystemAddresses.end() && iter->second.address == address)
    return;

  // Create a new user equate, and determine if its RAM or ROM
  // For now, these are the only types we care about
  // Technically, addresses above 0x1000 are ROM and are read-only
  // However, somes ROMs have dedicated RAM mapped to those addresses
  // as well, and we don't yet have an infrastructure to determine that,
  // so the entire region is marked as read-write
  equate_t flags = EQF_READ;
#if 0
  if(address >= 0x80 && address <= 0xff)
    flags = EQF_RW;
  else if((address & 0x1000) == 0x1000)
    flags = EQF_RW;
  else
  { 
    cerr << "label = " << label << ", address = " << hex << address << " discarded\n";
    return;  // don't know what else to do for now
  }
#else
  // The above section of code is deactivated until a better means of
  // determining constants vs. addresses is found
  flags = EQF_RW;
#endif

  removeLabel(label);

  Equate e;
  e.label = label;
  e.address = address;
  e.flags = flags;

  myUserAddresses.insert(make_pair(label, e));
  myUserLabels.insert(make_pair(address, e));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartDebug::removeLabel(const string& label)
{
  // Note that only user-defined equates can be removed
  LabelToAddr::iterator iter = myUserAddresses.find(label);
  if(iter != myUserAddresses.end())
  {
    // Erase the label
    myUserAddresses.erase(iter);

    // And also erase the address assigned to it
    AddrToLabel::iterator iter2 = myUserLabels.find(iter->second.address);
    if(iter2 != myUserLabels.end())
      myUserLabels.erase(iter2);

    return true;
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const string& CartDebug::getLabel(uInt16 addr, bool isRead, int places) const
{
  static string result;
  AddrToLabel::const_iterator iter;

  // Is this a read or write?
  // For now, there aren't separate read & write lists for user labels
  const AddrToLabel& systemLabels = isRead ?
    (const AddrToLabel&) mySystemReadLabels :
    (const AddrToLabel&) mySystemWriteLabels;

  // Determine the type of address to access the correct list
  // These addresses were based on (and checked against) Kroko's 2600 memory
  // map, found at http://www.qotile.net/minidig/docs/2600_mem_map.txt
  address_t type = ADDR_ROM;
  if(addr % 0x2000 < 0x1000)
  {
    uInt16 z = addr & 0x00ff;
    if(z < 0x80)
      type = ADDR_TIA;
    else
    {
      switch(addr & 0x0f00)
      {
        case 0x000:
        case 0x100:
        case 0x400:
        case 0x500:
        case 0x800:
        case 0x900:
        case 0xc00:
        case 0xd00:
          type = ADDR_RAM;
          break;
        case 0x200:
        case 0x300:
        case 0x600:
        case 0x700:
        case 0xa00:
        case 0xb00:
        case 0xe00:
        case 0xf00:
          type = ADDR_RIOT;
          break;
      }
    }
  }

  switch(type)
  {
    case ADDR_TIA:
      if((iter = systemLabels.find(addr&0x7f)) != systemLabels.end())
        return iter->second.label;
      else if((iter = myUserLabels.find(addr)) != myUserLabels.end())
        return iter->second.label;
      break;

    case ADDR_RIOT:  // FIXME - add mirrors for RIOT
      if((iter = systemLabels.find(addr)) != systemLabels.end())
        return iter->second.label;
      else if((iter = myUserLabels.find(addr)) != myUserLabels.end())
        return iter->second.label;
      break;

    case ADDR_RAM:
    case ADDR_ROM:
      // These addresses can never be in the system labels list
      if((iter = myUserLabels.find(addr)) != myUserLabels.end())
        return iter->second.label;
      break;
  }

  if(places > -1)
  {
    ostringstream buf;
    buf << "$" << setw(places) << hex << addr;
    return result = buf.str();
  }

  return EmptyString;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int CartDebug::getAddress(const string& label) const
{
  LabelToAddr::const_iterator iter;

  if((iter = mySystemAddresses.find(label)) != mySystemAddresses.end())
    return iter->second.address;
  else if((iter = myUserAddresses.find(label)) != myUserAddresses.end())
    return iter->second.address;
  else
    return -1;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string CartDebug::loadSymbolFile(const string& f)
{
  string file = f;
  string::size_type spos;
  if( (spos = file.find_last_of('.')) != string::npos )
    file.replace(spos, file.size(), ".sym");
  else
    file += ".sym";

  int pos = 0, lines = 0, curVal;
  string curLabel;
  char line[1024];

  ifstream in(file.c_str());
  if(!in.is_open())
    return "Unable to read symbols from " + file;

  myUserAddresses.clear();
  myUserLabels.clear();

  while( !in.eof() )
  {
    curVal = 0;
    curLabel = "";

    int got = in.get();

    if(got == -1 || got == '\r' || got == '\n' || pos == 1023) {
      line[pos] = '\0';
      pos = 0;

      if(strlen(line) > 0 && line[0] != '-')
      {
        curLabel = extractLabel(line);
        if((curVal = extractValue(line)) < 0)
          return "invalid symbol file";
  
        addLabel(curLabel, curVal);
  
        lines++;
      }
    }
    else
    {
      line[pos++] = got;
    }
  }
  in.close();
  return "loaded " + file + " OK";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartDebug::saveSymbolFile(const string& file)
{
  // Only user-defined equates are saved; system equates are always
  // available, so there's no need to save them
  char buf[256];

  ofstream out(file.c_str());
  if(!out.is_open())
    return false;

  out << "--- Symbol List (sorted by symbol)" << endl;

  LabelToAddr::const_iterator iter;
  for(iter = myUserAddresses.begin(); iter != myUserAddresses.end(); iter++)
  {
    sprintf(buf, "%-24s %04x                  \n", iter->second.label.c_str(), iter->second.address);
    out << buf;
  }

  out << "--- End of Symbol List." << endl;

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int CartDebug::countCompletions(const char *in)
{
  myCompletions = myCompPrefix = "";
  return countCompletions(in, mySystemAddresses) +
         countCompletions(in, myUserAddresses);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int CartDebug::countCompletions(const char *in, LabelToAddr& addresses)
{
  int count = 0;

  LabelToAddr::iterator iter;
  for(iter = addresses.begin(); iter != addresses.end(); iter++)
  {
    const char *l = iter->first.c_str();

    if(BSPF_strncasecmp(l, in, strlen(in)) == 0)
    {
      if(myCompPrefix == "")
        myCompPrefix += l;
      else
      {
        int nonMatch = 0;
        const char *c = myCompPrefix.c_str();
        while(*c != '\0' && tolower(*c) == tolower(l[nonMatch]))
        {
          c++;
          nonMatch++;
        }
        myCompPrefix.erase(nonMatch, myCompPrefix.length());
      }

      if(count++) myCompletions += "  ";
        myCompletions += l;
    }
  }

  return count;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string CartDebug::extractLabel(char *c) const
{
  string l = "";
  while(*c != ' ')
    l += *c++;

  return l;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int CartDebug::extractValue(char *c) const
{
  while(*c != ' ')
  {
    if(*c == '\0')
      return -1;
    c++;
  }

  while(*c == ' ')
  {
    if(*c == '\0')
      return -1;
    c++;
  }

  int ret = 0;
  for(int i=0; i<4; i++)
  {
    if(*c >= '0' && *c <= '9')
      ret = (ret << 4) + (*c) - '0';
    else if(*c >= 'a' && *c <= 'f')
      ret = (ret << 4) + (*c) - 'a' + 10;
    else
      return -1;
    c++;
  }
  return ret;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const CartDebug::Equate CartDebug::ourSystemEquates[kSystemEquateSize] = {
// Standard $00-based TIA write locations:
  { "VSYNC",  0x00, EQF_WRITE },
  { "VBLANK", 0x01, EQF_WRITE },
  { "WSYNC",  0x02, EQF_WRITE },
  { "RSYNC",  0x03, EQF_WRITE },
  { "NUSIZ0", 0x04, EQF_WRITE },
  { "NUSIZ1", 0x05, EQF_WRITE },
  { "COLUP0", 0x06, EQF_WRITE },
  { "COLUP1", 0x07, EQF_WRITE },
  { "COLUPF", 0x08, EQF_WRITE },
  { "COLUBK", 0x09, EQF_WRITE },
  { "CTRLPF", 0x0A, EQF_WRITE },
  { "REFP0",  0x0B, EQF_WRITE },
  { "REFP1",  0x0C, EQF_WRITE },
  { "PF0",    0x0D, EQF_WRITE },
  { "PF1",    0x0E, EQF_WRITE },
  { "PF2",    0x0F, EQF_WRITE },
  { "RESP0",  0x10, EQF_WRITE },
  { "RESP1",  0x11, EQF_WRITE },
  { "RESM0",  0x12, EQF_WRITE },
  { "RESM1",  0x13, EQF_WRITE },
  { "RESBL",  0x14, EQF_WRITE },
  { "AUDC0",  0x15, EQF_WRITE },
  { "AUDC1",  0x16, EQF_WRITE },
  { "AUDF0",  0x17, EQF_WRITE },
  { "AUDF1",  0x18, EQF_WRITE },
  { "AUDV0",  0x19, EQF_WRITE },
  { "AUDV1",  0x1A, EQF_WRITE },
  { "GRP0",   0x1B, EQF_WRITE },
  { "GRP1",   0x1C, EQF_WRITE },
  { "ENAM0",  0x1D, EQF_WRITE },
  { "ENAM1",  0x1E, EQF_WRITE },
  { "ENABL",  0x1F, EQF_WRITE },
  { "HMP0",   0x20, EQF_WRITE },
  { "HMP1",   0x21, EQF_WRITE },
  { "HMM0",   0x22, EQF_WRITE },
  { "HMM1",   0x23, EQF_WRITE },
  { "HMBL",   0x24, EQF_WRITE },
  { "VDELP0", 0x25, EQF_WRITE },
  { "VDEL01", 0x26, EQF_WRITE },
  { "VDELP1", 0x26, EQF_WRITE },
  { "VDELBL", 0x27, EQF_WRITE },
  { "RESMP0", 0x28, EQF_WRITE },
  { "RESMP1", 0x29, EQF_WRITE },
  { "HMOVE",  0x2A, EQF_WRITE },
  { "HMCLR",  0x2B, EQF_WRITE },
  { "CXCLR",  0x2C, EQF_WRITE },

// Mirrored $40-based TIA write locations:
  { "VSYNC.40",  0x40, EQF_WRITE },
  { "VBLANK.40", 0x41, EQF_WRITE },
  { "WSYNC.40",  0x42, EQF_WRITE },
  { "RSYNC.40",  0x43, EQF_WRITE },
  { "NUSIZ0.40", 0x44, EQF_WRITE },
  { "NUSIZ1.40", 0x45, EQF_WRITE },
  { "COLUP0.40", 0x46, EQF_WRITE },
  { "COLUP1.40", 0x47, EQF_WRITE },
  { "COLUPF.40", 0x48, EQF_WRITE },
  { "COLUBK.40", 0x49, EQF_WRITE },
  { "CTRLPF.40", 0x4A, EQF_WRITE },
  { "REFP0.40",  0x4B, EQF_WRITE },
  { "REFP1.40",  0x4C, EQF_WRITE },
  { "PF0.40",    0x4D, EQF_WRITE },
  { "PF1.40",    0x4E, EQF_WRITE },
  { "PF2.40",    0x4F, EQF_WRITE },
  { "RESP0.40",  0x50, EQF_WRITE },
  { "RESP1.40",  0x51, EQF_WRITE },
  { "RESM0.40",  0x52, EQF_WRITE },
  { "RESM1.40",  0x53, EQF_WRITE },
  { "RESBL.40",  0x54, EQF_WRITE },
  { "AUDC0.40",  0x55, EQF_WRITE },
  { "AUDC1.40",  0x56, EQF_WRITE },
  { "AUDF0.40",  0x57, EQF_WRITE },
  { "AUDF1.40",  0x58, EQF_WRITE },
  { "AUDV0.40",  0x59, EQF_WRITE },
  { "AUDV1.40",  0x5A, EQF_WRITE },
  { "GRP0.40",   0x5B, EQF_WRITE },
  { "GRP1.40",   0x5C, EQF_WRITE },
  { "ENAM0.40",  0x5D, EQF_WRITE },
  { "ENAM1.40",  0x5E, EQF_WRITE },
  { "ENABL.40",  0x5F, EQF_WRITE },
  { "HMP0.40",   0x60, EQF_WRITE },
  { "HMP1.40",   0x61, EQF_WRITE },
  { "HMM0.40",   0x62, EQF_WRITE },
  { "HMM1.40",   0x63, EQF_WRITE },
  { "HMBL.40",   0x64, EQF_WRITE },
  { "VDELP0.40", 0x65, EQF_WRITE },
  { "VDEL01.40", 0x66, EQF_WRITE },
  { "VDELP1.40", 0x66, EQF_WRITE },
  { "VDELBL.40", 0x67, EQF_WRITE },
  { "RESMP0.40", 0x68, EQF_WRITE },
  { "RESMP1.40", 0x69, EQF_WRITE },
  { "HMOVE.40",  0x6A, EQF_WRITE },
  { "HMCLR.40",  0x6B, EQF_WRITE },
  { "CXCLR.40",  0x6C, EQF_WRITE },

// Standard $00-based TIA read locations:
  { "CXM0P",  0x00, EQF_READ },
  { "CXM1P",  0x01, EQF_READ },
  { "CXP0FB", 0x02, EQF_READ },
  { "CXP1FB", 0x03, EQF_READ },
  { "CXM0FB", 0x04, EQF_READ },
  { "CXM1FB", 0x05, EQF_READ },
  { "CXBLPF", 0x06, EQF_READ },
  { "CXPPMM", 0x07, EQF_READ },
  { "INPT0",  0x08, EQF_READ },
  { "INPT1",  0x09, EQF_READ },
  { "INPT2",  0x0A, EQF_READ },
  { "INPT3",  0x0B, EQF_READ },
  { "INPT4",  0x0C, EQF_READ },
  { "INPT5",  0x0D, EQF_READ },

// Mirrored $10-based TIA read locations:
  { "CXM0P.10",  0x10, EQF_READ },
  { "CXM1P.10",  0x11, EQF_READ },
  { "CXP0FB.10", 0x12, EQF_READ },
  { "CXP1FB.10", 0x13, EQF_READ },
  { "CXM0FB.10", 0x14, EQF_READ },
  { "CXM1FB.10", 0x15, EQF_READ },
  { "CXBLPF.10", 0x16, EQF_READ },
  { "CXPPMM.10", 0x17, EQF_READ },
  { "INPT0.10",  0x18, EQF_READ },
  { "INPT1.10",  0x19, EQF_READ },
  { "INPT2.10",  0x1A, EQF_READ },
  { "INPT3.10",  0x1B, EQF_READ },
  { "INPT4.10",  0x1C, EQF_READ },
  { "INPT5.10",  0x1D, EQF_READ },

// Mirrored $20-based TIA read locations:
  { "CXM0P.20",  0x20, EQF_READ },
  { "CXM1P.20",  0x21, EQF_READ },
  { "CXP0FB.20", 0x22, EQF_READ },
  { "CXP1FB.20", 0x23, EQF_READ },
  { "CXM0FB.20", 0x24, EQF_READ },
  { "CXM1FB.20", 0x25, EQF_READ },
  { "CXBLPF.20", 0x26, EQF_READ },
  { "CXPPMM.20", 0x27, EQF_READ },
  { "INPT0.20",  0x28, EQF_READ },
  { "INPT1.20",  0x29, EQF_READ },
  { "INPT2.20",  0x2A, EQF_READ },
  { "INPT3.20",  0x2B, EQF_READ },
  { "INPT4.20",  0x2C, EQF_READ },
  { "INPT5.20",  0x2D, EQF_READ },

// Mirrored $30-based TIA read locations:
  { "CXM0P.30",  0x30, EQF_READ },
  { "CXM1P.30",  0x31, EQF_READ },
  { "CXP0FB.30", 0x32, EQF_READ },
  { "CXP1FB.30", 0x33, EQF_READ },
  { "CXM0FB.30", 0x34, EQF_READ },
  { "CXM1FB.30", 0x35, EQF_READ },
  { "CXBLPF.30", 0x36, EQF_READ },
  { "CXPPMM.30", 0x37, EQF_READ },
  { "INPT0.30",  0x38, EQF_READ },
  { "INPT1.30",  0x39, EQF_READ },
  { "INPT2.30",  0x3A, EQF_READ },
  { "INPT3.30",  0x3B, EQF_READ },
  { "INPT4.30",  0x3C, EQF_READ },
  { "INPT5.30",  0x3D, EQF_READ },

// Standard RIOT locations (read, write, or both):
  { "SWCHA",    0x280, EQF_RW    },
  { "SWCHB",    0x282, EQF_RW    },
  { "SWACNT",   0x281, EQF_WRITE },
  { "SWBCNT",   0x283, EQF_WRITE },
  { "INTIM",    0x284, EQF_READ  },
  { "TIMINT",   0x285, EQF_READ  },
  { "TIM1T",    0x294, EQF_WRITE },
  { "TIM8T",    0x295, EQF_WRITE },
  { "TIM64T",   0x296, EQF_WRITE },
  { "T1024T",   0x297, EQF_WRITE }
};

///////////////////////////////////////////////////////////////////
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartDebug::AddressingMode CartDebug::AddressModeTable[256] = {
  Implied,    IndirectX, Invalid,   IndirectX,    // 0x0?
  Zero,       Zero,      Zero,      Zero,
  Implied,    Immediate, Implied,   Immediate,
  Absolute,   Absolute,  Absolute,  Absolute,

  Relative,   IndirectY, Invalid,   IndirectY,    // 0x1?
  ZeroX,      ZeroX,     ZeroX,     ZeroX,
  Implied,    AbsoluteY, Implied,   AbsoluteY,
  AbsoluteX,  AbsoluteX, AbsoluteX, AbsoluteX,

  Absolute,   IndirectX, Invalid,   IndirectX,    // 0x2?
  Zero,       Zero,      Zero,      Zero,
  Implied,    Immediate, Implied,   Immediate,
  Absolute,   Absolute,  Absolute,  Absolute,

  Relative,   IndirectY, Invalid,   IndirectY,    // 0x3?
  ZeroX,      ZeroX,     ZeroX,     ZeroX,
  Implied,    AbsoluteY, Implied,   AbsoluteY,
  AbsoluteX,  AbsoluteX, AbsoluteX, AbsoluteX,

  Implied,    IndirectX, Invalid,   IndirectX,    // 0x4?
  Zero,       Zero,      Zero,      Zero,
  Implied,    Immediate, Implied,   Immediate,
  Absolute,   Absolute,  Absolute,  Absolute,

  Relative,   IndirectY, Invalid,   IndirectY,    // 0x5?
  ZeroX,      ZeroX,     ZeroX,     ZeroX,
  Implied,    AbsoluteY, Implied,   AbsoluteY,
  AbsoluteX,  AbsoluteX, AbsoluteX, AbsoluteX,

  Implied,    IndirectX, Invalid,   IndirectX,    // 0x6?
  Zero,       Zero,      Zero,      Zero,
  Implied,    Immediate, Implied,   Immediate,
  Indirect,   Absolute,  Absolute,  Absolute,

  Relative,   IndirectY, Invalid,   IndirectY,    // 0x7?
  ZeroX,      ZeroX,     ZeroX,     ZeroX,
  Implied,    AbsoluteY, Implied,   AbsoluteY,
  AbsoluteX,  AbsoluteX, AbsoluteX, AbsoluteX,

  Immediate,  IndirectX, Immediate, IndirectX,    // 0x8?
  Zero,       Zero,      Zero,      Zero,
  Implied,    Immediate, Implied,   Immediate,
  Absolute,   Absolute,  Absolute,  Absolute,

  Relative,   IndirectY, Invalid,   IndirectY,    // 0x9?
  ZeroX,      ZeroX,     ZeroY,     ZeroY,
  Implied,    AbsoluteY, Implied,   AbsoluteY,
  AbsoluteX,  AbsoluteX, AbsoluteY, AbsoluteY,

  Immediate,  IndirectX, Immediate, IndirectX,    // 0xA?
  Zero,       Zero,      Zero,      Zero,
  Implied,    Immediate, Implied,   Immediate,
  Absolute,   Absolute,  Absolute,  Absolute,

  Relative,   IndirectY, Invalid,   IndirectY,    // 0xB?
  ZeroX,      ZeroX,     ZeroY,     ZeroY,
  Implied,    AbsoluteY, Implied,   AbsoluteY,
  AbsoluteX,  AbsoluteX, AbsoluteY, AbsoluteY,

  Immediate,  IndirectX, Immediate, IndirectX,    // 0xC?
  Zero,       Zero,      Zero,      Zero,
  Implied,    Immediate, Implied,   Immediate,
  Absolute,   Absolute,  Absolute,  Absolute,

  Relative,   IndirectY, Invalid,   IndirectY,    // 0xD?
  ZeroX,      ZeroX,     ZeroX,     ZeroX,
  Implied,    AbsoluteY, Implied,   AbsoluteY,
  AbsoluteX,  AbsoluteX, AbsoluteX, AbsoluteX,

  Immediate,  IndirectX, Immediate, IndirectX,    // 0xE?
  Zero,       Zero,      Zero,      Zero,
  Implied,    Immediate, Implied,   Immediate,
  Absolute,   Absolute,  Absolute,  Absolute,

  Relative,   IndirectY, Invalid,   IndirectY,    // 0xF?
  ZeroX,      ZeroX,     ZeroX,     ZeroX,
  Implied,    AbsoluteY, Implied,   AbsoluteY,
  AbsoluteX,  AbsoluteX, AbsoluteX, AbsoluteX
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartDebug::AccessMode CartDebug::AccessModeTable[256] = {
  None,   Read,   None,   Write,    // 0x0?
  None,   Read,   Write,  Write,
  None,   Read,   Write,  Read,
  None,   Read,   Write,  Write,

  Read,   Read,   None,   Write,    // 0x1?
  None,   Read,   Write,  Write,
  None,   Read,   None,   Write,
  None,   Read,   Write,  Write,

  Read,   Read,   None,   Write,    // 0x2?
  Read,   Read,   Write,  Write,
  None,   Read,   Write,  Read,
  Read,   Read,   Write,  Write,

  Read,   Read,   None,   Write,    // 0x3?
  None,   Read,   Write,  Write,
  None,   Read,   None,   Write,
  None,   Read,   Write,  Write,
  
  None,   Read,   None,   Write,    // 0x4?
  None,   Read,   Write,  Write,
  None,   Read,   Write,  Read,
  Read,   Read,   Write,  Write,

  Read,   Read,   None,   Write,    // 0x5?
  None,   Read,   Write,  Write,
  None,   Read,   None,   Write,
  None,   Read,   Write,  Write,

  None,   Read,   None,   Write,    // 0x6?
  None,   Read,   Write,  Write,
  None,   Read,   Write,  Read,
  Read,   Read,   Write,  Write,

  Read,   Read,   None,   Write,    // 0x7?
  None,   Read,   Write,  Write,
  None,   Read,   None,   Write,
  None,   Read,   Write,  Write,

  None,   Write,  None,   Write,    // 0x8?
  Write,  Write,  Write,  Write,
  None,   None,   None,   Read,
  Write,  Write,  Write,  Write,

  Read,   Write,  None,   Write,    // 0x9?
  Write,  Write,  Write,  Write,
  None,   Write,  None,   Write,
  Write,  Write,  Write,  Write,

  Read,   Read,   Read,   Read,     // 0xA?
  Read,   Read,   Read,   Read,
  None,   Read,   None,   Read,
  Read,   Read,   Read,   Read,

  Read,   Read,   None,   Read,     // 0xB?
  Read,   Read,   Read,   Read,
  None,   Read,   None,   Read,
  Read,   Read,   Read,   Read,

  Read,   Read,   None,   Write,    // 0xC?
  Read,   Read,   Write,  Write,
  None,   Read,   None,   Read,
  Read,   Read,   Write,  Write,

  Read,   Read,   None,   Write,    // 0xD?
  None,   Read,   Write,  Write,
  None,   Read,   None,   Write,
  None,   Read,   Write,  Write,

  Read,   Read,   None,   Write,    // 0xE?
  Read,   Read,   Write,  Write,
  None,   Read,   None,   Read,
  Read,   Read,   Write,  Write,

  Read,   Read,   None,   Write,    // 0xF?
  None,   Read,   Write,  Write,
  None,   Read,   None,   Write,
  None,   Read,   Write,  Write
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const char* CartDebug::InstructionMnemonicTable[256] = {
  "BRK",  "ORA",  "n/a",  "slo",  "nop",  "ORA",  "ASL",  "slo",    // 0x0?
  "PHP",  "ORA",  "ASLA", "anc",  "nop",  "ORA",  "ASL",  "slo",

  "BPL",  "ORA",  "n/a",  "slo",  "nop",  "ORA",  "ASL",  "slo",    // 0x1?
  "CLC",  "ORA",  "nop",  "slo",  "nop",  "ORA",  "ASL",  "slo",

  "JSR",  "AND",  "n/a",  "rla",  "BIT",  "AND",  "ROL",  "rla",    // 0x2?
  "PLP",  "AND",  "ROLA", "anc",  "BIT",  "AND",  "ROL",  "rla",

  "BMI",  "AND",  "n/a",  "rla",  "nop",  "AND",  "ROL",  "rla",    // 0x3?
  "SEC",  "AND",  "nop",  "rla",  "nop",  "AND",  "ROL",  "rla",
  
  "RTI",  "EOR",  "n/a",  "sre",  "nop",  "EOR",  "LSR",  "sre",    // 0x4?
  "PHA",  "EOR",  "LSRA", "asr",  "JMP",  "EOR",  "LSR",  "sre",

  "BVC",  "EOR",  "n/a",  "sre",  "nop",  "EOR",  "LSR",  "sre",    // 0x5?
  "CLI",  "EOR",  "nop",  "sre",  "nop",  "EOR",  "LSR",  "sre",

  "RTS",  "ADC",  "n/a",  "rra",  "nop",  "ADC",  "ROR",  "rra",    // 0x6?
  "PLA",  "ADC",  "RORA", "arr",  "JMP",  "ADC",  "ROR",  "rra",

  "BVS",  "ADC",  "n/a",  "rra",  "nop",  "ADC",  "ROR",  "rra",    // 0x7?
  "SEI",  "ADC",  "nop",  "rra",  "nop",  "ADC",  "ROR",  "rra",

  "nop",  "STA",  "nop",  "sax",  "STY",  "STA",  "STX",  "sax",    // 0x8?
  "DEY",  "nop",  "TXA",  "ane",  "STY",  "STA",  "STX",  "sax",

  "BCC",  "STA",  "n/a",  "sha",  "STY",  "STA",  "STX",  "sax",    // 0x9?
  "TYA",  "STA",  "TXS",  "shs",  "shy",  "STA",  "shx",  "sha",

  "LDY",  "LDA",  "LDX",  "lax",  "LDY",  "LDA",  "LDX",  "lax",    // 0xA?
  "TAY",  "LDA",  "TAX",  "lxa",  "LDY",  "LDA",  "LDX",  "lax",

  "BCS",  "LDA",  "n/a",  "lax",  "LDY",  "LDA",  "LDX",  "lax",    // 0xB?
  "CLV",  "LDA",  "TSX",  "las",  "LDY",  "LDA",  "LDX",  "lax",

  "CPY",  "CMP",  "nop",  "dcp",  "CPY",  "CMP",  "DEC",  "dcp",    // 0xC?
  "INY",  "CMP",  "DEX",  "sbx",  "CPY",  "CMP",  "DEC",  "dcp",

  "BNE",  "CMP",  "n/a",  "dcp",  "nop",  "CMP",  "DEC",  "dcp",    // 0xD?
  "CLD",  "CMP",  "nop",  "dcp",  "nop",  "CMP",  "DEC",  "dcp",

  "CPX",  "SBC",  "nop",  "isb",  "CPX",  "SBC",  "INC",  "isb",    // 0xE?
  "INX",  "SBC",  "NOP",  "sbc",  "CPX",  "SBC",  "INC",  "isb",

  "BEQ",  "SBC",  "n/a",  "isb",  "nop",  "SBC",  "INC",  "isb",    // 0xF?
  "SED",  "SBC",  "nop",  "isb",  "nop",  "SBC",  "INC",  "isb"
};
