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

#include <cassert>

#include "bspf.hxx"

#include "OSystem.hxx"
#include "Version.hxx"
#include "AudioSettings.hxx"

#ifdef DEBUGGER_SUPPORT
  #include "DebuggerDialog.hxx"
#endif

#include "Settings.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Settings::Settings(OSystem& osystem)
  : myOSystem(osystem)
{
  // Video-related options
  setInternal("video", "");
  setInternal("speed", "1.0");
  setInternal("vsync", "true");
  setInternal("fullscreen", "false");
  setInternal("center", "false");
  setInternal("palette", "standard");
  setInternal("uimessages", "true");

  // TIA specific options
  setInternal("tia.zoom", "3");
  setInternal("tia.inter", "false");
  setInternal("tia.aspectn", "91");
  setInternal("tia.aspectp", "109");
  setInternal("tia.fsfill", "false");
  setInternal("tia.dbgcolors", "roygpb");

  // TV filtering options
  setInternal("tv.filter", "0");
  setInternal("tv.phosphor", "byrom");
  setInternal("tv.phosblend", "50");
  setInternal("tv.scanlines", "25");
  setInternal("tv.scaninter", "true");
  // TV options when using 'custom' mode
  setInternal("tv.contrast", "0.0");
  setInternal("tv.brightness", "0.0");
  setInternal("tv.hue", "0.0");
  setInternal("tv.saturation", "0.0");
  setInternal("tv.gamma", "0.0");
  setInternal("tv.sharpness", "0.0");
  setInternal("tv.resolution", "0.0");
  setInternal("tv.artifacts", "0.0");
  setInternal("tv.fringing", "0.0");
  setInternal("tv.bleed", "0.0");

  // Sound options
  setInternal(AudioSettings::SETTING_ENABLED, AudioSettings::DEFAULT_ENABLED);
  setInternal(AudioSettings::SETTING_VOLUME, AudioSettings::DEFAULT_VOLUME);
  setInternal(AudioSettings::SETTING_STEREO, AudioSettings::DEFAULT_STEREO);
  setInternal(AudioSettings::SETTING_PRESET, static_cast<int>(AudioSettings::DEFAULT_PRESET));
  setInternal(AudioSettings::SETTING_SAMPLE_RATE, AudioSettings::DEFAULT_SAMPLE_RATE);
  setInternal(AudioSettings::SETTING_FRAGMENT_SIZE, AudioSettings::DEFAULT_FRAGMENT_SIZE);
  setInternal(AudioSettings::SETTING_BUFFER_SIZE, AudioSettings::DEFAULT_BUFFER_SIZE);
  setInternal(AudioSettings::SETTING_HEADROOM, AudioSettings::DEFAULT_HEADROOM);
  setInternal(AudioSettings::SETTING_RESAMPLING_QUALITY, static_cast<int>(AudioSettings::DEFAULT_RESAMPLING_QUALITY));

  // Input event options
  setInternal("keymap", "");
  setInternal("joymap", "");
  setInternal("combomap", "");
  setInternal("joydeadzone", "13");
  setInternal("joyallow4", "false");
  setInternal("usemouse", "analog");
  setInternal("grabmouse", "true");
  setInternal("cursor", "2");
  setInternal("dsense", "10");
  setInternal("msense", "10");
  setInternal("tsense", "10");
  setInternal("saport", "lr");
  setInternal("ctrlcombo", "true");

  // Snapshot options
  setInternal("snapsavedir", "");
  setInternal("snaploaddir", "");
  setInternal("snapname", "int");
  setInternal("sssingle", "false");
  setInternal("ss1x", "false");
  setInternal("ssinterval", "2");

  // Config files and paths
  setInternal("romdir", "");
  setInternal("statedir", "");
  setInternal("cheatfile", "");
  setInternal("palettefile", "");
  setInternal("propsfile", "");
  setInternal("nvramdir", "");
  setInternal("cfgdir", "");

  // ROM browser options
  setInternal("exitlauncher", "false");
  setInternal("launcherres", GUI::Size(900, 600));
  setInternal("launcherfont", "medium");
  setInternal("launcherroms", "true");
  setInternal("romviewer", "1");
  setInternal("lastrom", "");

  // UI-related options
#ifdef DEBUGGER_SUPPORT
  setInternal("dbg.res",
    GUI::Size(DebuggerDialog::kMediumFontMinW,
              DebuggerDialog::kMediumFontMinH));
#endif
  setInternal("uipalette", "standard");
  setInternal("listdelay", "300");
  setInternal("mwheel", "4");

  // Misc options
  setInternal("autoslot", "false");
  setInternal("loglevel", "1");
  setInternal("logtoconsole", "0");
  setInternal("avoxport", "");
  setInternal("fastscbios", "true");
  setInternal("threads", "false");
  setExternal("romloadcount", "0");
  setExternal("maxres", "");

#ifdef DEBUGGER_SUPPORT
  // Debugger/disassembly options
  setInternal("dbg.fontsize", "medium");
  setInternal("dbg.fontstyle", "0");
  setInternal("dbg.uhex", "false");
  setInternal("dbg.ghostreadstrap", "true");
  setInternal("dis.resolve", "true");
  setInternal("dis.gfxformat", "2");
  setInternal("dis.showaddr", "true");
  setInternal("dis.relocate", "false");
  setInternal("plr.rwportbreak", "false");
  setInternal("dev.rwportbreak", "true");
#endif

  // player settings
  setInternal("plr.stats", "false");
  setInternal("plr.bankrandom", "false");
  setInternal("plr.ramrandom", "true");
  setInternal("plr.cpurandom", "AXYP");
  setInternal("plr.colorloss", "false");
  setInternal("plr.tv.jitter", "true");
  setInternal("plr.tv.jitter_recovery", "10");
  setInternal("plr.debugcolors", "false");
  setInternal("plr.tiadriven", "false");
  setInternal("plr.console", "2600"); // 7800
  setInternal("plr.timemachine", false);
  setInternal("plr.tm.size", 100);
  setInternal("plr.tm.uncompressed", 30);
  setInternal("plr.tm.interval", "30f"); // = 0.5 seconds
  setInternal("plr.tm.horizon", "10m"); // = ~10 minutes
  // Thumb ARM emulation options
  setInternal("plr.thumb.trapfatal", "false");
  setInternal("plr.eepromaccess", "false");

  // developer settings
  setInternal("dev.settings", "false");
  setInternal("dev.stats", "true");
  setInternal("dev.bankrandom", "true");
  setInternal("dev.ramrandom", "true");
  setInternal("dev.cpurandom", "SAXYP");
  setInternal("dev.colorloss", "true");
  setInternal("dev.tv.jitter", "true");
  setInternal("dev.tv.jitter_recovery", "2");
  setInternal("dev.debugcolors", "false");
  setInternal("dev.tiadriven", "true");
  setInternal("dev.console", "2600"); // 7800
  setInternal("dev.timemachine", true);
  setInternal("dev.tm.size", 100);
  setInternal("dev.tm.uncompressed", 60);
  setInternal("dev.tm.interval", "1f"); // = 1 frame
  setInternal("dev.tm.horizon", "10s"); // = ~10 seconds
  // Thumb ARM emulation options
  setInternal("dev.thumb.trapfatal", "true");
  setInternal("dev.eepromaccess", "true");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Settings::loadConfig()
{
  string line, key, value;
  string::size_type equalPos, garbage;

  ifstream in(myOSystem.configFile());
  if(!in || !in.is_open())
  {
    myOSystem.logMessage("ERROR: Couldn't load settings file", 0);
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
    key   = trim(line.substr(0, equalPos));
    value = trim(line.substr(equalPos + 1, line.length() - key.length() - 1));

    // Skip absent key
    if(key.length() == 0)
      continue;

    // Only settings which have been previously set are valid
    if(int idx = getInternalPos(key) != -1)
      setInternal(key, value, idx, true);
  }
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

      // Take care of arguments without an option or ones that shouldn't
      // be saved to the config file
      if(key == "rominfo" || key == "debug" || key == "holdreset" ||
         key == "holdselect" || key == "takesnapshot")
      {
        setExternal(key, "true");
        continue;
      }

      ostringstream buf;
      if(++i >= argc)
      {
        buf << "Missing argument for '" << key << "'" << endl;
        myOSystem.logMessage(buf.str(), 0);
        return "";
      }
      string value = argv[i];

      buf.str("");
      buf << "  key = '" << key << "', value = '" << value << "'";

      // Settings read from the commandline must not be saved to
      // the rc-file, unless they were previously set
      if(int idx = getInternalPos(key) != -1)
      {
        setInternal(key, value, idx);   // don't set initialValue here
        buf << "(I)\n";
      }
      else
      {
        setExternal(key, value);
        buf << "(E)\n";
      }

      myOSystem.logMessage(buf.str(), 2);
    }
    else
      return key;
  }

  return EmptyString;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Settings::validate()
{
  string s;
  int i;
  float f;

  f = getFloat("speed");
  if (f <= 0) setInternal("speed", "1.0");

  i = getInt("tia.aspectn");
  if(i < 80 || i > 120)  setInternal("tia.aspectn", "90");
  i = getInt("tia.aspectp");
  if(i < 80 || i > 120)  setInternal("tia.aspectp", "100");

  s = getString("tia.dbgcolors");
  sort(s.begin(), s.end());
  if(s != "bgopry")  setInternal("tia.dbgcolors", "roygpb");

  s = getString("tv.phosphor");
  if(s != "always" && s != "byrom")  setInternal("tv.phosphor", "byrom");

  i = getInt("tv.phosblend");
  if(i < 0 || i > 100)  setInternal("tv.phosblend", "50");

  i = getInt("tv.filter");
  if(i < 0 || i > 5)  setInternal("tv.filter", "0");

  i = getInt("dev.tv.jitter_recovery");
  if(i < 1 || i > 20) setInternal("dev.tv.jitter_recovery", "2");

  int size = getInt("dev.tm.size");
  if(size < 20 || size > 1000)
  {
    setInternal("dev.tm.size", 20);
    size = 20;
  }

  i = getInt("dev.tm.uncompressed");
  if(i < 0 || i > size) setInternal("dev.tm.uncompressed", size);

  /*i = getInt("dev.tm.interval");
  if(i < 0 || i > 5) setInternal("dev.tm.interval", 0);

  i = getInt("dev.tm.horizon");
  if(i < 0 || i > 6) setInternal("dev.tm.horizon", 1);*/

  i = getInt("plr.tv.jitter_recovery");
  if(i < 1 || i > 20) setInternal("plr.tv.jitter_recovery", "10");

  size = getInt("plr.tm.size");
  if(size < 20 || size > 1000)
  {
    setInternal("plr.tm.size", 20);
    size = 20;
  }

  i = getInt("plr.tm.uncompressed");
  if(i < 0 || i > size) setInternal("plr.tm.uncompressed", size);

  /*i = getInt("plr.tm.interval");
  if(i < 0 || i > 5) setInternal("plr.tm.interval", 3);

  i = getInt("plr.tm.horizon");
  if(i < 0 || i > 6) setInternal("plr.tm.horizon", 5);*/

#ifdef SOUND_SUPPORT
  AudioSettings::normalize(*this);
#endif

  i = getInt("joydeadzone");
  if(i < 0)        setInternal("joydeadzone", "0");
  else if(i > 29)  setInternal("joydeadzone", "29");

  i = getInt("cursor");
  if(i < 0 || i > 3)
    setInternal("cursor", "2");

  i = getInt("dsense");
  if(i < 1 || i > 20)
    setInternal("dsense", "10");

  i = getInt("msense");
  if(i < 1 || i > 20)
    setInternal("msense", "10");

  i = getInt("tsense");
  if(i < 1 || i > 20)
    setInternal("tsense", "10");

  i = getInt("ssinterval");
  if(i < 1)        setInternal("ssinterval", "2");
  else if(i > 10)  setInternal("ssinterval", "10");

  s = getString("palette");
  if(s != "standard" && s != "z26" && s != "user")
    setInternal("palette", "standard");

  s = getString("launcherfont");
  if(s != "small" && s != "medium" && s != "large")
    setInternal("launcherfont", "medium");

  s = getString("dbg.fontsize");
  if(s != "small" && s != "medium" && s != "large")
    setInternal("dbg.fontsize", "medium");

  i = getInt("romviewer");
  if(i < 0)       setInternal("romviewer", "0");
  else if(i > 2)  setInternal("romviewer", "2");

  i = getInt("loglevel");
  if(i < 0 || i > 2)
    setInternal("loglevel", "1");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Settings::usage() const
{
  cout << endl
    << "Stella version " << STELLA_VERSION << endl
    << endl
    << "Usage: stella [options ...] romfile" << endl
    << "       Run without any options or romfile to use the ROM launcher" << endl
    << "       Consult the manual for more in-depth information" << endl
    << endl
    << "Valid options are:" << endl
    << endl
    << "  -video        <type>         Type is one of the following:\n"
  #ifdef BSPF_WINDOWS
    << "                 direct3d        Direct3D acceleration\n"
  #endif
    << "                 opengl          OpenGL acceleration\n"
    << "                 opengles2       OpenGLES 2 acceleration\n"
    << "                 opengles        OpenGLES 1 acceleration\n"
    << "                 software        Software mode (no acceleration)\n"
    << endl
    << "  -vsync        <1|0>          Enable 'synchronize to vertical blank interrupt'\n"
    << "  -fullscreen   <1|0>          Enable fullscreen mode\n"
    << "  -center       <1|0>          Centers game window (if possible)\n"
    << "  -palette      <standard|     Use the specified color palette\n"
    << "                 z26|\n"
    << "                 user>\n"
    << "  -speed        <number>       Run emulation at the given speed\n"
    << "  -uimessages   <1|0>          Show onscreen UI messages for different events\n"
    << endl
  #ifdef SOUND_SUPPORT
    << "  -audio.enabled            <1|0>        Enable audio\n"
    << "  -audio.volume             <number>     Vokume (0-100)\n"
    << "  -audio.stereo             <1|0>        Enable stereo mode for all ROMs\n"
    << "  -audio.preset             <1-5>        Audio preset (or 1 for custom)\n"
    << "  -audio.sample_rate        <number>     Output sample rate (44100|48000|96000)\n"
    << "  -audio.fragment_size      <number>     Fragment size (128|256|512|1024|\n"
    << "                                          2048|4096)\n"
    << "  -audio.buffer_size        <number>     Max. number of additional half-\n"
    << "                                         frames to buffer(0-20)\n"
    << "  -audio.headroom           <number>     Additional half-frames to prebuffer\n"
    << "                                          (0-20)\n"
    << "  -audio.resampling_quality <1-3>        Resampling quality\n"
    << endl
  #endif
    << "  -tia.zoom      <zoom>         Use the specified zoom level (windowed mode)\n"
    << "                                 for TIA image\n"
    << "  -tia.inter     <1|0>          Enable interpolated (smooth) scaling for TIA\n"
    << "                                 image\n"
    << "  -tia.aspectn   <number>       Scale TIA width by the given percentage in NTS\n"
    << "                                 mode\n"
    << "  -tia.aspectp   <number>       Scale TIA width by the given percentage in PAL\n"
    << "                                 mode\n"
    << "  -tia.fsfill    <1|0>          Stretch TIA image to fill fullscreen mode\n"
    << "  -tia.dbgcolors <string>       Debug colors to use for each object (see manual\n"
    << "                                 for description)\n"
    << endl
    << "  -tv.filter    <0-5>           Set TV effects off (0) or to specified mode\n"
    << "                                 (1-5)\n"
    << "  -tv.phosphor  <always|byrom>  When to use phosphor mode\n"
    << "  -tv.phosblend <0-100>         Set default blend level in phosphor mode\n"
    << "  -tv.scanlines <0-100>         Set scanline intensity to percentage\n"
    << "                                 (0 disables completely)\n"
    << "  -tv.scaninter <1|0>           Enable interpolated (smooth) scanlines\n"
    << "  -tv.contrast    <-1.0 - 1.0>  Set TV effects custom contrast\n"
    << "  -tv.brightness  <-1.0 - 1.0>  Set TV effects custom brightness\n"
    << "  -tv.hue         <-1.0 - 1.0>  Set TV effects custom hue\n"
    << "  -tv.saturation  <-1.0 - 1.0>  Set TV effects custom saturation\n"
    << "  -tv.gamma       <-1.0 - 1.0>  Set TV effects custom gamma\n"
    << "  -tv.sharpness   <-1.0 - 1.0>  Set TV effects custom sharpness\n"
    << "  -tv.resolution  <-1.0 - 1.0>  Set TV effects custom resolution\n"
    << "  -tv.artifacts   <-1.0 - 1.0>  Set TV effects custom artifacts\n"
    << "  -tv.fringing    <-1.0 - 1.0>  Set TV effects custom fringing\n"
    << "  -tv.bleed       <-1.0 - 1.0>  Set TV effects custom bleed\n"
    << endl
    << "  -cheat        <code>         Use the specified cheatcode (see manual for\n"
    << "                                description)\n"
    << "  -loglevel     <0|1|2>        Set level of logging during application run\n"
    << endl
    << "  -logtoconsole <1|0>          Log output to console/commandline\n"
    << "  -joydeadzone  <number>       Sets 'deadzone' area for analog joysticks (0-29)\n"
    << "  -joyallow4    <1|0>          Allow all 4 directions on a joystick to be\n"
    << "                                pressed simultaneously\n"
    << "  -usemouse     <always|\n"
    << "                 analog|\n"
    << "                 never>        Use mouse as a controller as specified by ROM\n"
    << "                                properties in given mode(see manual)\n"
    << "  -grabmouse    <1|0>          Locks the mouse cursor in the TIA window\n"
    << "  -cursor       <0,1,2,3>      Set cursor state in UI/emulation modes\n"
    << "  -dsense       <number>       Sensitivity of digital emulated paddle movement\n"
    << "                                (1-20)\n"
    << "  -msense       <number>       Sensitivity of mouse emulated paddle movement\n"
    << "                                (1-20)\n"
    << "  -tsense       <number>       Sensitivity of mouse emulated trackball movement\n"
    << "                                (1-20)\n"
    << "  -saport       <lr|rl>        How to assign virtual ports to multiple\n"
    << "                                Stelladaptor/2600-daptors\n"
    << "  -ctrlcombo    <1|0>          Use key combos involving the Control key\n"
    << "                                (Control-Q for quit may be disabled!)\n"
    << "  -autoslot     <1|0>          Automatically switch to next save slot when\n"
    << "                                state saving\n"
    << "  -fastscbios   <1|0>          Disable Supercharger BIOS progress loading bars\n"
    << "  -threads      <1|0>          Whether to using multi-threading during\n"
    << "                                emulation\n"
    << "  -snapsavedir  <path>         The directory to save snapshot files to\n"
    << "  -snaploaddir  <path>         The directory to load snapshot files from\n"
    << "  -snapname     <int|rom>      Name snapshots according to internal database or\n"
    << "                                ROM\n"
    << "  -sssingle     <1|0>          Generate single snapshot instead of many\n"
    << "  -ss1x         <1|0>          Generate TIA snapshot in 1x mode (ignore\n"
    << "                                scaling/effects)\n"
    << "  -ssinterval   <number        Number of seconds between snapshots in\n"
    << "                                continuous snapshot mode\n"
    << endl
    << "  -rominfo      <rom>          Display detailed information for the given ROM\n"
    << "  -listrominfo                 Display contents of stella.pro, one line per ROM\n"
    << "                                entry\n"
    << "                               \n"
    << "  -exitlauncher <1|0>          On exiting a ROM, go back to the ROM launcher\n"
    << "  -launcherres  <WxH>          The resolution to use in ROM launcher mode\n"
    << "  -launcherfont <small|medium| Use the specified font in the ROM launcher\n"
    << "                 large>\n"
    << "  -launcherroms <1|0>          Show only ROMs in the launcher (vs. all files)\n"
    << "  -romviewer    <0|1|2>        Show ROM info viewer at given zoom level in ROM\n"
    << "                                launcher (0 for off)\n"
    << "  -listdelay    <delay>        Time to wait between keypresses in list widgets\n"
    << "                                (300-1000)\n"
    << "  -mwheel       <lines>        Number of lines the mouse wheel will scroll in\n"
    << "                                UI\n"
    << "  -romdir       <dir>          Directory in which to load ROM files\n"
    << "  -statedir     <dir>          Directory in which to save/load state files\n"
    << "  -cheatfile    <file>         Full pathname of cheatfile database\n"
    << "  -palettefile  <file>         Full pathname of user-defined palette file\n"
    << "  -propsfile    <file>         Full pathname of ROM properties file\n"
    << "  -nvramdir     <dir>          Directory in which to save/load flash/EEPROM\n"
    << "                                files\n"
    << "  -cfgdir       <dir>          Directory in which to save Distella config files\n"
    << "  -avoxport     <name>         The name of the serial port where an AtariVox is\n"
    << "                                connected\n"
    << "  -holdreset                   Start the emulator with the Game Reset switch\n"
    << "                                held down\n"
    << "  -holdselect                  Start the emulator with the Game Select switch\n"
    << "                                held down\n"
    << "  -holdjoy0     <U,D,L,R,F>    Start the emulator with the left joystick\n"
    << "                                direction/fire button held down\n"
    << "  -holdjoy1     <U,D,L,R,F>    Start the emulator with the right joystick\n"
    << "                                direction/fire button held down\n"
    << "  -maxres       <WxH>          Used by developers to force the maximum size of\n"
    << "                                the application window\n"
    << "  -help                        Show the text you're now reading\n"
  #ifdef DEBUGGER_SUPPORT
    << endl
    << " The following options are meant for developers\n"
    << " Arguments are more fully explained in the manual\n"
    << endl
    << "   -dis.resolve   <1|0>        Attempt to resolve code sections in disassembler\n"
    << "   -dis.gfxformat <2|16>       Set base to use for displaying GFX sections in\n"
    << "                                disassembler\n"
    << "   -dis.showaddr  <1|0>        Show opcode addresses in disassembler\n"
    << "   -dis.relocate  <1|0>        Relocate calls out of address range in\n"
    << "                                disassembler\n"
    << endl
    << "   -dbg.res       <WxH>          The resolution to use in debugger mode\n"
    << "   -dbg.fontsize  <small|medium| Font size to use in debugger window\n"
    << "                  large>\n"
    << "   -dbg.fontstyle <0-3>          Font style to use in debugger window (bold vs.\n"
    << "                                  normal)\n"
    << "   -dbg.ghostreadstrap <1|0>     Debugger traps on 'ghost' reads\n"
    << "   -dbg.uhex      <0|1>          lower-/uppercase HEX display\n"
    << "   -break         <address>      Set a breakpoint at 'address'\n"
    << "   -debug                        Start in debugger mode\n"
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
    << endl
  #endif

    << " Various development related parameters for player settings mode\n"
    << endl
    << "  -dev.settings     <1|0>          Select developer (1) or player (0) settings\n"
    << "                                    mode\n"
    << endl
    << "  -plr.stats        <1|0>          Overlay console info during emulation\n"
    << "  -plr.console      <2600|7800>    Select console for B/W and Pause key\n"
    << "                                    handling and RAM initialization\n"
    << "  -plr.bankrandom   <1|0>          Randomize the startup bank on reset\n"
    << "  -plr.ramrandom    <1|0>          Randomize the contents of RAM on reset\n"
    << "  -plr.cpurandom    <1|0>          Randomize the contents of CPU registers on\n"
    << "                                    reset\n"
    << "  -plr.debugcolors  <1|0>          Enable debug colors\n"
    << "  -plr.colorloss    <1|0>          Enable PAL color-loss effect\n"
    << "  -plr.tv.jitter    <1|0>          Enable TV jitter effect\n"
    << "  -plr.tv.jitter_recovery <1-20>   Set recovery time for TV jitter effect\n"
    << "  -plr.tiadriven    <1|0>          Drive unused TIA pins randomly on a\n"
    << "                                    read/peek\n"
  #ifdef DEBUGGER_SUPPORT
    << "  -plr.rwportbreak <1|0>           Debugger breaks on reads from write ports\n"
  #endif
    << "  -plr.thumb.trapfatal <1|0>       Determines whether errors in ARM emulation\n"
    << "                                    throw an exception\n"
    << "  -plr.eepromaccess <1|0>          Enable messages for AtariVox/SaveKey access\n"
    << "                                    messages\n"
    << endl
    << " The same parameters but for developer settings mode\n"
    << "  -dev.stats        <1|0>          Overlay console info during emulation\n"
    << "  -dev.console      <2600|7800>    Select console for B/W and Pause key\n"
    << "                                    handling and RAM initialization\n"
    << "  -dev.bankrandom   <1|0>          Randomize the startup bank on reset\n"
    << "  -dev.ramrandom    <1|0>          Randomize the contents of RAM on reset\n"
    << "  -dev.cpurandom    <1|0>          Randomize the contents of CPU registers on\n"
    << "                                    reset\n"
    << "  -dev.debugcolors  <1|0>          Enable debug colors\n"
    << "  -dev.colorloss    <1|0>          Enable PAL color-loss effect\n"
    << "  -dev.tv.jitter    <1|0>          Enable TV jitter effect\n"
    << "  -dev.tv.jitter_recovery <1-20>   Set recovery time for TV jitter effect\n"
    << "  -dev.tiadriven    <1|0>          Drive unused TIA pins randomly on a\n"
    << "                                    read/peek\n"
#ifdef DEBUGGER_SUPPORT
    << "  -dev.rwportbreak <1|0>           Debugger breaks on reads from write ports\n"
#endif
    << "  -dev.thumb.trapfatal <1|0>       Determines whether errors in ARM emulation\n"
    << "                                    throw an exception\n"
    << "  -dev.eepromaccess <1|0>          Enable messages for AtariVox/SaveKey access\n"
    << "                                    messages\n"
    << endl << std::flush;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const Variant& Settings::value(const string& key) const
{
  // Try to find the named setting and answer its value
  int idx = -1;
  if((idx = getInternalPos(key)) != -1)
    return myInternalSettings[idx].value;
  else if((idx = getExternalPos(key)) != -1)
    return myExternalSettings[idx].value;
  else
    return EmptyVariant;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Settings::setValue(const string& key, const Variant& value)
{
  if(int idx = getInternalPos(key) != -1)
    setInternal(key, value, idx);
  else
    setExternal(key, value);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Settings::saveConfig()
{
  // Do a quick scan of the internal settings to see if any have
  // changed.  If not, we don't need to save them at all.
  bool settingsChanged = false;
  for(const auto& s: myInternalSettings)
  {
    if(s.value != s.initialValue)
    {
      settingsChanged = true;
      break;
    }
  }

  if(!settingsChanged)
    return;

  ofstream out(myOSystem.configFile());
  if(!out || !out.is_open())
  {
    myOSystem.logMessage("ERROR: Couldn't save settings file", 0);
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
      << ";  Commands are the same as those specified on the commandline," << endl
      << ";  without the '-' character." << endl
      << ";" << endl
      << ";  Values are the same as those allowed on the commandline." << endl
      << ";  Boolean values are specified as 1 (or true) and 0 (or false)" << endl
      << ";" << endl;

  // Write out each of the key and value pairs
  for(const auto& s: myInternalSettings)
    out << s.key << " = " << s.value << endl;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int Settings::getInternalPos(const string& key) const
{
  for(uInt32 i = 0; i < myInternalSettings.size(); ++i)
    if(myInternalSettings[i].key == key)
      return i;

  return -1;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int Settings::getExternalPos(const string& key) const
{
  for(uInt32 i = 0; i < myExternalSettings.size(); ++i)
    if(myExternalSettings[i].key == key)
      return i;

  return -1;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int Settings::setInternal(const string& key, const Variant& value,
                          int pos, bool useAsInitial)
{
  int idx = -1;

  if(pos >= 0 && pos < int(myInternalSettings.size()) &&
     myInternalSettings[pos].key == key)
  {
    idx = pos;
  }
  else
  {
    for(uInt32 i = 0; i < myInternalSettings.size(); ++i)
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
    Setting setting(key, value);
    if(useAsInitial) setting.initialValue = value;

    myInternalSettings.push_back(setting);
    idx = int(myInternalSettings.size()) - 1;

    /*cerr << "insert internal: key = " << key
         << ", value  = " << value
         << ", ivalue = " << setting.initialValue
         << " @ index = " << idx
         << endl;*/
  }

  return idx;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int Settings::setExternal(const string& key, const Variant& value,
                          int pos, bool useAsInitial)
{
  int idx = -1;

  if(pos >= 0 && pos < int(myExternalSettings.size()) &&
     myExternalSettings[pos].key == key)
  {
    idx = pos;
  }
  else
  {
    for(uInt32 i = 0; i < myExternalSettings.size(); ++i)
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
    Setting setting(key, value);
    if(useAsInitial) setting.initialValue = value;

    myExternalSettings.push_back(setting);
    idx = int(myExternalSettings.size()) - 1;

    /*cerr << "insert external: key = " << key
         << ", value = " << value
         << " @ index = " << idx
         << endl;*/
  }

  return idx;
}
