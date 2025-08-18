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
// Copyright (c) 1995-2025 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#ifndef PATCH_ROM_CHEAT_HXX
#define PATCH_ROM_CHEAT_HXX

#include "Cheat.hxx"

class PatchRomCheat : public Cheat
{
  public:
    PatchRomCheat(OSystem& os, string_view name, string_view code);
    ~PatchRomCheat() override = default;

    bool enable() override;
    bool disable() override;
    void evaluate() override;

  private:
    uInt16 address{0};
    uInt8  new_value{0};
    uInt8  original_value{0};

  private:
    // Following constructors and assignment operators not supported
    PatchRomCheat() = delete;
    PatchRomCheat(const PatchRomCheat&) = delete;
    PatchRomCheat(PatchRomCheat&&) = delete;
    PatchRomCheat& operator=(const PatchRomCheat&) = delete;
    PatchRomCheat& operator=(PatchRomCheat&&) = delete;
};

#endif
