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
// $Id: Settings.cxx,v 1.16 2003-12-05 19:51:09 stephena Exp $
//============================================================================

#include <cassert>
#include <sstream>
#include <fstream>

#include "bspf.hxx"
#include "Console.hxx"
#include "EventHandler.hxx"
#include "Settings.hxx"

#ifdef DEVELOPER_SUPPORT
  #include "Props.hxx"
#endif


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Settings::Settings()
  :  myConsole(0)
{
  // First create the settings array
  myCapacity = 30;
  mySettings = new Setting[myCapacity];
  mySize = 0;

  // Now fill it with options that are common to all versions of Stella
  set("framerate", "60");
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
#ifdef DEVELOPER_SUPPORT
  userDefinedProperties.set("Display.Format", "-1");
  userDefinedProperties.set("Display.XStart", "-1");
  userDefinedProperties.set("Display.Width", "-1");
  userDefinedProperties.set("Display.YStart", "-1");
  userDefinedProperties.set("Display.Height", "-1");
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
    return false;

  for(Int32 i = 1; i < (argc - 1); ++i)
  {
    // strip off the '-' character
    string key = argv[i];
    key = key.substr(1, key.length());
    string value = argv[++i];

    // Settings read from the commandline must not be saved to 
    // the rc-file, unless they were previously set
    set(key, value, false);
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Settings::saveConfig()
{
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
void Settings::setInt(const string& key, const uInt32 value)
{
  ostringstream stream;
  stream << value;

  set(key, stream.str());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Settings::setFloat(const string& key, const float value)
{
  ostringstream stream;
  stream << value;

  set(key, stream.str());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Settings::setBool(const string& key, const bool value)
{
  ostringstream stream;
  stream << value;

  set(key, stream.str());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Settings::setString(const string& key, const string& value)
{
  ostringstream stream;
  stream << value;

  set(key, stream.str());
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
