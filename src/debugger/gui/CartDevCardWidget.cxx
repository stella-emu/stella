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

#include "CartDevCard.hxx"
#include "OSystem.hxx"
#include "Debugger.hxx"
#include "CartDebug.hxx"
#include "CartDevCardWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeDevCardWidget::CartridgeDevCardWidget(
      GuiObject* boss, const GUI::Font& lfont, const GUI::Font& nfont,
      int x, int y, int w, int h, CartridgeDevCard& cart)
  : CartDebugWidget(boss, lfont, nfont, x, y, w, h),
    myCart{cart}
{
  constexpr string_view info =
    "DevCard: 24K RAM across 6 non-contiguous 4K windows\n"
    "$5000-$5FFF  $7000-$7FFF  $9000-$9FFF\n"
    "$B000-$BFFF  $D000-$DFFF  $F000-$FFFF\n"
    "All windows readable and writable; no bankswitching.";

  // This tab is nothing but the ROM info block; reflow() lays it out
  createBaseInformation(CartridgeDevCard::RAM_SIZE, "Atari (DevKit)", info);
  reflow();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeDevCardWidget::saveOldState()
{
  myOldState.internalram.clear();
  myOldState.internalram.assign(myCart.myRAM.begin(), myCart.myRAM.end());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 CartridgeDevCardWidget::internalRamSize()
{
  return CartridgeDevCard::RAM_SIZE;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 CartridgeDevCardWidget::internalRamRPort(int start)
{
  // Map sequential RAM index back to the CPU address in the appropriate window
  const uInt32 windowIdx = static_cast<uInt32>(start) / CartridgeDevCard::WINDOW_SIZE;
  const uInt32 offset    = static_cast<uInt32>(start) % CartridgeDevCard::WINDOW_SIZE;
  return CartridgeDevCard::WINDOWS[windowIdx] + offset;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string CartridgeDevCardWidget::internalRamDescription()
{
  return "$5000-$5FFF, $7000-$7FFF, $9000-$9FFF,\n"
         "$B000-$BFFF, $D000-$DFFF, $F000-$FFFF\n"
         "(read/write everywhere)";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const ByteArray& CartridgeDevCardWidget::internalRamOld(int start, int count)
{
  myRamOld.clear();
  myRamOld.assign(myOldState.internalram.data() + start,
                  myOldState.internalram.data() + start + count);
  return myRamOld;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const ByteArray& CartridgeDevCardWidget::internalRamCurrent(int start, int count)
{
  myRamCurrent.clear();
  myRamCurrent.assign(myCart.myRAM.data() + start,
                      myCart.myRAM.data() + start + count);
  return myRamCurrent;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeDevCardWidget::internalRamSetValue(int addr, uInt8 value)
{
  myCart.myRAM[addr] = value;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 CartridgeDevCardWidget::internalRamGetValue(int addr)
{
  return myCart.myRAM[addr];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string CartridgeDevCardWidget::internalRamLabel(int addr)
{
  const CartDebug& dbg = instance().debugger().cartDebug();
  return dbg.getLabel(static_cast<uInt16>(internalRamRPort(addr)), false);
}
