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

// FIXME - perhaps add to namespace or something

#ifndef EVENTHANDLER_CONSTANTS_HXX
#define EVENTHANDLER_CONSTANTS_HXX

enum MouseButton {
  EVENT_LBUTTONDOWN,
  EVENT_LBUTTONUP,
  EVENT_RBUTTONDOWN,
  EVENT_RBUTTONUP,
  EVENT_WHEELDOWN,
  EVENT_WHEELUP
};

enum JoyHat {
  EVENT_HATUP     = 0,  // make sure these are set correctly,
  EVENT_HATDOWN   = 1,  // since they'll be used as array indices
  EVENT_HATLEFT   = 2,
  EVENT_HATRIGHT  = 3,
  EVENT_HATCENTER = 4
};

enum JoyHatMask {
  EVENT_HATUP_M     = 1<<0,
  EVENT_HATDOWN_M   = 1<<1,
  EVENT_HATLEFT_M   = 1<<2,
  EVENT_HATRIGHT_M  = 1<<3,
  EVENT_HATCENTER_M = 1<<4
};

enum EventMode {
  kEmulationMode = 0,  // make sure these are set correctly,
  kMenuMode      = 1,  // since they'll be used as array indices
  kNumModes      = 2
};

#endif // EVENTHANDLER_CONSTANTS_HXX
