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
  setPermanent("tia.fsfill", "false");  // start in 4:3 by default

  setPermanent("audio.buffer_size", "6");
  setPermanent("audio.enabled", "1");
  setPermanent("audio.fragment_size", "512");
  setPermanent("audio.headroom", "5");
  setPermanent("audio.preset", "1");
  setPermanent("audio.resampling_quality", "2");
  setPermanent("audio.sample_rate", "48000");
  setPermanent("audio.stereo", "0");
  setPermanent("audio.volume", "80");

  setPermanent("romdir", "/mnt/games");
  setPermanent("snapsavedir", "/mnt/stella/snapshots");
  setPermanent("snaploaddir", "/mnt/stella/snapshots");

  setPermanent("launcherres", "1280x720");
  setPermanent("launcherfont", "large");
  setPermanent("romviewer", "2");
  setPermanent("exitlauncher", "true");

  setTemporary("minimal_ui", true);
  setPermanent("dev.settings", false);
  setPermanent("plr.timemachine", false);

  setPermanent("threads", "1");
  setPermanent("tv.filter", "3");
  setPermanent("tv.phosphor", "always");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SettingsR77::saveConfigFile(const string& cfgfile) const
{
  // Almost no settings can be changed, so we completely disable saving
  // most of them.  This may also fix reported issues of the config file
  // becoming corrupt.
  //
  // There are currently only a few settings that can be changed
  // These will be expanded as more support is added

  ofstream out(cfgfile);
  if(!out || !out.is_open())
    return false;

  out << "fullscreen = " << getString("fullscreen") << endl;
  out << "lastrom = " << getString("lastrom") << endl;
  out << "tia.fsfill = " << getString("tia.fsfill") << endl;
//   out << "keymap = " << getString("keymap") << endl;
//   out << "joymap = " << getString("joymap") << endl;

  out.flush();
  out.close();
  system("/bin/fsync /mnt/stella/stellarc&");

  return true;
}
