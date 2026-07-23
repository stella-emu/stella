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

#include "M6532.hxx"
#include "System.hxx"
#include "CartFE.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeFE::CartridgeFE(ByteSpan image, string_view md5,
                         const Settings& settings, size_t bsSize)
  : CartridgeEnhanced(image, md5, settings, bsSize)
{
  myDirectPeek = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeFE::reset()
{
  CartridgeEnhanced::reset();
  myLastAccessWasFE = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeFE::install(System& system)
{
  CartridgeEnhanced::install(system);

  // The hotspot $01FE is in a mirror of zero-page RAM
  // We need to claim access to it here, and deal with it in peek/poke below
  mySystem->setPageAccess(0x1c0,
      System::PageAccess(this, System::PageAccessType::READWRITE));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeFE::checkSwitchBank(uInt16 address, uInt8 value)
{
  // The patent shows that the mapping circuit watches for 0x01fe to appear on
  // the address bus and then, with one cycle delay, stores bits 5 -- 7 on the data
  // bus into a latch. The latch drives a demux which controls bank selection.
  // The only thing not specify in the patent is the way the demux connects to
  // the ROM chip.
  //
  // The following expects that the lowest bit selects the ROM bank and the other
  // two bits are ignored. This has been validated for Space Shuttle and works on
  // all other known FE ROMs.
  //
  // Decathlon is a special case: during the boot sequence (which is not emulated
  // by Stella), the mapper switches to bank 1 if this demux scheme is assumed.
  // However, Decathlon contains a garbage boot vector in bank 1, so the CPU
  // now starts executing trash from TIA register space. Eventually, it hits a
  // 0x00 = BRK with S in the right state to switch to bank 0, executes the valid
  // BRK vector from there and starts up fine. While Stella always starts FE in
  // bank 0, this can be verified in Stella by randomizing the startup bank.
  //
  // We have not validated this with a real cartridge, though, and while this
  // supposedly is what real hardware does, other emulators may be better off
  // implementing a stricter scheme here to get Decathlon to start in bank 0.
  if(myLastAccessWasFE)
  {
    bank((value >> 5) ^ 0b111);
    myLastAccessWasFE = false; // was: address == 0x01FE;
    return true;
  }
  myLastAccessWasFE = address == 0x01FE;
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 CartridgeFE::peek(uInt16 address)
{
  const uInt8 value = (address < 0x200)
    ? mySystem->m6532().peek(address)
    : myImage[myCurrentSegOffset[(address & myBankMask) >> myBankShift] +
              (address & myBankMask)];

  // Check if we hit hotspot
  checkSwitchBank(address, value);

  return value;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeFE::poke(uInt16 address, uInt8 value)
{
  if(address < 0x200)
    mySystem->m6532().poke(address, value);

  // Check if we hit hotspot
  checkSwitchBank(address, value);

  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeFE::save(Serializer& out) const
{
  if(!CartridgeEnhanced::save(out))
    return false;
  try
  {
    out.putBool(myLastAccessWasFE);
  }
  catch(...)
  {
    cerr << "ERROR: CartridgeFE::save\n";
    return false;
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartridgeFE::load(Serializer& in)
{
  if(!CartridgeEnhanced::load(in))
    return false;
  try
  {
    myLastAccessWasFE = in.getBool();
  }
  catch(...)
  {
    cerr << "ERROR: CartridgeFE::load\n";
    return false;
  }

  return true;
}
