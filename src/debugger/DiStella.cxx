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

#include "bspf.hxx"
#include "Debugger.hxx"
#include "DiStella.hxx"
using Common::Base;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DiStella::DiStella(const CartDebug& dbg, CartDebug::DisassemblyList& list,
                   CartDebug::BankInfo& info, const DiStella::Settings& s,
                   uInt8* labels, uInt8* directives,
                   CartDebug::ReservedEquates& reserved)
  : myDbg(dbg),
    myList(list),
    mySettings(s),
    myReserved(reserved),
    myOffset(0),
    myPC(0),
    myPCEnd(0),
    myLabels(labels),
    myDirectives(directives)
{
  bool resolve_code = mySettings.resolveCode;
  CartDebug::AddressList& debuggerAddresses = info.addressList;
  uInt16 start = *debuggerAddresses.cbegin();

  myOffset = info.offset;
  if (start & 0x1000) {
    info.start = myAppData.start = 0x0000;
    info.end = myAppData.end = info.size - 1;
    // Keep previous offset; it may be different between banks
    if (info.offset == 0)
      info.offset = myOffset = (start - (start % info.size));
  } else { // ZP RAM
    // For now, we assume all accesses below $1000 are zero-page
    info.start = myAppData.start = 0x0080;
    info.end = myAppData.end = 0x00FF;
    info.offset = myOffset = 0;

    // Resolve code is never used in ZP RAM mode
    resolve_code = false;
  }
  myAppData.length = info.size;

  memset(myLabels, 0, 0x1000);
  memset(myDirectives, 0, 0x1000);

  // Process any directives first, as they override automatic code determination
  processDirectives(info.directiveList);

  myReserved.breakFound = false;

  if (resolve_code)
    // First pass
    disasmPass1(info.addressList);

  // Second pass
  disasm(myOffset, 2);

  // Add reserved line equates
  ostringstream reservedLabel;
  for (int k = 0; k <= myAppData.end; k++) {
    if ((myLabels[k] & (CartDebug::REFERENCED | CartDebug::VALID_ENTRY)) == CartDebug::REFERENCED) {
      // If we have a piece of code referenced somewhere else, but cannot
      // locate the label in code (i.e because the address is inside of a
      // multi-byte instruction, then we make note of that address for reference
      //
      // However, we only do this for labels pointing to ROM (above $1000)
      if (myDbg.addressType(k + myOffset) == CartDebug::ADDR_ROM) {
        reservedLabel.str("");
        reservedLabel << "L" << Base::HEX4 << (k + myOffset);
        myReserved.Label.emplace(k + myOffset, reservedLabel.str());
      }
    }
  }

  // Third pass
  disasm(myOffset, 3);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DiStella::disasm(uInt32 distart, int pass)
/*
// Here we have 3 passes:
   - pass 1 tries to detect code and data ranges and labels
   - pass 2 marks valid code
   - pass 3 generates output
*/
{
#define LABEL_A12_HIGH(address) labelA12High(nextLine, opcode, address, labelFound)
#define LABEL_A12_LOW(address)  labelA12Low(nextLine, opcode, address, labelFound)

  uInt8 opcode, d1;
  uInt16 ad;
  Int32 cycles = 0;
  AddressingMode addrMode;
  int labelFound = 0;
  stringstream nextLine, nextLineBytes;

  mySegType = CartDebug::NONE; // create extra lines between code and data

  myDisasmBuf.str("");

  /* pc=myAppData.start; */
  myPC = distart - myOffset;
  while (myPC <= myAppData.end) {

    // since -1 is used in m6502.m4 for clearing the last peek
    // and this results into an access at e.g. 0xffff,
    // we have to fix the consequences here (ugly!).
    if(myPC == myAppData.end)
      goto FIX_LAST;

    if (checkBits(myPC, CartDebug::GFX | CartDebug::PGFX,
        CartDebug::CODE)) {
      if (pass == 2)
        mark(myPC + myOffset, CartDebug::VALID_ENTRY);
      if (pass == 3)
        outputGraphics();
      myPC++;
    } else if (checkBits(myPC, CartDebug::DATA,
               CartDebug::CODE | CartDebug::GFX | CartDebug::PGFX)) {
      if (pass == 2)
        mark(myPC + myOffset, CartDebug::VALID_ENTRY);
      if (pass == 3)
        outputBytes(CartDebug::DATA);
      else
        myPC++;
    } else if (checkBits(myPC, CartDebug::ROW,
               CartDebug::CODE | CartDebug::DATA | CartDebug::GFX | CartDebug::PGFX)) {
FIX_LAST:
      if (pass == 2)
        mark(myPC + myOffset, CartDebug::VALID_ENTRY);

      if (pass == 3)
        outputBytes(CartDebug::ROW);
      else
        myPC++;
    } else {
      // The following sections must be CODE

      // add extra spacing line when switching from non-code to code
      if (pass == 3 && mySegType != CartDebug::CODE && mySegType != CartDebug::NONE) {
        myDisasmBuf << "    '     ' ";
        addEntry(CartDebug::NONE);
        mark(myPC + myOffset, CartDebug::REFERENCED); // add label when switching
      }
      mySegType = CartDebug::CODE;

      /* version 2.1 bug fix */
      if (pass == 2)
        mark(myPC + myOffset, CartDebug::VALID_ENTRY);

      // get opcode
      opcode = Debugger::debugger().peek(myPC + myOffset);
      // get address mode for opcode
      addrMode = ourLookup[opcode].addr_mode;

      if (pass == 3) {
        if (checkBit(myPC, CartDebug::REFERENCED))
          myDisasmBuf << Base::HEX4 << myPC + myOffset << "'L" << Base::HEX4 << myPC + myOffset << "'";
        else
          myDisasmBuf << Base::HEX4 << myPC + myOffset << "'     '";
      }
      myPC++;

      // detect labels inside instructions (e.g. BIT masks)
      labelFound = false;
      for (Uint8 i = 0; i < ourLookup[opcode].bytes - 1; i++) {
        if (checkBit(myPC + i, CartDebug::REFERENCED)) {
          labelFound = true;
          break;
        }
      }
      if (labelFound) {
        if (myOffset >= 0x1000) {
          // the opcode's operand address matches a label address
          if (pass == 3) {
            // output the byte of the opcode incl. cycles
            Uint8 nextOpcode = Debugger::debugger().peek(myPC + myOffset);

            cycles += int(ourLookup[opcode].cycles) - int(ourLookup[nextOpcode].cycles);
            nextLine << ".byte   $" << Base::HEX2 << int(opcode) << " ;";
            nextLine << ourLookup[opcode].mnemonic;

            myDisasmBuf << nextLine.str() << "'" << ";"
              << std::dec << int(ourLookup[opcode].cycles) << "-"
              << std::dec << int(ourLookup[nextOpcode].cycles) << " "
              << "'= " << std::setw(3) << std::setfill(' ') << std::dec << cycles;

            nextLine.str("");
            cycles = 0;
            addEntry(CartDebug::CODE); // add the new found CODE entry
          }
          // continue with the label's opcode
          continue;
        } else {
          if (pass == 3) {
            // TODO
          }
        }
      }

      // Undefined opcodes start with a '.'
      // These are undefined wrt DASM
      if (ourLookup[opcode].mnemonic[0] == '.' && pass == 3) {
        nextLine << ".byte   $" << Base::HEX2 << int(opcode) << " ;";
      }

      if (pass == 3) {
        nextLine << ourLookup[opcode].mnemonic;
        nextLineBytes << Base::HEX2 << int(opcode) << " ";
      }

      // Add operand(s) for PC values outside the app data range
      if (myPC >= myAppData.end) {
        switch (addrMode) {
          case ABSOLUTE:
          case ABSOLUTE_X:
          case ABSOLUTE_Y:
          case INDIRECT_X:
          case INDIRECT_Y:
          case ABS_INDIRECT:
          {
            if (pass == 3) {
              /* Line information is already printed; append .byte since last
                 instruction will put recompilable object larger that original
                 binary file */
              myDisasmBuf << ".byte $" << Base::HEX2 << int(opcode) << "              $"
                << Base::HEX4 << myPC + myOffset << "'"
                << Base::HEX2 << int(opcode);
              addEntry(CartDebug::DATA);

              if (myPC == myAppData.end) {
                if (checkBit(myPC, CartDebug::REFERENCED))
                  myDisasmBuf << Base::HEX4 << myPC + myOffset << "'L" << Base::HEX4 << myPC + myOffset << "'";
                else
                  myDisasmBuf << Base::HEX4 << myPC + myOffset << "'     '";

                opcode = Debugger::debugger().peek(myPC + myOffset);  myPC++;
                myDisasmBuf << ".byte $" << Base::HEX2 << int(opcode) << "              $"
                  << Base::HEX4 << myPC + myOffset << "'"
                  << Base::HEX2 << int(opcode);
                addEntry(CartDebug::DATA);
              }
            }
            myPCEnd = myAppData.end + myOffset;
            return;
          }

          case ZERO_PAGE:
          case IMMEDIATE:
          case ZERO_PAGE_X:
          case ZERO_PAGE_Y:
          case RELATIVE:
          {
            if (pass == 3) {
              /* Line information is already printed, but we can remove the
                  Instruction (i.e. BMI) by simply clearing the buffer to print */
              myDisasmBuf << ".byte $" << Base::HEX2 << int(opcode);
              addEntry(CartDebug::ROW);
              nextLine.str("");
              nextLineBytes.str("");
            }
            myPC++;
            myPCEnd = myAppData.end + myOffset;
            return;
          }

          default:
            break;
        }  // end switch(addr_mode)
      }

      // Add operand(s)
      ad = d1 = 0; // not WSYNC by default!
      /* Version 2.1 added the extensions to mnemonics */
      switch (addrMode) {
        case ACCUMULATOR:
        {
          if (pass == 3 && mySettings.aFlag)
            nextLine << "     A";
          break;
        }

        case ABSOLUTE:
        {
          ad = Debugger::debugger().dpeek(myPC + myOffset);  myPC += 2;
          labelFound = mark(ad, CartDebug::REFERENCED);
          if (pass == 3) {
            if (ad < 0x100 && mySettings.fFlag)
              nextLine << ".w   ";
            else
              nextLine << "     ";

            if (labelFound == 1) {
              LABEL_A12_HIGH(ad);
              nextLineBytes << Base::HEX2 << int(ad & 0xff) << " " << Base::HEX2 << int(ad >> 8);
            } else if (labelFound == 4) {
              if (mySettings.rFlag) {
                int tmp = (ad & myAppData.end) + myOffset;
                LABEL_A12_HIGH(tmp);
                nextLineBytes << Base::HEX2 << int(tmp & 0xff) << " " << Base::HEX2 << int(tmp >> 8);
              } else {
                nextLine << "$" << Base::HEX4 << ad;
                nextLineBytes << Base::HEX2 << int(ad & 0xff) << " " << Base::HEX2 << int(ad >> 8);
              }
            } else {
              LABEL_A12_LOW(ad);
              nextLineBytes << Base::HEX2 << int(ad & 0xff) << " " << Base::HEX2 << int(ad >> 8);
            }
          }
          break;
        }

        case ZERO_PAGE:
        {
          d1 = Debugger::debugger().peek(myPC + myOffset);  myPC++;
          labelFound = mark(d1, CartDebug::REFERENCED);
          if (pass == 3) {
            nextLine << "     ";
            LABEL_A12_LOW(int(d1));
            nextLineBytes << Base::HEX2 << int(d1);
          }
          break;
        }

        case IMMEDIATE:
        {
          d1 = Debugger::debugger().peek(myPC + myOffset);  myPC++;
          if (pass == 3) {
            nextLine << "     #$" << Base::HEX2 << int(d1) << " ";
            nextLineBytes << Base::HEX2 << int(d1);
          }
          break;
        }

        case ABSOLUTE_X:
        {
          ad = Debugger::debugger().dpeek(myPC + myOffset);  myPC += 2;
          labelFound = mark(ad, CartDebug::REFERENCED);
          if (pass == 2 && !checkBit(ad & myAppData.end, CartDebug::CODE)) {
            // Since we can't know what address is being accessed unless we also
            // know the current X value, this is marked as ROW instead of DATA
            // The processing is left here, however, in case future versions of
            // the code can somehow track access to CPU registers
            mark(ad, CartDebug::ROW);
          } else if (pass == 3) {
            if (ad < 0x100 && mySettings.fFlag)
              nextLine << ".wx  ";
            else
              nextLine << "     ";

            if (labelFound == 1) {
              LABEL_A12_HIGH(ad);
              nextLine << ",x";
              nextLineBytes << Base::HEX2 << int(ad & 0xff) << " " << Base::HEX2 << int(ad >> 8);
            } else if (labelFound == 4) {
              if (mySettings.rFlag) {
                int tmp = (ad & myAppData.end) + myOffset;
                LABEL_A12_HIGH(tmp);
                nextLine << ",x";
                nextLineBytes << Base::HEX2 << int(tmp & 0xff) << " " << Base::HEX2 << int(tmp >> 8);
              } else {
                nextLine << "$" << Base::HEX4 << ad << ",x";
                nextLineBytes << Base::HEX2 << int(ad & 0xff) << " " << Base::HEX2 << int(ad >> 8);
              }
            } else {
              LABEL_A12_LOW(ad);
              nextLine << ",x";
              nextLineBytes << Base::HEX2 << int(ad & 0xff) << " " << Base::HEX2 << int(ad >> 8);
            }
          }
          break;
        }

        case ABSOLUTE_Y:
        {
          ad = Debugger::debugger().dpeek(myPC + myOffset);  myPC += 2;
          labelFound = mark(ad, CartDebug::REFERENCED);
          if (pass == 2 && !checkBit(ad & myAppData.end, CartDebug::CODE)) {
            // Since we can't know what address is being accessed unless we also
            // know the current Y value, this is marked as ROW instead of DATA
            // The processing is left here, however, in case future versions of
            // the code can somehow track access to CPU registers
            mark(ad, CartDebug::ROW);
          } else if (pass == 3) {
            if (ad < 0x100 && mySettings.fFlag)
              nextLine << ".wy  ";
            else
              nextLine << "     ";

            if (labelFound == 1) {
              LABEL_A12_HIGH(ad);
              nextLine << ",y";
              nextLineBytes << Base::HEX2 << int(ad & 0xff) << " " << Base::HEX2 << int(ad >> 8);
            } else if (labelFound == 4) {
              if (mySettings.rFlag) {
                int tmp = (ad & myAppData.end) + myOffset;
                LABEL_A12_HIGH(tmp);
                nextLine << ",y";
                nextLineBytes << Base::HEX2 << int(tmp & 0xff) << " " << Base::HEX2 << int(tmp >> 8);
              } else {
                nextLine << "$" << Base::HEX4 << ad << ",y";
                nextLineBytes << Base::HEX2 << int(ad & 0xff) << " " << Base::HEX2 << int(ad >> 8);
              }
            } else {
              LABEL_A12_LOW(ad);
              nextLine << ",y";
              nextLineBytes << Base::HEX2 << int(ad & 0xff) << " " << Base::HEX2 << int(ad >> 8);
            }
          }
          break;
        }

        case INDIRECT_X:
        {
          d1 = Debugger::debugger().peek(myPC + myOffset);  myPC++;
          if (pass == 3) {
            labelFound = mark(d1, 0);  // dummy call to get address type
            nextLine << "     (";
            LABEL_A12_LOW(d1);
            nextLine << ",x)";
            nextLineBytes << Base::HEX2 << int(d1);
          }
          break;
        }

        case INDIRECT_Y:
        {
          d1 = Debugger::debugger().peek(myPC + myOffset);  myPC++;
          if (pass == 3) {
            labelFound = mark(d1, 0);  // dummy call to get address type
            nextLine << "     (";
            LABEL_A12_LOW(d1);
            nextLine << "),y";
            nextLineBytes << Base::HEX2 << int(d1);
          }
          break;
        }

        case ZERO_PAGE_X:
        {
          d1 = Debugger::debugger().peek(myPC + myOffset);  myPC++;
          labelFound = mark(d1, CartDebug::REFERENCED);
          if (pass == 3) {
            nextLine << "     ";
            LABEL_A12_LOW(d1);
            nextLine << ",x";
          }
          nextLineBytes << Base::HEX2 << int(d1);
          break;
        }

        case ZERO_PAGE_Y:
        {
          d1 = Debugger::debugger().peek(myPC + myOffset);  myPC++;
          labelFound = mark(d1, CartDebug::REFERENCED);
          if (pass == 3) {
            nextLine << "     ";
            LABEL_A12_LOW(d1);
            nextLine << ",y";
          }
          nextLineBytes << Base::HEX2 << int(d1);
          break;
        }

        case RELATIVE:
        {
          // SA - 04-06-2010: there seemed to be a bug in distella,
          // where wraparound occurred on a 32-bit int, and subsequent
          // indexing into the labels array caused a crash
          d1 = Debugger::debugger().peek(myPC + myOffset);  myPC++;
          ad = ((myPC + Int8(d1)) & 0xfff) + myOffset;

          labelFound = mark(ad, CartDebug::REFERENCED);
          if (pass == 3) {
            if (labelFound == 1) {
              nextLine << "     ";
              LABEL_A12_HIGH(ad);
            } else
              nextLine << "     $" << Base::HEX4 << ad;

            nextLineBytes << Base::HEX2 << int(d1);
          }
          break;
        }

        case ABS_INDIRECT:
        {
          ad = Debugger::debugger().dpeek(myPC + myOffset);  myPC += 2;
          labelFound = mark(ad, CartDebug::REFERENCED);
          if (pass == 2 && !checkBit(ad & myAppData.end, CartDebug::CODE)) {
            // Since we can't know what address is being accessed unless we also
            // know the current X value, this is marked as ROW instead of DATA
            // The processing is left here, however, in case future versions of
            // the code can somehow track access to CPU registers
            mark(ad, CartDebug::ROW);
          } else if (pass == 3) {
            if (ad < 0x100 && mySettings.fFlag)
              nextLine << ".ind ";
            else
              nextLine << "     ";
          }
          if (labelFound == 1) {
            nextLine << "(";
            LABEL_A12_HIGH(ad);
            nextLine << ")";
          }
          // TODO - should we consider case 4??
          else {
            nextLine << "(";
            LABEL_A12_LOW(ad);
            nextLine << ")";
          }

          nextLineBytes << Base::HEX2 << int(ad & 0xff) << " " << Base::HEX2 << int(ad >> 8);
          break;
        }

        default:
          break;
      } // end switch

      if (pass == 3) {
        cycles += int(ourLookup[opcode].cycles);
        // A complete line of disassembly (text, cycle count, and bytes)
        myDisasmBuf << nextLine.str() << "'"
          << ";" << std::dec << int(ourLookup[opcode].cycles)
          << (addrMode == RELATIVE ? (ad & 0xf00) != ((myPC + myOffset) & 0xf00) ? "/3!" : "/3 " : "   ");
        if ((opcode == 0x40 || opcode == 0x60 || opcode == 0x4c || opcode == 0x00 // code block end
            || checkBit(myPC, CartDebug::REFERENCED)                              // referenced address
            || (ourLookup[opcode].rw_mode == WRITE && d1 == WSYNC))               // strobe WSYNC
            && cycles > 0) {
          // output cycles for previous code block
          myDisasmBuf << "'= " << std::setw(3) << std::setfill(' ') << std::dec << cycles;
          cycles = 0;
        } else {
          myDisasmBuf << "'     ";
        }
        myDisasmBuf << "'" << nextLineBytes.str();

        addEntry(CartDebug::CODE);
        if (opcode == 0x40 || opcode == 0x60 || opcode == 0x4c || opcode == 0x00) {
          myDisasmBuf << "    '     ' ";
          addEntry(CartDebug::NONE);
          mySegType = CartDebug::NONE; // prevent extra lines if data follows
        }

        nextLine.str("");
        nextLineBytes.str("");
      }
    }
  }  /* while loop */

  /* Just in case we are disassembling outside of the address range, force the myPCEnd to EOF */
  myPCEnd = myAppData.end + myOffset;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DiStella::disasmPass1(CartDebug::AddressList& debuggerAddresses)
{
  auto it = debuggerAddresses.cbegin();
  uInt16 start = *it++;

  // After we've disassembled from all addresses in the address list,
  // use all access points determined by Stella during emulation
  int codeAccessPoint = 0;

  // Sometimes we get a circular reference, in that processing a certain
  // PC address leads us to a sequence of addresses that end up trying
  // to process the same address again.  We detect such consecutive PC
  // addresses and only process the first one
  uInt16 lastPC = 0;
  bool duplicateFound = false;

  while (!myAddressQueue.empty())
    myAddressQueue.pop();
  myAddressQueue.push(start);

  while (!(myAddressQueue.empty() || duplicateFound)) {
    uInt16 pcBeg = myPC = lastPC = myAddressQueue.front();
    myAddressQueue.pop();

    disasmFromAddress(myPC);

    if (pcBeg <= myPCEnd) {
      // Tentatively mark all addresses in the range as CODE
      // Note that this is a 'best-effort' approach, since
      // Distella will normally keep going until the end of the
      // range or branch is encountered
      // However, addresses *specifically* marked as DATA/GFX/PGFX
      // in the emulation core indicate that the CODE range has finished
      // Therefore, we stop at the first such address encountered
      for (uInt32 k = pcBeg; k <= myPCEnd; k++) {
        if (checkBits(k, CartDebug::CartDebug::DATA | CartDebug::GFX | CartDebug::PGFX,
                      CartDebug::CODE)) {
          //if (Debugger::debugger().getAccessFlags(k) &
          //    (CartDebug::DATA | CartDebug::GFX | CartDebug::PGFX)) {
          // TODO: this should never happen, remove when we are sure
          // TODO: NOT USED: uInt8 flags = Debugger::debugger().getAccessFlags(k);
          myPCEnd = k - 1;
          break;
        }
        mark(k, CartDebug::CODE);
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
    // the ::disasm method
    // All of these have to be exhausted before considering a new address
    while (myAddressQueue.empty() && it != debuggerAddresses.end()) {
      uInt16 addr = *it;

      if (!checkBit(addr - myOffset, CartDebug::CODE)) {
        myAddressQueue.push(addr);
        ++it;
      } else // remove this address, it is redundant
        it = debuggerAddresses.erase(it);
    }

    // Stella itself can provide hints on whether an address has ever
    // been referenced as CODE
    while (myAddressQueue.empty() && codeAccessPoint <= myAppData.end) {
      if ((Debugger::debugger().getAccessFlags(codeAccessPoint + myOffset) & CartDebug::CODE)
          && !(myLabels[codeAccessPoint & myAppData.end] & CartDebug::CODE)) {
        myAddressQueue.push(codeAccessPoint + myOffset);
        ++codeAccessPoint;
        break;
      }
      ++codeAccessPoint;
    }
    duplicateFound = !myAddressQueue.empty() && (myAddressQueue.front() == lastPC); // TODO: check!
  } // while

  for (int k = 0; k <= myAppData.end; k++) {
    // Let the emulation core know about tentative code
    if (checkBit(k, CartDebug::CODE) &&
      !(Debugger::debugger().getAccessFlags(k + myOffset) & CartDebug::CODE)
      && myOffset != 0) {
      Debugger::debugger().setAccessFlags(k + myOffset, CartDebug::TCODE);
    }

    // Must be ROW / unused bytes
    if (!checkBit(k, CartDebug::CODE | CartDebug::GFX |
        CartDebug::PGFX | CartDebug::DATA))
      mark(k + myOffset, CartDebug::ROW);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DiStella::disasmFromAddress(uInt32 distart)
{
  uInt8 opcode, d1;
  uInt16 ad;
  AddressingMode addrMode;

  myPC = distart - myOffset;

  while (myPC <= myAppData.end) {

    // abort when we reach non-code areas
    if (checkBits(myPC, CartDebug::CartDebug::DATA | CartDebug::GFX | CartDebug::PGFX, CartDebug::CODE)) {
      myPCEnd = (myPC - 1) + myOffset;
      return;
    }

    // so this should be code now...
    // get opcode
    opcode = Debugger::debugger().peek(myPC + myOffset);  myPC++;
    // get address mode for opcode
    addrMode = ourLookup[opcode].addr_mode;

    // Add operand(s) for PC values outside the app data range
    if (myPC >= myAppData.end) {
      switch (addrMode) {
        case ABSOLUTE:
        case ABSOLUTE_X:
        case ABSOLUTE_Y:
        case INDIRECT_X:
        case INDIRECT_Y:
        case ABS_INDIRECT:
          myPCEnd = myAppData.end + myOffset;
          return;

        case ZERO_PAGE:
        case IMMEDIATE:
        case ZERO_PAGE_X:
        case ZERO_PAGE_Y:
        case RELATIVE:
          if (myPC > myAppData.end) {
            myPC++;
            myPCEnd = myAppData.end + myOffset;
            return;
          }
          break;  // TODO - is this the intent?

        default:
          break;
      }  // end switch(addr_mode)
    } // end if (myPC >= myAppData.end)

    // Add operand(s)
    switch (addrMode) {
      case ABSOLUTE:
        ad = Debugger::debugger().dpeek(myPC + myOffset);  myPC += 2;
        mark(ad, CartDebug::REFERENCED);
        // handle JMP/JSR
        if (ourLookup[opcode].source == M_ADDR) {
          // do NOT use flags set by debugger, else known CODE will not analyzed statically.
          if (!checkBit(ad & myAppData.end, CartDebug::CODE, false)) {
            if (ad > 0xfff)
              myAddressQueue.push((ad & myAppData.end) + myOffset);
            mark(ad, CartDebug::CODE);
          }
        } else
          mark(ad, CartDebug::DATA);
        break;

      case ZERO_PAGE:
        d1 = Debugger::debugger().peek(myPC + myOffset);  myPC++;
        mark(d1, CartDebug::REFERENCED);
        break;

      case IMMEDIATE:
        myPC++;
        break;

      case ABSOLUTE_X:
        ad = Debugger::debugger().dpeek(myPC + myOffset);  myPC += 2;
        mark(ad, CartDebug::REFERENCED);
        break;

      case ABSOLUTE_Y:
        ad = Debugger::debugger().dpeek(myPC + myOffset);  myPC += 2;
        mark(ad, CartDebug::REFERENCED);
        break;

      case INDIRECT_X:
        myPC++;
        break;

      case INDIRECT_Y:
        myPC++;
        break;

      case ZERO_PAGE_X:
        d1 = Debugger::debugger().peek(myPC + myOffset);  myPC++;
        mark(d1, CartDebug::REFERENCED);
        break;

      case ZERO_PAGE_Y:
        d1 = Debugger::debugger().peek(myPC + myOffset);  myPC++;
        mark(d1, CartDebug::REFERENCED);
        break;

      case RELATIVE:
        // SA - 04-06-2010: there seemed to be a bug in distella,
        // where wraparound occurred on a 32-bit int, and subsequent
        // indexing into the labels array caused a crash
        d1 = Debugger::debugger().peek(myPC + myOffset);  myPC++;
        ad = ((myPC + Int8(d1)) & 0xfff) + myOffset;
        mark(ad, CartDebug::REFERENCED);
        // do NOT use flags set by debugger, else known CODE will not analyzed statically.
        if (!checkBit(ad - myOffset, CartDebug::CODE, false)) {
          myAddressQueue.push(ad);
          mark(ad, CartDebug::CODE);
        }
        break;

      case ABS_INDIRECT:
        ad = Debugger::debugger().dpeek(myPC + myOffset);  myPC += 2;
        mark(ad, CartDebug::REFERENCED);
        break;

      default:
        break;
    } // end switch

    // mark BRK vector
    if (opcode == 0x00) {
      ad = Debugger::debugger().dpeek(0xfffe, CartDebug::DATA);
      if (!myReserved.breakFound) {
        myAddressQueue.push(ad);
        mark(ad, CartDebug::CODE);
        myReserved.breakFound = true;
      }
    }

    // JMP/RTS/RTI always indicate the end of a block of CODE
    if (opcode == 0x4c || opcode == 0x60 || opcode == 0x40) {
      // code block end
      myPCEnd = (myPC - 1) + myOffset;
      return;
    }
  } // while
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int DiStella::mark(uInt32 address, uInt8 mask, bool directive)
{
  /*-----------------------------------------------------------------------
    For any given offset and code range...

    If we're between the offset and the end of the code range, we mark
    the bit in the labels array for that data.  The labels array is an
    array of label info for each code address.  If this is the case,
    return "1", else...

    We sweep for hardware/system equates, which are valid addresses,
    outside the scope of the code/data range.  For these, we mark its
    corresponding hardware/system array element, and return "2" or "3"
    (depending on which system/hardware element was accessed).
    If this was not the case...

    Next we check if it is a code "mirror".  For the 2600, address ranges
    are limited with 13 bits, so other addresses can exist outside of the
    standard code/data range.  For these, we mark the element in the "labels"
    array that corresponds to the mirrored address, and return "4"

    If all else fails, it's not a valid address, so return 0.

    A quick example breakdown for a 2600 4K cart:
    ===========================================================
      $00-$3d     = system equates (WSYNC, etc...); return 2.
      $80-$ff     = zero-page RAM (ram_80, etc...); return 5.
      $0280-$0297 = system equates (INPT0, etc...); mark the array's element
                    with the appropriate bit; return 3.
      $1000-$1FFF = mark the code/data array for the mirrored address
                    with the appropriate bit; return 4.
      $3000-$3FFF = mark the code/data array for the mirrored address
                    with the appropriate bit; return 4.
      $5000-$5FFF = mark the code/data array for the mirrored address
                    with the appropriate bit; return 4.
      $7000-$7FFF = mark the code/data array for the mirrored address
                    with the appropriate bit; return 4.
      $9000-$9FFF = mark the code/data array for the mirrored address
                    with the appropriate bit; return 4.
      $B000-$BFFF = mark the code/data array for the mirrored address
                    with the appropriate bit; return 4.
      $D000-$DFFF = mark the code/data array for the mirrored address
                    with the appropriate bit; return 4.
      $F000-$FFFF = mark the code/data array for the address
                    with the appropriate bit; return 1.
      Anything else = invalid, return 0.
    ===========================================================
  -----------------------------------------------------------------------*/

  // Check for equates before ROM/ZP-RAM accesses, because the original logic
  // of Distella assumed either equates or ROM; it didn't take ZP-RAM into account
  CartDebug::AddrType type = myDbg.addressType(address);
  if (type == CartDebug::ADDR_TIA) {
    return 2;
  } else if (type == CartDebug::ADDR_IO) {
    return 3;
  } else if (type == CartDebug::ADDR_ZPRAM && myOffset != 0) {
    return 5;
  } else if (address >= myOffset && address <= myAppData.end + myOffset) {
    myLabels[address - myOffset] = myLabels[address - myOffset] | mask;
    if (directive)  myDirectives[address - myOffset] = mask;
    return 1;
  } else if (address > 0x1000 && myOffset != 0)  // Exclude zero-page accesses
  {
    /* 2K & 4K case */
    myLabels[address & myAppData.end] = myLabels[address & myAppData.end] | mask;
    if (directive)  myDirectives[address & myAppData.end] = mask;
    return 4;
  } else
    return 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool DiStella::checkBit(uInt16 address, uInt8 mask, bool useDebugger) const
{
  // The REFERENCED and VALID_ENTRY flags are needed for any inspection of
  // an address
  // Since they're set only in the labels array (as the lower two bits),
  // they must be included in the other bitfields
  uInt8 label = myLabels[address & myAppData.end],
    lastbits = label & 0x03,
    directive = myDirectives[address & myAppData.end] & 0xFC,
    debugger = Debugger::debugger().getAccessFlags(address | myOffset) & 0xFC;

  // Any address marked by a manual directive always takes priority
  if (directive)
    return (directive | lastbits) & mask;
  // Next, the results from a dynamic/runtime analysis are used (except for pass 1)
  else if (useDebugger && ((debugger | lastbits) & mask))
    return true;
  // Otherwise, default to static analysis from Distella
  else
    return label & mask;
}

bool DiStella::checkBits(uInt16 address, uInt8 mask, uInt8 notMask, bool useDebugger) const
{
  return checkBit(address, mask, useDebugger) && !checkBit(address, notMask, useDebugger);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool DiStella::check_range(uInt16 beg, uInt16 end) const
{
  if (beg > end) {
    cerr << "Beginning of range greater than end: start = " << std::hex << beg
      << ", end = " << std::hex << end << endl;
    return false;
  } else if (beg > myAppData.end + myOffset) {
    cerr << "Beginning of range out of range: start = " << std::hex << beg
      << ", range = " << std::hex << (myAppData.end + myOffset) << endl;
    return false;
  } else if (beg < myOffset) {
    cerr << "Beginning of range out of range: start = " << std::hex << beg
      << ", offset = " << std::hex << myOffset << endl;
    return false;
  }
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DiStella::addEntry(CartDebug::DisasmType type)
{
  CartDebug::DisassemblyTag tag;

  // Type
  tag.type = type;

  // Address
  myDisasmBuf.seekg(0, std::ios::beg);
  if (myDisasmBuf.peek() == ' ')
    tag.address = 0;
  else
    myDisasmBuf >> std::setw(4) >> std::hex >> tag.address;

  // Only include addresses within the requested range
  if (tag.address < myAppData.start)
    goto DONE_WITH_ADD;

  // Label (a user-defined label always overrides any auto-generated one)
  myDisasmBuf.seekg(5, std::ios::beg);
  if (tag.address) {
    tag.label = myDbg.getLabel(tag.address, true);
    tag.hllabel = true;
    if (tag.label == EmptyString) {
      if (myDisasmBuf.peek() != ' ')
        getline(myDisasmBuf, tag.label, '\'');
      else if (mySettings.showAddresses && tag.type == CartDebug::CODE) {
        // Have addresses indented, to differentiate from actual labels
        tag.label = " " + Base::toString(tag.address, Base::F_16_4);
        tag.hllabel = false;
      }
    }
  }

  // Disassembly
  // Up to this point the field sizes are fixed, until we get to
  // variable length labels, cycle counts, etc
  myDisasmBuf.seekg(11, std::ios::beg);
  switch (tag.type) {
    case CartDebug::CODE:
      getline(myDisasmBuf, tag.disasm, '\'');
      getline(myDisasmBuf, tag.ccount, '\'');
      getline(myDisasmBuf, tag.ctotal, '\'');
      getline(myDisasmBuf, tag.bytes);

      // Make note of when we override CODE sections from the debugger
      // It could mean that the code hasn't been accessed up to this point,
      // but it could also indicate that code will *never* be accessed
      // Since it is impossible to tell the difference, marking the address
      // in the disassembly at least tells the user about it
      if (!(Debugger::debugger().getAccessFlags(tag.address) & CartDebug::CODE)
          && myOffset != 0) {
        tag.ccount += " *";
        Debugger::debugger().setAccessFlags(tag.address, CartDebug::TCODE);
      }
      break;
    case CartDebug::GFX:
    case CartDebug::PGFX:
      getline(myDisasmBuf, tag.disasm, '\'');
      getline(myDisasmBuf, tag.bytes);
      break;
    case CartDebug::DATA:
      getline(myDisasmBuf, tag.disasm, '\'');
      getline(myDisasmBuf, tag.bytes);
      break;
    case CartDebug::ROW:
      getline(myDisasmBuf, tag.disasm);
      break;
    case CartDebug::NONE:
    default:  // should never happen
      tag.disasm = " ";
      break;
  }
  myList.push_back(tag);

DONE_WITH_ADD:
  myDisasmBuf.clear();
  myDisasmBuf.str("");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DiStella::outputGraphics()
{
  bool isPGfx = checkBit(myPC, CartDebug::PGFX);
  const string& bitString = isPGfx ? "\x1f" : "\x1e";
  uInt8 byte = Debugger::debugger().peek(myPC + myOffset);

  // add extra spacing line when switching from non-graphics to graphics
  if (mySegType != CartDebug::GFX && mySegType != CartDebug::NONE) {
    myDisasmBuf << "    '     ' ";
    addEntry(CartDebug::NONE);
  }
  mySegType = CartDebug::GFX;

  if (checkBit(myPC, CartDebug::REFERENCED))
    myDisasmBuf << Base::HEX4 << myPC + myOffset << "'L" << Base::HEX4 << myPC + myOffset << "'";
  else
    myDisasmBuf << Base::HEX4 << myPC + myOffset << "'     '";
  myDisasmBuf << ".byte $" << Base::HEX2 << int(byte) << "  |";
  for (uInt8 i = 0, c = byte; i < 8; ++i, c <<= 1)
    myDisasmBuf << ((c > 127) ? bitString : " ");
  myDisasmBuf << "|  $" << Base::HEX4 << myPC + myOffset << "'";
  if (mySettings.gfxFormat == Base::F_2)
    myDisasmBuf << Base::toString(byte, Base::F_2_8);
  else
    myDisasmBuf << Base::HEX2 << int(byte);

  addEntry(isPGfx ? CartDebug::PGFX : CartDebug::GFX);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DiStella::outputBytes(CartDebug::DisasmType type)
{
  bool isType = true;
  bool referenced = checkBit(myPC, CartDebug::REFERENCED);
  bool lineEmpty = true;
  int numBytes = 0;

  // add extra spacing line when switching from non-data to data
  if (mySegType != CartDebug::DATA && mySegType != CartDebug::NONE) {
    myDisasmBuf << "    '     ' ";
    addEntry(CartDebug::NONE);
  }
  mySegType = CartDebug::DATA;

  while (isType && myPC <= myAppData.end) {
    if (referenced) {
      // start a new line with a label
      if (!lineEmpty)
        addEntry(type);

      myDisasmBuf << Base::HEX4 << myPC + myOffset << "'L" << Base::HEX4
        << myPC + myOffset << "'.byte " << "$" << Base::HEX2
        << int(Debugger::debugger().peek(myPC + myOffset));
      myPC++;
      numBytes = 1;
      lineEmpty = false;
    } else if (lineEmpty) {
      // start a new line without a label
      myDisasmBuf << Base::HEX4 << myPC + myOffset << "'     '"
        << ".byte $" << Base::HEX2 << int(Debugger::debugger().peek(myPC + myOffset));
      myPC++;
      numBytes = 1;
      lineEmpty = false;
    }
    // Otherwise, append bytes to the current line, up until the maximum
    else if (++numBytes == mySettings.bytesWidth) {
      addEntry(type);
      lineEmpty = true;
    } else {
      myDisasmBuf << ",$" << Base::HEX2 << int(Debugger::debugger().peek(myPC + myOffset));
      myPC++;
    }
    isType = checkBits(myPC, type,
                        CartDebug::CODE | (type != CartDebug::DATA ? CartDebug::DATA : 0) | CartDebug::GFX | CartDebug::PGFX);
    referenced = checkBit(myPC, CartDebug::REFERENCED);
  }
  if (!lineEmpty)
    addEntry(type);
  /*myDisasmBuf << "    '     ' ";
  addEntry(CartDebug::NONE);*/
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DiStella::processDirectives(const CartDebug::DirectiveList& directives)
{
  for (const auto& tag : directives) {
    if (check_range(tag.start, tag.end))
      for (uInt32 k = tag.start; k <= tag.end; ++k)
        mark(k, tag.type, true);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DiStella::Settings DiStella::settings = {
  Base::F_2, // gfxFormat
  true,      // resolveCode (opposite of -d in Distella)
  true,      // showAddresses (not used externally; always off)
  false,     // aFlag (-a in Distella)
  true,      // fFlag (-f in Distella)
  false,     // rFlag (-r in Distella)
  false,     // bFlag (-b in Distella)
  8+1        // number of bytes to use with .byte directive
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const DiStella::Instruction_tag DiStella::ourLookup[256] = {
  /****  Positive  ****/

  /* 00 */{"brk", IMPLIED,     M_NONE, NONE,  7, 1}, /* Pseudo Absolute */
  /* 01 */{"ora", INDIRECT_X,  M_INDX, READ,  6, 2}, /* (Indirect,X) */
  /* 02 */{".JAM",IMPLIED,     M_NONE, NONE,  0, 1}, /* TILT */
  /* 03 */{"SLO", INDIRECT_X,  M_INDX, WRITE, 8, 2},

  /* 04 */{"NOP", ZERO_PAGE,   M_NONE, NONE,  3, 2},
  /* 05 */{"ora", ZERO_PAGE,   M_ZERO, READ,  3, 2}, /* Zeropage */
  /* 06 */{"asl", ZERO_PAGE,   M_ZERO, WRITE, 5, 2}, /* Zeropage */
  /* 07 */{"SLO", ZERO_PAGE,   M_ZERO, WRITE, 5, 2},

  /* 08 */{"php", IMPLIED,     M_SR,   NONE,  3, 1},
  /* 09 */{"ora", IMMEDIATE,   M_IMM,  READ,  2, 2}, /* Immediate */
  /* 0a */{"asl", ACCUMULATOR, M_AC,   WRITE, 2, 1}, /* Accumulator */
  /* 0b */{"ANC", IMMEDIATE,   M_ACIM, READ,  2, 2},

  /* 0c */{"NOP", ABSOLUTE,    M_NONE, NONE,  4, 3},
  /* 0d */{"ora", ABSOLUTE,    M_ABS,  READ,  4, 3}, /* Absolute */
  /* 0e */{"asl", ABSOLUTE,    M_ABS,  WRITE, 6, 3}, /* Absolute */
  /* 0f */{"SLO", ABSOLUTE,    M_ABS,  WRITE, 6, 3},

  /* 10 */{"bpl", RELATIVE,    M_REL,  READ,  2, 2},
  /* 11 */{"ora", INDIRECT_Y,  M_INDY, READ,  5, 2}, /* (Indirect),Y */
  /* 12 */{".JAM",IMPLIED,     M_NONE, NONE,  0, 1}, /* TILT */
  /* 13 */{"SLO", INDIRECT_Y,  M_INDY, WRITE, 8, 2},

  /* 14 */{"NOP", ZERO_PAGE_X, M_NONE, NONE,  4, 2},
  /* 15 */{"ora", ZERO_PAGE_X, M_ZERX, READ,  4, 2}, /* Zeropage,X */
  /* 16 */{"asl", ZERO_PAGE_X, M_ZERX, WRITE, 6, 2}, /* Zeropage,X */
  /* 17 */{"SLO", ZERO_PAGE_X, M_ZERX, WRITE, 6, 2},

  /* 18 */{"clc", IMPLIED,     M_NONE, NONE,  2, 1},
  /* 19 */{"ora", ABSOLUTE_Y,  M_ABSY, READ,  4, 3}, /* Absolute,Y */
  /* 1a */{"NOP", IMPLIED,     M_NONE, NONE,  2, 1},
  /* 1b */{"SLO", ABSOLUTE_Y,  M_ABSY, WRITE, 7, 3},

  /* 1c */{"NOP", ABSOLUTE_X,  M_NONE, NONE,  4, 3},
  /* 1d */{"ora", ABSOLUTE_X,  M_ABSX, READ,  4, 3}, /* Absolute,X */
  /* 1e */{"asl", ABSOLUTE_X,  M_ABSX, WRITE, 7, 3}, /* Absolute,X */
  /* 1f */{"SLO", ABSOLUTE_X,  M_ABSX, WRITE, 7, 3},

  /* 20 */{"jsr", ABSOLUTE,    M_ADDR, READ,  6, 3},
  /* 21 */{"and", INDIRECT_X,  M_INDX, READ,  6, 2}, /* (Indirect ,X) */
  /* 22 */{".JAM",IMPLIED,     M_NONE, NONE,  0, 1}, /* TILT */
  /* 23 */{"RLA", INDIRECT_X,  M_INDX, WRITE, 8, 2},

  /* 24 */{"bit", ZERO_PAGE,   M_ZERO, READ,  3, 2}, /* Zeropage */
  /* 25 */{"and", ZERO_PAGE,   M_ZERO, READ,  3, 2}, /* Zeropage */
  /* 26 */{"rol", ZERO_PAGE,   M_ZERO, WRITE, 5, 2}, /* Zeropage */
  /* 27 */{"RLA", ZERO_PAGE,   M_ZERO, WRITE, 5, 2},

  /* 28 */{"plp", IMPLIED,     M_NONE, NONE,  4, 1},
  /* 29 */{"and", IMMEDIATE,   M_IMM,  READ,  2, 2}, /* Immediate */
  /* 2a */{"rol", ACCUMULATOR, M_AC,   WRITE, 2, 1}, /* Accumulator */
  /* 2b */{"ANC", IMMEDIATE,   M_ACIM, READ,  2, 2},

  /* 2c */{"bit", ABSOLUTE,    M_ABS,  READ,  4, 3}, /* Absolute */
  /* 2d */{"and", ABSOLUTE,    M_ABS,  READ,  4, 3}, /* Absolute */
  /* 2e */{"rol", ABSOLUTE,    M_ABS,  WRITE, 6, 3}, /* Absolute */
  /* 2f */{"RLA", ABSOLUTE,    M_ABS,  WRITE, 6, 3},

  /* 30 */{"bmi", RELATIVE,    M_REL,  READ,  2, 2},
  /* 31 */{"and", INDIRECT_Y,  M_INDY, READ,  5, 2}, /* (Indirect),Y */
  /* 32 */{".JAM",IMPLIED,     M_NONE, NONE,  0, 1}, /* TILT */
  /* 33 */{"RLA", INDIRECT_Y,  M_INDY, WRITE, 8, 2},

  /* 34 */{"NOP", ZERO_PAGE_X, M_NONE, NONE,  4, 2},
  /* 35 */{"and", ZERO_PAGE_X, M_ZERX, READ,  4, 2}, /* Zeropage,X */
  /* 36 */{"rol", ZERO_PAGE_X, M_ZERX, WRITE, 6, 2}, /* Zeropage,X */
  /* 37 */{"RLA", ZERO_PAGE_X, M_ZERX, WRITE, 6, 2},

  /* 38 */{"sec", IMPLIED,     M_NONE, NONE,  2, 1},
  /* 39 */{"and", ABSOLUTE_Y,  M_ABSY, READ,  4, 3}, /* Absolute,Y */
  /* 3a */{"NOP", IMPLIED,     M_NONE, NONE,  2, 1},
  /* 3b */{"RLA", ABSOLUTE_Y,  M_ABSY, WRITE, 7, 3},

  /* 3c */{"NOP", ABSOLUTE_X,  M_NONE, NONE,  4, 3},
  /* 3d */{"and", ABSOLUTE_X,  M_ABSX, READ,  4, 3}, /* Absolute,X */
  /* 3e */{"rol", ABSOLUTE_X,  M_ABSX, WRITE, 7, 3}, /* Absolute,X */
  /* 3f */{"RLA", ABSOLUTE_X,  M_ABSX, WRITE, 7, 3},

  /* 40 */{"rti", IMPLIED,     M_NONE, NONE,  6, 1},
  /* 41 */{"eor", INDIRECT_X,  M_INDX, READ,  6, 2}, /* (Indirect,X) */
  /* 42 */{".JAM",IMPLIED,     M_NONE, NONE,  0, 1}, /* TILT */
  /* 43 */{"SRE", INDIRECT_X,  M_INDX, WRITE, 8, 2},

  /* 44 */{"NOP", ZERO_PAGE,   M_NONE, NONE,  3, 2},
  /* 45 */{"eor", ZERO_PAGE,   M_ZERO, READ,  3, 2}, /* Zeropage */
  /* 46 */{"lsr", ZERO_PAGE,   M_ZERO, WRITE, 5, 2}, /* Zeropage */
  /* 47 */{"SRE", ZERO_PAGE,   M_ZERO, WRITE, 5, 2},

  /* 48 */{"pha", IMPLIED,     M_AC,   NONE,  3, 1},
  /* 49 */{"eor", IMMEDIATE,   M_IMM,  READ,  2, 2}, /* Immediate */
  /* 4a */{"lsr", ACCUMULATOR, M_AC,   WRITE, 2, 1}, /* Accumulator */
  /* 4b */{"ASR", IMMEDIATE,   M_ACIM, READ,  2, 2}, /* (AC & IMM) >>1 */

  /* 4c */{"jmp", ABSOLUTE,    M_ADDR, READ,  3, 3}, /* Absolute */
  /* 4d */{"eor", ABSOLUTE,    M_ABS,  READ,  4, 3}, /* Absolute */
  /* 4e */{"lsr", ABSOLUTE,    M_ABS,  WRITE, 6, 3}, /* Absolute */
  /* 4f */{"SRE", ABSOLUTE,    M_ABS,  WRITE, 6, 3},

  /* 50 */{"bvc", RELATIVE,    M_REL,  READ,  2, 2},
  /* 51 */{"eor", INDIRECT_Y,  M_INDY, READ,  5, 2}, /* (Indirect),Y */
  /* 52 */{".JAM",IMPLIED,     M_NONE, NONE,  0, 1}, /* TILT */
  /* 53 */{"SRE", INDIRECT_Y,  M_INDY, WRITE, 8, 2},

  /* 54 */{"NOP", ZERO_PAGE_X, M_NONE, NONE,  4, 2},
  /* 55 */{"eor", ZERO_PAGE_X, M_ZERX, READ,  4, 2}, /* Zeropage,X */
  /* 56 */{"lsr", ZERO_PAGE_X, M_ZERX, WRITE, 6, 2}, /* Zeropage,X */
  /* 57 */{"SRE", ZERO_PAGE_X, M_ZERX, WRITE, 6, 2},

  /* 58 */{"cli", IMPLIED,     M_NONE, NONE,  2, 1},
  /* 59 */{"eor", ABSOLUTE_Y,  M_ABSY, READ,  4, 3}, /* Absolute,Y */
  /* 5a */{"NOP", IMPLIED,     M_NONE, NONE,  2, 1},
  /* 5b */{"SRE", ABSOLUTE_Y,  M_ABSY, WRITE, 7, 3},

  /* 5c */{"NOP", ABSOLUTE_X,  M_NONE, NONE,  4, 3},
  /* 5d */{"eor", ABSOLUTE_X,  M_ABSX, READ,  4, 3}, /* Absolute,X */
  /* 5e */{"lsr", ABSOLUTE_X,  M_ABSX, WRITE, 7, 3}, /* Absolute,X */
  /* 5f */{"SRE", ABSOLUTE_X,  M_ABSX, WRITE, 7, 3},

  /* 60 */{"rts", IMPLIED,     M_NONE, NONE,  6, 1},
  /* 61 */{"adc", INDIRECT_X,  M_INDX, READ,  6, 2}, /* (Indirect,X) */
  /* 62 */{".JAM",IMPLIED,     M_NONE, NONE,  0, 1}, /* TILT */
  /* 63 */{"RRA", INDIRECT_X,  M_INDX, WRITE, 8, 2},

  /* 64 */{"NOP", ZERO_PAGE,   M_NONE, NONE,  3, 2},
  /* 65 */{"adc", ZERO_PAGE,   M_ZERO, READ,  3, 2}, /* Zeropage */
  /* 66 */{"ror", ZERO_PAGE,   M_ZERO, WRITE, 5, 2}, /* Zeropage */
  /* 67 */{"RRA", ZERO_PAGE,   M_ZERO, WRITE, 5, 2},

  /* 68 */{"pla", IMPLIED,     M_NONE, NONE,  4, 1},
  /* 69 */{"adc", IMMEDIATE,   M_IMM,  READ,  2, 2}, /* Immediate */
  /* 6a */{"ror", ACCUMULATOR, M_AC,   WRITE, 2, 1}, /* Accumulator */
  /* 6b */{"ARR", IMMEDIATE,   M_ACIM, READ,  2, 2}, /* ARR isn't typo */

  /* 6c */{"jmp", ABS_INDIRECT,M_AIND, READ,  5, 3}, /* Indirect */
  /* 6d */{"adc", ABSOLUTE,    M_ABS,  READ,  4, 3}, /* Absolute */
  /* 6e */{"ror", ABSOLUTE,    M_ABS,  WRITE, 6, 3}, /* Absolute */
  /* 6f */{"RRA", ABSOLUTE,    M_ABS,  WRITE, 6, 3},

  /* 70 */{"bvs", RELATIVE,    M_REL,  READ,  2, 2},
  /* 71 */{"adc", INDIRECT_Y,  M_INDY, READ,  5, 2}, /* (Indirect),Y */
  /* 72 */{".JAM",IMPLIED,     M_NONE, NONE,  0, 1}, /* TILT relative? */
  /* 73 */{"RRA", INDIRECT_Y,  M_INDY, WRITE, 8, 2},

  /* 74 */{"NOP", ZERO_PAGE_X, M_NONE, NONE,  4, 2},
  /* 75 */{"adc", ZERO_PAGE_X, M_ZERX, READ,  4, 2}, /* Zeropage,X */
  /* 76 */{"ror", ZERO_PAGE_X, M_ZERX, WRITE, 6, 2}, /* Zeropage,X */
  /* 77 */{"RRA", ZERO_PAGE_X, M_ZERX, WRITE, 6, 2},

  /* 78 */{"sei", IMPLIED,     M_NONE, NONE,  2, 1},
  /* 79 */{"adc", ABSOLUTE_Y,  M_ABSY, READ,  4, 3}, /* Absolute,Y */
  /* 7a */{"NOP", IMPLIED,     M_NONE, NONE,  2, 1},
  /* 7b */{"RRA", ABSOLUTE_Y,  M_ABSY, WRITE, 7, 3},

  /* 7c */{"NOP", ABSOLUTE_X,  M_NONE, NONE,  4, 3},
  /* 7d */{"adc", ABSOLUTE_X,  M_ABSX, READ,  4, 3},  /* Absolute,X */
  /* 7e */{"ror", ABSOLUTE_X,  M_ABSX, WRITE, 7, 3},  /* Absolute,X */
  /* 7f */{"RRA", ABSOLUTE_X,  M_ABSX, WRITE, 7, 3},

  /****  Negative  ****/

  /* 80 */{"NOP", IMMEDIATE,   M_NONE, NONE,  2, 2},
  /* 81 */{"sta", INDIRECT_X,  M_AC,   WRITE, 6, 2}, /* (Indirect,X) */
  /* 82 */{"NOP", IMMEDIATE,   M_NONE, NONE,  2, 2},
  /* 83 */{"SAX", INDIRECT_X,  M_ANXR, WRITE, 6, 2},

  /* 84 */{"sty", ZERO_PAGE,   M_YR,   WRITE, 3, 2}, /* Zeropage */
  /* 85 */{"sta", ZERO_PAGE,   M_AC,   WRITE, 3, 2}, /* Zeropage */
  /* 86 */{"stx", ZERO_PAGE,   M_XR,   WRITE, 3, 2}, /* Zeropage */
  /* 87 */{"SAX", ZERO_PAGE,   M_ANXR, WRITE, 3, 2},

  /* 88 */{"dey", IMPLIED,     M_YR,   NONE,  2, 1},
  /* 89 */{"NOP", IMMEDIATE,   M_NONE, NONE,  2, 2},
  /* 8a */{"txa", IMPLIED,     M_XR,   NONE,  2, 1},
  /****  very abnormal: usually AC = AC | #$EE & XR & #$oper  ****/
  /* 8b */{"ANE", IMMEDIATE,   M_AXIM, READ,  2, 2},

  /* 8c */{"sty", ABSOLUTE,    M_YR,   WRITE, 4, 3}, /* Absolute */
  /* 8d */{"sta", ABSOLUTE,    M_AC,   WRITE, 4, 3}, /* Absolute */
  /* 8e */{"stx", ABSOLUTE,    M_XR,   WRITE, 4, 3}, /* Absolute */
  /* 8f */{"SAX", ABSOLUTE,    M_ANXR, WRITE, 4, 3},

  /* 90 */{"bcc", RELATIVE,    M_REL,  READ,  2, 2},
  /* 91 */{"sta", INDIRECT_Y,  M_AC,   WRITE, 6, 2}, /* (Indirect),Y */
  /* 92 */{".JAM",IMPLIED,     M_NONE, NONE,  0, 1}, /* TILT relative? */
  /* 93 */{"SHA", INDIRECT_Y,  M_ANXR, WRITE, 6, 2},

  /* 94 */{"sty", ZERO_PAGE_X, M_YR,   WRITE, 4, 2}, /* Zeropage,X */
  /* 95 */{"sta", ZERO_PAGE_X, M_AC,   WRITE, 4, 2}, /* Zeropage,X */
  /* 96 */{"stx", ZERO_PAGE_Y, M_XR,   WRITE, 4, 2}, /* Zeropage,Y */
  /* 97 */{"SAX", ZERO_PAGE_Y, M_ANXR, WRITE, 4, 2},

  /* 98 */{"tya", IMPLIED,     M_YR,   NONE,  2, 1},
  /* 99 */{"sta", ABSOLUTE_Y,  M_AC,   WRITE, 5, 3}, /* Absolute,Y */
  /* 9a */{"txs", IMPLIED,     M_XR,   NONE,  2, 1},
  /*** This is very mysterious command ... */
  /* 9b */{"SHS", ABSOLUTE_Y,  M_ANXR, WRITE, 5, 3},

  /* 9c */{"SHY", ABSOLUTE_X,  M_YR,   WRITE, 5, 3},
  /* 9d */{"sta", ABSOLUTE_X,  M_AC,   WRITE, 5, 3}, /* Absolute,X */
  /* 9e */{"SHX", ABSOLUTE_Y,  M_XR  , WRITE, 5, 3},
  /* 9f */{"SHA", ABSOLUTE_Y,  M_ANXR, WRITE, 5, 3},

  /* a0 */{"ldy", IMMEDIATE,   M_IMM,  READ,  2, 2}, /* Immediate */
  /* a1 */{"lda", INDIRECT_X,  M_INDX, READ,  6, 2}, /* (indirect,X) */
  /* a2 */{"ldx", IMMEDIATE,   M_IMM,  READ,  2, 2}, /* Immediate */
  /* a3 */{"LAX", INDIRECT_X,  M_INDX, READ,  6, 2}, /* (indirect,X) */

  /* a4 */{"ldy", ZERO_PAGE,   M_ZERO, READ,  3, 2}, /* Zeropage */
  /* a5 */{"lda", ZERO_PAGE,   M_ZERO, READ,  3, 2}, /* Zeropage */
  /* a6 */{"ldx", ZERO_PAGE,   M_ZERO, READ,  3, 2}, /* Zeropage */
  /* a7 */{"LAX", ZERO_PAGE,   M_ZERO, READ,  3, 2},

  /* a8 */{"tay", IMPLIED,     M_AC,   NONE,  2, 1},
  /* a9 */{"lda", IMMEDIATE,   M_IMM,  READ,  2, 2}, /* Immediate */
  /* aa */{"tax", IMPLIED,     M_AC,   NONE,  2, 1},
  /* ab */{"LXA", IMMEDIATE,   M_ACIM, READ,  2, 2}, /* LXA isn't a typo */

  /* ac */{"ldy", ABSOLUTE,    M_ABS,  READ,  4, 3}, /* Absolute */
  /* ad */{"lda", ABSOLUTE,    M_ABS,  READ,  4, 3}, /* Absolute */
  /* ae */{"ldx", ABSOLUTE,    M_ABS,  READ,  4, 3}, /* Absolute */
  /* af */{"LAX", ABSOLUTE,    M_ABS,  READ,  4, 3},

  /* b0 */{"bcs", RELATIVE,    M_REL,  READ,  2, 2},
  /* b1 */{"lda", INDIRECT_Y,  M_INDY, READ,  5, 2}, /* (indirect),Y */
  /* b2 */{".JAM",IMPLIED,     M_NONE, NONE,  0, 1}, /* TILT */
  /* b3 */{"LAX", INDIRECT_Y,  M_INDY, READ,  5, 2},

  /* b4 */{"ldy", ZERO_PAGE_X, M_ZERX, READ,  4, 2}, /* Zeropage,X */
  /* b5 */{"lda", ZERO_PAGE_X, M_ZERX, READ,  4, 2}, /* Zeropage,X */
  /* b6 */{"ldx", ZERO_PAGE_Y, M_ZERY, READ,  4, 2}, /* Zeropage,Y */
  /* b7 */{"LAX", ZERO_PAGE_Y, M_ZERY, READ,  4, 2},

  /* b8 */{"clv", IMPLIED,     M_NONE, NONE,  2, 1},
  /* b9 */{"lda", ABSOLUTE_Y,  M_ABSY, READ,  4, 3}, /* Absolute,Y */
  /* ba */{"tsx", IMPLIED,     M_SP,   NONE,  2, 1},
  /* bb */{"LAS", ABSOLUTE_Y,  M_SABY, READ,  4, 3},

  /* bc */{"ldy", ABSOLUTE_X,  M_ABSX, READ,  4, 3}, /* Absolute,X */
  /* bd */{"lda", ABSOLUTE_X,  M_ABSX, READ,  4, 3}, /* Absolute,X */
  /* be */{"ldx", ABSOLUTE_Y,  M_ABSY, READ,  4, 3}, /* Absolute,Y */
  /* bf */{"LAX", ABSOLUTE_Y,  M_ABSY, READ,  4, 3},

  /* c0 */{"cpy", IMMEDIATE,   M_IMM,  READ,  2, 2}, /* Immediate */
  /* c1 */{"cmp", INDIRECT_X,  M_INDX, READ,  6, 2}, /* (Indirect,X) */
  /* c2 */{"NOP", IMMEDIATE,   M_NONE, NONE,  2, 2}, /* occasional TILT */
  /* c3 */{"DCP", INDIRECT_X,  M_INDX, WRITE, 8, 2},

  /* c4 */{"cpy", ZERO_PAGE,   M_ZERO, READ,  3, 2}, /* Zeropage */
  /* c5 */{"cmp", ZERO_PAGE,   M_ZERO, READ,  3, 2}, /* Zeropage */
  /* c6 */{"dec", ZERO_PAGE,   M_ZERO, WRITE, 5, 2}, /* Zeropage */
  /* c7 */{"DCP", ZERO_PAGE,   M_ZERO, WRITE, 5, 2},

  /* c8 */{"iny", IMPLIED,     M_YR,   NONE,  2, 1},
  /* c9 */{"cmp", IMMEDIATE,   M_IMM,  READ,  2, 2}, /* Immediate */
  /* ca */{"dex", IMPLIED,     M_XR,   NONE,  2, 1},
  /* cb */{"SBX", IMMEDIATE,   M_IMM,  READ,  2, 2},

  /* cc */{"cpy", ABSOLUTE,    M_ABS,  READ,  4, 3}, /* Absolute */
  /* cd */{"cmp", ABSOLUTE,    M_ABS,  READ,  4, 3}, /* Absolute */
  /* ce */{"dec", ABSOLUTE,    M_ABS,  WRITE, 6, 3}, /* Absolute */
  /* cf */{"DCP", ABSOLUTE,    M_ABS,  WRITE, 6, 3},

  /* d0 */{"bne", RELATIVE,    M_REL,  READ,  2, 2},
  /* d1 */{"cmp", INDIRECT_Y,  M_INDY, READ,  5, 2}, /* (Indirect),Y */
  /* d2 */{".JAM",IMPLIED,     M_NONE, NONE,  0, 1}, /* TILT */
  /* d3 */{"DCP", INDIRECT_Y,  M_INDY, WRITE, 8, 2},

  /* d4 */{"NOP", ZERO_PAGE_X, M_NONE, NONE,  4, 2},
  /* d5 */{"cmp", ZERO_PAGE_X, M_ZERX, READ,  4, 2}, /* Zeropage,X */
  /* d6 */{"dec", ZERO_PAGE_X, M_ZERX, WRITE, 6, 2}, /* Zeropage,X */
  /* d7 */{"DCP", ZERO_PAGE_X, M_ZERX, WRITE, 6, 2},

  /* d8 */{"cld", IMPLIED,     M_NONE, NONE,  2, 1},
  /* d9 */{"cmp", ABSOLUTE_Y,  M_ABSY, READ,  4, 3}, /* Absolute,Y */
  /* da */{"NOP", IMPLIED,     M_NONE, NONE,  2, 1},
  /* db */{"DCP", ABSOLUTE_Y,  M_ABSY, WRITE, 7, 3},

  /* dc */{"NOP", ABSOLUTE_X,  M_NONE, NONE,  4, 3},
  /* dd */{"cmp", ABSOLUTE_X,  M_ABSX, READ,  4, 3}, /* Absolute,X */
  /* de */{"dec", ABSOLUTE_X,  M_ABSX, WRITE, 7, 3}, /* Absolute,X */
  /* df */{"DCP", ABSOLUTE_X,  M_ABSX, WRITE, 7, 3},

  /* e0 */{"cpx", IMMEDIATE,   M_IMM,  READ,  2, 2}, /* Immediate */
  /* e1 */{"sbc", INDIRECT_X,  M_INDX, READ,  6, 2}, /* (Indirect,X) */
  /* e2 */{"NOP", IMMEDIATE,   M_NONE, NONE,  2, 2},
  /* e3 */{"ISB", INDIRECT_X,  M_INDX, WRITE, 8, 2},

  /* e4 */{"cpx", ZERO_PAGE,   M_ZERO, READ,  3, 2}, /* Zeropage */
  /* e5 */{"sbc", ZERO_PAGE,   M_ZERO, READ,  3, 2}, /* Zeropage */
  /* e6 */{"inc", ZERO_PAGE,   M_ZERO, WRITE, 5, 2}, /* Zeropage */
  /* e7 */{"ISB", ZERO_PAGE,   M_ZERO, WRITE, 5, 2},

  /* e8 */{"inx", IMPLIED,     M_XR,   NONE,  2, 1},
  /* e9 */{"sbc", IMMEDIATE,   M_IMM,  READ,  2, 2}, /* Immediate */
  /* ea */{"nop", IMPLIED,     M_NONE, NONE,  2, 1},
  /* eb */{"SBC", IMMEDIATE,   M_IMM,  READ,  2, 2}, /* same as e9 */

  /* ec */{"cpx", ABSOLUTE,    M_ABS,  READ,  4, 3}, /* Absolute */
  /* ed */{"sbc", ABSOLUTE,    M_ABS,  READ,  4, 3}, /* Absolute */
  /* ee */{"inc", ABSOLUTE,    M_ABS,  WRITE, 6, 3}, /* Absolute */
  /* ef */{"ISB", ABSOLUTE,    M_ABS,  WRITE, 6, 3},

  /* f0 */{"beq", RELATIVE,    M_REL,  READ,  2, 2},
  /* f1 */{"sbc", INDIRECT_Y,  M_INDY, READ,  5, 2}, /* (Indirect),Y */
  /* f2 */{".JAM",IMPLIED,     M_NONE, NONE,  0, 1}, /* TILT */
  /* f3 */{"ISB", INDIRECT_Y,  M_INDY, WRITE, 8, 2},

  /* f4 */{"NOP", ZERO_PAGE_X, M_NONE, NONE,  4, 2},
  /* f5 */{"sbc", ZERO_PAGE_X, M_ZERX, READ,  4, 2}, /* Zeropage,X */
  /* f6 */{"inc", ZERO_PAGE_X, M_ZERX, WRITE, 6, 2}, /* Zeropage,X */
  /* f7 */{"ISB", ZERO_PAGE_X, M_ZERX, WRITE, 6, 2},

  /* f8 */{"sed", IMPLIED,     M_NONE, NONE,  2, 1},
  /* f9 */{"sbc", ABSOLUTE_Y,  M_ABSY, READ,  4, 3}, /* Absolute,Y */
  /* fa */{"NOP", IMPLIED,     M_NONE, NONE,  2, 1},
  /* fb */{"ISB", ABSOLUTE_Y,  M_ABSY, WRITE, 7, 3},

  /* fc */{"NOP" ,ABSOLUTE_X,  M_NONE, NONE,  4, 3},
  /* fd */{"sbc", ABSOLUTE_X,  M_ABSX, READ,  4, 3}, /* Absolute,X */
  /* fe */{"inc", ABSOLUTE_X,  M_ABSX, WRITE, 7, 3}, /* Absolute,X */
  /* ff */{"ISB", ABSOLUTE_X,  M_ABSX, WRITE, 7, 3}
};
