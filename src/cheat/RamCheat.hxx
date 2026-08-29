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

#ifndef RAM_CHEAT_HXX
#define RAM_CHEAT_HXX

#include "Cheat.hxx"

/**
  A cheat that pokes a fixed value into RAM every frame while enabled,
  decoded from a 4-digit cheatcode.

  @author  Stephen Anthony
*/
class RamCheat : public Cheat
{
  public:
    // Decodes a 4-digit RAM cheat into address/value
    RamCheat(OSystem& os, string_view name, string_view code);
    ~RamCheat() override = default;

    // Adds the cheat to CheatManager's per-frame list
    bool enable() override;
    // Removes the cheat from CheatManager's per-frame list
    bool disable() override;
    // Pokes value into RAM at address; called every frame while enabled
    void evaluate() override;

  private:
    // RAM address to poke
    uInt16 address{0};
    // Byte value to poke at address
    uInt8  value{0};

  private:
    // Following constructors and assignment operators not supported
    RamCheat() = delete;
    RamCheat(const RamCheat&) = delete;
    RamCheat(RamCheat&&) = delete;
    RamCheat& operator=(const RamCheat&) = delete;
    RamCheat& operator=(RamCheat&&) = delete;
};

#endif  // RAM_CHEAT_HXX
