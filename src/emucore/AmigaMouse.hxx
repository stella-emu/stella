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

#ifndef AMIGAMOUSE_HXX
#define AMIGAMOUSE_HXX

#include "PointingDevice.hxx"

namespace {

  class AmigaMouseHelper {

    public:
      static uInt8 ioPortA(uInt8 countH, uInt8 countV, uInt8 left, uInt8 down) {
        static constexpr uInt32 ourTableH[4] = { 0x00, 0x10, 0x50, 0x40 };
        static constexpr uInt32 ourTableV[4] = { 0x00, 0x80, 0xa0, 0x20 };

        return ourTableV[countV] | ourTableH[countH];
      }

    public:
      static constexpr Controller::Type controllerType = Controller::AmigaMouse;
      static constexpr uInt8 counterDivide = 2;
  };
}

using AmigaMouse = PointingDevice<AmigaMouseHelper>;

#endif // AMIGAMOUSE_HXX
