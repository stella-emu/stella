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
// $Id: Settings.cxx,v 1.30 2004-09-14 16:10:28 stephena Exp $
//============================================================================

#include <cassert>
#include <sstream>
#include <fstream>

#include "bspf.hxx"
#include "Settings.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Settings::Settings()
{
  // First create the settings array
  myCapacity = 30;
  mySettings = new Setting[myCapacity];
  mySize = 0;

  // Now fill it with options that are common to all versions of Stella
  set("video", "soft");
  set("video_driver", "");
#ifdef DISPLAY_OPENGL
  set("gl_filter", "nearest");
  set("gl_aspect", "2");
  set("gl_fsmax", "false");
#endif
  set("sound", "true");
  set("fragsize", "512");
  set("fullscreen", "false");
  set("grabmouse", "false");
  set("hidecursor", "false");
  set("volume", "-1");
  set("accurate", "false");
  set("framerate", "-1");
  set("keymap", "");
  set("joymap", "");
  set("zoom", "1");
  set("showinfo", "false");
  set("mergeprops", "false");
  set("paddle", "0");
  set("palette", "standard");
#ifdef SNAPSHOT_SUPPORT
  set("ssdir", ".");
  set("ssname", "romname");
  set("sssingle", "false");
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Settings::~Settings()
{
  // Free the settings array
  delete[] mySettings;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Settings::loadConfig()
{
  string line, key, value;
  uInt32 equalPos;

  ifstream in(myConfigInputFile.c_str());
  if(!in || !in.is_open())
  {
    cout << "Error: Couldn't load settings file\n";
    return;
  }

  while(getline(in, line))
  {
    // Strip all whitespace and tabs from the line
    uInt32 garbage;
    while((garbage = line.find("\t")) != string::npos)
      line.erase(garbage, 1);

    // Ignore commented and empty lines
    if((line.length() == 0) || (line[0] == ';'))
      continue;

    // Search for the equal sign and discard the line if its not found
    if((equalPos = line.find("=")) == string::npos)
      continue;

    // Split the line into key/value pairs and trim any whitespace
    key   = line.substr(0, equalPos);
    value = line.substr(equalPos + 1, line.length() - key.length() - 1);
    key   = trim(key);
    value = trim(value);

    // Check for absent key or value
    if((key.length() == 0) || (value.length() == 0))
      continue;

    // Only settings which have been previously set are valid
    if(contains(key))
      set(key, value);
    else
      cerr << "Invalid setting: " << key << endl;
  }

  in.close();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Settings::loadCommandLine(Int32 argc, char** argv)
{
  // Make sure we have the correct number of command line arguments
  if(argc == 1)
  {
    usage();
    return false;
  }

  for(Int32 i = 1; i < argc; ++i)
  {
    // strip off the '-' character
    string key = argv[i];
    if(key[0] != '-')
      return true;     // stop processing here, ignore the remaining items

    key = key.substr(1, key.length());

    // Take care of the arguments which are meant to be executed immediately
    // (and then Stella should exit)
    if(key == "help")
    {
      usage();
      return false;
    }
    else if(key == "listrominfo")
    {
      set(key, "true", false);  // this confusing line means set 'listrominfo'
      return true;              // to true, but don't save to the settings file
    }

    if(++i >= argc)
    {
      cerr << "Missing argument for '" << key << "'" << endl;
      return false;
    }
    string value = argv[i];

    // Settings read from the commandline must not be saved to 
    // the rc-file, unless they were previously set
    set(key, value, false);
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Settings::usage()
{
#ifndef MAC_OSX
  cout << endl
    << "Stella version 1.4.2_cvs\n\nUsage: stella [options ...] romfile" << endl
    << endl
    << "Valid options are:" << endl
    << endl
    << "  -video        <type>         Type is one of the following:\n"
    << "                 soft            SDL software mode\n"
  #ifdef DISPLAY_OPENGL
    << "                 gl              SDL OpenGL mode\n"
    << "  -video_driver <type>         SDL Video driver (see manual).\n"
    << endl
    << "  -gl_filter    <type>         Type is one of the following:\n"
    << "                 nearest         Normal scaling (GL_NEAREST)\n"
    << "                 linear          Blurred scaling (GL_LINEAR)\n"
    << "  -gl_aspect    <number>       Scale the width by the given amount\n"
    << "  -gl_fsmax     <1|0>          Use the largest available screenmode in fullscreen OpenGL\n"
    << endl
  #endif
    << "  -sound        <1|0>          Enable sound generation\n"
    << "  -fragsize     <number>       The size of sound fragments (must be a power of two)\n"
    << "  -framerate    <number>       Display the given number of frames per second\n"
    << "  -zoom         <size>         Makes window be 'size' times normal\n"
    << "  -fullscreen   <1|0>          Play the game in fullscreen mode\n"
    << "  -grabmouse    <1|0>          Keeps the mouse in the game window\n"
    << "  -hidecursor   <1|0>          Hides the mouse cursor in the game window\n"
    << "  -volume       <number>       Set the volume (0 - 100)\n"
    << "  -paddle       <0|1|2|3>      Indicates which paddle the mouse should emulate\n"
    << "  -altpro       <props file>   Use the given properties file instead of stella.pro\n"
    << "  -showinfo     <1|0>          Shows some game info\n"
    << "  -accurate     <1|0>          Accurate game timing (uses more CPU)\n"
  #ifdef SNAPSHOT_SUPPORT
    << "  -ssdir        <path>         The directory to save snapshot files to\n"
    << "  -ssname       <name>         How to name the snapshot (romname or md5sum)\n"
    << "  -sssingle     <1|0>          Generate single snapshot instead of many\n"
  #endif
    << "  -mergeprops   <1|0>          Merge changed properties into properties file,\n"
    << "                               or save into a separate file\n"
    << endl;
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Settings::saveConfig()
{
  ofstream out(myConfigOutputFile.c_str());
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
      << ";  Boolean values are specified as 1 (or true) and 0 (or false)" << endl
      << ";" << endl;

  // Write out each of the key and value pairs
  for(uInt32 i = 0; i < mySize; ++i)
    if(mySettings[i].save)
      out << mySettings[i].key << " = " << mySettings[i].value << endl;

  out.close();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Settings::set(const string& key, const string& value, bool save)
{
  // See if the setting already exists
  for(uInt32 i = 0; i < mySize; ++i)
  {
    // If a key is already present in the array, then we assume
    // that it was set by the emulation core and must be saved
    // to the rc-file.
    if(key == mySettings[i].key)
    {
      mySettings[i].value = value;
      mySettings[i].save  = true;
      return;
    }
  }

  // See if the array needs to be resized
  if(mySize == myCapacity)
  {
    // Yes, so we'll make the array twice as large
    Setting* newSettings = new Setting[myCapacity * 2];

    for(uInt32 i = 0; i < mySize; ++i)
    {
      newSettings[i] = mySettings[i];
    }

    delete[] mySettings;

    mySettings = newSettings;
    myCapacity *= 2;
  } 

  // Add new property to the array
  mySettings[mySize].key   = key;
  mySettings[mySize].value = value;
  mySettings[mySize].save  = save;

  ++mySize;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Settings::setInt(const string& key, const uInt32 value, bool save)
{
  ostringstream stream;
  stream << value;

  set(key, stream.str(), save);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Settings::setFloat(const string& key, const float value, bool save)
{
  ostringstream stream;
  stream << value;

  set(key, stream.str(), save);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Settings::setBool(const string& key, const bool value, bool save)
{
  ostringstream stream;
  stream << value;

  set(key, stream.str(), save);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Settings::setString(const string& key, const string& value, bool save)
{
  ostringstream stream;
  stream << value;

  set(key, stream.str(), save);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Int32 Settings::getInt(const string& key) const
{
  // Try to find the named setting and answer its value
  for(uInt32 i = 0; i < mySize; ++i)
    if(key == mySettings[i].key)
      return atoi(mySettings[i].value.c_str());

  return -1;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
float Settings::getFloat(const string& key) const
{
  // Try to find the named setting and answer its value
  for(uInt32 i = 0; i < mySize; ++i)
    if(key == mySettings[i].key)
      return (float) atof(mySettings[i].value.c_str());

  return -1.0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Settings::getBool(const string& key) const
{
  // Try to find the named setting and answer its value
  for(uInt32 i = 0; i < mySize; ++i)
  {
    if(key == mySettings[i].key)
    {
      if(mySettings[i].value == "1" || mySettings[i].value == "true")
        return true;
      else if(mySettings[i].value == "0" || mySettings[i].value == "false")
        return false;
      else
        return false;
    }
  }

  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string Settings::getString(const string& key) const
{
  // Try to find the named setting and answer its value
  for(uInt32 i = 0; i < mySize; ++i)
    if(key == mySettings[i].key)
      return mySettings[i].value;

  return "";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Settings::contains(const string& key)
{
  // Try to find the named setting
  for(uInt32 i = 0; i < mySize; ++i)
    if(key == mySettings[i].key)
      return true;

  return false;
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
