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

#include <chrono>

#include "bspf.hxx"

#include "OSystem.hxx"
#include "Version.hxx"
#include "AudioSettings.hxx"

#ifdef DEBUGGER_SUPPORT
  #include "DebuggerDialog.hxx"
#endif

#include "Settings.hxx"
#include "repository/KeyValueRepositoryNoop.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Settings::Settings()
{
  myRespository = make_shared<KeyValueRepositoryNoop>();

  // Video-related options
  setPermanent("video", "");
  setPermanent("speed", "1.0");
  setPermanent("vsync", "true");
  setPermanent("fullscreen", "false");
  setPermanent("center", "false");
  setPermanent("palette", "standard");
  setPermanent("uimessages", "true");

  // TIA specific options
  setPermanent("tia.zoom", "3");
  setPermanent("tia.inter", "false");
  setPermanent("tia.aspectn", "100");
  setPermanent("tia.aspectp", "100");
  setPermanent("tia.fs_stretch", "false");
  setPermanent("tia.dbgcolors", "roygpb");

  // TV filtering options
  setPermanent("tv.filter", "0");
  setPermanent("tv.phosphor", "byrom");
  setPermanent("tv.phosblend", "50");
  setPermanent("tv.scanlines", "25");
  // TV options when using 'custom' mode
  setPermanent("tv.contrast", "0.0");
  setPermanent("tv.brightness", "0.0");
  setPermanent("tv.hue", "0.0");
  setPermanent("tv.saturation", "0.0");
  setPermanent("tv.gamma", "0.0");
  setPermanent("tv.sharpness", "0.0");
  setPermanent("tv.resolution", "0.0");
  setPermanent("tv.artifacts", "0.0");
  setPermanent("tv.fringing", "0.0");
  setPermanent("tv.bleed", "0.0");

  // Sound options
  setPermanent(AudioSettings::SETTING_ENABLED, AudioSettings::DEFAULT_ENABLED);
  setPermanent(AudioSettings::SETTING_VOLUME, AudioSettings::DEFAULT_VOLUME);
  setPermanent(AudioSettings::SETTING_PRESET, static_cast<int>(AudioSettings::DEFAULT_PRESET));
  setPermanent(AudioSettings::SETTING_FRAGMENT_SIZE, AudioSettings::DEFAULT_FRAGMENT_SIZE);
  setPermanent(AudioSettings::SETTING_SAMPLE_RATE, AudioSettings::DEFAULT_SAMPLE_RATE);
  setPermanent(AudioSettings::SETTING_RESAMPLING_QUALITY, static_cast<int>(AudioSettings::DEFAULT_RESAMPLING_QUALITY));
  setPermanent(AudioSettings::SETTING_HEADROOM, AudioSettings::DEFAULT_HEADROOM);
  setPermanent(AudioSettings::SETTING_BUFFER_SIZE, AudioSettings::DEFAULT_BUFFER_SIZE);
  setPermanent(AudioSettings::SETTING_STEREO, AudioSettings::DEFAULT_STEREO);

  // Input event options
  setPermanent("keymap", "");
  setPermanent("joymap", "");
  setPermanent("combomap", "");
  setPermanent("joydeadzone", "13");
  setPermanent("joyallow4", "false");
  setPermanent("usemouse", "analog");
  setPermanent("grabmouse", "true");
  setPermanent("cursor", "2");
  setPermanent("dsense", "10");
  setPermanent("msense", "10");
  setPermanent("tsense", "10");
  setPermanent("saport", "lr");
  setPermanent("ctrlcombo", "true");

  // Snapshot options
  setPermanent("snapsavedir", "");
  setPermanent("snaploaddir", "");
  setPermanent("snapname", "int");
  setPermanent("sssingle", "false");
  setPermanent("ss1x", "false");
  setPermanent("ssinterval", "2");

  // Config files and paths
  setPermanent("romdir", "");

  // ROM browser options
  setPermanent("exitlauncher", "false");
  setPermanent("launcherres", Common::Size(900, 600));
  setPermanent("launcherfont", "medium");
  setPermanent("launcherroms", "true");
  setPermanent("romviewer", "1");
  setPermanent("lastrom", "");

  // UI-related options
#ifdef DEBUGGER_SUPPORT
  setPermanent("dbg.res",
    Common::Size(DebuggerDialog::kMediumFontMinW,
                 DebuggerDialog::kMediumFontMinH));
#endif
  setPermanent("uipalette", "standard");
  setPermanent("listdelay", "300");
  setPermanent("mwheel", "4");
  setPermanent("basic_settings", false);
  setPermanent("dialogpos", 0);

  // Misc options
  setPermanent("autoslot", "false");
  setPermanent("loglevel", "1");
  setPermanent("logtoconsole", "0");
  setPermanent("avoxport", "");
  setPermanent("fastscbios", "true");
  setPermanent("threads", "false");
  setTemporary("romloadcount", "0");
  setTemporary("maxres", "");

#ifdef DEBUGGER_SUPPORT
  // Debugger/disassembly options
  setPermanent("dbg.fontsize", "medium");
  setPermanent("dbg.fontstyle", "0");
  setPermanent("dbg.uhex", "false");
  setPermanent("dbg.ghostreadstrap", "true");
  setPermanent("dis.resolve", "true");
  setPermanent("dis.gfxformat", "2");
  setPermanent("dis.showaddr", "true");
  setPermanent("dis.relocate", "false");
  setPermanent("dev.rwportbreak", "true");
#endif

  // Player settings
  setPermanent("plr.stats", "false");
  setPermanent("plr.bankrandom", "false");
  setPermanent("plr.ramrandom", "true");
  setPermanent("plr.cpurandom", "AXYP");
  setPermanent("plr.colorloss", "false");
  setPermanent("plr.tv.jitter", "true");
  setPermanent("plr.tv.jitter_recovery", "10");
  setPermanent("plr.debugcolors", "false");
  setPermanent("plr.console", "2600"); // 7800
  setPermanent("plr.timemachine", true);
  setPermanent("plr.tm.size", 200);
  setPermanent("plr.tm.uncompressed", 60);
  setPermanent("plr.tm.interval", "30f"); // = 0.5 seconds
  setPermanent("plr.tm.horizon", "10m"); // = ~10 minutes
  setPermanent("plr.eepromaccess", "false");

  // Developer settings
  setPermanent("dev.settings", "false");
  setPermanent("dev.stats", "true");
  setPermanent("dev.bankrandom", "true");
  setPermanent("dev.ramrandom", "true");
  setPermanent("dev.cpurandom", "SAXYP");
  setPermanent("dev.colorloss", "true");
  setPermanent("dev.tv.jitter", "true");
  setPermanent("dev.tv.jitter_recovery", "2");
  setPermanent("dev.debugcolors", "false");
  setPermanent("dev.tiadriven", "true");
  setPermanent("dev.console", "2600"); // 7800
  setPermanent("dev.tia.type", "standard");
  setPermanent("dev.tia.plinvphase", "true");
  setPermanent("dev.tia.msinvphase", "true");
  setPermanent("dev.tia.blinvphase", "true");
  setPermanent("dev.tia.delaypfbits", "true");
  setPermanent("dev.tia.delaypfcolor", "true");
  setPermanent("dev.tia.delayplswap", "true");
  setPermanent("dev.tia.delayblswap", "true");
  setPermanent("dev.timemachine", true);
  setPermanent("dev.tm.size", 1000);
  setPermanent("dev.tm.uncompressed", 600);
  setPermanent("dev.tm.interval", "1f"); // = 1 frame
  setPermanent("dev.tm.horizon", "30s"); // = ~30 seconds
  // Thumb ARM emulation options
  setPermanent("dev.thumb.trapfatal", "true");
  setPermanent("dev.eepromaccess", "true");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Settings::setRepository(shared_ptr<KeyValueRepository> repository)
{
  myRespository = repository;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Settings::load(const Options& options)
{
  Options fromFile =  myRespository->load();
  for (const auto& opt: fromFile)
    setValue(opt.first, opt.second, false);

  // Apply commandline options, which override those from settings file
  for(const auto& opt: options)
    setValue(opt.first, opt.second, false);

  // Finally, validate some settings, so the rest of the codebase
  // can assume the values are valid
  validate();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Settings::save()
{
  myRespository->save(myPermanentSettings);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Settings::validate()
{
  string s;
  int i;
  float f;

  f = getFloat("speed");
  if (f <= 0) setValue("speed", "1.0");

  i = getInt("tia.aspectn");
  if(i < 80 || i > 120)  setValue("tia.aspectn", "90");
  i = getInt("tia.aspectp");
  if(i < 80 || i > 120)  setValue("tia.aspectp", "100");

  s = getString("tia.dbgcolors");
  sort(s.begin(), s.end());
  if(s != "bgopry")  setValue("tia.dbgcolors", "roygpb");

  s = getString("tv.phosphor");
  if(s != "always" && s != "byrom")  setValue("tv.phosphor", "byrom");

  i = getInt("tv.phosblend");
  if(i < 0 || i > 100)  setValue("tv.phosblend", "50");

  i = getInt("tv.filter");
  if(i < 0 || i > 5)  setValue("tv.filter", "0");

  i = getInt("dev.tv.jitter_recovery");
  if(i < 1 || i > 20) setValue("dev.tv.jitter_recovery", "2");

  int size = getInt("dev.tm.size");
  if(size < 20 || size > 1000)
  {
    setValue("dev.tm.size", 20);
    size = 20;
  }

  i = getInt("dev.tm.uncompressed");
  if(i < 0 || i > size) setValue("dev.tm.uncompressed", size);

  /*i = getInt("dev.tm.interval");
  if(i < 0 || i > 5) setValue("dev.tm.interval", 0);

  i = getInt("dev.tm.horizon");
  if(i < 0 || i > 6) setValue("dev.tm.horizon", 1);*/

  i = getInt("plr.tv.jitter_recovery");
  if(i < 1 || i > 20) setValue("plr.tv.jitter_recovery", "10");

  size = getInt("plr.tm.size");
  if(size < 20 || size > 1000)
  {
    setValue("plr.tm.size", 20);
    size = 20;
  }

  i = getInt("plr.tm.uncompressed");
  if(i < 0 || i > size) setValue("plr.tm.uncompressed", size);

  /*i = getInt("plr.tm.interval");
  if(i < 0 || i > 5) setValue("plr.tm.interval", 3);

  i = getInt("plr.tm.horizon");
  if(i < 0 || i > 6) setValue("plr.tm.horizon", 5);*/

#ifdef SOUND_SUPPORT
  AudioSettings::normalize(*this);
#endif

  i = getInt("joydeadzone");
  if(i < 0)        setValue("joydeadzone", "0");
  else if(i > 29)  setValue("joydeadzone", "29");

  i = getInt("cursor");
  if(i < 0 || i > 3)
    setValue("cursor", "2");

  i = getInt("dsense");
  if(i < 1 || i > 20)
    setValue("dsense", "10");

  i = getInt("msense");
  if(i < 1 || i > 20)
    setValue("msense", "10");

  i = getInt("tsense");
  if(i < 1 || i > 20)
    setValue("tsense", "10");

  i = getInt("ssinterval");
  if(i < 1)        setValue("ssinterval", "2");
  else if(i > 10)  setValue("ssinterval", "10");

  s = getString("palette");
  if(s != "standard" && s != "z26" && s != "user")
    setValue("palette", "standard");

  s = getString("launcherfont");
  if(s != "small" && s != "medium" && s != "large")
    setValue("launcherfont", "medium");

  s = getString("dbg.fontsize");
  if(s != "small" && s != "medium" && s != "large")
    setValue("dbg.fontsize", "medium");

  i = getInt("romviewer");
  if(i < 0)       setValue("romviewer", "0");
  else if(i > 2)  setValue("romviewer", "2");

  i = getInt("loglevel");
  if(i < 0 || i > 2)
    setValue("loglevel", "1");
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
    << "  -audio.enabled            <1|0>      Enable audio\n"
    << "  -audio.volume             <0-100>    Volume\n"
    << "  -audio.preset             <1-5>      Audio preset (or 1 for custom)\n"
    << "  -audio.sample_rate        <number>   Output sample rate (44100|48000|96000)\n"
    << "  -audio.fragment_size      <number>   Fragment size (128|256|512|1024|\n"
    << "                                        2048|4096)\n"
    << "  -audio.resampling_quality <1-3>      Resampling quality\n"
    << "  -audio.headroom           <0-20>     Additional half-frames to prebuffer\n"
    << "  -audio.buffer_size        <0-20>     Max. number of additional half-\n"
    << "                                        frames to buffer\n"
    << "  -audio.stereo             <1|0>      Enable stereo mode for all ROMs\n"
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
    << "  -tia.fs_stretch <1|0>         Stretch TIA image to fill fullscreen mode\n"
    << "  -tia.dbgcolors <string>       Debug colors to use for each object (see manual\n"
    << "                                 for description)\n"
    << endl
    << "  -tv.filter    <0-5>           Set TV effects off (0) or to specified mode\n"
    << "                                 (1-5)\n"
    << "  -tv.phosphor  <always|byrom>  When to use phosphor mode\n"
    << "  -tv.phosblend <0-100>         Set default blend level in phosphor mode\n"
    << "  -tv.scanlines <0-100>         Set scanline intensity to percentage\n"
    << "                                 (0 disables completely)\n"
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
    << "  -basic_settings <0|1>        Display only a basic settings dialog\n"
    << "  -dialogpos    <0..4>         Display all dialogs at given positions\n"
    << "  -romdir       <dir>          Directory in which to load ROM files\n"
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
    << "  -dev.tia.type <standard|custom|  Selects a TIA type\n"
    << "                 koolaidman|\n"
    << "                 cosmicark|pesco|\n"
    << "                 quickstep|heman|>\n"
    << "  -dev.tia.plinvphase <1|0>        Enable inverted HMOVE clock phase for players\n"
    << "  -dev.tia.msinvphase <1|0>        Enable inverted HMOVE clock phase for\n"
    << "                                    missiles\n"
    << "  -dev.tia.blinvphase <1|0>        Enable inverted HMOVE clock phase for ball\n"
    << "  -dev.tia.delaypfbits <1|0>       Enable extra delay cycle for PF bits access\n"
    << "  -dev.tia.delaypfcolor <1|0>      Enable extra delay cycle for PF color\n"
    << "  -dev.tia.delayplswap <1|0>       Enable extra delay cycle for VDELP0/1 swap\n"
    << "  -dev.tia.delayblswap <1|0>       Enable extra delay cycle for VDELBL swap\n"
    << endl << std::flush;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const Variant& Settings::value(const string& key) const
{
  // Try to find the named setting and answer its value
  auto it = myPermanentSettings.find(key);
  if(it != myPermanentSettings.end())
    return it->second;
  else
  {
    it = myTemporarySettings.find(key);
    if(it != myTemporarySettings.end())
      return it->second;
  }
  return EmptyVariant;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Settings::setValue(const string& key, const Variant& value, bool persist)
{
  auto it = myPermanentSettings.find(key);
  if(it != myPermanentSettings.end()) {
    if (persist && it->second != value) {
      std::chrono::high_resolution_clock::time_point ts = std::chrono::high_resolution_clock::now();

      myRespository->save(key, value);

      double duration = std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::high_resolution_clock::now() - ts).count();
      cout << "persistence " << key << " -> " << value << " took " << duration << "seconds" << endl << std::flush;
    }
    it->second = value;
  }
  else
    myTemporarySettings[key] = value;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Settings::setPermanent(const string& key, const Variant& value)
{
  myPermanentSettings[key] = value;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Settings::setTemporary(const string& key, const Variant& value)
{
  myTemporarySettings[key] = value;
}
