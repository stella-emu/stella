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
// $Id: Settings.cxx,v 1.6 2003-09-21 14:33:33 stephena Exp $
//============================================================================

#include <assert.h>
#include <sstream>
#include <fstream>

#include "bspf.hxx"
#include "Console.hxx"
#include "EventHandler.hxx"
#include "Settings.hxx"

#ifdef DEVELOPER_SUPPORT
  #include "Props.hxx"
#endif

class Console;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Settings::Settings()
    : myPauseIndicator(false),
      myQuitIndicator(false),
      myConsole(0)
{
  theDesiredFrameRate = 60;
  theKeymapList = "";
  theJoymapList = "";
  theZoomLevel = 1;

#ifdef SNAPSHOT_SUPPORT
  theSnapshotDir = "";
  theSnapshotName = "romname";
  theMultipleSnapshotFlag = true;
#endif
#ifdef DEVELOPER_SUPPORT
  userDefinedProperties.set("Display.Format", "-1");
  userDefinedProperties.set("Display.XStart", "-1");
  userDefinedProperties.set("Display.Width", "-1");
  userDefinedProperties.set("Display.YStart", "-1");
  userDefinedProperties.set("Display.Height", "-1");

  theMergePropertiesFlag = false;
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Settings::~Settings()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Settings::loadConfig()
{
  string line, key, value;
  uInt32 equalPos;

  ifstream in(mySettingsInputFilename.c_str());
  if(!in || !in.is_open())
  {
    cout << "Error: Couldn't load settings file\n";
    return;
  }

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

    set(key, value);
  }

  in.close();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Settings::loadCommandLine(Int32 argc, char** argv)
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

    set(key, value);
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Settings::saveConfig()
{
  if(!myConsole)
    return;

  ofstream out(mySettingsOutputFilename.c_str());
  if(!out || !out.is_open())
  {
    cout << "Error: Couldn't save settings file\n";
    return;
  }

  out << ";  Stella configuration file" << endl
      << ";" << endl
      << ";  Lines starting with ';' are comments and are ignored." << endl
      << ";  Spaces and tabs are ignored." << endl
      << ";" << endl
      << ";  Format MUST be as follows:" << endl
      << ";    command = value" << endl
      << ";" << endl
      << ";  Commmands are the same as those specified on the commandline," << endl
      << ";  without the '-' character." << endl
      << ";" << endl
      << ";  Values are the same as those allowed on the commandline." << endl
      << ";  Boolean values are specified as 1 (for true) and 0 (for false)" << endl
      << ";" << endl
      << "fps = " << theDesiredFrameRate << endl
      << "zoom = " << theZoomLevel << endl
      << "keymap = " << myConsole->eventHandler().getKeymap() << endl
      << "joymap = " << myConsole->eventHandler().getJoymap() << endl
#ifdef SNAPSHOT_SUPPORT
      << "ssdir = " << theSnapshotDir << endl
      << "ssname = " << theSnapshotName << endl
      << "sssingle = " << theMultipleSnapshotFlag << endl
#endif
#ifdef DEVELOPER_SUPPORT
      << "Dmerge = " << theMergePropertiesFlag << endl
#endif
      << getArguments() << endl;

  out.close();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Settings::set(string& key, string& value)
{
  // Now set up the options by key
  if(key == "fps")
  {
    // They're setting the desired frame rate
    uInt32 rate = atoi(value.c_str());
    if(rate < 1)
      cout << "Invalid rate " << rate << endl;
    else
      theDesiredFrameRate = rate;
  }
  else if(key == "zoom")
  {
    // They're setting the initial window size
    uInt32 zoom = atoi(value.c_str());
    if(zoom < 1)
      cout << "Invalid zoom value " << zoom << endl;
    else
      theZoomLevel = zoom;
  }
  else if(key == "keymap")
  {
    theKeymapList = value;
  }
  else if(key == "joymap")
  {
    theJoymapList = value;
  }
#ifdef SNAPSHOT_SUPPORT
  else if(key == "ssdir")
  {
    theSnapshotDir = value;
  }
  else if(key == "ssname")
  {
    if((value != "md5sum") && (value != "romname"))
      cout << "Invalid snapshot name " << value << endl;
    else
      theSnapshotName = value;
  }
  else if(key == "sssingle")
  {
    uInt32 option = atoi(value.c_str());
    if(option == 1)
      theMultipleSnapshotFlag = false;
    else if(option == 0)
      theMultipleSnapshotFlag = true;
  }
#endif
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
    // Pass the key to the derived class to see if it knows
    // what to do with it
    setArgument(key, value);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Settings::Settings(const Settings&)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Settings& Settings::operator = (const Settings&)
{
  assert(false);

  return *this;
}
