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

#ifndef TRAKBALL_HXX
#define TRAKBALL_HXX

#include "PointingDevice.hxx"

namespace {

  class TrakBallHelper {

    public:
      static uInt8 ioPortA(uInt8 countH, uInt8 countV, uInt8 left, uInt8 down) {
        static constexpr uInt32 ourTableH[2][2] = {{ 0x40, 0x00 }, { 0xc0, 0x80 }};
        static constexpr uInt32 ourTableV[2][2] = {{ 0x00, 0x10 }, { 0x20, 0x30 }};

        return ourTableV[countV & 0x01][down] |
               ourTableH[countH & 0x01][left];
      }

    public:
      static constexpr Controller::Type controllerType = Controller::TrakBall;

      static constexpr uInt8 counterDivide = 4;
  };

}

typedef PointingDevice<TrakBallHelper> TrakBall;

#endif // TRAKBALL_HXX
