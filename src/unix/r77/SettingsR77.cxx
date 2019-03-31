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
  setPermanent("tia.fsfill", "true");

  setPermanent("audio.buffer_size", "6");
  setPermanent("audio.enabled", "1");
  setPermanent("audio.fragment_size", "512");
  setPermanent("audio.headroom", "5");
  setPermanent("audio.preset", "1");
  setPermanent("audio.resampling_quality", "2");
  setPermanent("audio.sample_rate", "48000");
  setPermanent("audio.stereo", "0");
  setPermanent("audio.volume", "80");

  // TODO - use new argument that differentiates between fullscreen and
  //        fullscreen without aspect correction
  //        Re-add ability to use a specific fullscreen resolution
  setPermanent("fullscreen", "true");  // start in 16:9 mode by default

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

  // FIXME - these are out of date, since the # of events has changed since 3.x
  setPermanent("keymap", "116:40:0:0:0:0:0:0:0:98:95:0:0:0:15:0:0:0:0:0:94:0:0:0:0:0:0:0:99:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:62:0:63:64:55:41:42:43:16:17:23:24:53:54:0:61:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:96:0:0:0:97:47:0:52:49:46:22:20:19:56:21:59:60:0:0:57:58:44:0:48:0:0:0:45:51:18:50:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:11:12:14:13:0:0:0:0:0:9:10:3:4:5:6:7:8:91:89:90:92:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:115:0:0:0:0:110:0:0:0:0:0:0:0:0:0:0:0:0:0:114:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:102:103:109:108:0:106:107:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0");
  setPermanent("joymap", "116^i2c_controller|2 27 27 31 31 0 0 0 0|8 28 32 0 0 0 0 0 0 0 0 0 0 0 0 0 0|0^i2c_controller 2|2 35 35 39 39 0 0 0 0|8 36 40 0 0 0 0 0 0 0 0 0 0 0 0 0 0|0");
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

  out.flush();
  out.close();
// FIXME   system("/bin/fsync /mnt/stella/stellarc&");

  return true;
}
