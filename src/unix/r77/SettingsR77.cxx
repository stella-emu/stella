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
// Copyright (c) 1995-2014 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

#include <unistd.h>

#include "bspf.hxx"
#include "OSystem.hxx"
#include "Settings.hxx"
#include "SettingsR77.hxx"

/**
  The Retron77 system is a locked-down, set piece of hardware.
  No configuration of Stella is possible, since the UI isn't exposed.
  So we hardcode the specific settings here.
*/
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SettingsR77::SettingsR77(OSystem* osystem)
  : Settings(osystem)
{
  setInternal("video", "soft");
  setInternal("tia_filter", "zoom3x");
  setInternal("fullscreen", "0");  // start in 16:9 mode by default
  setInternal("fullres", "1280x720");

  setInternal("snapsavedir", "/mnt/stella/snapshots");
  setInternal("snaploaddir", "/mnt/stella/snapshots");

  setInternal("romdir", "/mnt/games");
  setInternal("statedir", "/mnt/stella/statedir");
  setInternal("cheatfile", "/mnt/stella/stella.cht");
  setInternal("palettefile", "/mnt/stella/stella.pal");
  setInternal("propsfile", "/mnt/stella/stella.pro");
  setInternal("nvramdir", "/mnt/stella/");
  setInternal("cfgdir", "/mnt/stella/cfg/");

  setInternal("launcherres", "1280x720");
  setInternal("launcherfont", "large");
  setInternal("romviewer", "2");
  setInternal("exitlauncher", "true");

  setInternal("keymap", "116:40:0:0:0:0:0:0:0:98:95:0:0:0:15:0:0:0:0:0:94:0:0:0:0:0:0:0:99:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:62:0:63:64:55:41:42:43:16:17:23:24:53:54:0:61:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:96:0:0:0:97:47:0:52:49:46:22:20:19:56:21:59:60:0:0:57:58:44:0:48:0:0:0:45:51:18:50:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:11:12:14:13:0:0:0:0:0:9:10:3:4:5:6:7:8:91:89:90:92:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:115:0:0:0:0:110:0:0:0:0:0:0:0:0:0:0:0:0:0:114:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:102:103:109:108:0:106:107:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0");
  setInternal("joymap", "116^i2c_controller|2 27 27 31 31 0 0 0 0|8 28 32 0 0 0 0 0 0 0 0 0 0 0 0 0 0|0^i2c_controller 2|2 35 35 39 39 0 0 0 0|8 36 40 0 0 0 0 0 0 0 0 0 0 0 0 0 0|0");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SettingsR77::~SettingsR77()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SettingsR77::saveConfig()
{
  // Almost no settings can be changed, so we completely disable saving them.
  // This may also fix reported issues of the config file becoming corrupt.
  // As settings are able to be saved for this device, we will add them here.

  ofstream out("/mnt/stella/stellarc");
  if(!out || !out.is_open())
  {
    myOSystem->logMessage("ERROR: Couldn't save settings file", 0);
    return;
  }

  out << "fullscreen = " << getString("fullscreen") << endl
      << "lastrom = " << getString("lastrom") << endl;

  out.flush();
  out.close();
  system("/bin/fsync /mnt/stella/stellarc&");
}
