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
// $Id: SettingsUNIX.cxx,v 1.3 2003-09-19 15:45:01 stephena Exp $
//============================================================================

#include <cstdlib>
#include <sstream>
#include <fstream>

#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "bspf.hxx"
#include "Console.hxx"
#include "EventHandler.hxx"
#include "StellaEvent.hxx"

#include "Settings.hxx"
#include "SettingsUNIX.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SettingsUNIX::SettingsUNIX()
{
  // Initialize UNIX specific settings
  theUseFullScreenFlag = false;
  theGrabMouseFlag = false;
  theCenterWindowFlag = false;
  theShowInfoFlag = false;
  theHideCursorFlag = false;
  theUsePrivateColormapFlag = false;
  theAccurateTimingFlag = true;
  theDesiredVolume = -1;
  thePaddleMode = 0;
  theAlternateProFile = "";
  theSoundDriver = "oss";
  theLeftJoystickNumber = 0;
  theRightJoystickNumber = 1;

  // Set up user specific filenames
  myHomeDir = getenv("HOME");
  string basepath = myHomeDir + "/.stella";

  if(access(basepath.c_str(), R_OK|W_OK|X_OK) != 0 )
    mkdir(basepath.c_str(), 0777);

  myStateDir = basepath + "/state/";
  if(access(myStateDir.c_str(), R_OK|W_OK|X_OK) != 0 )
    mkdir(myStateDir.c_str(), 0777);

  myUserPropertiesFile   = basepath + "/stella.pro";
  mySystemPropertiesFile = "/etc/stella.pro";
  myUserConfigFile       = basepath + "/stellarc";
  mySystemConfigFile     = "/etc/stellarc";

  // Set up the names of the input and output config files
  mySettingsOutputFilename = myUserConfigFile;
  if(access(myUserConfigFile.c_str(), R_OK) == 0)
    mySettingsInputFilename = myUserConfigFile;
  else
    mySettingsInputFilename = mySystemConfigFile;

  mySnapshotFile = "";
  myStateFile    = "";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SettingsUNIX::~SettingsUNIX()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SettingsUNIX::setArgument(string& key, string& value)
{
  // Now set up the options by key
  if(key == "paddle")
  {
    // They're trying to set the paddle emulation mode
    uInt32 pMode;
    if(value == "real")
    {
      thePaddleMode = 4;
    }
    else
    {
      pMode = atoi(value.c_str());
      if((pMode > 0) && (pMode < 4))
        thePaddleMode = pMode;
    }
  }
  else if(key == "owncmap")
  {
    uInt32 option = atoi(value.c_str());
    if(option == 1)
      theUsePrivateColormapFlag = true;
    else if(option == 0)
      theUsePrivateColormapFlag = false;
  }
  else if(key == "fullscreen")
  {
    uInt32 option = atoi(value.c_str());
    if(option == 1)
      theUseFullScreenFlag = true;
    else if(option == 0)
      theUseFullScreenFlag = false;
  }
  else if(key == "grabmouse")
  {
    uInt32 option = atoi(value.c_str());
    if(option == 1)
      theGrabMouseFlag = true;
    else if(option == 0)
      theGrabMouseFlag = false;
  }
  else if(key == "hidecursor")
  {
    uInt32 option = atoi(value.c_str());
    if(option == 1)
      theHideCursorFlag = true;
    else if(option == 0)
      theHideCursorFlag = false;
  }
  else if(key == "center")
  {
    uInt32 option = atoi(value.c_str());
    if(option == 1)
      theCenterWindowFlag = true;
    else if(option == 0)
      theCenterWindowFlag = false;
  }
  else if(key == "showinfo")
  {
    uInt32 option = atoi(value.c_str());
    if(option == 1)
      theShowInfoFlag = true;
    else if(option == 0)
      theShowInfoFlag = false;
  }
  else if(key == "accurate")
  {
    uInt32 option = atoi(value.c_str());
    if(option == 1)
      theAccurateTimingFlag = true;
    else if(option == 0)
      theAccurateTimingFlag = false;
  }
  else if(key == "volume")
  {
    // They're setting the desired volume
    Int32 volume = atoi(value.c_str());
    if(volume < -1)
      volume = -1;
    else if(volume > 100)
      volume = 100;

    theDesiredVolume = volume;
  }
  else if(key == "sound")
  {
    if((value != "oss") && (value != "sdl") && (value != "alsa"))
      value = "0";

    theSoundDriver = value;
  }
  else if(key == "joyleft")
  {
    Int32 joynum = atoi(value.c_str());
    if((joynum < 0) || (joynum >= StellaEvent::LastJSTICK))
      cout << "Invalid left joystick.\n";
    else
      theLeftJoystickNumber = joynum;
  }
  else if(key == "joyright")
  {
    Int32 joynum = atoi(value.c_str());
    if((joynum < 0) || (joynum >= StellaEvent::LastJSTICK))
      cout << "Invalid right joystick.\n";
    else
      theRightJoystickNumber = joynum;
  }
  else
  {
    cout << "Undefined option " << key << endl;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string SettingsUNIX::getArguments()
{
  ostringstream buf;

  buf << "paddle = " << thePaddleMode << endl
      << "owncmap = " << theUsePrivateColormapFlag << endl
      << "fullscreen = " << theUseFullScreenFlag << endl
      << "grabmouse = " << theGrabMouseFlag << endl
      << "hidecursor = " << theHideCursorFlag << endl
      << "center = " << theCenterWindowFlag << endl
      << "showinfo = " << theShowInfoFlag << endl
      << "accurate = " << theAccurateTimingFlag << endl
      << "zoom = " << theZoomLevel << endl
      << "volume = " << theDesiredVolume << endl
      << "sound = " << theSoundDriver << endl
      << "joyleft = " << theLeftJoystickNumber << endl
      << "joyright = " << theRightJoystickNumber << endl;

  return buf.str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SettingsUNIX::usage(string& message)
{
  cout << endl
    << message << endl
    << endl
    << "Valid options are:" << endl
    << endl
    << "  -fps        <number>       Display the given number of frames per second\n"
    << "  -zoom       <size>         Makes window be 'size' times normal\n"
    << "  -owncmap    <0|1>          Install a private colormap\n"
    << "  -fullscreen <0|1>          Play the game in fullscreen mode\n"
    << "  -grabmouse  <0|1>          Keeps the mouse in the game window\n"
    << "  -hidecursor <0|1>          Hides the mouse cursor in the game window\n"
    << "  -center     <0|1>          Centers the game window onscreen\n"
    << "  -volume     <number>       Set the volume (0 - 100)\n"
#ifdef HAVE_JOYSTICK
    << "  -paddle     <0|1|2|3|real> Indicates which paddle the mouse should emulate\n"
    << "                             or that real Atari 2600 paddles are being used\n"
    << "  -joyleft    <number>       The joystick number representing the left controller\n"
    << "  -joyright   <number>       The joystick number representing the right controller\n"
#else
    << "  -paddle     <0|1|2|3>      Indicates which paddle the mouse should emulate\n"
#endif
    << "  -pro        <props file>   Use the given properties file instead of stella.pro\n"
    << "  -showinfo   <0|1>          Shows some game info\n"
    << "  -accurate   <0|1>          Accurate game timing (uses more CPU)\n"
  #ifdef SNAPSHOT_SUPPORT
    << "  -ssdir      <path>         The directory to save snapshot files to\n"
    << "  -ssname     <name>         How to name the snapshot (romname or md5sum)\n"
    << "  -sssingle   <0|1>          Generate single snapshot instead of many\n"
  #endif
    << endl
    << "  -sound      <type>          Type is one of the following:\n"
    << "               0                Disables all sound generation\n"
  #ifdef SOUND_ALSA
    << "               alsa             ALSA version 0.9 driver\n"
  #endif
  #ifdef SOUND_OSS
    << "               oss              Open Sound System driver\n"
  #endif
    << endl
  #ifdef DEVELOPER_SUPPORT
    << " DEVELOPER options (see Stella manual for details)\n"
    << "  -Dformat                    Sets \"Display.Format\"\n"
    << "  -Dxstart                    Sets \"Display.XStart\"\n"
    << "  -Dwidth                     Sets \"Display.Width\"\n"
    << "  -Dystart                    Sets \"Display.YStart\"\n"
    << "  -Dheight                    Sets \"Display.Height\"\n"
    << "  -Dmerge     <0|1>           Merge changed properties into properties file,\n"
    << "                              or save into a separate file\n"
  #endif
    << endl;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string SettingsUNIX::stateFilename(uInt32 state)
{
  if(!myConsole)
    return "";

  ostringstream buf;
  buf << myStateDir << myConsole->properties().get("Cartridge.MD5")
      << ".st" << state;

  myStateFile = buf.str();
  return myStateFile;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string SettingsUNIX::snapshotFilename()
{
  if(!myConsole)
    return "";

  string filename;
  string path = theSnapshotDir;

  if(theSnapshotName == "romname")
    path = path + "/" + myConsole->properties().get("Cartridge.Name");
  else if(theSnapshotName == "md5sum")
    path = path + "/" + myConsole->properties().get("Cartridge.MD5");

  // Replace all spaces in name with underscores
  replace(path.begin(), path.end(), ' ', '_');

  // Check whether we want multiple snapshots created
  if(theMultipleSnapshotFlag)
  {
    // Determine if the file already exists, checking each successive filename
    // until one doesn't exist
    filename = path + ".png";
    if(access(filename.c_str(), F_OK) == 0 )
    {
      ostringstream buf;
      for(uInt32 i = 1; ;++i)
      {
        buf.str("");
        buf << path << "_" << i << ".png";
        if(access(buf.str().c_str(), F_OK) == -1 )
          break;
      }
      filename = buf.str();
    }
  }
  else
    filename = path + ".png";

  mySnapshotFile = filename;
  return mySnapshotFile;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string SettingsUNIX::userHomeDir()
{
  return myHomeDir;
}
