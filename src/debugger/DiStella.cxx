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
#include "Debugger.hxx"
#include "Device.hxx"
#include "DiStella.hxx"
#include "TIAConstants.hxx"
using Common::Base;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DiStella::DiStella(const CartDebug& dbg, CartDebug::DisassemblyList& list,
                   CartDebug::BankInfo& info, const DiStella::Settings& s,
                   CartDebug::AddrTypeArray& labels,
                   CartDebug::AddrTypeArray& directives,
                   CartDebug::ReservedEquates& reserved)
  : myDbg{dbg},
    myList{list},
    mySettings{s},
    myReserved{reserved},
    myOffset{info.offset},
    myLabels{labels},
    myDirectives{directives}
{
  bool resolveCode = mySettings.resolveCode;
  const CartDebug::AddressList& debuggerAddresses = info.addressList;
  const uInt16 start = *debuggerAddresses.cbegin();

  if (start & 0x1000) {
    info.start = myAppData.start = 0x0000;
    info.end = myAppData.end = static_cast<uInt16>(info.size - 1);
    // Keep previous offset; it may be different between banks
    if (info.offset == 0)
      info.offset = myOffset = (start - (start % info.size));
  } else { // ZP RAM
    // For now, we assume all accesses below $1000 are zero-page
    info.start = myAppData.start = 0x0080;
    info.end = myAppData.end = 0x00FF;
    info.offset = myOffset = 0;

    // Resolve code is never used in ZP RAM mode
    resolveCode = false;
  }
  myAppData.length = static_cast<uInt16>(info.size);

  myLabels.fill(0);
  myDirectives.fill(0);

  // Process any directives first, as they override automatic code determination
  processDirectives(info.directiveList);

  myReserved.breakFound = false;

  if (resolveCode) {
    // First pass
    disasmPass1(info.addressList);
  } else if (myOffset == 0) {
    // ZP RAM: no static recursive analysis; seed myLabels directly from
    // runtime access flags so pass 2 can classify bytes correctly.
    // Bytes actually executed get CODE; unaccessed bytes get ROW to prevent
    // them from falling into the default CODE branch spuriously.
    constexpr uInt16 dataFlags = Device::DATA | Device::GFX | Device::PGFX |
                                 Device::COL | Device::PCOL | Device::BCOL | Device::AUD;
    for (int k = myAppData.start; std::cmp_less_equal(k, myAppData.end); ++k) {
      const auto addr = static_cast<uInt16>(k);
      const auto flags = Debugger::debugger().getAccessFlags(addr);
      if (flags & Device::CODE)
        mark(addr, Device::CODE);
      else if (!(flags & dataFlags))
        mark(addr, Device::ROW);
    }
  }

  // Second pass
  disasm(myOffset, DisasmPass::MarkValid);

  // Add reserved line equates
  for(int k = 0; std::cmp_less_equal(k, myAppData.end); k++) {
    if((myLabels[k] & (Device::REFERENCED | Device::VALID_ENTRY)) == Device::REFERENCED) {
      // If we have a piece of code referenced somewhere else, but cannot
      // locate the label in code (i.e because the address is inside of a
      // multi-byte instruction, then we make note of that address for reference
      //
      // However, we only do this for labels pointing to ROM (above $1000)
      if(CartDebug::addressType(k + myOffset) == CartDebug::AddrType::ROM) {
        const uInt32 labelAddr = mySettings.useOrgLabels
            ? static_cast<uInt32>(k) + mySettings.orgBase
            : static_cast<uInt32>(k + myOffset);
        myReserved.Label.emplace(
          'L' + Base::hexN(static_cast<int>(labelAddr), mySettings.labelDigits),
          static_cast<uInt16>(k + myOffset));
      }
    }
  }

  // Third pass
  disasm(myOffset, DisasmPass::Output);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DiStella::disasm(uInt32 distart, DisasmPass pass)
// pass 1 (disasmPass1): detect code/data ranges and labels
// pass 2 (MarkValid):   mark valid entries
// pass 3 (Output):      generate disassembly output
{
  uInt8 opcode = 0, d1 = 0;
  uInt16 ad = 0;
  Int32 cycles = 0;
  AddressingMode addrMode{};
  AddressType labelFound = AddressType::INVALID;
  std::ostringstream nextLine, nextLineBytes;

  mySegType = Device::NONE; // create extra lines between code and data

  myLine = {};

  myPC = distart - myOffset;
  while(myPC <= myAppData.end)
  {
    // since -1 is used in m6502.m4 for clearing the last peek
    // and this results into an access at e.g. 0xffff,
    // we have to fix the consequences here (ugly!).
    // The end-of-data byte is always treated as ROW regardless of its type flags.
    if(myPC == myAppData.end ||
       checkBits(myPC, Device::ROW,
                 Device::CODE | Device::GFX | Device::PGFX |
                 Device::COL | Device::PCOL | Device::BCOL |
                 Device::AUD | Device::DATA))
    {
      if(pass == DisasmPass::MarkValid)
        mark(myPC + myOffset, Device::VALID_ENTRY);

      if(pass == DisasmPass::Output)
        outputBytes(Device::ROW);
      else
        ++myPC;
    }
    else if(checkBits(myPC, Device::GFX | Device::PGFX,
            Device::CODE))
    {
      if(pass == DisasmPass::MarkValid)
        mark(myPC + myOffset, Device::VALID_ENTRY);
      if(pass == DisasmPass::Output)
        outputGraphics();
      ++myPC;
    }
    else if(checkBits(myPC, Device::COL | Device::PCOL | Device::BCOL,
            Device::CODE | Device::GFX | Device::PGFX))
    {
      if(pass == DisasmPass::MarkValid)
        mark(myPC + myOffset, Device::VALID_ENTRY);
      if(pass == DisasmPass::Output)
        outputColors();
      ++myPC;
    }
    else if(checkBits(myPC, Device::AUD,
            Device::CODE | Device::GFX | Device::PGFX |
            Device::COL | Device::PCOL | Device::BCOL))
    {
      if(pass == DisasmPass::MarkValid)
        mark(myPC + myOffset, Device::VALID_ENTRY);
      if(pass == DisasmPass::Output)
        outputBytes(Device::AUD);
      else
        ++myPC;
    }
    else if(checkBits(myPC, Device::DATA,
            Device::CODE | Device::GFX | Device::PGFX |
            Device::COL | Device::PCOL | Device::BCOL |
            Device::AUD))
    {
      if(pass == DisasmPass::MarkValid)
        mark(myPC + myOffset, Device::VALID_ENTRY);
      if(pass == DisasmPass::Output)
        outputBytes(Device::DATA);
      else
        ++myPC;
    }
    else {
   // The following sections must be CODE

   // add extra spacing line when switching from non-code to code
      if(pass == DisasmPass::Output && mySegType != Device::CODE && mySegType != Device::NONE) {
        myLine = {};
        addEntry(Device::NONE);
        mark(myPC + myOffset, Device::REFERENCED); // add label when switching
      }
      mySegType = Device::CODE;

      if(pass == DisasmPass::MarkValid)
        mark(myPC + myOffset, Device::VALID_ENTRY);

      // get opcode
      opcode = Debugger::debugger().peek(myPC + myOffset);
      // get address mode for opcode
      addrMode = ourLookup[opcode].addr_mode;

      if(pass == DisasmPass::Output) {
        myLine.address      = myPC + myOffset;
        myLine.hasAutoLabel = checkBit(myPC, Device::REFERENCED);
      }
      ++myPC;

      // detect labels inside instructions (e.g. BIT masks)
      labelFound = AddressType::INVALID;
      for(uInt8 i = 0; i < ourLookup[opcode].bytes - 1; i++) {
        if(checkBit(myPC + i, Device::REFERENCED)) {
          labelFound = AddressType::ROM;
          break;
        }
      }
      if(labelFound != AddressType::INVALID) {
        if(myOffset >= 0x1000) {
          // the opcode's operand address matches a label address
          if(pass == DisasmPass::Output) {
            // output the byte of the opcode incl. cycles
            const uInt8 nextOpcode = Debugger::debugger().peek(myPC + myOffset);

            cycles += static_cast<int>(ourLookup[opcode].cycles) -
                      static_cast<int>(ourLookup[nextOpcode].cycles);
            nextLine << ".byte   $" << Base::HEX2 << static_cast<int>(opcode) << " ;";
            nextLine << ourLookup[opcode].mnemonic;

            myLine.disasm = nextLine.str();
            myLine.ccount = std::format(";{}-{} ",
              static_cast<int>(ourLookup[opcode].cycles),
              static_cast<int>(ourLookup[nextOpcode].cycles));
            myLine.ctotal = std::format("= {:3}", cycles);

            nextLine.str("");
            cycles = 0;
            addEntry(Device::CODE); // add the new found CODE entry
          }
          // continue with the label's opcode
          continue;
        }
      }

      // Undefined opcodes start with a '.'
      // These are undefined wrt DASM
      if(ourLookup[opcode].mnemonic[0] == '.' && pass == DisasmPass::Output) {
        nextLine << ".byte   $" << Base::HEX2 << static_cast<int>(opcode) << " ;";
      }

      if(pass == DisasmPass::Output) {
        nextLine << ourLookup[opcode].mnemonic;
        nextLineBytes << Base::HEX2 << static_cast<int>(opcode) << " ";
        myLine.mnemonicColor = mnemonicColorForOpcode(opcode);
      }

      // Add operand(s) for PC values outside the app data range
      if(myPC >= myAppData.end) {
        switch(addrMode) {
          case AddressingMode::ABSOLUTE:
          case AddressingMode::ABSOLUTE_X:
          case AddressingMode::ABSOLUTE_Y:
          case AddressingMode::INDIRECT_X:
          case AddressingMode::INDIRECT_Y:
          case AddressingMode::ABS_INDIRECT:
          {
            if(pass == DisasmPass::Output) {
              /* Line information is already printed; append .byte since last
                 instruction will put recompilable object larger that original
                 binary file */
              {
                std::ostringstream s;
                s << ".byte $" << Base::HEX2 << static_cast<int>(opcode)
                  << " ;" << ourLookup[opcode].mnemonic;
                myLine.disasm = s.str();
              }
              addEntry(Device::DATA);

              if(myPC == myAppData.end) {
                myLine.address      = myPC + myOffset;
                myLine.hasAutoLabel = checkBit(myPC, Device::REFERENCED);

                opcode = Debugger::debugger().peek(myPC + myOffset);  ++myPC;
                std::ostringstream s;
                s << ".byte $" << Base::HEX2 << static_cast<int>(opcode);
                myLine.disasm = s.str();
                addEntry(Device::DATA);
              }
            }
            myPCEnd = myAppData.end + myOffset;
            return;
          }

          case AddressingMode::ZERO_PAGE:
          case AddressingMode::IMMEDIATE:
          case AddressingMode::ZERO_PAGE_X:
          case AddressingMode::ZERO_PAGE_Y:
          case AddressingMode::RELATIVE:
          {
            if(pass == DisasmPass::Output) {
              /* Line information is already printed, but we can remove the
                  Instruction (i.e. BMI) by simply clearing the buffer to print */
              std::ostringstream s;
              s << ".byte $" << Base::HEX2 << static_cast<int>(opcode);
              myLine.disasm = s.str();
              addEntry(Device::ROW);
              nextLine.str("");
              nextLineBytes.str("");
            }
            ++myPC;
            myPCEnd = myAppData.end + myOffset;
            return;
          }

          default:
            break;
        }  // end switch(addr_mode)
      }

      // Add operand(s)
      ad = 0;
      switch(addrMode) {
        case AddressingMode::ACCUMULATOR:
        {
          if(pass == DisasmPass::Output && mySettings.aFlag)
            nextLine << "     A";
          break;
        }

        case AddressingMode::ABSOLUTE:
        {
          ad = Debugger::debugger().dpeek(myPC + myOffset);  myPC += 2;
          labelFound = mark(ad, Device::REFERENCED);
          if(pass == DisasmPass::Output) {
            if(ad < 0x100 && mySettings.fFlag)
              nextLine << ".w   ";
            else
              nextLine << "     ";

            if(labelFound == AddressType::ROM) {
              labelA12High(nextLine, ad);
              nextLineBytes << Base::HEX2 << static_cast<int>(ad & 0xff) << " "
                            << Base::HEX2 << static_cast<int>(ad >> 8);
              myLine.operandColor = colorA12High(ad);
            }
            else if(labelFound == AddressType::ROM_MIRROR) {
              if(mySettings.rFlag) {
                const int tmp = (ad & myAppData.end) + myOffset;
                labelA12High(nextLine, tmp);
                nextLineBytes << Base::HEX2 << static_cast<int>(tmp & 0xff) << " "
                              << Base::HEX2 << static_cast<int>(tmp >> 8);
                myLine.operandColor = colorA12High(static_cast<uInt16>(tmp));
              }
              else {
                nextLine << "$" << Base::HEX4 << ad;
                nextLineBytes << Base::HEX2 << static_cast<int>(ad & 0xff) << " "
                              << Base::HEX2 << static_cast<int>(ad >> 8);
                myLine.operandColor = CartDebug::DisasmSegColor::ROM;
              }
            }
            else {
              labelA12Low(nextLine, opcode, ad, labelFound);
              nextLineBytes << Base::HEX2 << static_cast<int>(ad & 0xff) << " "
                            << Base::HEX2 << static_cast<int>(ad >> 8);
              myLine.operandColor = colorA12Low(ad, labelFound,
                ourLookup[opcode].rw_mode == RWMode::READ);
            }
          }
          break;
        }

        case AddressingMode::ZERO_PAGE:
        {
          d1 = Debugger::debugger().peek(myPC + myOffset);  ++myPC;
          labelFound = mark(d1, Device::REFERENCED);
          if(pass == DisasmPass::Output) {
            nextLine << "     ";
            labelA12Low(nextLine, opcode, d1, labelFound);
            nextLineBytes << Base::HEX2 << static_cast<int>(d1);
            myLine.operandColor = colorA12Low(d1, labelFound,
              ourLookup[opcode].rw_mode == RWMode::READ);
          }
          break;
        }

        case AddressingMode::IMMEDIATE:
        {
          d1 = Debugger::debugger().peek(myPC + myOffset);
          if(pass == DisasmPass::Output) {
            if (checkBits(myPC, Device::COL | Device::PCOL | Device::BCOL,
                Device::GFX | Device::PGFX))  // CODE does not block color display
              nextLine << "     #" << getColor(d1);
            else
              nextLine << "     #$" << Base::HEX2 << static_cast<int>(d1) << " ";
            nextLineBytes << Base::HEX2 << static_cast<int>(d1);
            myLine.operandColor = CartDebug::DisasmSegColor::Immediate;
          }
          ++myPC;
          break;
        }

        case AddressingMode::ABSOLUTE_X:
        {
          ad = Debugger::debugger().dpeek(myPC + myOffset);  myPC += 2;
          labelFound = mark(ad, Device::REFERENCED);
          if(pass == DisasmPass::MarkValid && !checkBit(ad & myAppData.end, Device::CODE)) {
            // Since we can't know what address is being accessed unless we also
            // know the current X value, this is marked as ROW instead of DATA
            // The processing is left here, however, in case future versions of
            // the code can somehow track access to CPU registers
            mark(ad, Device::ROW);
          }
          else if(pass == DisasmPass::Output) {
            if(ad < 0x100 && mySettings.fFlag)
              nextLine << ".wx  ";
            else
              nextLine << "     ";

            if(labelFound == AddressType::ROM) {
              labelA12High(nextLine, ad);
              nextLine << ",x";
              nextLineBytes << Base::HEX2 << static_cast<int>(ad & 0xff) << " "
                            << Base::HEX2 << static_cast<int>(ad >> 8);
              myLine.operandColor = colorA12High(ad);
            }
            else if(labelFound == AddressType::ROM_MIRROR) {
              if(mySettings.rFlag) {
                const int tmp = (ad & myAppData.end) + myOffset;
                labelA12High(nextLine, tmp);
                nextLine << ",x";
                nextLineBytes << Base::HEX2 << static_cast<int>(tmp & 0xff) << " "
                              << Base::HEX2 << static_cast<int>(tmp >> 8);
                myLine.operandColor = colorA12High(static_cast<uInt16>(tmp));
              }
              else {
                nextLine << "$" << Base::HEX4 << ad << ",x";
                nextLineBytes << Base::HEX2 << static_cast<int>(ad & 0xff) << " "
                              << Base::HEX2 << static_cast<int>(ad >> 8);
                myLine.operandColor = CartDebug::DisasmSegColor::ROM;
              }
            }
            else {
              labelA12Low(nextLine, opcode, ad, labelFound);
              nextLine << ",x";
              nextLineBytes << Base::HEX2 << static_cast<int>(ad & 0xff) << " "
                            << Base::HEX2 << static_cast<int>(ad >> 8);
              myLine.operandColor = colorA12Low(ad, labelFound,
                ourLookup[opcode].rw_mode == RWMode::READ);
            }
          }
          break;
        }

        case AddressingMode::ABSOLUTE_Y:
        {
          ad = Debugger::debugger().dpeek(myPC + myOffset);  myPC += 2;
          labelFound = mark(ad, Device::REFERENCED);
          if(pass == DisasmPass::MarkValid && !checkBit(ad & myAppData.end, Device::CODE)) {
            // Since we can't know what address is being accessed unless we also
            // know the current Y value, this is marked as ROW instead of DATA
            // The processing is left here, however, in case future versions of
            // the code can somehow track access to CPU registers
            mark(ad, Device::ROW);
          }
          else if(pass == DisasmPass::Output) {
            if(ad < 0x100 && mySettings.fFlag)
              nextLine << ".wy  ";
            else
              nextLine << "     ";

            if(labelFound == AddressType::ROM) {
              labelA12High(nextLine, ad);
              nextLine << ",y";
              nextLineBytes << Base::HEX2 << static_cast<int>(ad & 0xff) << " "
                            << Base::HEX2 << static_cast<int>(ad >> 8);
              myLine.operandColor = colorA12High(ad);
            }
            else if(labelFound == AddressType::ROM_MIRROR) {
              if(mySettings.rFlag) {
                const int tmp = (ad & myAppData.end) + myOffset;
                labelA12High(nextLine, tmp);
                nextLine << ",y";
                nextLineBytes << Base::HEX2 << static_cast<int>(tmp & 0xff) << " "
                              << Base::HEX2 << static_cast<int>(tmp >> 8);
                myLine.operandColor = colorA12High(static_cast<uInt16>(tmp));
              }
              else {
                nextLine << "$" << Base::HEX4 << ad << ",y";
                nextLineBytes << Base::HEX2 << static_cast<int>(ad & 0xff) << " "
                              << Base::HEX2 << static_cast<int>(ad >> 8);
                myLine.operandColor = CartDebug::DisasmSegColor::ROM;
              }
            }
            else {
              labelA12Low(nextLine, opcode, ad, labelFound);
              nextLine << ",y";
              nextLineBytes << Base::HEX2 << static_cast<int>(ad & 0xff) << " "
                            << Base::HEX2 << static_cast<int>(ad >> 8);
              myLine.operandColor = colorA12Low(ad, labelFound,
                ourLookup[opcode].rw_mode == RWMode::READ);
            }
          }
          break;
        }

        case AddressingMode::INDIRECT_X:
        {
          d1 = Debugger::debugger().peek(myPC + myOffset);  ++myPC;
          if(pass == DisasmPass::Output) {
            labelFound = mark(d1, 0);  // dummy call to get address type
            nextLine << "     (";
            labelA12Low(nextLine, opcode, d1, labelFound);
            nextLine << ",x)";
            nextLineBytes << Base::HEX2 << static_cast<int>(d1);
            myLine.operandColor = colorA12Low(d1, labelFound,
              ourLookup[opcode].rw_mode == RWMode::READ);
          }
          break;
        }

        case AddressingMode::INDIRECT_Y:
        {
          d1 = Debugger::debugger().peek(myPC + myOffset);  ++myPC;
          if(pass == DisasmPass::Output) {
            labelFound = mark(d1, 0);  // dummy call to get address type
            nextLine << "     (";
            labelA12Low(nextLine, opcode, d1, labelFound);
            nextLine << "),y";
            nextLineBytes << Base::HEX2 << static_cast<int>(d1);
            myLine.operandColor = colorA12Low(d1, labelFound,
              ourLookup[opcode].rw_mode == RWMode::READ);
          }
          break;
        }

        case AddressingMode::ZERO_PAGE_X:
        {
          d1 = Debugger::debugger().peek(myPC + myOffset);  ++myPC;
          labelFound = mark(d1, Device::REFERENCED);
          if(pass == DisasmPass::Output) {
            nextLine << "     ";
            labelA12Low(nextLine, opcode, d1, labelFound);
            nextLine << ",x";
            nextLineBytes << Base::HEX2 << static_cast<int>(d1);
            myLine.operandColor = colorA12Low(d1, labelFound,
              ourLookup[opcode].rw_mode == RWMode::READ);
          }
          break;
        }

        case AddressingMode::ZERO_PAGE_Y:
        {
          d1 = Debugger::debugger().peek(myPC + myOffset);  ++myPC;
          labelFound = mark(d1, Device::REFERENCED);
          if(pass == DisasmPass::Output) {
            nextLine << "     ";
            labelA12Low(nextLine, opcode, d1, labelFound);
            nextLine << ",y";
            nextLineBytes << Base::HEX2 << static_cast<int>(d1);
            myLine.operandColor = colorA12Low(d1, labelFound,
              ourLookup[opcode].rw_mode == RWMode::READ);
          }
          break;
        }

        case AddressingMode::RELATIVE:
        {
          // SA - 04-06-2010: there seemed to be a bug in distella,
          // where wraparound occurred on a 32-bit int, and subsequent
          // indexing into the labels array caused a crash
          d1 = Debugger::debugger().peek(myPC + myOffset);  ++myPC;
          ad = ((myPC + static_cast<Int8>(d1)) & 0xfff) + myOffset;

          labelFound = mark(ad, Device::REFERENCED);
          if(pass == DisasmPass::Output) {
            if(labelFound == AddressType::ROM) {
              nextLine << "     ";
              labelA12High(nextLine, ad);
              myLine.operandColor = colorA12High(ad);
            }
            else {
              nextLine << "     $" << Base::HEX4 << ad;
              myLine.operandColor = CartDebug::DisasmSegColor::Default;
            }

            nextLineBytes << Base::HEX2 << static_cast<int>(d1);
          }
          break;
        }

        case AddressingMode::ABS_INDIRECT:
        {
          ad = Debugger::debugger().dpeek(myPC + myOffset);  myPC += 2;
          labelFound = mark(ad, Device::REFERENCED);
          if(pass == DisasmPass::MarkValid && !checkBit(ad & myAppData.end, Device::CODE)) {
            // The jump target is not statically knowable; mark as ROW
            mark(ad, Device::ROW);
          }
          else if(pass == DisasmPass::Output) {
            if(ad < 0x100 && mySettings.fFlag)
              nextLine << ".ind ";
            else
              nextLine << "     ";

            if(labelFound == AddressType::ROM) {
              nextLine << "(";
              labelA12High(nextLine, ad);
              nextLine << ")";
              myLine.operandColor = colorA12High(ad);
            }
            else if(labelFound == AddressType::ROM_MIRROR) {
              nextLine << "(";
              if(mySettings.rFlag) {
                const int tmp = (ad & myAppData.end) + myOffset;
                labelA12High(nextLine, tmp);
                myLine.operandColor = colorA12High(static_cast<uInt16>(tmp));
              }
              else {
                labelA12Low(nextLine, opcode, ad, labelFound);
                myLine.operandColor = CartDebug::DisasmSegColor::ROM;
              }
              nextLine << ")";
            }
            else {
              nextLine << "(";
              labelA12Low(nextLine, opcode, ad, labelFound);
              nextLine << ")";
              myLine.operandColor = colorA12Low(ad, labelFound,
                ourLookup[opcode].rw_mode == RWMode::READ);
            }

            nextLineBytes << Base::HEX2 << static_cast<int>(ad & 0xff) << " "
                          << Base::HEX2 << static_cast<int>(ad >> 8);
          }
          break;
        }

        default:
          break;
      } // end switch

      if(pass == DisasmPass::Output) {
        cycles += static_cast<int>(ourLookup[opcode].cycles);
        // A complete line of disassembly (text, cycle count, and bytes)
        myLine.disasm = nextLine.str();
        const string_view branchSuffix =
          (addrMode == AddressingMode::RELATIVE)
            ? ((ad & 0xf00) != ((myPC + myOffset) & 0xf00) ? "/3!" : "/3 ")
            : "   ";
        myLine.ccount = std::format(";{}{}", static_cast<int>(ourLookup[opcode].cycles), branchSuffix);
        if((opcode == OP_RTI || opcode == OP_RTS || opcode == OP_JMP || opcode == OP_BRK // code block end
           || checkBit(myPC, Device::REFERENCED)                              // referenced address
           || (ourLookup[opcode].rw_mode == RWMode::WRITE                        // strobe WSYNC
               && (addrMode == AddressingMode::ZERO_PAGE
                   || addrMode == AddressingMode::ZERO_PAGE_X
                   || addrMode == AddressingMode::ZERO_PAGE_Y)
               && d1 == WSYNC))
           && cycles > 0) {
          myLine.ctotal = std::format("= {:3}", cycles);
          cycles = 0;
        }
        else {
          myLine.ctotal = "     ";
        }
        myLine.bytes = nextLineBytes.str();

        addEntry(Device::CODE);
        if(opcode == OP_RTI || opcode == OP_RTS || opcode == OP_JMP || opcode == OP_BRK) {
          myLine = {};
          addEntry(Device::NONE);
          mySegType = Device::NONE; // prevent extra lines if data follows
        }

        nextLine.str("");
        nextLineBytes.str("");
      }
    } // CODE
  } /* while loop */

  /* Just in case we are disassembling outside of the address range, force the myPCEnd to EOF */
  myPCEnd = myAppData.end + myOffset;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DiStella::disasmPass1(CartDebug::AddressList& debuggerAddresses)
{
  auto it = debuggerAddresses.begin();
  const uInt16 start = *it++;

  // Pre-collect runtime CODE hints and sort descending by peek count so that
  // frequently-executed (high-confidence) addresses seed the disassembler
  // before rarely-executed or speculatively-flagged ones.
  using RuntimeHint = std::pair<Device::AccessCounter, uInt16>;
  std::vector<RuntimeHint> runtimeHints;
  for (int i = 0; std::cmp_less_equal(i, myAppData.end); ++i) {
    const auto addr = static_cast<uInt16>(i + myOffset);
    if (Debugger::debugger().getAccessFlags(addr) & Device::CODE)
      runtimeHints.emplace_back(Debugger::debugger().getAccessCounter(addr), addr);
  }
  std::ranges::sort(runtimeHints, std::ranges::greater{}, &RuntimeHint::first);
  auto runtimeIt = runtimeHints.begin();

  std::unordered_set<uInt16> visited;

  myAddressQueue = {};
  myAddressQueue.push(start);

  while (!myAddressQueue.empty()) {
    const uInt16 pcBeg = myAddressQueue.front();
    myAddressQueue.pop();

    if (!visited.insert(pcBeg).second) continue;

    myPC = pcBeg;
    disasmFromAddress(myPC);

    if (pcBeg <= myPCEnd) {
      // Tentatively mark all addresses in the range as CODE
      // Note that this is a 'best-effort' approach, since
      // Distella will normally keep going until the end of the
      // range or branch is encountered
      // However, addresses *specifically* marked as DATA/GFX/PGFX/COL/PCOL/BCOL/AUD
      // in the emulation core indicate that the CODE range has finished
      // Therefore, we stop at the first such address encountered
      for (uInt32 k = pcBeg; k <= myPCEnd; ++k) {
        if (checkBits(k, Device::DATA | Device::GFX | Device::PGFX |
            Device::COL | Device::PCOL | Device::BCOL | Device::AUD,
            Device::CODE)) {
          myPCEnd = k - 1;
          break;
        }
        mark(k, Device::CODE);
      }
    }

    // When we get to this point, all addresses have been processed
    // starting from the initial one in the address list
    // If so, process the next one in the list that hasn't already
    // been marked as CODE
    // If it *has* been marked, it can be removed from consideration
    // in all subsequent passes
    //
    // Once the address list has been exhausted, we process all addresses
    // determined during emulation to represent code, which *haven't* already
    // been considered
    //
    // Note that we can't simply add all addresses right away, since
    // the processing of a single address can cause others to be added in
    // disasmFromAddress; all of these have to be exhausted before
    // considering a new address
    while (myAddressQueue.empty() && it != debuggerAddresses.end()) {
      const uInt16 addr = *it;

      if (!checkBit(addr - myOffset, Device::CODE)) {
        myAddressQueue.push(addr);
        ++it;
      } else // remove this address, it is redundant
        it = debuggerAddresses.erase(it);
    }

    // Stella itself can provide hints on whether an address has ever
    // been referenced as CODE; process highest-count (most confident) first
    while (myAddressQueue.empty() && runtimeIt != runtimeHints.end()) {
      const uInt16 rAddr = runtimeIt->second;
      ++runtimeIt;
      if (!(myLabels[rAddr & myAppData.end] & Device::CODE)) {
        myAddressQueue.push(rAddr);
        break;
      }
    }
  } // while

  for (int k = 0; std::cmp_less_equal(k, myAppData.end); k++) {
    // Let the emulation core know about tentative code
    if (checkBit(k, Device::CODE) &&
      !(Debugger::debugger().getAccessFlags(k + myOffset) & Device::CODE)
      && myOffset != 0) {
      Debugger::debugger().setAccessFlags(k + myOffset, Device::TCODE);
    }

    // Must be ROW / unused bytes
    if (!checkBit(k, Device::CODE | Device::GFX | Device::PGFX |
        Device::COL | Device::PCOL | Device::BCOL | Device::AUD |
        Device::DATA))
      mark(k + myOffset, Device::ROW);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DiStella::disasmFromAddress(uInt32 distart)
{
  uInt8 opcode = 0, d1 = 0;
  uInt16 ad = 0;
  AddressingMode addrMode{};

  myPC = distart - myOffset;

  while (myPC <= myAppData.end) {

    // abort when we reach non-code areas
    if (checkBits(myPC, Device::DATA | Device::GFX | Device::PGFX |
                        Device::COL | Device::PCOL | Device::BCOL |
                        Device::AUD,
                  Device::CODE)) {
      myPCEnd = (myPC - 1) + myOffset;
      return;
    }

    opcode = Debugger::debugger().peek(myPC + myOffset);  ++myPC;
    addrMode = ourLookup[opcode].addr_mode;

    // Add operand(s) for PC values outside the app data range
    if (myPC >= myAppData.end) {
      switch (addrMode) {
        case AddressingMode::ABSOLUTE:
        case AddressingMode::ABSOLUTE_X:
        case AddressingMode::ABSOLUTE_Y:
        case AddressingMode::INDIRECT_X:
        case AddressingMode::INDIRECT_Y:
        case AddressingMode::ABS_INDIRECT:
          myPCEnd = myAppData.end + myOffset;
          return;

        case AddressingMode::ZERO_PAGE:
        case AddressingMode::IMMEDIATE:
        case AddressingMode::ZERO_PAGE_X:
        case AddressingMode::ZERO_PAGE_Y:
        case AddressingMode::RELATIVE:
          if (myPC > myAppData.end) {
            ++myPC;
            myPCEnd = myAppData.end + myOffset;
            return;
          }
          break;  // operand fits; fall through to process it normally

        default:
          break;
      }  // end switch(addr_mode)
    } // end if (myPC >= myAppData.end)

    // Add operand(s)
    switch (addrMode) {
      case AddressingMode::ABSOLUTE:
        ad = Debugger::debugger().dpeek(myPC + myOffset);  myPC += 2;
        mark(ad, Device::REFERENCED);
        // handle JMP/JSR
        if (ourLookup[opcode].source == AccessMode::ADDR) {
          // do NOT use flags set by debugger, else known CODE will not analyzed statically.
          if (!checkBit(ad & myAppData.end, Device::CODE | Device::ROW, false)) {
            if (ad > 0xfff)
              myAddressQueue.push((ad & myAppData.end) + myOffset);
            mark(ad, Device::CODE);
          }
        } else
          mark(ad, Device::DATA);
        break;

      case AddressingMode::ABSOLUTE_X:
      case AddressingMode::ABSOLUTE_Y:
        ad = Debugger::debugger().dpeek(myPC + myOffset);  myPC += 2;
        mark(ad, Device::REFERENCED);
        break;

      case AddressingMode::ABS_INDIRECT:
        ad = Debugger::debugger().dpeek(myPC + myOffset);  myPC += 2;
        mark(ad, Device::REFERENCED);
        // Resolve the jump target when the vector is in ROM (static content known)
        if (ad > 0xfff) {
          const uInt16 target = Debugger::debugger().dpeek(ad);
          if (!checkBit(target & myAppData.end, Device::CODE | Device::ROW, false))
            myAddressQueue.push(target);
          mark(target, Device::CODE);
        }
        break;

      case AddressingMode::IMMEDIATE:
      case AddressingMode::INDIRECT_X:
      case AddressingMode::INDIRECT_Y:
        ++myPC;
        break;

      case AddressingMode::ZERO_PAGE:
      case AddressingMode::ZERO_PAGE_X:
      case AddressingMode::ZERO_PAGE_Y:
        d1 = Debugger::debugger().peek(myPC + myOffset);  ++myPC;
        mark(d1, Device::REFERENCED);
        break;

      case AddressingMode::RELATIVE:
        // SA - 04-06-2010: there seemed to be a bug in distella,
        // where wraparound occurred on a 32-bit int, and subsequent
        // indexing into the labels array caused a crash
        d1 = Debugger::debugger().peek(myPC + myOffset);  ++myPC;
        ad = ((myPC + static_cast<Int8>(d1)) & 0xfff) + myOffset;
        mark(ad, Device::REFERENCED);
        // do NOT use flags set by debugger, else known CODE will not analyzed statically.
        if (!checkBit(ad - myOffset, Device::CODE | Device::ROW, false)) {
          myAddressQueue.push(ad);
          mark(ad, Device::CODE);
        }
        break;

      default:
        break;
    } // end switch

    // mark BRK vector
    if (opcode == OP_BRK) {
      ad = Debugger::debugger().dpeek(0xfffe, Device::DATA);
      if (!myReserved.breakFound) {
        myAddressQueue.push(ad);
        mark(ad, Device::CODE);
        myReserved.breakFound = true;
      }
    }

    // JMP/RTS/RTI always indicate the end of a block of CODE
    if (opcode == OP_JMP || opcode == OP_JMP_I || opcode == OP_RTS || opcode == OP_RTI) {
      // code block end
      myPCEnd = (myPC - 1) + myOffset;
      return;
    }
  } // while
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DiStella::AddressType DiStella::mark(uInt32 address, uInt16 mask, bool directive)
{
  /*-----------------------------------------------------------------------
    For any given offset and code range...

    If we're between the offset and the end of the code range, we mark
    the bit in the labels array for that data.  The labels array is an
    array of label info for each code address.  If this is the case,
    return "ROM", else...

    We sweep for hardware/system equates, which are valid addresses,
    outside the scope of the code/data range.  For these, we mark its
    corresponding hardware/system array element, and return "TIA" or "RIOT"
    (depending on which system/hardware element was accessed).
    If this was not the case...

    Next we check if it is a code "mirror".  For the 2600, address ranges
    are limited with 13 bits, so other addresses can exist outside of the
    standard code/data range.  For these, we mark the element in the "labels"
    array that corresponds to the mirrored address, and return "ROM_MIRROR"

    If all else fails, it's not a valid address, so return INVALID.

    A quick example breakdown for a 2600 4K cart:
    ===========================================================
      $00-$3d     = system equates (WSYNC, etc...); return TIA.
      $80-$ff     = zero-page RAM (ram_80, etc...); return ZP_RAM.
      $0280-$0297 = system equates (INPT0, etc...); mark the array's element
                    with the appropriate bit; return RIOT.
      $1000-$1FFF = mark the code/data array for the mirrored address
                    with the appropriate bit; return ROM_MIRROR.
      $3000-$3FFF = mark the code/data array for the mirrored address
                    with the appropriate bit; return ROM_MIRROR.
      $5000-$5FFF = mark the code/data array for the mirrored address
                    with the appropriate bit; return ROM_MIRROR.
      $7000-$7FFF = mark the code/data array for the mirrored address
                    with the appropriate bit; return ROM_MIRROR.
      $9000-$9FFF = mark the code/data array for the mirrored address
                    with the appropriate bit; return ROM_MIRROR.
      $B000-$BFFF = mark the code/data array for the mirrored address
                    with the appropriate bit; return ROM_MIRROR.
      $D000-$DFFF = mark the code/data array for the mirrored address
                    with the appropriate bit; return ROM_MIRROR.
      $F000-$FFFF = mark the code/data array for the address
                    with the appropriate bit; return ROM.
      Anything else = invalid, return INVALID.
    ===========================================================
  -----------------------------------------------------------------------*/

  // Check for equates before ROM/ZP-RAM accesses, because the original logic
  // of Distella assumed either equates or ROM; it didn't take ZP-RAM into account
  const CartDebug::AddrType type = CartDebug::addressType(address);
  if(type == CartDebug::AddrType::TIA) {
    return AddressType::TIA;
  }
  else if(type == CartDebug::AddrType::IO) {
    return AddressType::RIOT;
  }
  else if(type == CartDebug::AddrType::ZPRAM && myOffset != 0) {
    return AddressType::ZP_RAM;
  }
  else if(std::cmp_greater_equal(address, myOffset) &&
          std::cmp_less_equal(address, myAppData.end + myOffset)) {
    myLabels[address - myOffset] = myLabels[address - myOffset] | mask;
    if(directive)
      myDirectives[address - myOffset] = mask;
    return AddressType::ROM;
  }
  else if(address > 0x1000 && myOffset != 0 && mySettings.rFlag)  // Exclude zero-page accesses
  {
    myLabels[address & myAppData.end] = myLabels[address & myAppData.end] | mask;
    if(directive)
      myDirectives[address & myAppData.end] = mask;
    return AddressType::ROM_MIRROR;
  }
  else
    return AddressType::INVALID;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool DiStella::checkBit(uInt16 address, uInt16 mask, bool useDebugger) const
{
  // The REFERENCED and VALID_ENTRY flags are needed for any inspection of
  // an address
  // Since they're set only in the labels array (as the lower two bits),
  // they must be included in the other bitfields
  const uInt16 label = myLabels[address & myAppData.end],
    lastbits = label & (Device::REFERENCED | Device::VALID_ENTRY),
    directive = myDirectives[address & myAppData.end] & ~(Device::REFERENCED | Device::VALID_ENTRY),
    // Exclude TCODE: it's output-only annotation from the previous run, not runtime evidence
    debugger = Debugger::debugger().getAccessFlags(address | myOffset)
               & ~(Device::REFERENCED | Device::VALID_ENTRY | Device::TCODE);

  // Any address marked by a manual directive always takes priority
  if (directive)
    return (directive | lastbits) & mask;
  // Next, the results from a dynamic/runtime analysis are used (except for pass 1)
  if (useDebugger && ((debugger | lastbits) & mask))
    return true;
  // Otherwise, default to static analysis from Distella
  return label & mask;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool DiStella::checkBits(uInt16 address, uInt16 mask, uInt16 notMask, bool useDebugger) const
{
  return checkBit(address, mask, useDebugger) && !checkBit(address, notMask, useDebugger);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool DiStella::checkRange(uInt16 start, uInt16 end) const
{
  if (start > end) {
    cerr << std::format("Beginning of range greater than end: start = {:04x}, end = {:04x}\n",
      start, end);
    return false;
  } else if (start > myAppData.end + myOffset) {
    cerr << std::format("Beginning of range out of range: start = {:04x}, range = {:04x}\n",
      start, myAppData.end + myOffset);
    return false;
  } else if (start < myOffset) {
    cerr << std::format("Beginning of range out of range: start = {:04x}, offset = {:04x}\n",
      start, myOffset);
    return false;
  }
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartDebug::DisasmSegColor DiStella::mnemonicColorForOpcode(uInt8 opcode)
{
  const string_view mn = ourLookup[opcode].mnemonic;
  if(mn[0] == '.')
    return CartDebug::DisasmSegColor::Default;  // illegal / undefined opcode
  if(ourLookup[opcode].addr_mode == AddressingMode::RELATIVE)
    return CartDebug::DisasmSegColor::Branch;
  switch(opcode) {
    case OP_BRK: case OP_JSR: case OP_RTI:
    case OP_RTS: case OP_JMP: case OP_JMP_I:
      return CartDebug::DisasmSegColor::Jump;
    default:
      break;
  }
  if(mn[0] == 'L' && mn[1] == 'D')  return CartDebug::DisasmSegColor::LoadStore;
  if(mn[0] == 'S' && mn[1] == 'T')  return CartDebug::DisasmSegColor::LoadStore;
  return CartDebug::DisasmSegColor::ALU;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartDebug::DisasmSegColor DiStella::colorA12High(uInt16 addr) const
{
  return myDbg.getLabel(addr, true).empty()
    ? CartDebug::DisasmSegColor::AutoEquate
    : CartDebug::DisasmSegColor::UserEquate;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartDebug::DisasmSegColor DiStella::colorA12Low(uInt16 addr, AddressType labfound,
                                                bool isRead) const
{
  switch(labfound) {
    case AddressType::TIA:    return CartDebug::DisasmSegColor::TIAEquate;
    case AddressType::RIOT:   return CartDebug::DisasmSegColor::RIOTEquate;
    case AddressType::ZP_RAM:
      return myDbg.getLabel(addr, isRead).empty()
        ? CartDebug::DisasmSegColor::ZeroPage
        : CartDebug::DisasmSegColor::UserEquate;
    default:                  return CartDebug::DisasmSegColor::Default;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DiStella::addEntry(Device::AccessType type)
{
  CartDebug::DisassemblyTag tag;
  tag.type    = type;
  tag.address = myLine.address;

  if (tag.address >= myAppData.start)
  {
    if (tag.address) {
      // A user-defined label always overrides any auto-generated one
      tag.label = myDbg.getLabel(tag.address, true);
      if (tag.label.empty()) {
        if (myLine.hasAutoLabel) {
          const uInt32 labelAddr = mySettings.useOrgLabels
              ? static_cast<uInt32>(tag.address - myOffset) + mySettings.orgBase
              : tag.address;
          tag.label = 'L' + Base::hexN(static_cast<int>(labelAddr), mySettings.labelDigits);
          tag.labelColor = CartDebug::DisasmSegColor::AutoLabel;
        }
        else if (mySettings.showAddresses && type == Device::CODE) {
          // Indent address-as-label to differentiate from real labels
          tag.label = " " + Base::toString(tag.address, Base::Fmt::_16_4);
          tag.labelColor = CartDebug::DisasmSegColor::AddressLabel;
        }
      }
      else {
        tag.labelColor = CartDebug::DisasmSegColor::UserLabel;
      }
    }
    tag.mnemonicColor = myLine.mnemonicColor;
    tag.operandColor  = myLine.operandColor;

    switch (type) {
      case Device::CODE:
        tag.disasm = myLine.disasm;
        tag.ccount = myLine.ccount;
        tag.ctotal = myLine.ctotal;
        tag.bytes  = myLine.bytes;
        if (myOffset != 0) {
          const auto flags = Debugger::debugger().getAccessFlags(tag.address);
          // Mark addresses that DiStella treats as CODE but the runtime hasn't confirmed
          if (!(flags & Device::CODE)) {
            tag.ccount += " *";
            Debugger::debugger().setAccessFlags(tag.address, Device::TCODE);
          }
          // Flag self-modifying code: a location that has been both executed and written
          if ((flags & Device::CODE) && (flags & Device::WRITE))
            tag.ccount += " ~";
        }
        break;

      case Device::GFX:
      case Device::PGFX:
      case Device::COL:
      case Device::PCOL:
      case Device::BCOL:
      case Device::DATA:
      case Device::AUD:
        tag.disasm = myLine.disasm;
        tag.bytes  = myLine.bytes;
        break;

      case Device::ROW:
        tag.disasm = myLine.disasm;
        break;

      case Device::NONE:
      default:
        tag.disasm = " ";
        break;
    }
    myList.push_back(tag);
  }

  myLine = {};
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DiStella::outputGraphics()
{
  const bool isPGfx = checkBit(myPC, Device::PGFX);
  const string& bitString = isPGfx ? "\x1f" : "\x1e";
  const uInt8 byte = Debugger::debugger().peek(myPC + myOffset);

  // add extra spacing line when switching from non-graphics to graphics
  if (mySegType != Device::GFX && mySegType != Device::NONE) {
    myLine = {};
    addEntry(Device::NONE);
  }
  mySegType = Device::GFX;

  myLine.address      = myPC + myOffset;
  myLine.hasAutoLabel = checkBit(myPC, Device::REFERENCED);

  {
    std::ostringstream s;
    s << ".byte $" << Base::HEX2 << static_cast<int>(byte) << "  |";
    for (uInt8 i = 0, c = byte; i < 8; ++i, c <<= 1)
      s << ((c > 127) ? bitString : " ");
    s << "|   $" << Base::HEX4 << myPC + myOffset;
    myLine.disasm = s.str();
  }
  if (mySettings.gfxFormat == Base::Fmt::_2)
    myLine.bytes = Base::toString(byte, Base::Fmt::_2_8);
  else {
    std::ostringstream s;
    s << Base::HEX2 << static_cast<int>(byte);
    myLine.bytes = s.str();
  }

  addEntry(isPGfx ? Device::PGFX : Device::GFX);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DiStella::outputColors()
{
  const uInt8 byte = Debugger::debugger().peek(myPC + myOffset);

  const Device::AccessType colorType =
    checkBit(myPC, Device::COL) ? Device::COL :
    checkBit(myPC, Device::PCOL) ? Device::PCOL : Device::BCOL;

  // add extra spacing line when switching from non-colors to colors
  if(mySegType != Device::COL && mySegType != Device::PCOL &&
     mySegType != Device::BCOL && mySegType != Device::NONE)
  {
    myLine = {};
    addEntry(Device::NONE);
  }
  mySegType = colorType;

  myLine.address      = myPC + myOffset;
  myLine.hasAutoLabel = checkBit(myPC, Device::REFERENCED);

  // output color
  const string color = getColor(byte);
  const string_view colorLabel =
    (colorType == Device::COL) ? "(Px)" : (colorType == Device::PCOL) ? "(PF)" : "(BK)";

  {
    std::ostringstream s;
    s << ".byte " << color
      << std::setw(static_cast<int>(16 + 3 - color.length())) << std::setfill(' ')
      << "; $" << Base::HEX4 << myPC + myOffset << " " << colorLabel;
    myLine.disasm = s.str();
  }
  {
    std::ostringstream s;
    s << Base::HEX2 << static_cast<int>(byte);
    myLine.bytes = s.str();
  }

  addEntry(colorType);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string DiStella::getColor(uInt8 byte)
{
  static constexpr std::array<string_view, 16> NTSC_COLOR = {
    "BLACK", "YELLOW", "BROWN", "ORANGE",
    "RED", "MAUVE", "VIOLET", "PURPLE",
    "BLUE", "BLUE_CYAN", "CYAN", "CYAN_GREEN",
    "GREEN", "GREEN_YELLOW", "GREEN_BEIGE", "BEIGE"
  };
  static constexpr std::array<string_view, 16> PAL_COLOR = {
    "BLACK0", "BLACK1", "YELLOW", "GREEN_YELLOW",
    "ORANGE", "GREEN", "RED", "CYAN_GREEN",
    "MAUVE", "CYAN", "VIOLET", "BLUE_CYAN",
    "PURPLE", "BLUE", "BLACKE", "BLACKF"
  };
  static constexpr std::array<string_view, 8> SECAM_COLOR = {
    "BLACK", "BLUE", "RED", "PURPLE",
    "GREEN", "CYAN", "YELLOW", "WHITE"
  };

  if(myDbg.myConsole.timing() == ConsoleTiming::ntsc)
    return std::format("{}|${}", NTSC_COLOR[byte >> 4], Base::hex1(byte & 0xf));
  else if(myDbg.myConsole.timing() == ConsoleTiming::pal)
    return std::format("{}|${}", PAL_COLOR[byte >> 4], Base::hex1(byte & 0xf));
  else
    return std::format("${}|{}", Base::hex1(byte >> 4), SECAM_COLOR[(byte >> 1) & 0x7]);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DiStella::outputBytes(Device::AccessType type)
{
  bool isType = true;
  bool referenced = checkBit(myPC, Device::REFERENCED);
  bool lineEmpty = true;
  int numBytes = 0;
  std::ostringstream disasmStream;

  // add extra spacing line when switching from non-data to data
  if (mySegType != Device::DATA && mySegType != Device::NONE) {
    myLine = {};
    addEntry(Device::NONE);
  }
  mySegType = Device::DATA;

  while (isType && myPC <= myAppData.end) {
    if (referenced) {
      // start a new line with a label
      if (!lineEmpty) {
        myLine.disasm = disasmStream.str();
        addEntry(type);
        disasmStream.str("");
      }

      myLine.address      = myPC + myOffset;
      myLine.hasAutoLabel = true;
      disasmStream << ".byte $" << Base::HEX2
        << static_cast<int>(Debugger::debugger().peek(myPC + myOffset));
      ++myPC;
      numBytes = 1;
      lineEmpty = false;
    } else if (lineEmpty) {
      // start a new line without a label
      myLine.address      = myPC + myOffset;
      myLine.hasAutoLabel = false;
      disasmStream << ".byte $" << Base::HEX2
        << static_cast<int>(Debugger::debugger().peek(myPC + myOffset));
      ++myPC;
      numBytes = 1;
      lineEmpty = false;
    }
    // Otherwise, append bytes to the current line, up until the maximum
    else if (++numBytes == mySettings.bytesWidth) {
      myLine.disasm = disasmStream.str();
      addEntry(type);
      disasmStream.str("");
      lineEmpty = true;
    } else {
      disasmStream << ",$" << Base::HEX2
        << static_cast<int>(Debugger::debugger().peek(myPC + myOffset));
      ++myPC;
    }
    isType = checkBits(myPC, type,
                       Device::CODE | (type != Device::DATA ? Device::DATA : 0) |
                       Device::GFX | Device::PGFX |
                       Device::COL | Device::PCOL | Device::BCOL | Device::AUD);
    referenced = checkBit(myPC, Device::REFERENCED);
  }
  if (!lineEmpty) {
    myLine.disasm = disasmStream.str();
    addEntry(type);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DiStella::processDirectives(const CartDebug::DirectiveList& directives)
{
  for (const auto& tag : directives) {
    if (checkRange(tag.start, tag.end))
      for (uInt32 k = tag.start; k <= tag.end; ++k)
        mark(k, tag.type, true);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DiStella::Settings DiStella::settings;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const std::array<DiStella::Instruction_tag, 256> DiStella::ourLookup = { {
  /****  Positive  ****/

  /* 00 */{"brk", AddressingMode::IMPLIED,     AccessMode::NONE, RWMode::NONE,  7, 1}, /* Pseudo Absolute */
  /* 01 */{"ora", AddressingMode::INDIRECT_X,  AccessMode::INDX, RWMode::READ,  6, 2}, /* (Indirect,X) */
  /* 02 */{".JAM",AddressingMode::IMPLIED,     AccessMode::NONE, RWMode::NONE,  0, 1}, /* TILT */
  /* 03 */{"SLO", AddressingMode::INDIRECT_X,  AccessMode::INDX, RWMode::WRITE, 8, 2},

  /* 04 */{"NOP", AddressingMode::ZERO_PAGE,   AccessMode::NONE, RWMode::NONE,  3, 2},
  /* 05 */{"ora", AddressingMode::ZERO_PAGE,   AccessMode::ZERO, RWMode::READ,  3, 2}, /* Zeropage */
  /* 06 */{"asl", AddressingMode::ZERO_PAGE,   AccessMode::ZERO, RWMode::WRITE, 5, 2}, /* Zeropage */
  /* 07 */{"SLO", AddressingMode::ZERO_PAGE,   AccessMode::ZERO, RWMode::WRITE, 5, 2},

  /* 08 */{"php", AddressingMode::IMPLIED,     AccessMode::SR,   RWMode::NONE,  3, 1},
  /* 09 */{"ora", AddressingMode::IMMEDIATE,   AccessMode::IMM,  RWMode::READ,  2, 2}, /* Immediate */
  /* 0a */{"asl", AddressingMode::ACCUMULATOR, AccessMode::AC,   RWMode::WRITE, 2, 1}, /* Accumulator */
  /* 0b */{"ANC", AddressingMode::IMMEDIATE,   AccessMode::ACIM, RWMode::READ,  2, 2},

  /* 0c */{"NOP", AddressingMode::ABSOLUTE,    AccessMode::NONE, RWMode::NONE,  4, 3},
  /* 0d */{"ora", AddressingMode::ABSOLUTE,    AccessMode::ABS,  RWMode::READ,  4, 3}, /* Absolute */
  /* 0e */{"asl", AddressingMode::ABSOLUTE,    AccessMode::ABS,  RWMode::WRITE, 6, 3}, /* Absolute */
  /* 0f */{"SLO", AddressingMode::ABSOLUTE,    AccessMode::ABS,  RWMode::WRITE, 6, 3},

  /* 10 */{"bpl", AddressingMode::RELATIVE,    AccessMode::REL,  RWMode::READ,  2, 2},
  /* 11 */{"ora", AddressingMode::INDIRECT_Y,  AccessMode::INDY, RWMode::READ,  5, 2}, /* (Indirect),Y */
  /* 12 */{".JAM",AddressingMode::IMPLIED,     AccessMode::NONE, RWMode::NONE,  0, 1}, /* TILT */
  /* 13 */{"SLO", AddressingMode::INDIRECT_Y,  AccessMode::INDY, RWMode::WRITE, 8, 2},

  /* 14 */{"NOP", AddressingMode::ZERO_PAGE_X, AccessMode::NONE, RWMode::NONE,  4, 2},
  /* 15 */{"ora", AddressingMode::ZERO_PAGE_X, AccessMode::ZERX, RWMode::READ,  4, 2}, /* Zeropage,X */
  /* 16 */{"asl", AddressingMode::ZERO_PAGE_X, AccessMode::ZERX, RWMode::WRITE, 6, 2}, /* Zeropage,X */
  /* 17 */{"SLO", AddressingMode::ZERO_PAGE_X, AccessMode::ZERX, RWMode::WRITE, 6, 2},

  /* 18 */{"clc", AddressingMode::IMPLIED,     AccessMode::NONE, RWMode::NONE,  2, 1},
  /* 19 */{"ora", AddressingMode::ABSOLUTE_Y,  AccessMode::ABSY, RWMode::READ,  4, 3}, /* Absolute,Y */
  /* 1a */{"NOP", AddressingMode::IMPLIED,     AccessMode::NONE, RWMode::NONE,  2, 1},
  /* 1b */{"SLO", AddressingMode::ABSOLUTE_Y,  AccessMode::ABSY, RWMode::WRITE, 7, 3},

  /* 1c */{"NOP", AddressingMode::ABSOLUTE_X,  AccessMode::NONE, RWMode::NONE,  4, 3},
  /* 1d */{"ora", AddressingMode::ABSOLUTE_X,  AccessMode::ABSX, RWMode::READ,  4, 3}, /* Absolute,X */
  /* 1e */{"asl", AddressingMode::ABSOLUTE_X,  AccessMode::ABSX, RWMode::WRITE, 7, 3}, /* Absolute,X */
  /* 1f */{"SLO", AddressingMode::ABSOLUTE_X,  AccessMode::ABSX, RWMode::WRITE, 7, 3},

  /* 20 */{"jsr", AddressingMode::ABSOLUTE,    AccessMode::ADDR, RWMode::READ,  6, 3},
  /* 21 */{"and", AddressingMode::INDIRECT_X,  AccessMode::INDX, RWMode::READ,  6, 2}, /* (Indirect ,X) */
  /* 22 */{".JAM",AddressingMode::IMPLIED,     AccessMode::NONE, RWMode::NONE,  0, 1}, /* TILT */
  /* 23 */{"RLA", AddressingMode::INDIRECT_X,  AccessMode::INDX, RWMode::WRITE, 8, 2},

  /* 24 */{"bit", AddressingMode::ZERO_PAGE,   AccessMode::ZERO, RWMode::READ,  3, 2}, /* Zeropage */
  /* 25 */{"and", AddressingMode::ZERO_PAGE,   AccessMode::ZERO, RWMode::READ,  3, 2}, /* Zeropage */
  /* 26 */{"rol", AddressingMode::ZERO_PAGE,   AccessMode::ZERO, RWMode::WRITE, 5, 2}, /* Zeropage */
  /* 27 */{"RLA", AddressingMode::ZERO_PAGE,   AccessMode::ZERO, RWMode::WRITE, 5, 2},

  /* 28 */{"plp", AddressingMode::IMPLIED,     AccessMode::NONE, RWMode::NONE,  4, 1},
  /* 29 */{"and", AddressingMode::IMMEDIATE,   AccessMode::IMM,  RWMode::READ,  2, 2}, /* Immediate */
  /* 2a */{"rol", AddressingMode::ACCUMULATOR, AccessMode::AC,   RWMode::WRITE, 2, 1}, /* Accumulator */
  /* 2b */{"ANC", AddressingMode::IMMEDIATE,   AccessMode::ACIM, RWMode::READ,  2, 2},

  /* 2c */{"bit", AddressingMode::ABSOLUTE,    AccessMode::ABS,  RWMode::READ,  4, 3}, /* Absolute */
  /* 2d */{"and", AddressingMode::ABSOLUTE,    AccessMode::ABS,  RWMode::READ,  4, 3}, /* Absolute */
  /* 2e */{"rol", AddressingMode::ABSOLUTE,    AccessMode::ABS,  RWMode::WRITE, 6, 3}, /* Absolute */
  /* 2f */{"RLA", AddressingMode::ABSOLUTE,    AccessMode::ABS,  RWMode::WRITE, 6, 3},

  /* 30 */{"bmi", AddressingMode::RELATIVE,    AccessMode::REL,  RWMode::READ,  2, 2},
  /* 31 */{"and", AddressingMode::INDIRECT_Y,  AccessMode::INDY, RWMode::READ,  5, 2}, /* (Indirect),Y */
  /* 32 */{".JAM",AddressingMode::IMPLIED,     AccessMode::NONE, RWMode::NONE,  0, 1}, /* TILT */
  /* 33 */{"RLA", AddressingMode::INDIRECT_Y,  AccessMode::INDY, RWMode::WRITE, 8, 2},

  /* 34 */{"NOP", AddressingMode::ZERO_PAGE_X, AccessMode::NONE, RWMode::NONE,  4, 2},
  /* 35 */{"and", AddressingMode::ZERO_PAGE_X, AccessMode::ZERX, RWMode::READ,  4, 2}, /* Zeropage,X */
  /* 36 */{"rol", AddressingMode::ZERO_PAGE_X, AccessMode::ZERX, RWMode::WRITE, 6, 2}, /* Zeropage,X */
  /* 37 */{"RLA", AddressingMode::ZERO_PAGE_X, AccessMode::ZERX, RWMode::WRITE, 6, 2},

  /* 38 */{"sec", AddressingMode::IMPLIED,     AccessMode::NONE, RWMode::NONE,  2, 1},
  /* 39 */{"and", AddressingMode::ABSOLUTE_Y,  AccessMode::ABSY, RWMode::READ,  4, 3}, /* Absolute,Y */
  /* 3a */{"NOP", AddressingMode::IMPLIED,     AccessMode::NONE, RWMode::NONE,  2, 1},
  /* 3b */{"RLA", AddressingMode::ABSOLUTE_Y,  AccessMode::ABSY, RWMode::WRITE, 7, 3},

  /* 3c */{"NOP", AddressingMode::ABSOLUTE_X,  AccessMode::NONE, RWMode::NONE,  4, 3},
  /* 3d */{"and", AddressingMode::ABSOLUTE_X,  AccessMode::ABSX, RWMode::READ,  4, 3}, /* Absolute,X */
  /* 3e */{"rol", AddressingMode::ABSOLUTE_X,  AccessMode::ABSX, RWMode::WRITE, 7, 3}, /* Absolute,X */
  /* 3f */{"RLA", AddressingMode::ABSOLUTE_X,  AccessMode::ABSX, RWMode::WRITE, 7, 3},

  /* 40 */{"rti", AddressingMode::IMPLIED,     AccessMode::NONE, RWMode::NONE,  6, 1},
  /* 41 */{"eor", AddressingMode::INDIRECT_X,  AccessMode::INDX, RWMode::READ,  6, 2}, /* (Indirect,X) */
  /* 42 */{".JAM",AddressingMode::IMPLIED,     AccessMode::NONE, RWMode::NONE,  0, 1}, /* TILT */
  /* 43 */{"SRE", AddressingMode::INDIRECT_X,  AccessMode::INDX, RWMode::WRITE, 8, 2},

  /* 44 */{"NOP", AddressingMode::ZERO_PAGE,   AccessMode::NONE, RWMode::NONE,  3, 2},
  /* 45 */{"eor", AddressingMode::ZERO_PAGE,   AccessMode::ZERO, RWMode::READ,  3, 2}, /* Zeropage */
  /* 46 */{"lsr", AddressingMode::ZERO_PAGE,   AccessMode::ZERO, RWMode::WRITE, 5, 2}, /* Zeropage */
  /* 47 */{"SRE", AddressingMode::ZERO_PAGE,   AccessMode::ZERO, RWMode::WRITE, 5, 2},

  /* 48 */{"pha", AddressingMode::IMPLIED,     AccessMode::AC,   RWMode::NONE,  3, 1},
  /* 49 */{"eor", AddressingMode::IMMEDIATE,   AccessMode::IMM,  RWMode::READ,  2, 2}, /* Immediate */
  /* 4a */{"lsr", AddressingMode::ACCUMULATOR, AccessMode::AC,   RWMode::WRITE, 2, 1}, /* Accumulator */
  /* 4b */{"ASR", AddressingMode::IMMEDIATE,   AccessMode::ACIM, RWMode::READ,  2, 2}, /* (AC & IMM) >>1 */

  /* 4c */{"jmp", AddressingMode::ABSOLUTE,    AccessMode::ADDR, RWMode::READ,  3, 3}, /* Absolute */
  /* 4d */{"eor", AddressingMode::ABSOLUTE,    AccessMode::ABS,  RWMode::READ,  4, 3}, /* Absolute */
  /* 4e */{"lsr", AddressingMode::ABSOLUTE,    AccessMode::ABS,  RWMode::WRITE, 6, 3}, /* Absolute */
  /* 4f */{"SRE", AddressingMode::ABSOLUTE,    AccessMode::ABS,  RWMode::WRITE, 6, 3},

  /* 50 */{"bvc", AddressingMode::RELATIVE,    AccessMode::REL,  RWMode::READ,  2, 2},
  /* 51 */{"eor", AddressingMode::INDIRECT_Y,  AccessMode::INDY, RWMode::READ,  5, 2}, /* (Indirect),Y */
  /* 52 */{".JAM",AddressingMode::IMPLIED,     AccessMode::NONE, RWMode::NONE,  0, 1}, /* TILT */
  /* 53 */{"SRE", AddressingMode::INDIRECT_Y,  AccessMode::INDY, RWMode::WRITE, 8, 2},

  /* 54 */{"NOP", AddressingMode::ZERO_PAGE_X, AccessMode::NONE, RWMode::NONE,  4, 2},
  /* 55 */{"eor", AddressingMode::ZERO_PAGE_X, AccessMode::ZERX, RWMode::READ,  4, 2}, /* Zeropage,X */
  /* 56 */{"lsr", AddressingMode::ZERO_PAGE_X, AccessMode::ZERX, RWMode::WRITE, 6, 2}, /* Zeropage,X */
  /* 57 */{"SRE", AddressingMode::ZERO_PAGE_X, AccessMode::ZERX, RWMode::WRITE, 6, 2},

  /* 58 */{"cli", AddressingMode::IMPLIED,     AccessMode::NONE, RWMode::NONE,  2, 1},
  /* 59 */{"eor", AddressingMode::ABSOLUTE_Y,  AccessMode::ABSY, RWMode::READ,  4, 3}, /* Absolute,Y */
  /* 5a */{"NOP", AddressingMode::IMPLIED,     AccessMode::NONE, RWMode::NONE,  2, 1},
  /* 5b */{"SRE", AddressingMode::ABSOLUTE_Y,  AccessMode::ABSY, RWMode::WRITE, 7, 3},

  /* 5c */{"NOP", AddressingMode::ABSOLUTE_X,  AccessMode::NONE, RWMode::NONE,  4, 3},
  /* 5d */{"eor", AddressingMode::ABSOLUTE_X,  AccessMode::ABSX, RWMode::READ,  4, 3}, /* Absolute,X */
  /* 5e */{"lsr", AddressingMode::ABSOLUTE_X,  AccessMode::ABSX, RWMode::WRITE, 7, 3}, /* Absolute,X */
  /* 5f */{"SRE", AddressingMode::ABSOLUTE_X,  AccessMode::ABSX, RWMode::WRITE, 7, 3},

  /* 60 */{"rts", AddressingMode::IMPLIED,     AccessMode::NONE, RWMode::NONE,  6, 1},
  /* 61 */{"adc", AddressingMode::INDIRECT_X,  AccessMode::INDX, RWMode::READ,  6, 2}, /* (Indirect,X) */
  /* 62 */{".JAM",AddressingMode::IMPLIED,     AccessMode::NONE, RWMode::NONE,  0, 1}, /* TILT */
  /* 63 */{"RRA", AddressingMode::INDIRECT_X,  AccessMode::INDX, RWMode::WRITE, 8, 2},

  /* 64 */{"NOP", AddressingMode::ZERO_PAGE,   AccessMode::NONE, RWMode::NONE,  3, 2},
  /* 65 */{"adc", AddressingMode::ZERO_PAGE,   AccessMode::ZERO, RWMode::READ,  3, 2}, /* Zeropage */
  /* 66 */{"ror", AddressingMode::ZERO_PAGE,   AccessMode::ZERO, RWMode::WRITE, 5, 2}, /* Zeropage */
  /* 67 */{"RRA", AddressingMode::ZERO_PAGE,   AccessMode::ZERO, RWMode::WRITE, 5, 2},

  /* 68 */{"pla", AddressingMode::IMPLIED,     AccessMode::NONE, RWMode::NONE,  4, 1},
  /* 69 */{"adc", AddressingMode::IMMEDIATE,   AccessMode::IMM,  RWMode::READ,  2, 2}, /* Immediate */
  /* 6a */{"ror", AddressingMode::ACCUMULATOR, AccessMode::AC,   RWMode::WRITE, 2, 1}, /* Accumulator */
  /* 6b */{"ARR", AddressingMode::IMMEDIATE,   AccessMode::ACIM, RWMode::READ,  2, 2}, /* ARR isn't typo */

  /* 6c */{"jmp", AddressingMode::ABS_INDIRECT,AccessMode::AIND, RWMode::READ,  5, 3}, /* Indirect */
  /* 6d */{"adc", AddressingMode::ABSOLUTE,    AccessMode::ABS,  RWMode::READ,  4, 3}, /* Absolute */
  /* 6e */{"ror", AddressingMode::ABSOLUTE,    AccessMode::ABS,  RWMode::WRITE, 6, 3}, /* Absolute */
  /* 6f */{"RRA", AddressingMode::ABSOLUTE,    AccessMode::ABS,  RWMode::WRITE, 6, 3},

  /* 70 */{"bvs", AddressingMode::RELATIVE,    AccessMode::REL,  RWMode::READ,  2, 2},
  /* 71 */{"adc", AddressingMode::INDIRECT_Y,  AccessMode::INDY, RWMode::READ,  5, 2}, /* (Indirect),Y */
  /* 72 */{".JAM",AddressingMode::IMPLIED,     AccessMode::NONE, RWMode::NONE,  0, 1}, /* TILT relative? */
  /* 73 */{"RRA", AddressingMode::INDIRECT_Y,  AccessMode::INDY, RWMode::WRITE, 8, 2},

  /* 74 */{"NOP", AddressingMode::ZERO_PAGE_X, AccessMode::NONE, RWMode::NONE,  4, 2},
  /* 75 */{"adc", AddressingMode::ZERO_PAGE_X, AccessMode::ZERX, RWMode::READ,  4, 2}, /* Zeropage,X */
  /* 76 */{"ror", AddressingMode::ZERO_PAGE_X, AccessMode::ZERX, RWMode::WRITE, 6, 2}, /* Zeropage,X */
  /* 77 */{"RRA", AddressingMode::ZERO_PAGE_X, AccessMode::ZERX, RWMode::WRITE, 6, 2},

  /* 78 */{"sei", AddressingMode::IMPLIED,     AccessMode::NONE, RWMode::NONE,  2, 1},
  /* 79 */{"adc", AddressingMode::ABSOLUTE_Y,  AccessMode::ABSY, RWMode::READ,  4, 3}, /* Absolute,Y */
  /* 7a */{"NOP", AddressingMode::IMPLIED,     AccessMode::NONE, RWMode::NONE,  2, 1},
  /* 7b */{"RRA", AddressingMode::ABSOLUTE_Y,  AccessMode::ABSY, RWMode::WRITE, 7, 3},

  /* 7c */{"NOP", AddressingMode::ABSOLUTE_X,  AccessMode::NONE, RWMode::NONE,  4, 3},
  /* 7d */{"adc", AddressingMode::ABSOLUTE_X,  AccessMode::ABSX, RWMode::READ,  4, 3},  /* Absolute,X */
  /* 7e */{"ror", AddressingMode::ABSOLUTE_X,  AccessMode::ABSX, RWMode::WRITE, 7, 3},  /* Absolute,X */
  /* 7f */{"RRA", AddressingMode::ABSOLUTE_X,  AccessMode::ABSX, RWMode::WRITE, 7, 3},

  /****  Negative  ****/

  /* 80 */{"NOP", AddressingMode::IMMEDIATE,   AccessMode::NONE, RWMode::NONE,  2, 2},
  /* 81 */{"sta", AddressingMode::INDIRECT_X,  AccessMode::AC,   RWMode::WRITE, 6, 2}, /* (Indirect,X) */
  /* 82 */{"NOP", AddressingMode::IMMEDIATE,   AccessMode::NONE, RWMode::NONE,  2, 2},
  /* 83 */{"SAX", AddressingMode::INDIRECT_X,  AccessMode::ANXR, RWMode::WRITE, 6, 2},

  /* 84 */{"sty", AddressingMode::ZERO_PAGE,   AccessMode::YR,   RWMode::WRITE, 3, 2}, /* Zeropage */
  /* 85 */{"sta", AddressingMode::ZERO_PAGE,   AccessMode::AC,   RWMode::WRITE, 3, 2}, /* Zeropage */
  /* 86 */{"stx", AddressingMode::ZERO_PAGE,   AccessMode::XR,   RWMode::WRITE, 3, 2}, /* Zeropage */
  /* 87 */{"SAX", AddressingMode::ZERO_PAGE,   AccessMode::ANXR, RWMode::WRITE, 3, 2},

  /* 88 */{"dey", AddressingMode::IMPLIED,     AccessMode::YR,   RWMode::NONE,  2, 1},
  /* 89 */{"NOP", AddressingMode::IMMEDIATE,   AccessMode::NONE, RWMode::NONE,  2, 2},
  /* 8a */{"txa", AddressingMode::IMPLIED,     AccessMode::XR,   RWMode::NONE,  2, 1},
  /****  very abnormal: usually AC = AC | #$EE & XR & #$oper  ****/
  /* 8b */{"ANE", AddressingMode::IMMEDIATE,   AccessMode::AXIM, RWMode::READ,  2, 2},

  /* 8c */{"sty", AddressingMode::ABSOLUTE,    AccessMode::YR,   RWMode::WRITE, 4, 3}, /* Absolute */
  /* 8d */{"sta", AddressingMode::ABSOLUTE,    AccessMode::AC,   RWMode::WRITE, 4, 3}, /* Absolute */
  /* 8e */{"stx", AddressingMode::ABSOLUTE,    AccessMode::XR,   RWMode::WRITE, 4, 3}, /* Absolute */
  /* 8f */{"SAX", AddressingMode::ABSOLUTE,    AccessMode::ANXR, RWMode::WRITE, 4, 3},

  /* 90 */{"bcc", AddressingMode::RELATIVE,    AccessMode::REL,  RWMode::READ,  2, 2},
  /* 91 */{"sta", AddressingMode::INDIRECT_Y,  AccessMode::AC,   RWMode::WRITE, 6, 2}, /* (Indirect),Y */
  /* 92 */{".JAM",AddressingMode::IMPLIED,     AccessMode::NONE, RWMode::NONE,  0, 1}, /* TILT relative? */
  /* 93 */{"SHA", AddressingMode::INDIRECT_Y,  AccessMode::ANXR, RWMode::WRITE, 6, 2},

  /* 94 */{"sty", AddressingMode::ZERO_PAGE_X, AccessMode::YR,   RWMode::WRITE, 4, 2}, /* Zeropage,X */
  /* 95 */{"sta", AddressingMode::ZERO_PAGE_X, AccessMode::AC,   RWMode::WRITE, 4, 2}, /* Zeropage,X */
  /* 96 */{"stx", AddressingMode::ZERO_PAGE_Y, AccessMode::XR,   RWMode::WRITE, 4, 2}, /* Zeropage,Y */
  /* 97 */{"SAX", AddressingMode::ZERO_PAGE_Y, AccessMode::ANXR, RWMode::WRITE, 4, 2},

  /* 98 */{"tya", AddressingMode::IMPLIED,     AccessMode::YR,   RWMode::NONE,  2, 1},
  /* 99 */{"sta", AddressingMode::ABSOLUTE_Y,  AccessMode::AC,   RWMode::WRITE, 5, 3}, /* Absolute,Y */
  /* 9a */{"txs", AddressingMode::IMPLIED,     AccessMode::XR,   RWMode::NONE,  2, 1},
  /*** This is very mysterious command ... */
  /* 9b */{"SHS", AddressingMode::ABSOLUTE_Y,  AccessMode::ANXR, RWMode::WRITE, 5, 3},

  /* 9c */{"SHY", AddressingMode::ABSOLUTE_X,  AccessMode::YR,   RWMode::WRITE, 5, 3},
  /* 9d */{"sta", AddressingMode::ABSOLUTE_X,  AccessMode::AC,   RWMode::WRITE, 5, 3}, /* Absolute,X */
  /* 9e */{"SHX", AddressingMode::ABSOLUTE_Y,  AccessMode::XR  , RWMode::WRITE, 5, 3},
  /* 9f */{"SHA", AddressingMode::ABSOLUTE_Y,  AccessMode::ANXR, RWMode::WRITE, 5, 3},

  /* a0 */{"ldy", AddressingMode::IMMEDIATE,   AccessMode::IMM,  RWMode::READ,  2, 2}, /* Immediate */
  /* a1 */{"lda", AddressingMode::INDIRECT_X,  AccessMode::INDX, RWMode::READ,  6, 2}, /* (indirect,X) */
  /* a2 */{"ldx", AddressingMode::IMMEDIATE,   AccessMode::IMM,  RWMode::READ,  2, 2}, /* Immediate */
  /* a3 */{"LAX", AddressingMode::INDIRECT_X,  AccessMode::INDX, RWMode::READ,  6, 2}, /* (indirect,X) */

  /* a4 */{"ldy", AddressingMode::ZERO_PAGE,   AccessMode::ZERO, RWMode::READ,  3, 2}, /* Zeropage */
  /* a5 */{"lda", AddressingMode::ZERO_PAGE,   AccessMode::ZERO, RWMode::READ,  3, 2}, /* Zeropage */
  /* a6 */{"ldx", AddressingMode::ZERO_PAGE,   AccessMode::ZERO, RWMode::READ,  3, 2}, /* Zeropage */
  /* a7 */{"LAX", AddressingMode::ZERO_PAGE,   AccessMode::ZERO, RWMode::READ,  3, 2},

  /* a8 */{"tay", AddressingMode::IMPLIED,     AccessMode::AC,   RWMode::NONE,  2, 1},
  /* a9 */{"lda", AddressingMode::IMMEDIATE,   AccessMode::IMM,  RWMode::READ,  2, 2}, /* Immediate */
  /* aa */{"tax", AddressingMode::IMPLIED,     AccessMode::AC,   RWMode::NONE,  2, 1},
  /* ab */{"LXA", AddressingMode::IMMEDIATE,   AccessMode::ACIM, RWMode::READ,  2, 2}, /* LXA isn't a typo */

  /* ac */{"ldy", AddressingMode::ABSOLUTE,    AccessMode::ABS,  RWMode::READ,  4, 3}, /* Absolute */
  /* ad */{"lda", AddressingMode::ABSOLUTE,    AccessMode::ABS,  RWMode::READ,  4, 3}, /* Absolute */
  /* ae */{"ldx", AddressingMode::ABSOLUTE,    AccessMode::ABS,  RWMode::READ,  4, 3}, /* Absolute */
  /* af */{"LAX", AddressingMode::ABSOLUTE,    AccessMode::ABS,  RWMode::READ,  4, 3},

  /* b0 */{"bcs", AddressingMode::RELATIVE,    AccessMode::REL,  RWMode::READ,  2, 2},
  /* b1 */{"lda", AddressingMode::INDIRECT_Y,  AccessMode::INDY, RWMode::READ,  5, 2}, /* (indirect),Y */
  /* b2 */{".JAM",AddressingMode::IMPLIED,     AccessMode::NONE, RWMode::NONE,  0, 1}, /* TILT */
  /* b3 */{"LAX", AddressingMode::INDIRECT_Y,  AccessMode::INDY, RWMode::READ,  5, 2},

  /* b4 */{"ldy", AddressingMode::ZERO_PAGE_X, AccessMode::ZERX, RWMode::READ,  4, 2}, /* Zeropage,X */
  /* b5 */{"lda", AddressingMode::ZERO_PAGE_X, AccessMode::ZERX, RWMode::READ,  4, 2}, /* Zeropage,X */
  /* b6 */{"ldx", AddressingMode::ZERO_PAGE_Y, AccessMode::ZERY, RWMode::READ,  4, 2}, /* Zeropage,Y */
  /* b7 */{"LAX", AddressingMode::ZERO_PAGE_Y, AccessMode::ZERY, RWMode::READ,  4, 2},

  /* b8 */{"clv", AddressingMode::IMPLIED,     AccessMode::NONE, RWMode::NONE,  2, 1},
  /* b9 */{"lda", AddressingMode::ABSOLUTE_Y,  AccessMode::ABSY, RWMode::READ,  4, 3}, /* Absolute,Y */
  /* ba */{"tsx", AddressingMode::IMPLIED,     AccessMode::SP,   RWMode::NONE,  2, 1},
  /* bb */{"LAS", AddressingMode::ABSOLUTE_Y,  AccessMode::SABY, RWMode::READ,  4, 3},

  /* bc */{"ldy", AddressingMode::ABSOLUTE_X,  AccessMode::ABSX, RWMode::READ,  4, 3}, /* Absolute,X */
  /* bd */{"lda", AddressingMode::ABSOLUTE_X,  AccessMode::ABSX, RWMode::READ,  4, 3}, /* Absolute,X */
  /* be */{"ldx", AddressingMode::ABSOLUTE_Y,  AccessMode::ABSY, RWMode::READ,  4, 3}, /* Absolute,Y */
  /* bf */{"LAX", AddressingMode::ABSOLUTE_Y,  AccessMode::ABSY, RWMode::READ,  4, 3},

  /* c0 */{"cpy", AddressingMode::IMMEDIATE,   AccessMode::IMM,  RWMode::READ,  2, 2}, /* Immediate */
  /* c1 */{"cmp", AddressingMode::INDIRECT_X,  AccessMode::INDX, RWMode::READ,  6, 2}, /* (Indirect,X) */
  /* c2 */{"NOP", AddressingMode::IMMEDIATE,   AccessMode::NONE, RWMode::NONE,  2, 2}, /* occasional TILT */
  /* c3 */{"DCP", AddressingMode::INDIRECT_X,  AccessMode::INDX, RWMode::WRITE, 8, 2},

  /* c4 */{"cpy", AddressingMode::ZERO_PAGE,   AccessMode::ZERO, RWMode::READ,  3, 2}, /* Zeropage */
  /* c5 */{"cmp", AddressingMode::ZERO_PAGE,   AccessMode::ZERO, RWMode::READ,  3, 2}, /* Zeropage */
  /* c6 */{"dec", AddressingMode::ZERO_PAGE,   AccessMode::ZERO, RWMode::WRITE, 5, 2}, /* Zeropage */
  /* c7 */{"DCP", AddressingMode::ZERO_PAGE,   AccessMode::ZERO, RWMode::WRITE, 5, 2},

  /* c8 */{"iny", AddressingMode::IMPLIED,     AccessMode::YR,   RWMode::NONE,  2, 1},
  /* c9 */{"cmp", AddressingMode::IMMEDIATE,   AccessMode::IMM,  RWMode::READ,  2, 2}, /* Immediate */
  /* ca */{"dex", AddressingMode::IMPLIED,     AccessMode::XR,   RWMode::NONE,  2, 1},
  /* cb */{"SBX", AddressingMode::IMMEDIATE,   AccessMode::IMM,  RWMode::READ,  2, 2},

  /* cc */{"cpy", AddressingMode::ABSOLUTE,    AccessMode::ABS,  RWMode::READ,  4, 3}, /* Absolute */
  /* cd */{"cmp", AddressingMode::ABSOLUTE,    AccessMode::ABS,  RWMode::READ,  4, 3}, /* Absolute */
  /* ce */{"dec", AddressingMode::ABSOLUTE,    AccessMode::ABS,  RWMode::WRITE, 6, 3}, /* Absolute */
  /* cf */{"DCP", AddressingMode::ABSOLUTE,    AccessMode::ABS,  RWMode::WRITE, 6, 3},

  /* d0 */{"bne", AddressingMode::RELATIVE,    AccessMode::REL,  RWMode::READ,  2, 2},
  /* d1 */{"cmp", AddressingMode::INDIRECT_Y,  AccessMode::INDY, RWMode::READ,  5, 2}, /* (Indirect),Y */
  /* d2 */{".JAM",AddressingMode::IMPLIED,     AccessMode::NONE, RWMode::NONE,  0, 1}, /* TILT */
  /* d3 */{"DCP", AddressingMode::INDIRECT_Y,  AccessMode::INDY, RWMode::WRITE, 8, 2},

  /* d4 */{"NOP", AddressingMode::ZERO_PAGE_X, AccessMode::NONE, RWMode::NONE,  4, 2},
  /* d5 */{"cmp", AddressingMode::ZERO_PAGE_X, AccessMode::ZERX, RWMode::READ,  4, 2}, /* Zeropage,X */
  /* d6 */{"dec", AddressingMode::ZERO_PAGE_X, AccessMode::ZERX, RWMode::WRITE, 6, 2}, /* Zeropage,X */
  /* d7 */{"DCP", AddressingMode::ZERO_PAGE_X, AccessMode::ZERX, RWMode::WRITE, 6, 2},

  /* d8 */{"cld", AddressingMode::IMPLIED,     AccessMode::NONE, RWMode::NONE,  2, 1},
  /* d9 */{"cmp", AddressingMode::ABSOLUTE_Y,  AccessMode::ABSY, RWMode::READ,  4, 3}, /* Absolute,Y */
  /* da */{"NOP", AddressingMode::IMPLIED,     AccessMode::NONE, RWMode::NONE,  2, 1},
  /* db */{"DCP", AddressingMode::ABSOLUTE_Y,  AccessMode::ABSY, RWMode::WRITE, 7, 3},

  /* dc */{"NOP", AddressingMode::ABSOLUTE_X,  AccessMode::NONE, RWMode::NONE,  4, 3},
  /* dd */{"cmp", AddressingMode::ABSOLUTE_X,  AccessMode::ABSX, RWMode::READ,  4, 3}, /* Absolute,X */
  /* de */{"dec", AddressingMode::ABSOLUTE_X,  AccessMode::ABSX, RWMode::WRITE, 7, 3}, /* Absolute,X */
  /* df */{"DCP", AddressingMode::ABSOLUTE_X,  AccessMode::ABSX, RWMode::WRITE, 7, 3},

  /* e0 */{"cpx", AddressingMode::IMMEDIATE,   AccessMode::IMM,  RWMode::READ,  2, 2}, /* Immediate */
  /* e1 */{"sbc", AddressingMode::INDIRECT_X,  AccessMode::INDX, RWMode::READ,  6, 2}, /* (Indirect,X) */
  /* e2 */{"NOP", AddressingMode::IMMEDIATE,   AccessMode::NONE, RWMode::NONE,  2, 2},
  /* e3 */{"ISB", AddressingMode::INDIRECT_X,  AccessMode::INDX, RWMode::WRITE, 8, 2},

  /* e4 */{"cpx", AddressingMode::ZERO_PAGE,   AccessMode::ZERO, RWMode::READ,  3, 2}, /* Zeropage */
  /* e5 */{"sbc", AddressingMode::ZERO_PAGE,   AccessMode::ZERO, RWMode::READ,  3, 2}, /* Zeropage */
  /* e6 */{"inc", AddressingMode::ZERO_PAGE,   AccessMode::ZERO, RWMode::WRITE, 5, 2}, /* Zeropage */
  /* e7 */{"ISB", AddressingMode::ZERO_PAGE,   AccessMode::ZERO, RWMode::WRITE, 5, 2},

  /* e8 */{"inx", AddressingMode::IMPLIED,     AccessMode::XR,   RWMode::NONE,  2, 1},
  /* e9 */{"sbc", AddressingMode::IMMEDIATE,   AccessMode::IMM,  RWMode::READ,  2, 2}, /* Immediate */
  /* ea */{"nop", AddressingMode::IMPLIED,     AccessMode::NONE, RWMode::NONE,  2, 1},
  /* eb */{"SBC", AddressingMode::IMMEDIATE,   AccessMode::IMM,  RWMode::READ,  2, 2}, /* same as e9 */

  /* ec */{"cpx", AddressingMode::ABSOLUTE,    AccessMode::ABS,  RWMode::READ,  4, 3}, /* Absolute */
  /* ed */{"sbc", AddressingMode::ABSOLUTE,    AccessMode::ABS,  RWMode::READ,  4, 3}, /* Absolute */
  /* ee */{"inc", AddressingMode::ABSOLUTE,    AccessMode::ABS,  RWMode::WRITE, 6, 3}, /* Absolute */
  /* ef */{"ISB", AddressingMode::ABSOLUTE,    AccessMode::ABS,  RWMode::WRITE, 6, 3},

  /* f0 */{"beq", AddressingMode::RELATIVE,    AccessMode::REL,  RWMode::READ,  2, 2},
  /* f1 */{"sbc", AddressingMode::INDIRECT_Y,  AccessMode::INDY, RWMode::READ,  5, 2}, /* (Indirect),Y */
  /* f2 */{".JAM",AddressingMode::IMPLIED,     AccessMode::NONE, RWMode::NONE,  0, 1}, /* TILT */
  /* f3 */{"ISB", AddressingMode::INDIRECT_Y,  AccessMode::INDY, RWMode::WRITE, 8, 2},

  /* f4 */{"NOP", AddressingMode::ZERO_PAGE_X, AccessMode::NONE, RWMode::NONE,  4, 2},
  /* f5 */{"sbc", AddressingMode::ZERO_PAGE_X, AccessMode::ZERX, RWMode::READ,  4, 2}, /* Zeropage,X */
  /* f6 */{"inc", AddressingMode::ZERO_PAGE_X, AccessMode::ZERX, RWMode::WRITE, 6, 2}, /* Zeropage,X */
  /* f7 */{"ISB", AddressingMode::ZERO_PAGE_X, AccessMode::ZERX, RWMode::WRITE, 6, 2},

  /* f8 */{"sed", AddressingMode::IMPLIED,     AccessMode::NONE, RWMode::NONE,  2, 1},
  /* f9 */{"sbc", AddressingMode::ABSOLUTE_Y,  AccessMode::ABSY, RWMode::READ,  4, 3}, /* Absolute,Y */
  /* fa */{"NOP", AddressingMode::IMPLIED,     AccessMode::NONE, RWMode::NONE,  2, 1},
  /* fb */{"ISB", AddressingMode::ABSOLUTE_Y,  AccessMode::ABSY, RWMode::WRITE, 7, 3},

  /* fc */{"NOP" ,AddressingMode::ABSOLUTE_X,  AccessMode::NONE, RWMode::NONE,  4, 3},
  /* fd */{"sbc", AddressingMode::ABSOLUTE_X,  AccessMode::ABSX, RWMode::READ,  4, 3}, /* Absolute,X */
  /* fe */{"inc", AddressingMode::ABSOLUTE_X,  AccessMode::ABSX, RWMode::WRITE, 7, 3}, /* Absolute,X */
  /* ff */{"ISB", AddressingMode::ABSOLUTE_X,  AccessMode::ABSX, RWMode::WRITE, 7, 3}
} };
