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

#ifndef CART_DISASSEMBLY_WRITER_HXX
#define CART_DISASSEMBLY_WRITER_HXX

#include "bspf.hxx"

class CartDebug;

class CartDisassemblyWriter
{
  public:
    explicit CartDisassemblyWriter(CartDebug& cartDebug);
    ~CartDisassemblyWriter() = default;

    /**
      Save a DASM-compatible disassembly of the ROM to the given path.
      If path is empty, a default path is constructed from the ROM name.

      @param path  Output file path (optional; .asm extension appended if missing)
      @return      Status message suitable for display in the debugger
    */
    string save(string path = {});

  private:
    CartDebug& myCartDebug;

  private:
    // Following constructors and assignment operators not supported
    CartDisassemblyWriter() = delete;
    CartDisassemblyWriter(const CartDisassemblyWriter&) = delete;
    CartDisassemblyWriter(CartDisassemblyWriter&&) = delete;
    CartDisassemblyWriter& operator=(const CartDisassemblyWriter&) = delete;
    CartDisassemblyWriter& operator=(CartDisassemblyWriter&&) = delete;
};

#endif  // CART_DISASSEMBLY_WRITER_HXX
