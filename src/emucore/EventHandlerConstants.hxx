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

#ifndef EVENT_HANDLER_CONSTANTS_HXX
#define EVENT_HANDLER_CONSTANTS_HXX

#include <string>

#include "BitmaskEnum.hxx"
#include "bspf.hxx"

// Enumeration representing the different states of operation
enum class EventHandlerState: uInt8 {
  EMULATION,
  TIMEMACHINE,
  PLAYBACK,
  PAUSE,
  LAUNCHER,
  OPTIONSMENU,
  CMDMENU,
  HIGHSCORESMENU,
  MESSAGEMENU,
  PLUSROMSMENU,
  DEBUGGER,
  NONE
};

enum class MouseButton: uInt8 {
  LEFT,
  RIGHT,
  MIDDLE,
  WHEELDOWN,
  WHEELUP,
  NONE
};

static constexpr int JOY_CTRL_NONE = 0x7FFFFFFF;

enum class JoyAxis: uInt8 {
  X    = 0,    // make sure these are set correctly,
  Y    = 1,    // since they'll be used as array indices
  Z    = 2,
  A3   = 3,
  A4   = 4,
  A5   = 5,
  A6   = 6,
  A7   = 7,
  NONE = 0xFF
};

enum class JoyDir: uInt8 {
  NONE   = 0,
  NEG    = 1,
  POS    = 2,
  ANALOG = 3
};

enum class JoyHatDir: uInt8 {
  UP     = 0,  // make sure these are set correctly,
  DOWN   = 1,  // since they'll be used as array indices
  LEFT   = 2,
  RIGHT  = 3,
  CENTER = 4
};

enum class JoyHatMask: uInt8 {
  NONE   = 0,
  UP     = 1 << 0,
  DOWN   = 1 << 1,
  LEFT   = 1 << 2,
  RIGHT  = 1 << 3,
  CENTER = 1 << 4
};
template<> inline constexpr bool Bitmask::is_enum_v<JoyHatMask> = true;

enum class EventMode: uInt8 {
  kEmulationMode, // active mapping used for emulation
  kMenuMode,      // mapping used for dialogs
  kJoystickMode,  // 5 extra modes for mapping controller keys separately for emulation mode
  kPaddlesMode,
  kKeyboardMode,
  kDrivingMode,
  kCompuMateMode, // cannot be remapped
  kCommonMode,    // mapping common between controllers
  kEditMode,      // mapping used in editable widgets
  kPromptMode,    // extra mappings used in debugger's prompt widget
  kNumModes
};

namespace GUI {
  static constexpr string_view SELECT = "Select";
  static constexpr string_view LEFT_DIFFICULTY = "Left difficulty";
  static constexpr string_view LEFT_DIFFICULTY_A = "Left difficulty A";
  static constexpr string_view LEFT_DIFFICULTY_B = "Left difficulty B";
  static constexpr string_view RIGHT_DIFFICULTY = "Right difficulty";
  static constexpr string_view RIGHT_DIFFICULTY_A = "Right difficulty A";
  static constexpr string_view RIGHT_DIFFICULTY_B = "Right difficulty B";
  static constexpr string_view LEFT_DIFF = "Left Diff";
  static constexpr string_view LEFT_DIFF_A = "Left Diff A";
  static constexpr string_view LEFT_DIFF_B = "Left Diff B";
  static constexpr string_view RIGHT_DIFF = "Right Diff";
  static constexpr string_view RIGHT_DIFF_A = "Right Diff A";
  static constexpr string_view RIGHT_DIFF_B = "Right Diff B";
}  // namespace GUI

#endif  // EVENT_HANDLER_CONSTANTS_HXX
