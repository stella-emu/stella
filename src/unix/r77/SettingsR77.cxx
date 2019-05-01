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
  setTemporary("video", "opengles2");
  setTemporary("vsync", "true");

  setTemporary("tia.zoom", "3");
  setTemporary("tia.fs_stretch", "false");  // start in 4:3 by default

  setTemporary("audio.buffer_size", "6");
  setPermanent("audio.enabled", "1");
  setTemporary("audio.fragment_size", "512");
  setTemporary("audio.headroom", "5");
  setTemporary("audio.preset", "1");
  setTemporary("audio.resampling_quality", "2");
  setTemporary("audio.sample_rate", "48000");
  setPermanent("audio.stereo", "0");
  setPermanent("audio.volume", "100");

  setTemporary("romdir", "/mnt/games");
  setTemporary("snapsavedir", "/mnt/stella/snapshots");
  setTemporary("snaploaddir", "/mnt/stella/snapshots");

  setTemporary("launcherres", "1280x720");
  setTemporary("launcherfont", "large");
  setTemporary("romviewer", "2");
  setTemporary("exitlauncher", "true");

  setTemporary("minimal_ui", true);
  setPermanent("basic_settings", true);

  setTemporary("dev.settings", false);
  // record states for 60 seconds
  setPermanent("plr.timemachine", true);
  setTemporary("plr.tm.size", 60);
  setTemporary("plr.tm.uncompressed", 60);
  setTemporary("plr.tm.interval", "1s");

  setTemporary("threads", "1");

  // all TV effects off by default (aligned to StellaSettingsDialog defaults!)
  setPermanent("tv.filter", "1"); // RGB
  setPermanent("tv.phosphor", "always");
  setPermanent("tv.phosblend", "45"); // level 6
  setPermanent("tv.scanlines", "18"); // level 3
}
