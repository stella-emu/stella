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
// Copyright (c) 1995-2016 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

#ifndef TIA_6502TS_CORE_DRAW_COUNTER_DECODES
#define TIA_6502TS_CORE_DRAW_COUNTER_DECODES

#include "bspf.hxx"

namespace TIA6502tsCore {

class DrawCounterDecodes {

  public:

    const uInt8* const* playerDecodes() const;

    const uInt8* const* missileDecodes() const;

    static DrawCounterDecodes& get();

    ~DrawCounterDecodes();

  protected:

    DrawCounterDecodes();

  private:

    uInt8 *myPlayerDecodes[8];

    uInt8 *myMissileDecodes[8];

    uInt8 *myDecodes0, *myDecodes1, *myDecodes2, *myDecodes3, *myDecodes4, *myDecodes6, *myDecodesWide;

    static DrawCounterDecodes myInstance;

  private:

    DrawCounterDecodes(const DrawCounterDecodes&) = delete;
    DrawCounterDecodes(DrawCounterDecodes&&) = delete;
    DrawCounterDecodes& operator=(const DrawCounterDecodes&) = delete;
    DrawCounterDecodes& operator=(DrawCounterDecodes&&) = delete;
};

} // namespace TIA6502tsCore

#endif // TIA_6502TS_CORE_DRAW_COUNTER_DECODES
