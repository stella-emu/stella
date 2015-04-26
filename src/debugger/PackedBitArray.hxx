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
// Copyright (c) 1995-2015 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

#ifndef PACKED_BIT_ARRAY_HXX
#define PACKED_BIT_ARRAY_HXX

#include <bitset>

#include "bspf.hxx"

class PackedBitArray
{
  public:
    PackedBitArray() : myInitialized(false) { }

    bool isSet(uInt32 bit) const   { return myBits[bit];  }
    bool isClear(uInt32 bit) const { return !myBits[bit]; }

    void set(uInt32 bit)    { myBits[bit] = true;  }
    void clear(uInt32 bit)  { myBits[bit] = false; }
    void toggle(uInt32 bit) { myBits.flip(bit);    }

    void initialize() { myInitialized = true; }
    void clearAll() { myInitialized = false; myBits.reset(); }

    bool isInitialized() const { return myInitialized; }

  private:
    // The actual bits
    bitset<0x10000> myBits;

    // Indicates whether we should treat this bitset as initialized
    bool myInitialized;

  private:
    // Following constructors and assignment operators not supported
    PackedBitArray(const PackedBitArray&) = delete;
    PackedBitArray(PackedBitArray&&) = delete;
    PackedBitArray& operator=(const PackedBitArray&) = delete;
    PackedBitArray& operator=(PackedBitArray&&) = delete;
};

#endif
