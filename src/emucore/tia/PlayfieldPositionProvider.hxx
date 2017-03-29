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

#ifndef TIA_PLAYFIELD_PROVIDER
#define TIA_PLAYFIELD_PROVIDER

#include "bspf.hxx"

/**
  This is an abstract interface class that provides a subset of TIA
  functionality for sprite positioning while avoiding circular dependencies
  between TIA and sprites.

  @author  Christian Speckner (DirtyHairy) and Stephen Anthony
*/
class PlayfieldPositionProvider
{
  public:
    /**
      Get the current x value
    */
    virtual uInt8 getPosition() const = 0;

  protected:
    ~PlayfieldPositionProvider() = default;

};

#endif // TIA_POSITIONING_PROVIDER
