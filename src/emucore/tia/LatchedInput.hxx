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
// Copyright (c) 1995-2017 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#ifndef TIA_6502TS_CORE_LATCHED_INPUT
#define TIA_6502TS_CORE_LATCHED_INPUT

#include "bspf.hxx"

class LatchedInput
{
  public:
    LatchedInput();

  public:

    void reset();

    void vblank(uInt8 value);

    uInt8 inpt(bool pinState);

  private:

    bool myModeLatched;

    uInt8 myLatchedValue;

  private:
    LatchedInput(const LatchedInput&) = delete;
    LatchedInput(LatchedInput&&) = delete;
    LatchedInput& operator=(const LatchedInput&) = delete;
    LatchedInput& operator=(LatchedInput&&) = delete;
};

#endif // TIA_6502TS_CORE_LATCHED_INPUT
