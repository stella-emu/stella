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
// $Id: SettingsUNIX.cxx,v 1.1 2003-09-11 20:53:51 stephena Exp $
//============================================================================

#include <fstream>

#include "Console.hxx"
#include "EventHandler.hxx"
#include "StellaEvent.hxx"

#ifdef DEVELOPER_SUPPORT
  #include "Props.hxx"
#endif

#include "Settings.hxx"
#include "SettingsUNIX.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SettingsUNIX::SettingsUNIX(const string& infile, const string& outfile)
    : mySettingsInputFilename(infile),
      mySettingsOutputFilename(outfile)
{
  theKeymapList = "";
  theJoymapList = "";
  theUseFullScreenFlag = false;
  theGrabMouseFlag = false;
  theCenterWindowFlag = false;
  theShowInfoFlag = false;
  theHideCursorFlag = false;
  theUsePrivateColormapFlag = false;
  theMultipleSnapShotFlag = true;
  theAccurateTimingFlag = true;
  theDesiredVolume = -1;
  theDesiredFrameRate = 60;
  thePaddleMode = 0;
  theAlternateProFile = "";
  theSnapShotDir = "";
  theSnapShotName = "";
  theSoundDriver = "oss";
  theWindowSize = 1;
  theLeftJoystickNumber = 0;
  theRightJoystickNumber = 1;

#ifdef DEVELOPER_SUPPORT
  userDefinedProperties.set("Display.Format", "-1");
  userDefinedProperties.set("Display.XStart", "-1");
  userDefinedProperties.set("Display.Width", "-1");
  userDefinedProperties.set("Display.YStart", "-1");
  userDefinedProperties.set("Display.Height", "-1");

  theMergePropertiesFlag = false;
#endif

  handleRCFile();
}

SettingsUNIX::~SettingsUNIX()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SettingsUNIX::setConsole(Console* console)
{
  myConsole = console;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SettingsUNIX::handleCommandLineArgs(int argc, char* argv[])
{
  // Make sure we have the correct number of command line arguments
  if(argc == 1)
    return false;

  for(Int32 i = 1; i < (argc - 1); ++i)
  {
    // strip off the '-' character
    string key = argv[i];
    key = key.substr(1, key.length());
    string value = argv[++i];

    parseArg(key, value);
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SettingsUNIX::handleRCFile()
{
  string line, key, value;
  uInt32 equalPos;

  ifstream in(mySettingsInputFilename.c_str());
  if(!in || !in.is_open())
    return;

  while(getline(in, line))
  {
    // Strip all whitespace and tabs from the line
    uInt32 garbage;
    while((garbage = line.find(" ")) != string::npos)
      line.erase(garbage, 1);
    while((garbage = line.find("\t")) != string::npos)
      line.erase(garbage, 1);

    // Ignore commented and empty lines
    if((line.length() == 0) || (line[0] == ';'))
      continue;

    // Search for the equal sign and discard the line if its not found
    if((equalPos = line.find("=")) == string::npos)
      continue;

    key   = line.substr(0, equalPos);
    value = line.substr(equalPos + 1, line.length() - key.length() - 1);

    // Check for absent key or value
    if((key.length() == 0) || (value.length() == 0))
      continue;

    parseArg(key, value);
  }

  in.close();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SettingsUNIX::parseArg(string& key, string& value)
{
  // Now set up the options by key
  if(key == "fps")
  {
    // They're setting the desired frame rate
    uInt32 rate = atoi(value.c_str());
    if((rate < 1) || (rate > 300))
      cout << "Invalid rate " << rate << " (1-300)\n";
    else
      theDesiredFrameRate = rate;
  }
  else if(key == "paddle")
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
  else if(key == "zoom")
  {
    // They're setting the initial window size
    uInt32 size = atoi(value.c_str());
    if((size < 1) || (size > 4))
      cout << "Invalid zoom value " << size << " (1-4)\n";
    else
      theWindowSize = size;
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
  else if(key == "ssdir")
  {
    theSnapShotDir = value;
  }
  else if(key == "ssname")
  {
    theSnapShotName = value;
  }
  else if(key == "sssingle")
  {
    uInt32 option = atoi(value.c_str());
    if(option == 1)
      theMultipleSnapShotFlag = false;
    else if(option == 0)
      theMultipleSnapShotFlag = true;
  }
  else if(key == "sound")
  {
    if((value != "oss") && (value != "sdl") && (value != "alsa"))
      value = "0";

    theSoundDriver = value;
  }
  else if(key == "keymap")
  {
    theKeymapList = value;
  }
  else if(key == "joymap")
  {
    theJoymapList = value;
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
#ifdef DEVELOPER_SUPPORT
  else if(key == "Dmerge")
  {
    uInt32 option = atoi(value.c_str());
    if(option == 1)
      theMergePropertiesFlag = true;
    else if(option == 0)
      theMergePropertiesFlag = false;
  }
#endif
  else
  {
    cout << "Undefined option " << key << endl;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SettingsUNIX::save()
{
  if(!myConsole)
    return;

  ofstream out(mySettingsOutputFilename.c_str());
  if(!out || !out.is_open())
    return;

  out << "fps = " << theDesiredFrameRate << endl
      << "paddle = " << thePaddleMode << endl
      << "owncmap = " << theUsePrivateColormapFlag << endl
      << "fullscreen = " << theUseFullScreenFlag << endl
      << "grabmouse = " << theGrabMouseFlag << endl
      << "hidecursor = " << theHideCursorFlag << endl
      << "center = " << theCenterWindowFlag << endl
      << "showinfo = " << theShowInfoFlag << endl
      << "accurate = " << theAccurateTimingFlag << endl
      << "zoom = " << theWindowSize << endl
      << "volume = " << theDesiredVolume << endl
      << "ssdir = " << theSnapShotDir << endl
      << "ssname = " << theSnapShotName << endl
      << "sssingle = " << theMultipleSnapShotFlag << endl
      << "sound = " << theSoundDriver << endl
#ifdef DEVELOPER_SUPPORT
      << "Dmerge = " << theMergePropertiesFlag << endl
#endif
      << "keymap = " << myConsole->eventHandler().getKeymap() << endl
      << "joymap = " << myConsole->eventHandler().getJoymap() << endl
      << "joyleft = " << theLeftJoystickNumber << endl
      << "joyright = " << theRightJoystickNumber << endl
      << endl;

  out.close();
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
  #ifdef HAVE_PNG
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
