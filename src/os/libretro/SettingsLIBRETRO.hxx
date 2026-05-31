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

#ifndef SETTINGS_LIBRETRO_HXX
#define SETTINGS_LIBRETRO_HXX

#include "bspf.hxx"
#include "AudioSettings.hxx"
#include "TVMode.hxx"
#include "TVSignal.hxx"
#include "PaletteHandler.hxx"

/**
  Single source of truth for all Stella core settings managed by libretro.
  Owned by libretro.cxx; passed to StellaLIBRETRO::create() and updated by
  live setters between resets.
*/
struct SettingsLIBRETRO {
  string console_format{"AUTO"};

  TVMode video_filter{TVMode::None};
  string video_palette{PaletteHandler::SETTING_STANDARD};
  string video_phosphor{"byrom"};
  uInt32 video_phosphor_blend{60};
  uInt32 video_aspect_ntsc{0};
  uInt32 video_aspect_pal{0};
  bool detect_pal60{false};
  bool detect_ntsc50{false};
  float pal_contrast{0.F};
  float pal_brightness{0.F};
  float pal_hue{0.F};
  float pal_saturation{0.F};
  float pal_gamma{0.F};

  string audio_mode{"byrom"};
  uInt32 dpc_pitch{AudioSettings::DEFAULT_DPC_PITCH};

  bool info_messages{false};

  int paddle_joypad_sensitivity{3};
  int paddle_analog_sensitivity{20};
  int paddle_mouse_sensitivity{10};
  int paddle_analog_deadzone{15};
  bool paddle_analog_absolute{false};
  bool lightgun_crosshair{false};

  bool crop_hoverscan{false};
  int  crop_voverscan{0};

  bool reload{false};
};

#endif  // SETTINGS_LIBRETRO_HXX
