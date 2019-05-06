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
// Copyright (c) 1995-2019 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include <unistd.h>

#include "SettingsR77.hxx"

/**
  The Retron77 system is a locked-down, set piece of hardware.
  No configuration of Stella is possible, since the UI isn't exposed.
  So we hardcode the specific settings here.
*/

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SettingsR77::SettingsR77()
  : Settings()
{
  setPermanent("video", "opengles2");
  setPermanent("vsync", "true");

  setPermanent("tia.zoom", "3");
  setPermanent("tia.fs_stretch", "false");  // start in 4:3 by default

  setPermanent("audio.buffer_size", "6");
  setPermanent("audio.enabled", "1");
  setPermanent("audio.fragment_size", "512");
  setPermanent("audio.headroom", "5");
  setPermanent("audio.preset", "1");
  setPermanent("audio.resampling_quality", "2");
  setPermanent("audio.sample_rate", "48000");
  setPermanent("audio.stereo", "0");
  setPermanent("audio.volume", "100");

  setPermanent("romdir", "/mnt/games");
  setPermanent("snapsavedir", "/mnt/stella/snapshots");
  setPermanent("snaploaddir", "/mnt/stella/snapshots");

  setPermanent("launcherres", "1280x720");
  setPermanent("launcherfont", "large");
  setPermanent("romviewer", "2");
  setPermanent("exitlauncher", "true");

  setTemporary("minimal_ui", true);
  setPermanent("basic_settings", true);

  setPermanent("dev.settings", false);
  // record states for 60 seconds
  setPermanent("plr.timemachine", true);
  setPermanent("plr.tm.size", 60);
  setPermanent("plr.tm.uncompressed", 60);
  setPermanent("plr.tm.interval", "1s");

  setPermanent("threads", "1");

  // all TV effects off by default (aligned to StellaSettingsDialog defaults!)
  setPermanent("tv.filter", "1"); // RGB
  setPermanent("tv.phosphor", "always");
  setPermanent("tv.phosblend", "45"); // level 6
  setPermanent("tv.scanlines", "18"); // level 3

  setPermanent("joymap",
    string("joymap") +
    "|126^i2c_controller|2 31 31 35 35 119 120 110 111|8 19 20 100 99 103 0 0 0 118 122 124 125 0 0 0 0" +
    "|0^i2c_controller #2|2 39 39 43 43 119 120 110 111|8 26 27 100 99 103 0 0 0 118 122 124 125 0 0 0 0|0"
  );
}
