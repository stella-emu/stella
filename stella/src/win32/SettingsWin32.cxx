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
// Copyright (c) 1995-1999 by Bradford W. Mott
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: SettingsWin32.cxx,v 1.6 2004-06-28 23:13:53 stephena Exp $
//============================================================================

#include <sstream>
#include <fstream>
#include <direct.h>

#include "bspf.hxx"
#include "Settings.hxx"
#include "SettingsWin32.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SettingsWin32::SettingsWin32()
{
  // FIXME - there really should be code here to determine which version
  // of Windows is being used.
  // If using a version which supports multiple users (NT and above),
  // the relevant directories should be created in per-user locations.
  // For now, we just put it in the same directory as the executable.

  // First set variables that the parent class needs
  myBaseDir = ".\\";
  string stelladir = myBaseDir;

  myStateDir = stelladir + "state\\";
  _mkdir(myStateDir.c_str());

  myUserPropertiesFile   = stelladir + "stella.pro";
  mySystemPropertiesFile = stelladir + "stella.pro";
  myUserConfigFile       = stelladir + "stella.ini";
  mySystemConfigFile     = stelladir + "stella.ini";

  // Set up the names of the input and output config files
  mySettingsOutputFilename = myUserConfigFile;
  if(fileExists(myUserConfigFile))
    mySettingsInputFilename = myUserConfigFile;
  else
    mySettingsInputFilename = mySystemConfigFile;

  mySnapshotFile = "";
  myStateFile    = "";

  // Now create Win32 specific settings
  set("romdir", "roms");
  set("accurate", "false");  // Don't change this, or the sound will skip
  set("fragsize", "2048");   // Anything less than this usually causes sound skipping
#ifdef SNAPSHOT_SUPPORT
  set("ssdir", ".\\");
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SettingsWin32::~SettingsWin32()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SettingsWin32::usage(string& message)
{
  cout << endl
    << message << endl
    << endl
    << "Valid options are:" << endl
    << endl
    << "  -video      <type>         Type is one of the following:\n"
    << "               soft            SDL software mode\n"
  #ifdef DISPLAY_OPENGL
    << "               gl              SDL OpenGL mode\n"
    << endl
    << "  -gl_filter  <type>         Type is one of the following:\n"
    << "               nearest         Normal scaling (GL_NEAREST)\n"
    << "               linear          Blurred scaling (GL_LINEAR)\n"
    << "  -gl_aspect  <number>       Scale the width by the given amount\n"
    << endl
  #endif
    << "  -sound      <0|1>          Enable sound generation\n"
    << "  -fragsize   <number>       The size of sound fragments (must be a power of two)\n"
    << "  -framerate  <number>       Display the given number of frames per second\n"
    << "  -zoom       <size>         Makes window be 'size' times normal\n"
    << "  -fullscreen <0|1>          Play the game in fullscreen mode\n"
    << "  -grabmouse  <0|1>          Keeps the mouse in the game window\n"
    << "  -hidecursor <0|1>          Hides the mouse cursor in the game window\n"
    << "  -volume     <number>       Set the volume (0 - 100)\n"
    << "  -paddle     <0|1|2|3>      Indicates which paddle the mouse should emulate\n"
    << "  -altpro     <props file>   Use the given properties file instead of stella.pro\n"
    << "  -showinfo   <0|1>          Shows some game info\n"
    << "  -accurate   <0|1>          Accurate game timing (uses more CPU)\n"
  #ifdef SNAPSHOT_SUPPORT
    << "  -ssdir      <path>         The directory to save snapshot files to\n"
    << "  -ssname     <name>         How to name the snapshot (romname or md5sum)\n"
    << "  -sssingle   <0|1>          Generate single snapshot instead of many\n"
  #endif
    << endl
  #ifdef DEVELOPER_SUPPORT
    << " DEVELOPER options (see Stella manual for details)\n"
    << "  -Dformat                   Sets \"Display.Format\"\n"
    << "  -Dxstart                   Sets \"Display.XStart\"\n"
    << "  -Dwidth                    Sets \"Display.Width\"\n"
    << "  -Dystart                   Sets \"Display.YStart\"\n"
    << "  -Dheight                   Sets \"Display.Height\"\n"
    << "  -mergeprops  <0|1>         Merge changed properties into properties file,\n"
    << "                             or save into a separate file\n"
  #endif
    << endl;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string SettingsWin32::stateFilename(const string& md5, uInt32 state)
{
  ostringstream buf;
  buf << myStateDir << md5 << ".st" << state;

  myStateFile = buf.str();

  return myStateFile;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SettingsWin32::fileExists(const string& filename)
{
  // FIXME - Since I don't have time to figure out the correct
  // and fast 'Win32' way of doing this, I'll cheat a bit
  ifstream in(filename.c_str());
  if(in)
  {
    in.close();
    return true;
  }

  return false;
}
