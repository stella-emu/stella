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
// Copyright (c) 1995-2005 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: Settings.cxx,v 1.81 2006-03-17 19:44:18 stephena Exp $
//============================================================================

#include <cassert>
#include <sstream>
#include <fstream>

#include "OSystem.hxx"
#include "Version.hxx"
#include "bspf.hxx"
#include "Settings.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Settings::Settings(OSystem* osystem)
  : myOSystem(osystem)
{
  // Add this settings object to the OSystem
  myOSystem->attach(this);

  // Add options that are common to all versions of Stella
  setInternal("video", "soft");
  setInternal("dirtyrects", "true");
  setInternal("ppblend", "77");

  setInternal("gl_filter", "nearest");
  setInternal("gl_aspect", "2.0");
  setInternal("gl_fsmax", "false");
  setInternal("gl_lib", "");

  setInternal("zoom", "2");
  setInternal("fullscreen", "false");
  setInternal("center", "true");
  setInternal("grabmouse", "false");
  setInternal("palette", "standard");
  setInternal("debugheight", "0");

  setInternal("sound", "true");
  setInternal("fragsize", "512");
  setInternal("freq", "31400");
  setInternal("tiafreq", "31400");
  setInternal("volume", "100");
  setInternal("clipvol", "true");

  setInternal("keymap", "");
  setInternal("joymap", "");
  setInternal("joyaxismap", "");
  setInternal("joyhatmap", "");
  setInternal("paddle", "0");
  setInternal("sa1", "left");
  setInternal("sa2", "right");
  setInternal("joymouse", "false");
  setInternal("p1speed", "50");
  setInternal("p2speed", "50");
  setInternal("p3speed", "50");
  setInternal("p4speed", "50");

  setInternal("showinfo", "false");

  setInternal("ssdir", "");
  setInternal("ssname", "romname");
  setInternal("sssingle", "false");

  setInternal("romdir", "");
  setInternal("rombrowse", "false");
  setInternal("lastrom", "");
  setInternal("modtime", "");  // romdir last modification time
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Settings::~Settings()
{
  myInternalSettings.clear();
  myExternalSettings.clear();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Settings::loadConfig()
{
  string line, key, value;
  string::size_type equalPos, garbage;

  ifstream in(myOSystem->configFile().c_str());
  if(!in || !in.is_open())
  {
    cout << "Error: Couldn't load settings file\n";
    return;
  }

  while(getline(in, line))
  {
    // Strip all whitespace and tabs from the line
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
    if(int idx = getInternalPos(key) != -1)
      setInternal(key, value, idx, true);
  }

  in.close();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Settings::loadCommandLine(int argc, char** argv)
{
  for(int i = 1; i < argc; ++i)
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
      setExternal(key, "true");
      return true;
    }
    else if(key == "debug") // this doesn't make Stella exit
    {
      setExternal(key, "true");
      return true;
    }
    else if(key == "holdreset") // this doesn't make Stella exit
    {
      setExternal(key, "true");
      return true;
    }
    else if(key == "holdselect") // this doesn't make Stella exit
    {
      setExternal(key, "true");
      return true;
    }
    else if(key == "holdbutton0") // this doesn't make Stella exit
    {
      setExternal(key, "true");
      return true;
    }

    if(++i >= argc)
    {
      cerr << "Missing argument for '" << key << "'" << endl;
      return false;
    }
    string value = argv[i];

    // Settings read from the commandline must not be saved to 
    // the rc-file, unless they were previously set
    if(int idx = getInternalPos(key) != -1)
      setInternal(key, value, idx, true);
    else
      setExternal(key, value);
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Settings::validate()
{
  string s;
  int i;

  s = getString("video");
  if(s != "soft" && s != "gl")
    setInternal("video", "soft");

#ifdef DISPLAY_OPENGL
  s = getString("gl_filter");
  if(s != "linear" && s != "nearest")
    setInternal("gl_filter", "nearest");

  float f = getFloat("gl_aspect");
  if(f < 1.1 || f > 2.0)
    setInternal("gl_aspect", "2.0");
#endif

#ifdef SOUND_SUPPORT
  i = getInt("fragsize");
  if(i != 256 && i != 512 && i != 1024 && i != 2048 && i != 4096)
  #ifdef WIN32
    setInternal("fragsize", "2048");
  #else
    setInternal("fragsize", "512");
  #endif

  i = getInt("volume");
  if(i < 0 || i > 100)
    setInternal("volume", "100");
  i = getInt("freq");
  if(i < 0 || i > 48000)
    setInternal("freq", "31400");
  i = getInt("tiafreq");
  if(i < 0 || i > 48000)
    setInternal("tiafreq", "31400");
#endif

  i = getInt("zoom");
  if(i < 1 || i > 6)
    setInternal("zoom", "2");

  i = getInt("paddle");
  if(i < 0 || i > 3)
    setInternal("paddle", "0");

  s = getString("palette");
  if(s != "standard" && s != "original" && s != "z26")
    setInternal("palette", "standard");

  i = getInt("ppblend");
  if(i < 0) setInternal("ppblend", "0");
  if(i > 100) setInternal("ppblend", "100");

  s = getString("romname");
  if(s != "romname" && s != "md5sum")
    setInternal("ssname", "romname");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Settings::usage()
{
#ifndef MAC_OSX
  cout << endl
    << "Stella version " << STELLA_VERSION << endl
    << endl
    << "Usage: stella [options ...] romfile" << endl
    << "       Run without any options or romfile to use the ROM launcher" << endl
    << endl
    << "Valid options are:" << endl
    << endl
    << "  -video        <type>         Type is one of the following:\n"
    << "                 soft            SDL software mode\n"
  #ifdef DISPLAY_OPENGL
    << "                 gl              SDL OpenGL mode\n"
    << endl
    << "  -gl_filter    <type>         Type is one of the following:\n"
    << "                 nearest         Normal scaling (GL_NEAREST)\n"
    << "                 linear          Blurred scaling (GL_LINEAR)\n"
    << "  -gl_aspect    <number>       Scale the width by the given amount\n"
    << "  -gl_fsmax     <1|0>          Use the largest available screenmode in fullscreen OpenGL\n"
    << "  -gl_lib       <filename>     Specify the OpenGL library\n"
    << endl
  #endif
    << "  -zoom         <size>         Makes window be 'size' times normal\n"
    << "  -fullscreen   <1|0>          Play the game in fullscreen mode\n"
    << "  -center       <1|0>          Centers game window (if possible)\n"
    << "  -grabmouse    <1|0>          Keeps the mouse in the game window\n"
    << "  -palette      <original|     Use the specified color palette\n"
    << "                 standard|\n"
    << "                 z26>\n"
    << "  -framerate    <number>       Display the given number of frames per second\n"
    << "  -ppblend      <number>       Set blending for phosphor effect, if enabled (0-100)\n"
    << endl
  #ifdef SOUND_SUPPORT
    << "  -sound        <1|0>          Enable sound generation\n"
    << "  -channels     <1|2>          Enable mono or stereo sound\n"
    << "  -fragsize     <number>       The size of sound fragments (must be a power of two)\n"
    << "  -freq         <number>       Set sound sample output frequency (0 - 48000)\n"
    << "  -tiafreq      <number>       Set sound sample generation frequency (0 - 48000)\n"
    << "  -volume       <number>       Set the volume (0 - 100)\n"
    << "  -clipvol      <1|0>          Enable volume clipping (eliminates popping)\n"
    << endl
  #endif
    << "  -cheat        <code>         Use the specified cheatcode (see manual for description)\n"
    << "  -showinfo     <1|0>          Shows some game info\n"
    << "  -paddle       <0|1|2|3>      Indicates which paddle the mouse should emulate\n"
    << "  -sa1          <left|right>   Stelladaptor 1 emulates specified joystick port\n"
    << "  -sa2          <left|right>   Stelladaptor 2 emulates specified joystick port\n"
    << "  -joymouse     <1|0>          Enable mouse emulation using joystick in GUI\n"
    << "  -p1speed      <number>       Speed of emulated mouse movement for paddle 1 (0-100)\n"
    << "  -p2speed      <number>       Speed of emulated mouse movement for paddle 2 (0-100)\n"
    << "  -p3speed      <number>       Speed of emulated mouse movement for paddle 3 (0-100)\n"
    << "  -p4speed      <number>       Speed of emulated mouse movement for paddle 4 (0-100)\n"
  #ifdef UNIX
    << "  -accurate     <1|0>          Accurate game timing (uses more CPU)\n"
  #endif
  #ifdef SNAPSHOT_SUPPORT
    << "  -ssdir        <path>         The directory to save snapshot files to\n"
    << "  -ssname       <name>         How to name the snapshot (romname or md5sum)\n"
    << "  -sssingle     <1|0>          Generate single snapshot instead of many\n"
    << endl
  #endif
    << "  -listrominfo                 Display contents of stella.pro, one line per ROM entry\n"
    << "  -help                        Show the text you're now reading\n"
  #ifdef DEVELOPER_SUPPORT
    << endl
    << " The following options are meant for developers\n"
    << " Arguments are more fully explained in the manual\n"
    << endl
    << "   -break        <address>     Set a breakpoint at 'address'\n"
    << "   -debugheight  <number>      Set height of debugger in lines of text (NOT pixels)\n"
    << "   -debug                      Start in debugger mode\n"
    << "   -holdreset                  Start the emulator with the Game Reset switch held down\n"
    << "   -holdselect                 Start the emulator with the Game Select switch held down\n"
    << "   -holdbutton0                Start the emulator with the left joystick button held down\n"
    << endl
    << "   -pro         <props file>   Use the given properties file instead of stella.pro\n"
    << "   -type        <arg>          Sets the 'Cartridge.Type' property\n"
    << "   -ld          <arg>          Sets the 'Console.LeftDifficulty' property\n"
    << "   -rd          <arg>          Sets the 'Console.RightDifficulty' property\n"
    << "   -tv          <arg>          Sets the 'Console.TelevisionType' property\n"
    << "   -sp          <arg>          Sets the 'Console.SwapPorts' property\n"
    << "   -lc          <arg>          Sets the 'Controller.Left' property\n"
    << "   -rc          <arg>          Sets the 'Controller.Right' property\n"
    << "   -bc          <arg>          Same as using both -lc and -rc\n"
    << "   -format      <arg>          Sets the 'Display.Format' property\n"
    << "   -xstart      <arg>          Sets the 'Display.XStart' property\n"
    << "   -ystart      <arg>          Sets the 'Display.YStart' property\n"
    << "   -width       <arg>          Sets the 'Display.Width' property\n"
    << "   -height      <arg>          Sets the 'Display.Height' property\n"
    << "   -pp          <arg>          Sets the 'Display.Phosphor' property\n"
    << "   -hmove       <arg>          Sets the 'Emulation.HmoveBlanks' property\n"
  #endif
    << endl;
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Settings::saveConfig()
{

  // Do a quick scan of the internal settings to see if any have
  // changed.  If not, we don't need to save them at all.
  bool settingsChanged = false;
  for(unsigned int i = 0; i < myInternalSettings.size(); ++i)
  {
    if(myInternalSettings[i].value != myInternalSettings[i].initialValue)
    {
      settingsChanged = true;
      break;
    }
  }

  if(!settingsChanged)
    return;

  ofstream out(myOSystem->configFile().c_str());
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
  for(unsigned int i = 0; i < myInternalSettings.size(); ++i)
  {
    out << myInternalSettings[i].key << " = " <<
           myInternalSettings[i].value << endl;
  }

  out.close();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Settings::setInt(const string& key, const int value)
{
  ostringstream stream;
  stream << value;

  if(int idx = getInternalPos(key) != -1)
    setInternal(key, stream.str(), idx);
  else
    setExternal(key, stream.str());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Settings::setFloat(const string& key, const float value)
{
  ostringstream stream;
  stream << value;

  if(int idx = getInternalPos(key) != -1)
    setInternal(key, stream.str(), idx);
  else
    setExternal(key, stream.str());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Settings::setBool(const string& key, const bool value)
{
  ostringstream stream;
  stream << value;

  if(int idx = getInternalPos(key) != -1)
    setInternal(key, stream.str(), idx);
  else
    setExternal(key, stream.str());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Settings::setString(const string& key, const string& value)
{
  if(int idx = getInternalPos(key) != -1)
    setInternal(key, value, idx);
  else
    setExternal(key, value);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int Settings::getInt(const string& key) const
{
  // Try to find the named setting and answer its value
  int idx = -1;
  if((idx = getInternalPos(key)) != -1)
    return (int) atoi(myInternalSettings[idx].value.c_str());
  else if((idx = getExternalPos(key)) != -1)
    return (int) atoi(myExternalSettings[idx].value.c_str());
  else
    return 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
float Settings::getFloat(const string& key) const
{
  // Try to find the named setting and answer its value
  int idx = -1;
  if((idx = getInternalPos(key)) != -1)
    return (float) atof(myInternalSettings[idx].value.c_str());
  else if((idx = getExternalPos(key)) != -1)
    return (float) atof(myExternalSettings[idx].value.c_str());
  else
    return -1.0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Settings::getBool(const string& key) const
{
  // Try to find the named setting and answer its value
  int idx = -1;
  if((idx = getInternalPos(key)) != -1)
  {
    if(myInternalSettings[idx].value == "1" ||
       myInternalSettings[idx].value == "true")
      return true;
    else if(myInternalSettings[idx].value == "0" ||
            myInternalSettings[idx].value == "false")
      return false;
    else
      return false;
  }
  else if((idx = getExternalPos(key)) != -1)
  {
    if(myInternalSettings[idx].value == "1" ||
       myInternalSettings[idx].value == "true")
      return true;
    else if(myInternalSettings[idx].value == "0" ||
            myInternalSettings[idx].value == "false")
      return false;
    else
      return false;
  }
  else
    return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const string& Settings::getString(const string& key) const
{
  // Try to find the named setting and answer its value
  int idx = -1;
  if((idx = getInternalPos(key)) != -1)
    return myInternalSettings[idx].value;
  else if((idx = getExternalPos(key)) != -1)
    return myExternalSettings[idx].value;
  else
    return EmptyString;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int Settings::getInternalPos(const string& key) const
{
  for(unsigned int i = 0; i < myInternalSettings.size(); ++i)
    if(myInternalSettings[i].key == key)
      return i;

  return -1;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int Settings::getExternalPos(const string& key) const
{
  for(unsigned int i = 0; i < myExternalSettings.size(); ++i)
    if(myExternalSettings[i].key == key)
      return i;

  return -1;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int Settings::setInternal(const string& key, const string& value,
                          int pos, bool useAsInitial)
{
  int idx = -1;

  if(pos != -1 && pos >= 0 && pos < (int)myInternalSettings.size() &&
     myInternalSettings[pos].key == key)
  {
    idx = pos;
  }
  else
  {
    for(unsigned int i = 0; i < myInternalSettings.size(); ++i)
    {
      if(myInternalSettings[i].key == key)
      {
        idx = i;
        break;
      }
    }
  }

  if(idx != -1)
  {
    myInternalSettings[idx].key   = key;
    myInternalSettings[idx].value = value;
    if(useAsInitial) myInternalSettings[idx].initialValue = value;

    /*cerr << "modify internal: key = " << key
         << ", value = " << value
         << " @ index = " << idx
         << endl;*/
  }
  else
  {
    Setting setting;
    setting.key   = key;
    setting.value = value;
    if(useAsInitial) setting.initialValue = value;

    myInternalSettings.push_back(setting);
    idx = myInternalSettings.size() - 1;

    /*cerr << "insert internal: key = " << key
         << ", value = " << value
         << " @ index = " << idx
         << endl;*/
  }

  return idx;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int Settings::setExternal(const string& key, const string& value,
                          int pos, bool useAsInitial)
{
  int idx = -1;

  if(pos != -1 && pos >= 0 && pos < (int)myExternalSettings.size() &&
     myExternalSettings[pos].key == key)
  {
    idx = pos;
  }
  else
  {
    for(unsigned int i = 0; i < myExternalSettings.size(); ++i)
    {
      if(myExternalSettings[i].key == key)
      {
        idx = i;
        break;
      }
    }
  }

  if(idx != -1)
  {
    myExternalSettings[idx].key   = key;
    myExternalSettings[idx].value = value;
    if(useAsInitial) myExternalSettings[idx].initialValue = value;

    /*cerr << "modify external: key = " << key
         << ", value = " << value
         << " @ index = " << idx
         << endl;*/
  }
  else
  {
    Setting setting;
    setting.key   = key;
    setting.value = value;
    if(useAsInitial) setting.initialValue = value;

    myExternalSettings.push_back(setting);
    idx = myExternalSettings.size() - 1;

    /*cerr << "insert external: key = " << key
         << ", value = " << value
         << " @ index = " << idx
         << endl;*/
  }

  return idx;
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
