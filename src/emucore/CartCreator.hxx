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

#ifndef CARTRIDGE_CREATOR_HXX
#define CARTRIDGE_CREATOR_HXX

class Cartridge;
class Settings;

#include "Bankswitch.hxx"
#include "bspf.hxx"

/**
  Create a cartridge based on the given information.  Internally, it will
  use autodetection and various heuristics to determine the cart type.

  @author  Stephen Anthony
*/
namespace CartCreator
{
  /**
    Create a new cartridge object allocated on the heap.  The
    type of cartridge created depends on the properties object.

    @param image    A pointer to the ROM image
    @param size     The size of the ROM image
    @param md5      The md5sum for the given ROM image (can be updated)
    @param dtype    The detected bankswitch type of the ROM image
    @param settings The settings container
    @return   Pointer to the new cartridge object allocated on the heap
  */
  unique_ptr<Cartridge> create(const FSNode& file,
      const ByteBuffer& image, size_t size, string& md5,
      string_view dtype, Settings& settings);
};  // namespace CartCreator

#endif
