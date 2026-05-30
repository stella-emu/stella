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

#include "bspf.hxx"
#include "Base.hxx"
#include "Cart.hxx"
#include "CartDebug.hxx"
#include "Console.hxx"
#include "Debugger.hxx"
#include "DebuggerParser.hxx"
#include "Device.hxx"
#include "DiStella.hxx"
#include "FSNode.hxx"
#include "OSystem.hxx"
#include "System.hxx"
#include "Version.hxx"
#include "CartDisassemblyWriter.hxx"

using Common::Base;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartDisassemblyWriter::CartDisassemblyWriter(CartDebug& cartDebug)
  : myCartDebug{cartDebug}
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string CartDisassemblyWriter::save(string path)
{
  // We can't print the header to the disassembly until it's actually
  // been processed; therefore buffer output to a string first
  string buf;

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
  settings.bFlag = DiStella::settings.bFlag; // process break routine (TODO)

  CartDebug::Disassembly disasm;
  disasm.list.reserve(2048);
  Cartridge& cart = myCartDebug.myConsole.cartridge();
  const uInt16 romBankCount = cart.romBankCount();
  const uInt16 oldBank = cart.getBank();
  const bool multiBank = romBankCount > 1;

  // prepare for switching banks
  uInt32 origin = 0;

  // Use cart.bankOrigin() to get each bank's true RORG base regardless of
  // whether that bank has been visited during emulation (info.offset is only
  // reliable for visited banks; unvisited banks leave it as 0).
  vector<uInt16> bankOrigins(romBankCount);
  for(int b = 0; b < romBankCount; ++b)
  {
    cart.unlockHotspots();
    cart.bank(b);
    cart.lockHotspots();
    bankOrigins[b] = cart.bankOrigin(b);
  }

  // Detect whether any two banks have overlapping RORG ranges.  When they do,
  // plain RORG-relative labels (e.g. LF103) would collide across banks, so we
  // encode the bank index into the upper 16 bits of orgBase and use a 6-digit
  // label format (e.g. L00F103 / L01F103) to guarantee uniqueness.
  bool needsExtendedLabels = false;
  for(int b1 = 0; b1 < romBankCount && !needsExtendedLabels; ++b1)
  {
    const size_t size1 = myCartDebug.myBankInfo[b1].size;
    for(int b2 = b1 + 1; b2 < romBankCount; ++b2)
    {
      const size_t size2 = myCartDebug.myBankInfo[b2].size;
      if(bankOrigins[b1] < bankOrigins[b2] + size2 &&
         bankOrigins[b2] < bankOrigins[b1] + size1)
      {
        needsExtendedLabels = true;
        break;
      }
    }
  }

  for(int bank = 0; std::cmp_less(bank, romBankCount); ++bank)
  {
    cart.unlockHotspots();
    cart.bank(bank);
    cart.lockHotspots();

    CartDebug::BankInfo& info = myCartDebug.myBankInfo[bank];

    // Seed the address list for banks not yet visited during emulation so
    // DiStella has a valid entry point.  The reset vector in the currently-
    // mapped bank is the best static approximation we have.
    if(info.addressList.empty())
    {
      const uInt16 seed = myCartDebug.myDebugger.dpeek(0xFFFC, Device::DATA);
      info.addressList.push_back(seed >= 0x1000 ? seed : info.offset);
    }

    myCartDebug.disassembleBank(bank);

    if(multiBank)
    {
      settings.useOrgLabels = true;
      settings.labelDigits = needsExtendedLabels
          ? (romBankCount <= 16 ? 5 : 6)
          : 4;
      settings.orgBase = needsExtendedLabels
          ? bankOrigins[bank] + static_cast<uInt32>(bank) * 0x10000
          : bankOrigins[bank];
    }

    buf += std::format("\n\n;***********************************************************\n;      Bank {}", bank);
    if(multiBank)
      buf += std::format(" / 0..{}", romBankCount - 1);
    buf += "\n;***********************************************************\n\n";

    // Disassemble bank with save-specific settings
    disasm.list.clear();
    const DiStella distella(myCartDebug, disasm.list, info, settings,
                            myCartDebug.myDisLabels, myCartDebug.myDisDirectives,
                            myCartDebug.myReserved);

    if(myCartDebug.myReserved.breakFound)
      myCartDebug.addLabel("Break", myCartDebug.myDebugger.dpeek(0xfffe));

    const uInt32 ramSize = cart.internalRamSize();

    if(ramSize > 0)
    {
      buf += "    SEG.U   RAM\n";
      if(!multiBank)
        buf += std::format("    ORG     ${}\n\n", Base::hex4(info.offset));
      else
      {
        buf += std::format("    ORG     ${}\n", Base::hex4(origin));
        buf += std::format("    RORG    ${}\n\n", Base::hex4(info.offset));
      }
      buf += std::format("    ds.b    {:<8}; write port (${}-${})\n",
                         ramSize, Base::hex4(info.offset),
                         Base::hex4(info.offset + ramSize - 1));
      buf += std::format("    ds.b    {:<8}; read port  (${}-${})\n\n",
                         ramSize, Base::hex4(info.offset + ramSize),
                         Base::hex4(info.offset + 2 * ramSize - 1));
    }

    buf += "    SEG     CODE\n";

    if(!multiBank)
      buf += std::format("    ORG     ${}\n\n", Base::hex4(info.offset + 2 * ramSize));
    else
    {
      buf += std::format("    ORG     ${}\n", Base::hex4(origin));
      buf += std::format("    RORG    ${}\n\n", Base::hex4(info.offset));
    }
    origin += static_cast<uInt32>(info.size);

    // Format in 'distella' style
    for(const auto& tag: disasm.list)
    {
      // Skip the RAM area: it holds runtime-modified values, not the original
      // assembled content. DASM zero-fills the gap, keeping both 128-byte
      // halves identical so isProbablySC still detects the correct type.
      if(ramSize > 0 && tag.address >= info.offset &&
         tag.address < info.offset + 2 * ramSize)
        continue;

      // Add label (if any)
      if(!tag.label.empty())
        buf += std::format("{:<4}\n", tag.label);
      buf += "    ";

      switch(tag.type)
      {
        case Device::CODE:
          buf += std::format("{:<32}{}{}{}", tag.disasm,
            tag.ccount.substr(0, 5), tag.ctotal, tag.ccount.substr(5, 2));
          if(tag.disasm.find("WSYNC") != std::string::npos)
            buf += "\n;---------------------------------------";
          break;

        case Device::ROW:
          buf += std::format(".byte   {:<32}; ${} (*)", tag.disasm.substr(6, 8*4-1),
                             Base::hex4(tag.address));
          break;

        case Device::GFX:
          buf += ".byte   ";
          buf += (settings.gfxFormat == Base::Fmt::_2 ? "%" : "$");
          buf += tag.bytes;
          buf += " ; |";
          for(int c = 12; c < 20; ++c)
            buf += (tag.disasm[c] == '\x1e') ? '#' : ' ';
          buf += std::format("{:<13}${} (G)", "|", Base::hex4(tag.address));
          break;

        case Device::PGFX:
          buf += ".byte   ";
          buf += (settings.gfxFormat == Base::Fmt::_2 ? "%" : "$");
          buf += tag.bytes;
          buf += " ; |";
          for(int c = 12; c < 20; ++c)
            buf += (tag.disasm[c] == '\x1f') ? '*' : ' ';
          buf += std::format("{:<13}${} (P)", "|", Base::hex4(tag.address));
          break;

        case Device::COL:
          buf += std::format(".byte   {:<32}; ${} (C)", tag.disasm.substr(6, 15),
                             Base::hex4(tag.address));
          break;

        case Device::PCOL:
          buf += std::format(".byte   {:<32}; ${} (CP)", tag.disasm.substr(6, 15),
                             Base::hex4(tag.address));
          break;

        case Device::BCOL:
          buf += std::format(".byte   {:<32}; ${} (CB)", tag.disasm.substr(6, 15),
                             Base::hex4(tag.address));
          break;

        case Device::AUD:
          buf += std::format(".byte   {:<32}; ${} (A)", tag.disasm.substr(6, 8 * 4 - 1),
                             Base::hex4(tag.address));
          break;

        case Device::DATA:
          buf += std::format(".byte   {:<32}; ${} (D)", tag.disasm.substr(6, 8 * 4 - 1),
                             Base::hex4(tag.address));
          break;

        case Device::NONE:
        default:
          break;
      }
      buf += '\n';
    }
  }
  cart.unlockHotspots();
  cart.bank(oldBank);
  cart.lockHotspots();

  // Some boilerplate, similar to what DiStella adds
  const auto timeinfo = BSPF::localTime();
  std::ostringstream out;
  out << "; Disassembly of " << myCartDebug.myOSystem.romFile().getShortPath() << "\n"
      << "; Disassembled " << std::put_time(&timeinfo, "%c\n")
      << "; Using Stella " << STELLA_VERSION << "\n;\n"
      << "; ROM properties name : "
      << myCartDebug.myConsole.properties().get(PropType::Cart_Name) << "\n"
      << "; ROM properties MD5  : "
      << myCartDebug.myConsole.properties().get(PropType::Cart_MD5) << "\n"
      << "; Bankswitch type     : " << cart.about() << "\n;\n"
      << "; Legend: *  = CODE not yet run (tentative code)\n"
      << ";         ~  = self-modifying code (address has been both executed and written)\n"
      << ";         D  = DATA directive (referenced in some way)\n"
      << ";         G  = GFX directive, shown as '#' (stored in player, missile, ball)\n"
      << ";         P  = PGFX directive, shown as '*' (stored in playfield)\n"
      << ";         C  = COL directive, shown as color constants (stored in player color)\n"
      << ";         CP = PCOL directive, shown as color constants (stored in playfield color)\n"
      << ";         CB = BCOL directive, shown as color constants (stored in background color)\n"
      << ";         A  = AUD directive (stored in audio registers)\n"
      << ";         i  = indexed accessed only\n"
      << ";         c  = used by code executed in RAM\n"
      << ";         s  = used by stack\n"
      << ";         !  = page crossed, 1 cycle penalty\n"
      << "\n    processor 6502\n\n";

  // Bankswitch equates for multi-bank ROMs
  if(multiBank)
  {
    const uInt16 hs = cart.hotspot();
    if(hs >= 0x1000)
    {
      out << "\n;-----------------------------------------------------------\n"
          << ";      Bankswitch equates\n"
          << ";-----------------------------------------------------------\n\n";
      for(uInt16 b = 0; b < romBankCount; ++b)
        out << std::format("{:<16}= ${}\n", "BANK" + std::to_string(b),
                           Base::hex4(hs + b));
      out << "\n";
    }
  }

  out << "\n;-----------------------------------------------------------\n"
      << ";      Color constants\n"
      << ";-----------------------------------------------------------\n\n";

  if(myCartDebug.myConsole.timing() == ConsoleTiming::ntsc)
  {
    static constexpr std::array<string_view, 16> NTSC_COLOR = {
      "BLACK", "YELLOW", "BROWN", "ORANGE",
      "RED", "MAUVE", "VIOLET", "PURPLE",
      "BLUE", "BLUE_CYAN", "CYAN", "CYAN_GREEN",
      "GREEN", "GREEN_YELLOW", "GREEN_BEIGE", "BEIGE"
    };

    for(int i = 0; i < 16; ++i)
      out << std::format("{:<16} = ${}\n", NTSC_COLOR[i], Base::hex2(i << 4));
  }
  else if(myCartDebug.myConsole.timing() == ConsoleTiming::pal)
  {
    static constexpr std::array<string_view, 16> PAL_COLOR = {
      "BLACK0", "BLACK1", "YELLOW", "GREEN_YELLOW",
      "ORANGE", "GREEN", "RED", "CYAN_GREEN",
      "MAUVE", "CYAN", "VIOLET", "BLUE_CYAN",
      "PURPLE", "BLUE", "BLACKE", "BLACKF"
    };

    for(int i = 0; i < 16; ++i)
      out << std::format("{:<16} = ${}\n", PAL_COLOR[i], Base::hex2(i << 4));
  }
  else
  {
    static constexpr std::array<string_view, 8> SECAM_COLOR = {
      "BLACK", "BLUE", "RED", "PURPLE",
      "GREEN", "CYAN", "YELLOW", "WHITE"
    };

    for(int i = 0; i < 8; ++i)
      out << std::format("{:<16} = ${}\n", SECAM_COLOR[i], Base::hex1(i << 1));
  }
  out << "\n";

  bool addrUsed = false;
  for(uInt16 addr = 0x00; addr <= 0x0F; ++addr)
    addrUsed = addrUsed || myCartDebug.myReserved.TIARead[addr]
      || (myCartDebug.mySystem.getAccessFlags(addr) & Device::WRITE);
  for(uInt16 addr = 0x00; addr <= 0x3F; ++addr)
    addrUsed = addrUsed || myCartDebug.myReserved.TIAWrite[addr]
      || (myCartDebug.mySystem.getAccessFlags(addr) & Device::DATA);
  for(uInt16 addr = 0x00; addr <= 0x17; ++addr)
    addrUsed = addrUsed || myCartDebug.myReserved.IOReadWrite[addr];

  if(addrUsed)
  {
    out << "\n;-----------------------------------------------------------\n"
        << ";      TIA and IO constants accessed\n"
        << ";-----------------------------------------------------------\n\n";

    // TIA read access
    for(uInt16 addr = 0x00; addr <= 0x0F; ++addr)
      if(myCartDebug.myReserved.TIARead[addr])
        out << std::format("{:<16}= ${}  ; (R)\n",
                           CartDebug::ourTIAMnemonicR[addr], Base::hex2(addr));
      else if (myCartDebug.mySystem.getAccessFlags(addr) & Device::DATA)
        out << std::format(";{:<15}= ${}  ; (Ri)\n",
                           CartDebug::ourTIAMnemonicR[addr], Base::hex2(addr));
    out << "\n";

    // TIA write access
    for(uInt16 addr = 0x00; addr <= 0x3F; ++addr)
      if(myCartDebug.myReserved.TIAWrite[addr])
        out << std::format("{:<16}= ${}  ; (W)\n",
                           CartDebug::ourTIAMnemonicW[addr], Base::hex2(addr));
      else if (myCartDebug.mySystem.getAccessFlags(addr) & Device::WRITE)
        out << std::format(";{:<15}= ${}  ; (Wi)\n",
                           CartDebug::ourTIAMnemonicW[addr], Base::hex2(addr));
    out << "\n";

    // RIOT IO access
    for(uInt16 addr = 0x00; addr <= 0x1F; ++addr)
      if(myCartDebug.myReserved.IOReadWrite[addr])
        out << std::format("{:<16}= ${}\n",
                           CartDebug::ourIOMnemonic[addr], Base::hex4(addr + 0x280));
  }

  addrUsed = false;
  for(uInt16 addr = 0x80; addr <= 0xFF; ++addr)
    addrUsed = addrUsed || myCartDebug.myReserved.ZPRAM[addr-0x80]
      || (myCartDebug.mySystem.getAccessFlags(addr) & (Device::DATA | Device::WRITE))
      || (myCartDebug.mySystem.getAccessFlags(addr|0x100) &
         (Device::DATA | Device::WRITE));
  if(addrUsed)
  {
    bool addLine = false;
    out << "\n\n;-----------------------------------------------------------\n"
        << ";      RIOT RAM (zero-page) labels\n"
        << ";-----------------------------------------------------------\n\n";

    for(uInt16 addr = 0x80; addr <= 0xFF; ++addr)
    {
      const bool ramUsed = (myCartDebug.mySystem.getAccessFlags(addr) &
                           (Device::DATA | Device::WRITE));
      const bool codeUsed = (myCartDebug.mySystem.getAccessFlags(addr) & Device::CODE);
      const bool stackUsed = (myCartDebug.mySystem.getAccessFlags(addr|0x100) &
                             (Device::DATA | Device::WRITE));

      if(myCartDebug.myReserved.ZPRAM[addr - 0x80] &&
         !myCartDebug.myUserLabels.contains(addr))
      {
        if(addLine)
          out << "\n";
        out << std::format("{:<16}= ${}{}\n", CartDebug::ourZPMnemonic[addr - 0x80],
          Base::hex2(addr),
          (stackUsed || codeUsed)
            ? std::format("; ({}{})", codeUsed ? "c" : "", stackUsed ? "s" : "")
            : std::string{});
        addLine = false;
      }
      else if(ramUsed || codeUsed || stackUsed)
      {
        if(addLine)
          out << "\n";
        out << std::format("{:<18}${}  ({}{}{})\n", ";", Base::hex2(addr),
          ramUsed ? "i" : "", codeUsed ? "c" : "", stackUsed ? "s" : "");
        addLine = false;
      }
      else
        addLine = true;
    }
  }

  if(!myCartDebug.myReserved.Label.empty())
  {
    out << "\n\n;-----------------------------------------------------------\n"
        << ";      Non Locatable Labels\n"
        << ";-----------------------------------------------------------\n\n";
    for(const auto& [label, addr]: myCartDebug.myReserved.Label)
      out << std::format("{:<16}= ${}\n", label, Base::hex4(addr));
  }

  if(!myCartDebug.myUserLabels.empty())
  {
    out << "\n\n;-----------------------------------------------------------\n"
        << ";      User Defined Labels\n"
        << ";-----------------------------------------------------------\n\n";
    int max_len = 16;
    for(const auto& [addr, label]: myCartDebug.myUserLabels)
      max_len = std::max(max_len, static_cast<int>(label.size()));
    for(const auto& [addr, label]: myCartDebug.myUserLabels)
      out << std::format("{:<{}}= ${}\n", label, max_len, Base::hex4(addr));
  }

  // And finally, output the disassembly
  out << buf;

  if(path.empty())
    path = std::format("{}{}.asm", myCartDebug.myOSystem.userDir().getPath(),
                       myCartDebug.myConsole.properties().get(PropType::Cart_Name));
  else
    // Append default extension when missing
    if(path.find_last_of('.') == string::npos)
      path += ".asm";

  const FSNode node(path);
  try
  {
    node.write(out.view());
    if(multiBank && !cart.supportsSaveDisassembly())
      return DebuggerParser::red(
        std::format("saved {} (multi-bank type not fully supported)",
                    node.getShortPath()));
    return std::format("saved {} OK", node.getShortPath());
  }
  catch(...)
  {
    return std::format("Unable to save disassembly to {}", node.getShortPath());
  }
}
