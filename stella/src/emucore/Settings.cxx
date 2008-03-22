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
// Copyright (c) 1995-2008 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: Settings.cxx,v 1.135 2008-03-22 17:35:02 stephena Exp $
//============================================================================

#include <cassert>
#include <sstream>
#include <fstream>
#include <algorithm>

#include "bspf.hxx"

#include "OSystem.hxx"
#include "Version.hxx"

#include "Settings.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Settings::Settings(OSystem* osystem)
  : myOSystem(osystem)
{
  // Add this settings object to the OSystem
  myOSystem->attach(this);

  // Add options that are common to all versions of Stella
  setInternal("video", "soft");

  // OpenGL specific options
  setInternal("gl_filter", "nearest");
  setInternal("gl_aspect", "100");
  setInternal("gl_fsmax", "never");
  setInternal("gl_lib", "libGL.so");
  setInternal("gl_vsync", "false");
  setInternal("gl_texrect", "false");

  // Framebuffer-related options
  setInternal("zoom_ui", "2");
  setInternal("zoom_tia", "2");
  setInternal("fullscreen", "false");
  setInternal("fullres", "");
  setInternal("center", "true");
  setInternal("grabmouse", "true");
  setInternal("palette", "standard");
  setInternal("colorloss", "false");

  // Sound options
  setInternal("sound", "true");
  setInternal("fragsize", "512");
  setInternal("freq", "31400");
  setInternal("tiafreq", "31400");
  setInternal("volume", "100");
  setInternal("clipvol", "true");

  // Input event options
  setInternal("keymap", "");
  setInternal("joymap", "");
  setInternal("joyaxismap", "");
  setInternal("joyhatmap", "");
  setInternal("paddle", "0");
  setInternal("pspeed", "6");
  setInternal("sa1", "left");
  setInternal("sa2", "right");

  // Snapshot options
  setInternal("ssdir", string(".") + BSPF_PATH_SEPARATOR);
  setInternal("sssingle", "false");

  // Config files and paths
  setInternal("romdir", "");
  setInternal("statedir", "");
  setInternal("cheatfile", "");
  setInternal("palettefile", "");
  setInternal("propsfile", "");

  // ROM browser options
  setInternal("romviewer", "false");

  // UI-related options
  setInternal("debuggerres", "1030x690");
  setInternal("launcherres", "400x300");
  setInternal("uipalette", "0");
  setInternal("mwheel", "4");

  // Misc options
  setInternal("autoslot", "false");
  setInternal("showinfo", "false");
  setInternal("tiafloat", "true");
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
string Settings::loadCommandLine(int argc, char** argv)
{
  for(int i = 1; i < argc; ++i)
  {
    // strip off the '-' character
    string key = argv[i];
    if(key[0] == '-')
    {
      key = key.substr(1, key.length());

      // Take care of the arguments which are meant to be executed immediately
      // (and then Stella should exit)
      if(key == "help" || key == "listrominfo")
      {
        setExternal(key, "true");
        return "";
      }

      // Take care of arguments without an option
      if(key == "rominfo" || key == "debug" || key == "holdreset" ||
         key == "holdselect" || key == "holdbutton0" || key == "takesnapshot")
      {
        setExternal(key, "true");
        continue;
      }

      if(++i >= argc)
      {
        cerr << "Missing argument for '" << key << "'" << endl;
        return "";
      }
      string value = argv[i];

      // Settings read from the commandline must not be saved to 
      // the rc-file, unless they were previously set
      if(int idx = getInternalPos(key) != -1)
        setInternal(key, value, idx);   // don't set initialValue here
      else
        setExternal(key, value);
    }
    else
      return key;
  }

  return "";
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

  i = getInt("gl_aspect");
  if(i < 50 || i > 100)
    setInternal("gl_aspect", "100");

  s = getString("gl_fsmax");
  if(s != "never" && s != "ui" && s != "tia" && s != "always")
    setInternal("gl_fsmax", "never");
#endif

#ifdef SOUND_SUPPORT
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

  i = getInt("zoom_ui");
  if(i < 1 || i > 10)
    setInternal("zoom_ui", "2");

  i = getInt("zoom_tia");
  if(i < 1 || i > 10)
    setInternal("zoom_tia", "2");

  i = getInt("pspeed");
  if(i < 1)
    setInternal("pspeed", "1");
  else if(i > 15)
    setInternal("pspeed", "15");

  s = getString("palette");
  if(s != "standard" && s != "z26" && s != "user")
    setInternal("palette", "standard");
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
    << "  -gl_lib       <name>         Specify the OpenGL library\n"
    << "  -gl_filter    <type>         Type is one of the following:\n"
    << "                 nearest         Normal scaling (GL_NEAREST)\n"
    << "                 linear          Blurred scaling (GL_LINEAR)\n"
    << "  -gl_aspect    <number>       Scale the width by the given percentage\n"
    << "  -gl_fsmax     <never|always| Stretch GL image in fullscreen mode\n"
    << "                 ui|tia\n"
    << "  -gl_vsync     <1|0>          Enable synchronize to vertical blank interrupt\n"
    << "  -gl_texrect   <1|0>          Enable GL_TEXTURE_RECTANGLE extension\n"
    << endl
  #endif
    << "  -zoom_tia     <zoom>         Use the specified zoom level in emulation mode\n"
    << "  -zoom_ui      <zoom>         Use the specified zoom level in non-emulation mode (ROM browser/debugger)\n"
    << "  -fullscreen   <1|0>          Play the game in fullscreen mode\n"
    << "  -fullres      <WxH>          The resolution to use in fullscreen mode\n"
    << "  -center       <1|0>          Centers game window (if possible)\n"
    << "  -grabmouse    <1|0>          Keeps the mouse in the game window\n"
    << "  -palette      <standard|     Use the specified color palette\n"
    << "                 z26|\n"
    << "                 user>\n"
    << "  -colorloss    <1|0>          Enable PAL color-loss effect\n"
    << "  -framerate    <number>       Display the given number of frames per second\n"
    << endl
  #ifdef SOUND_SUPPORT
    << "  -sound        <1|0>          Enable sound generation\n"
    << "  -fragsize     <number>       The size of sound fragments (must be a power of two)\n"
    << "  -freq         <number>       Set sound sample output frequency (0 - 48000)\n"
    << "  -tiafreq      <number>       Set sound sample generation frequency (0 - 48000)\n"
    << "  -volume       <number>       Set the volume (0 - 100)\n"
    << "  -clipvol      <1|0>          Enable volume clipping (eliminates popping)\n"
    << endl
  #endif
    << "  -cheat        <code>         Use the specified cheatcode (see manual for description)\n"
    << "  -showinfo     <1|0>          Shows some game info\n"
    << "  -pspeed       <number>       Speed of digital emulated paddle movement (1-15)\n"
    << "  -sa1          <left|right>   Stelladaptor 1 emulates specified joystick port\n"
    << "  -sa2          <left|right>   Stelladaptor 2 emulates specified joystick port\n"
    << "  -romviewer    <1|0>          Show ROM info viewer in ROM launcher\n"
    << "  -autoslot     <1|0>          Automatically switch to next save slot when state saving\n"
    << "  -ssdir        <path>         The directory to save snapshot files to\n"
    << "  -sssingle     <1|0>          Generate single snapshot instead of many\n"
    << endl
    << "  -listrominfo                 Display contents of stella.pro, one line per ROM entry\n"
    << "  -rominfo      <rom>          Display detailed information for the given ROM\n"
    << "  -launcherres  <WxH>          The resolution to use in ROM launcher mode\n"
    << "  -uipalette    <1|2>          Used the specified palette for UI elements\n"
    << "  -mwheel       <lines>        Number of lines the mouse wheel will scroll in UI\n"
    << "  -statedir     <dir>          Directory in which to save state files\n"
    << "  -cheatfile    <file>         Full pathname of cheatfile database\n"
    << "  -palettefile  <file>         Full pathname of user-defined palette file\n"
    << "  -propsfile    <file>         Full pathname of ROM properties file\n"
    << "  -help                        Show the text you're now reading\n"
  #ifdef DEBUGGER_SUPPORT
    << endl
    << " The following options are meant for developers\n"
    << " Arguments are more fully explained in the manual\n"
    << endl
    << "   -debuggerres  <WxH>         The resolution to use in debugger mode\n"
    << "   -break        <address>     Set a breakpoint at 'address'\n"
    << "   -debug                      Start in debugger mode\n"
    << "   -holdreset                  Start the emulator with the Game Reset switch held down\n"
    << "   -holdselect                 Start the emulator with the Game Select switch held down\n"
    << "   -holdbutton0                Start the emulator with the left joystick button held down\n"
    << "   -tiafloat     <1|0>         Set unused TIA pins floating on a read/peek\n"
    << endl
    << "   -bs          <arg>          Sets the 'Cartridge.Type' (bankswitch) property\n"
    << "   -type        <arg>          Same as using -bs\n"
    << "   -channels    <arg>          Sets the 'Cartridge.Sound' property\n"
    << "   -ld          <arg>          Sets the 'Console.LeftDifficulty' property\n"
    << "   -rd          <arg>          Sets the 'Console.RightDifficulty' property\n"
    << "   -tv          <arg>          Sets the 'Console.TelevisionType' property\n"
    << "   -sp          <arg>          Sets the 'Console.SwapPorts' property\n"
    << "   -lc          <arg>          Sets the 'Controller.Left' property\n"
    << "   -rc          <arg>          Sets the 'Controller.Right' property\n"
    << "   -bc          <arg>          Same as using both -lc and -rc\n"
    << "   -cp          <arg>          Sets the 'Controller.SwapPaddles' property\n"
    << "   -format      <arg>          Sets the 'Display.Format' property\n"
    << "   -ystart      <arg>          Sets the 'Display.YStart' property\n"
    << "   -height      <arg>          Sets the 'Display.Height' property\n"
    << "   -pp          <arg>          Sets the 'Display.Phosphor' property\n"
    << "   -ppblend     <arg>          Sets the 'Display.PPBlend' property\n"
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
void Settings::getSize(const string& key, int& x, int& y) const
{
  string size = getString(key);
  replace(size.begin(), size.end(), 'x', ' ');
  istringstream buf(size);
  buf >> x;
  buf >> y;
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
    return -1;
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
    const string& value = myInternalSettings[idx].value;
    if(value == "1" || value == "true")
      return true;
    else if(value == "0" || value == "false")
      return false;
    else
      return false;
  }
  else if((idx = getExternalPos(key)) != -1)
  {
    const string& value = myExternalSettings[idx].value;
    if(value == "1" || value == "true")
      return true;
    else if(value == "0" || value == "false")
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
void Settings::setSize(const string& key, const int value1, const int value2)
{
  ostringstream buf;
  buf << value1 << "x" << value2;
  setString(key, buf.str());
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
         << ", value  = " << value
         << ", ivalue = " << myInternalSettings[idx].initialValue
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
         << ", value  = " << value
         << ", ivalue = " << setting.initialValue
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
