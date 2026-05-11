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

#ifndef DRAW_COUNTER_DECODES_HXX
#define DRAW_COUNTER_DECODES_HXX

#include "bspf.hxx"

/**
  Singleton that precomputes the player and missile draw-counter decode
  tables. Each table maps a horizontal counter value (0–159) to a copy
  number, determining where each sprite copy begins on a scanline for
  every NUSIZ number/size combination.

  @author  Christian Speckner (DirtyHairy)
*/
class DrawCounterDecodes
{
  public:
    /**
      Returns the player decode table: 8 entries (one per NUSIZ number/size
      combination), each a 160-element array. A non-zero value at position x
      indicates a copy start at that pixel; the value is the copy number.
     */
    const uInt8* const* playerDecodes() const { return myPlayerDecodes; }

    /**
      Returns the missile decode table, same layout as the player table.
     */
    const uInt8* const* missileDecodes() const { return myMissileDecodes; }

    /**
      Singleton accessor.
     */
    static DrawCounterDecodes& get() {
      static DrawCounterDecodes myInstance;
      return myInstance;
    }

  protected:
    DrawCounterDecodes();
    ~DrawCounterDecodes() = default;

  private:
    // 8 entries, one per NUSIZ number/size combination
    uInt8* myPlayerDecodes[8]{};
    // 8 entries, one per NUSIZ number/size combination
    uInt8* myMissileDecodes[8]{};

    // Six 160-element scanline arrays, one for each supported copy pattern
    // (NUSIZ patterns 0, 1, 2, 3, 4, and 6)
    uInt8 myDecodes0[160]{}, myDecodes1[160]{}, myDecodes2[160]{},
          myDecodes3[160]{}, myDecodes4[160]{}, myDecodes6[160]{};

  private:
    // Following constructors and assignment operators not supported
    DrawCounterDecodes(const DrawCounterDecodes&) = delete;
    DrawCounterDecodes(DrawCounterDecodes&&) = delete;
    DrawCounterDecodes& operator=(const DrawCounterDecodes&) = delete;
    DrawCounterDecodes& operator=(DrawCounterDecodes&&) = delete;
};

#endif  // DRAW_COUNTER_DECODES_HXX
