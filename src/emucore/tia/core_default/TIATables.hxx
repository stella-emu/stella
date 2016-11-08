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

#ifndef TIA_DEFAULT_CORE_TABLES_HXX
#define TIA_DEFAULT_CORE_TABLES_HXX

#include "bspf.hxx"
#include "TIATypes.hxx"

/**
  The TIA class uses some static tables that aren't dependent on the actual
  TIA state.  For code organization, it's better to place that functionality
  here.

  @author  Stephen Anthony
  @version $Id$
*/

namespace TIADefaultCore {

class TIATables
{
  public:
    /**
      Compute all static tables used by the TIA
    */
    static void computeAllTables();

    // Player mask table
    // [suppress mode][nusiz][pixel]
    static uInt8 PxMask[2][8][320];

    // Missle mask table (entries are true or false)
    // [number][size][pixel]
    // There are actually only 4 possible size combinations on a real system
    // The fifth size is used for simulating the starfield effect in
    // Cosmic Ark and Stay Frosty
    static uInt8 MxMask[8][5][320];

    // Ball mask table (entries are true or false)
    // [size][pixel]
    static uInt8 BLMask[4][320];

    // Playfield mask table for reflected and non-reflected playfields
    // [reflect, pixel]
    static uInt32 PFMask[2][160];

    // A mask table which can be used when an object is disabled
    static uInt8 DisabledMask[640];

    // Used to set the collision register to the correct value
    static uInt16 CollisionMask[64];

    // Indicates the update delay associated with poking at a TIA address
    static const Int16 PokeDelay[64];

#if 0
    // Used to convert value written in a motion register into 
    // its internal representation
    static const Int32 CompleteMotion[76][16];
#endif

    // Indicates if HMOVE blanks should occur for the corresponding cycle
    static const bool HMOVEBlankEnableCycles[76];

    // Used to reflect a players graphics
    static uInt8 GRPReflect[256];

    // Indicates if player is being reset during delay, display or other times
    // [nusiz][old pixel][new pixel]
    static Int8 PxPosResetWhen[8][160][160];

  private:
    // Compute the collision decode table
    static void buildCollisionMaskTable();

    // Compute the player mask table
    static void buildPxMaskTable();

    // Compute the missle mask table
    static void buildMxMaskTable();

    // Compute the ball mask table
    static void buildBLMaskTable();

    // Compute playfield mask table
    static void buildPFMaskTable();

    // Compute the player reflect table
    static void buildGRPReflectTable();

    // Compute the player position reset when table
    static void buildPxPosResetWhenTable();

  private:
    // Following constructors and assignment operators not supported
    TIATables() = delete;
    TIATables(const TIATables&) = delete;
    TIATables(TIATables&&) = delete;
    TIATables& operator=(const TIATables&) = delete;
    TIATables& operator=(TIATables&&) = delete;
};

} // namespace TIADefaultCore

#endif