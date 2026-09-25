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

#ifndef CART_CREATOR_HXX
#define CART_CREATOR_HXX

class Cartridge;
class Settings;

#include "Bankswitch.hxx"
#include "FSNode.hxx"
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
    @param md5      The md5sum for the given ROM image (can be updated)
    @param dtype    The detected bankswitch type of the ROM image
    @param settings The settings container
    @param baseDir  Base directory searched for auxiliary files (e.g. BIOS ROMs)
    @return   Pointer to the new cartridge object allocated on the heap
  */
  unique_ptr<Cartridge> create(const FSNode& file, ByteSpan image,
                               string& md5, string_view dtype,
                               Settings& settings, const FSNode& baseDir);

  /**
    Create a cartridge from an image already in memory, whose type is
    already known.  Unlike create() above there is no file behind it, so
    none of the path-based handling -- Supercharger sound loads, multicart
    slicing, MovieCart streaming -- applies or is attempted.

    This is what a cartridge that produces another cartridge uses: a
    FujiNet client booting a game it pulled over the network, say.

    @param image     A const span of the ROM image
    @param type      The known bankswitch type; AUTO to autodetect
    @param md5       The md5sum for the ROM image
    @param settings  The settings container
    @return  Pointer to the new cartridge, or nullptr if type is unsupported
  */
  unique_ptr<Cartridge> createFromKnownImage(ByteSpan image,
                                             Bankswitch::Type type,
                                             string_view md5,
                                             Settings& settings);
};  // namespace CartCreator

#endif  // CART_CREATOR_HXX
