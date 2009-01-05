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
// Copyright (c) 1995-2009 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: OSystem.cxx,v 1.143 2009-01-05 19:44:29 stephena Exp $
//============================================================================

#include <cassert>
#include <sstream>
#include <fstream>
#include <zlib.h>

#include "bspf.hxx"

#include "MediaFactory.hxx"

#ifdef DEBUGGER_SUPPORT
  #include "Debugger.hxx"
#endif

#ifdef CHEATCODE_SUPPORT
  #include "CheatManager.hxx"
#endif

#include "SerialPort.hxx"
#if defined(UNIX)
  #include "SerialPortUNIX.hxx"
#elif defined(WIN32)
  #include "SerialPortWin32.hxx"
#elif defined(MAC_OSX)
  #include "SerialPortMACOSX.hxx"
#endif

#include "FSNode.hxx"
#include "unzip.h"
#include "MD5.hxx"
#include "Settings.hxx"
#include "PropsSet.hxx"
#include "EventHandler.hxx"
#include "Menu.hxx"
#include "CommandMenu.hxx"
#include "Launcher.hxx"
#include "Font.hxx"
#include "StellaFont.hxx"
#include "StellaMediumFont.hxx"
#include "StellaLargeFont.hxx"
#include "ConsoleFont.hxx"
#include "Widget.hxx"
#include "Console.hxx"
#include "Random.hxx"
#include "StateManager.hxx"

#include "OSystem.hxx"

#define MAX_ROM_SIZE  512 * 1024

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
OSystem::OSystem()
  : myEventHandler(NULL),
    myFrameBuffer(NULL),
    mySound(NULL),
    mySettings(NULL),
    myPropSet(NULL),
    myConsole(NULL),
    mySerialPort(NULL),
    myMenu(NULL),
    myCommandMenu(NULL),
    myLauncher(NULL),
    myDebugger(NULL),
    myCheatManager(NULL),
    myStateManager(NULL),
    myQuitLoop(false),
    myRomFile(""),
    myRomMD5(""),
    myFeatures(""),
    myFont(NULL),
    myConsoleFont(NULL)
{
#ifdef DISPLAY_OPENGL
  myFeatures += "OpenGL ";
#endif
#ifdef SOUND_SUPPORT
  myFeatures += "Sound ";
#endif
#ifdef JOYSTICK_SUPPORT
  myFeatures += "Joystick ";
#endif
#ifdef DEBUGGER_SUPPORT
  myFeatures += "Debugger ";
#endif
#ifdef CHEATCODE_SUPPORT
  myFeatures += "Cheats";
#endif

#if 0
  // Debugging info for the GUI widgets
  cerr << "  kStaticTextWidget   = " << kStaticTextWidget   << endl;
  cerr << "  kEditTextWidget     = " << kEditTextWidget     << endl;
  cerr << "  kButtonWidget       = " << kButtonWidget       << endl;
  cerr << "  kCheckboxWidget     = " << kCheckboxWidget     << endl;
  cerr << "  kSliderWidget       = " << kSliderWidget       << endl;
  cerr << "  kListWidget         = " << kListWidget         << endl;
  cerr << "  kScrollBarWidget    = " << kScrollBarWidget    << endl;
  cerr << "  kPopUpWidget        = " << kPopUpWidget        << endl;
  cerr << "  kTabWidget          = " << kTabWidget	        << endl;
  cerr << "  kEventMappingWidget = " << kEventMappingWidget << endl;
  cerr << "  kEditableWidget     = " << kEditableWidget     << endl;
  cerr << "  kAudioWidget        = " << kAudioWidget        << endl;
  cerr << "  kColorWidget        = " << kColorWidget        << endl;
  cerr << "  kCpuWidget          = " << kCpuWidget          << endl;
  cerr << "  kDataGridOpsWidget  = " << kDataGridOpsWidget  << endl;
  cerr << "  kDataGridWidget     = " << kDataGridWidget     << endl;
  cerr << "  kPromptWidget       = " << kPromptWidget       << endl;
  cerr << "  kRamWidget          = " << kRamWidget          << endl;
  cerr << "  kRomListWidget      = " << kRomListWidget      << endl;
  cerr << "  kRomWidget          = " << kRomWidget          << endl;
  cerr << "  kTiaInfoWidget      = " << kTiaInfoWidget      << endl;
  cerr << "  kTiaOutputWidget    = " << kTiaOutputWidget    << endl;
  cerr << "  kTiaWidget          = " << kTiaWidget          << endl;
  cerr << "  kTiaZoomWidget      = " << kTiaZoomWidget      << endl;
  cerr << "  kToggleBitWidget    = " << kToggleBitWidget    << endl;
  cerr << "  kTogglePixelWidget  = " << kTogglePixelWidget  << endl;
  cerr << "  kToggleWidget       = " << kToggleWidget       << endl;
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
OSystem::~OSystem()
{
  delete myMenu;
  delete myCommandMenu;
  delete myLauncher;
  delete myFont;
  delete myConsoleFont;

  // Remove any game console that is currently attached
  deleteConsole();

  // OSystem takes responsibility for framebuffer and sound,
  // since it created them
  delete myFrameBuffer;
  delete mySound;

  // These must be deleted after all the others
  // This is a bit hacky, since it depends on ordering
  // of d'tor calls
#ifdef DEBUGGER_SUPPORT
  delete myDebugger;
#endif
#ifdef CHEATCODE_SUPPORT
  delete myCheatManager;
#endif

  delete myStateManager;
  delete myPropSet;
  delete myEventHandler;

  delete mySerialPort;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool OSystem::create()
{
  // Get updated paths for all configuration files
  setConfigPaths();

  // Get relevant information about the video hardware
  // This must be done before any graphics context is created, since
  // it may be needed to initialize the size of graphical objects
  queryVideoHardware();

  ////////////////////////////////////////////////////////////////////
  // Create fonts to draw text
  // NOTE: the logic determining appropriate font sizes is done here,
  //       so that the UI classes can just use the font they expect,
  //       and not worry about it
  //       This logic should also take into account the size of the
  //       framebuffer, and try to be intelligent about font sizes
  //       We can probably add ifdefs to take care of corner cases,
  //       but the means we've failed to abstract it enough ...
  ////////////////////////////////////////////////////////////////////
  bool smallScreen = myDesktopWidth <= 320 || myDesktopHeight <= 240;

  // This font is used in a variety of situations when a really small
  // font is needed; we let the specific widget/dialog decide when to
  // use it
  mySmallFont = new GUI::Font(GUI::stellaDesc);

  // The console font is always the same size (for now at least)
  myConsoleFont  = new GUI::Font(GUI::consoleDesc);

  // The general font used in all UI elements
  // This is determined by the size of the framebuffer
  myFont = new GUI::Font(smallScreen ? GUI::stellaDesc : GUI::stellaMediumDesc);

  // The font used by the ROM launcher
  // Normally, this is configurable by the user, except in the case of
  // very small screens
  if(!smallScreen)
  {    
    if(mySettings->getString("launcherfont") == "small")
      myLauncherFont = new GUI::Font(GUI::consoleDesc);
    else if(mySettings->getString("launcherfont") == "medium")
      myLauncherFont = new GUI::Font(GUI::stellaMediumDesc);
    else
      myLauncherFont = new GUI::Font(GUI::stellaLargeDesc);
  }
  else
    myLauncherFont = new GUI::Font(GUI::stellaDesc);

  // Create the event handler for the system
  myEventHandler = new EventHandler(this);
  myEventHandler->initialize();

  // Create a properties set for us to use and set it up
  myPropSet = new PropertiesSet(this);

#ifdef CHEATCODE_SUPPORT
  myCheatManager = new CheatManager(this);
  myCheatManager->loadCheatDatabase();
#endif

  // Create menu and launcher GUI objects
  myMenu = new Menu(this);
  myCommandMenu = new CommandMenu(this);
  myLauncher = new Launcher(this);
#ifdef DEBUGGER_SUPPORT
  myDebugger = new Debugger(this);
#endif
  myStateManager = new StateManager(this);

  // Create the sound object; the sound subsystem isn't actually
  // opened until needed, so this is non-blocking (on those systems
  // that only have a single sound device (no hardware mixing)
  createSound();

  // Create the serial port object
  // This is used by any controller that wants to directly access
  // a real serial port on the system
#if defined(UNIX)
  mySerialPort = new SerialPortUNIX();
#elif defined(WIN32)
  mySerialPort = new SerialPortWin32();
#elif defined(MAC_OSX)
  mySerialPort = new SerialPortMACOSX();
#else
  // Create an 'empty' serial port
  mySerialPort = new SerialPort();
#endif

  // Let the random class know about us; it needs access to getTicks()
  Random::setSystem(this);

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OSystem::setConfigPaths()
{
  myStateDir = mySettings->getString("statedir");
  if(myStateDir == "")
    myStateDir = myBaseDir + BSPF_PATH_SEPARATOR + "state";
  if(!FilesystemNode::dirExists(myStateDir))
    FilesystemNode::makeDir(myStateDir);
  mySettings->setString("statedir", myStateDir);

  mySnapshotDir = mySettings->getString("ssdir");
  if(mySnapshotDir == "")
    mySnapshotDir = myBaseDir + BSPF_PATH_SEPARATOR + "snapshots";
  if(!FilesystemNode::dirExists(mySnapshotDir))
    FilesystemNode::makeDir(mySnapshotDir);
  mySettings->setString("ssdir", mySnapshotDir);

  myCheatFile = mySettings->getString("cheatfile");
  if(myCheatFile == "")
    myCheatFile = myBaseDir + BSPF_PATH_SEPARATOR + "stella.cht";
  mySettings->setString("cheatfile", myCheatFile);

  myPaletteFile = mySettings->getString("palettefile");
  if(myPaletteFile == "")
    myPaletteFile = myBaseDir + BSPF_PATH_SEPARATOR + "stella.pal";
  mySettings->setString("palettefile", myPaletteFile);

  myPropertiesFile = mySettings->getString("propsfile");
  if(myPropertiesFile == "")
    myPropertiesFile = myBaseDir + BSPF_PATH_SEPARATOR + "stella.pro";
  mySettings->setString("propsfile", myPropertiesFile);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OSystem::setUIPalette()
{
  int palette = mySettings->getInt("uipalette") - 1;
  if(palette < 0 || palette >= kNumUIPalettes) palette = 0;
  myFrameBuffer->setUIPalette(&ourGUIColors[palette][0]);
  myFrameBuffer->refresh();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OSystem::setBaseDir(const string& basedir)
{
  myBaseDir = basedir;
  if(!FilesystemNode::dirExists(myBaseDir))
    FilesystemNode::makeDir(myBaseDir);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OSystem::setFramerate(float framerate)
{
  myDisplayFrameRate = framerate;
  myTimePerFrame = (uInt32)(1000000.0 / myDisplayFrameRate);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool OSystem::createFrameBuffer()
{
  // There is only ever one FrameBuffer created per run of Stella
  // Due to the multi-surface nature of the FrameBuffer, repeatedly
  // creating and destroying framebuffer objects causes crashes which
  // are far too invasive to fix right now
  // Besides, how often does one really switch between software and
  // OpenGL rendering modes, and even when they do, does it really
  // need to be dynamic?

  bool firstTime = (myFrameBuffer == NULL);
  if(firstTime)
    myFrameBuffer = MediaFactory::createVideo(this);

  // Re-initialize the framebuffer to current settings
  switch(myEventHandler->state())
  {
    case EventHandler::S_EMULATE:
    case EventHandler::S_PAUSE:
    case EventHandler::S_MENU:
    case EventHandler::S_CMDMENU:
      if(!myConsole->initializeVideo())
        return false;
      break;  // S_EMULATE, S_PAUSE, S_MENU, S_CMDMENU

    case EventHandler::S_LAUNCHER:
      if(!myLauncher->initializeVideo())
        return false;
      break;  // S_LAUNCHER

#ifdef DEBUGGER_SUPPORT
    case EventHandler::S_DEBUGGER:
      if(!myDebugger->initializeVideo())
        return false;
      break;  // S_DEBUGGER
#endif

    default:
      break;
  }

  // The following only need to be done once
  if(firstTime)
  {
    // Setup the SDL joysticks (must be done after FrameBuffer is created)
    myEventHandler->setupJoysticks();

    // Update the UI palette
    setUIPalette();
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OSystem::createSound()
{
  delete mySound;  mySound = NULL;
  mySound = MediaFactory::createAudio(this);
#ifndef SOUND_SUPPORT
  mySettings->setBool("sound", false);
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool OSystem::createConsole(const string& romfile, const string& md5sum)
{
  // Do a little error checking; it shouldn't be necessary
  if(myConsole) deleteConsole();

  bool retval = false, showmessage = false;

  // If a blank ROM has been given, we reload the current one (assuming one exists)
  if(romfile == "")
  {
    showmessage = true;  // we show a message if a ROM is being reloaded
    if(myRomFile == "")
    {
      cerr << "ERROR: Rom file not specified ..." << endl;
      return false;
    }
  }
  else
  {
    myRomFile = romfile;
    myRomMD5  = md5sum;
  }

  // Create an instance of the 2600 game console
  myConsole = openConsole(myRomFile, myRomMD5);
  if(myConsole)
  {
  #ifdef CHEATCODE_SUPPORT
    myCheatManager->loadCheats(myRomMD5);
  #endif
    myEventHandler->reset(EventHandler::S_EMULATE);
    if(!createFrameBuffer())  // Takes care of initializeVideo()
    {
      cerr << "ERROR: Couldn't create framebuffer for console" << endl;
      return false;
    }
    myConsole->initializeAudio();
  #ifdef DEBUGGER_SUPPORT
    myDebugger->setConsole(myConsole);
    myDebugger->initialize();
  #endif

    if(showmessage)
      myFrameBuffer->showMessage("New console created");
    if(mySettings->getBool("showinfo"))
      cout << "Game console created:" << endl
           << "  ROM file: " << myRomFile << endl << endl
           << getROMInfo(myConsole) << endl;

    // Update the timing info for a new console run
    resetLoopTiming();

    myFrameBuffer->setCursorState();
    retval = true;
  }
  else
  {
    cerr << "ERROR: Couldn't create console for " << myRomFile << endl;
    retval = false;
  }

  return retval;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OSystem::deleteConsole()
{
  if(myConsole)
  {
    mySound->close();
  #ifdef CHEATCODE_SUPPORT
    myCheatManager->saveCheats(myConsole->properties().get(Cartridge_MD5));
  #endif
    if(mySettings->getBool("showinfo"))
    {
      double executionTime   = (double) myTimingInfo.totalTime / 1000000.0;
      double framesPerSecond = (double) myTimingInfo.totalFrames / executionTime;
      cout << "Game console stats:" << endl
           << "  Total frames drawn: " << myTimingInfo.totalFrames << endl
           << "  Total time (sec):   " << executionTime << endl
           << "  Frames per second:  " << framesPerSecond << endl
           << endl;
    }
    delete myConsole;  myConsole = NULL;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool OSystem::createLauncher()
{
  myEventHandler->reset(EventHandler::S_LAUNCHER);
  if(!createFrameBuffer())
  {
    cerr << "ERROR: Couldn't create launcher" << endl;
    return false;
  }
  myLauncher->reStack();
  myFrameBuffer->setCursorState();
  myFrameBuffer->refresh();

  setFramerate(60);
  resetLoopTiming();

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string OSystem::getROMInfo(const string& romfile)
{
  string md5, result = "";
  Console* console = openConsole(romfile, md5);
  if(console)
  {
    result = getROMInfo(console);
    delete console;
  }
  else
    result = "ERROR: Couldn't get ROM info for " + romfile + " ...";

  return result;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string OSystem::MD5FromFile(const string& filename)
{
  string md5 = "";

  uInt8* image = 0;
  uInt32 size  = 0;
  if((image = openROM(filename, md5, size)) != 0)
    if(image != 0 && size > 0)
      delete[] image;

  return md5;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Console* OSystem::openConsole(const string& romfile, string& md5)
{
#define CMDLINE_PROPS_UPDATE(cl_name, prop_name) \
  s = mySettings->getString(cl_name);            \
  if(s != "") props.set(prop_name, s);

  Console* console = (Console*) NULL;

  // Open the cartridge image and read it in
  uInt8* image = 0;
  uInt32 size  = 0;
  if((image = openROM(romfile, md5, size)) != 0)
  {
    // Get a valid set of properties, including any entered on the commandline
    Properties props;
    myPropSet->getMD5(md5, props);

    string s = "";
    CMDLINE_PROPS_UPDATE("bs", Cartridge_Type);
    CMDLINE_PROPS_UPDATE("type", Cartridge_Type);
    CMDLINE_PROPS_UPDATE("channels", Cartridge_Sound);
    CMDLINE_PROPS_UPDATE("ld", Console_LeftDifficulty);
    CMDLINE_PROPS_UPDATE("rd", Console_RightDifficulty);
    CMDLINE_PROPS_UPDATE("tv", Console_TelevisionType);
    CMDLINE_PROPS_UPDATE("sp", Console_SwapPorts);
    CMDLINE_PROPS_UPDATE("lc", Controller_Left);
    CMDLINE_PROPS_UPDATE("rc", Controller_Right);
    s = mySettings->getString("bc");
    if(s != "") { props.set(Controller_Left, s); props.set(Controller_Right, s); }
    CMDLINE_PROPS_UPDATE("cp", Controller_SwapPaddles);
    CMDLINE_PROPS_UPDATE("format", Display_Format);
    CMDLINE_PROPS_UPDATE("ystart", Display_YStart);
    CMDLINE_PROPS_UPDATE("height", Display_Height);
    CMDLINE_PROPS_UPDATE("pp", Display_Phosphor);
    CMDLINE_PROPS_UPDATE("ppblend", Display_PPBlend);
    CMDLINE_PROPS_UPDATE("hmove", Emulation_HmoveBlanks);

    Cartridge* cart = Cartridge::create(image, size, props, *mySettings);
    if(cart)
      console = new Console(this, cart, props);
  }
  else
    cerr << "ERROR: Couldn't open " << romfile << endl;

  // Free the image since we don't need it any longer
  if(image != 0 && size > 0)
    delete[] image;

  return console;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8* OSystem::openROM(const string& file, string& md5, uInt32& size)
{
  // This method has a documented side-effect:
  // It not only loads a ROM and creates an array with its contents,
  // but also adds a properties entry if the one for the ROM doesn't
  // contain a valid name

  uInt8* image = 0;

  // Try to open the file as a zipped archive
  // If that fails, we assume it's just a gzipped or normal data file
  unzFile tz;
  if((tz = unzOpen(file.c_str())) != NULL)
  {
    if(unzGoToFirstFile(tz) == UNZ_OK)
    {
      unz_file_info ufo;

      for(;;)  // Loop through all files for valid 2600 images
      {
        // Longer filenames might be possible, but I don't
        // think people would name files that long in zip files...
        char filename[1024];

        unzGetCurrentFileInfo(tz, &ufo, filename, 1024, 0, 0, 0, 0);
        filename[1023] = '\0';

        if(strlen(filename) >= 4)
        {
          // Grab 3-character extension
          char* ext = filename + strlen(filename) - 4;

          if(!BSPF_strcasecmp(ext, ".bin") || !BSPF_strcasecmp(ext, ".a26"))
            break;
        }

        // Scan the next file in the zip
        if(unzGoToNextFile(tz) != UNZ_OK)
          break;
      }

      // Now see if we got a valid image
      if(ufo.uncompressed_size <= 0)
      {
        unzClose(tz);
        return image;
      }
      size  = ufo.uncompressed_size;
      image = new uInt8[size];

      // We don't have to check for any return errors from these functions,
      // since if there are, 'image' will not contain a valid ROM and the
      // calling method can take of it
      unzOpenCurrentFile(tz);
      unzReadCurrentFile(tz, image, size);
      unzCloseCurrentFile(tz);
      unzClose(tz);
    }
    else
    {
      unzClose(tz);
      return image;
    }
  }
  else
  {
    // Assume the file is either gzip'ed or not compressed at all
    gzFile f = gzopen(file.c_str(), "rb");
    if(!f)
      return image;

    image = new uInt8[MAX_ROM_SIZE];
    size = gzread(f, image, MAX_ROM_SIZE);
    gzclose(f);
  }

  // If we get to this point, we know we have a valid file to open
  // Now we make sure that the file has a valid properties entry
  // To save time, only generate an MD5 if we really need one
  if(md5 == "")
    md5 = MD5(image, size);

  // Some games may not have a name, since there may not
  // be an entry in stella.pro.  In that case, we use the rom name
  // and reinsert the properties object
  Properties props;
  myPropSet->getMD5(md5, props);

  string name = props.get(Cartridge_Name);
  if(name == "Untitled")
  {
    // Get the filename from the rom pathname
    string::size_type pos = file.find_last_of(BSPF_PATH_SEPARATOR);
    if(pos+1 != string::npos)
    {
      name = file.substr(pos+1);
      props.set(Cartridge_MD5, md5);
      props.set(Cartridge_Name, name);
      myPropSet->insert(props, false);
    }
  }

  return image;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string OSystem::getROMInfo(const Console* console)
{
  const ConsoleInfo& info = console->about();
  ostringstream buf;

  buf << "  Cart Name:       " << info.CartName << endl
      << "  Cart MD5:        " << info.CartMD5 << endl
      << "  Controller 0:    " << info.Control0 << endl
      << "  Controller 1:    " << info.Control1 << endl
      << "  Display Format:  " << info.DisplayFormat << endl
      << "  Bankswitch Type: " << info.BankSwitch << endl;

  return buf.str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OSystem::resetLoopTiming()
{
  memset(&myTimingInfo, 0, sizeof(TimingInfo));
  myTimingInfo.start = getTicks();
  myTimingInfo.virt = getTicks();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OSystem::setDefaultJoymap()
{
  EventMode mode;

  mode = kEmulationMode;  // Default emulation events
  // Left joystick (assume joystick zero, button zero)
  myEventHandler->setDefaultJoyMapping(Event::JoystickZeroFire1, mode, 0, 0);
  // Right joystick (assume joystick one, button zero)
  myEventHandler->setDefaultJoyMapping(Event::JoystickOneFire1, mode, 1, 0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OSystem::setDefaultJoyAxisMap()
{
  EventMode mode;

  mode = kEmulationMode;  // Default emulation events
  // Left joystick left/right directions (assume joystick zero)
  myEventHandler->setDefaultJoyAxisMapping(Event::JoystickZeroLeft, mode, 0, 0, 0);
  myEventHandler->setDefaultJoyAxisMapping(Event::JoystickZeroRight, mode, 0, 0, 1);
  // Left joystick up/down directions (assume joystick zero)
  myEventHandler->setDefaultJoyAxisMapping(Event::JoystickZeroUp, mode, 0, 1, 0);
  myEventHandler->setDefaultJoyAxisMapping(Event::JoystickZeroDown, mode, 0, 1, 1);
  // Right joystick left/right directions (assume joystick one)
  myEventHandler->setDefaultJoyAxisMapping(Event::JoystickOneLeft, mode, 1, 0, 0);
  myEventHandler->setDefaultJoyAxisMapping(Event::JoystickOneRight, mode, 1, 0, 1);
  // Right joystick left/right directions (assume joystick one)
  myEventHandler->setDefaultJoyAxisMapping(Event::JoystickOneUp, mode, 1, 1, 0);
  myEventHandler->setDefaultJoyAxisMapping(Event::JoystickOneDown, mode, 1, 1, 1);

  mode = kMenuMode;  // Default menu/UI events
  myEventHandler->setDefaultJoyAxisMapping(Event::UILeft, mode, 0, 0, 0);
  myEventHandler->setDefaultJoyAxisMapping(Event::UIRight, mode, 0, 0, 1);
  myEventHandler->setDefaultJoyAxisMapping(Event::UIUp, mode, 0, 1, 0);
  myEventHandler->setDefaultJoyAxisMapping(Event::UIDown, mode, 0, 1, 1);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OSystem::setDefaultJoyHatMap()
{
// FIXME - add emul and UI events
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OSystem::pollEvent()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool OSystem::joyButtonHandled(int button)
{
  // Since we don't do any platform-specific event polling,
  // no button is ever handled at this level
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OSystem::stateChanged(EventHandler::State state)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OSystem::mainLoop()
{
  if(mySettings->getString("timing") == "sleep")
  {
    // Sleep-based wait: good for CPU, bad for graphical sync
    for(;;)
    {
      myTimingInfo.start = getTicks();
      myEventHandler->poll(myTimingInfo.start);
      if(myQuitLoop) break;  // Exit if the user wants to quit
      myFrameBuffer->update();
      myTimingInfo.current = getTicks();
      myTimingInfo.virt += myTimePerFrame;

      if(myTimingInfo.current < myTimingInfo.virt)
        SDL_Delay((myTimingInfo.virt - myTimingInfo.current) / 1000);

      myTimingInfo.totalTime += (getTicks() - myTimingInfo.start);
      myTimingInfo.totalFrames++;
    }
  }
  else
  {
    // Busy-wait: bad for CPU, good for graphical sync
    for(;;)
    {
      myTimingInfo.start = getTicks();
      myEventHandler->poll(myTimingInfo.start);
      if(myQuitLoop) break;  // Exit if the user wants to quit
      myFrameBuffer->update();
      myTimingInfo.virt += myTimePerFrame;

      while(getTicks() < myTimingInfo.virt)
        ;  // busy-wait

      myTimingInfo.totalTime += (getTicks() - myTimingInfo.start);
      myTimingInfo.totalFrames++;
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OSystem::queryVideoHardware()
{
  if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0)
    return;

  // First get the maximum windowed desktop resolution
  const SDL_VideoInfo* info = SDL_GetVideoInfo();
  myDesktopWidth  = info->current_w;
  myDesktopHeight = info->current_h;

  // Then get the valid fullscreen modes
  // If there are any errors, just use the desktop resolution
  ostringstream buf;
  SDL_Rect** modes = SDL_ListModes(NULL, SDL_FULLSCREEN);
  if((modes == (SDL_Rect**)0) || (modes == (SDL_Rect**)-1))
  {
    Resolution r;
    r.width  = myDesktopWidth;
    r.height = myDesktopHeight;
    buf << r.width << "x" << r.height;
    r.name = buf.str();
    myResolutions.push_back(r);
  }
  else
  {
    for(uInt32 i = 0; modes[i]; ++i)
    {
      Resolution r;
      r.width  = modes[i]->w;
      r.height = modes[i]->h;
      buf.str("");
      buf << r.width << "x" << r.height;
      r.name = buf.str();
      myResolutions.insert_at(0, r);  // insert in opposite (of descending) order
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/*
  Palette is defined as follows:
    // Base colors
    kColor         TODO
    kBGColor       TODO
    kShadowColor      Item is disabled
    kTextColor        Normal text color
    kTextColorHi      Highlighted text color
    kTextColorEm   TODO

    // UI elements (dialog and widgets)
    kDlgColor         Dialog background
    kWidColor         Widget background
    kWidFrameColor    Border for currently selected widget

    // Button colors
    kBtnColor         Normal button background
    kBtnColorHi       Highlighted button background
    kBtnTextColor     Normal button font color
    kBtnTextColorHi   Highlighted button font color

    // Checkbox colors
    kCheckColor       Color of 'X' in checkbox

    // Scrollbar colors
    kScrollColor      Normal scrollbar color
    kScrollColorHi    Highlighted scrollbar color

    // Debugger colors
    kDbgChangedColor      Background color for changed cells
    kDbgChangedTextColor  Text color for changed cells
    kDbgColorHi           Highlighted color in debugger data cells
*/
uInt32 OSystem::ourGUIColors[kNumUIPalettes][kNumColors-256] = {
  // Standard
  { 0x686868, 0x000000, 0x404040, 0x000000, 0x62a108, 0x9f0000,
    0xc9af7c, 0xf0f0cf, 0xc80000,
    0xac3410, 0xd55941, 0xffffff, 0xffd652,
    0xac3410,
    0xac3410, 0xd55941,
    0xac3410, 0xd55941,
    0xc80000, 0x00ff00, 0xc8c8ff
  },

  // Classic
  { 0x686868, 0x000000, 0x404040, 0x20a020, 0x00ff00, 0xc80000,
    0x000000, 0x000000, 0xc80000,
    0x000000, 0x000000, 0x20a020, 0x00ff00,
    0x20a020,
    0x20a020, 0x00ff00,
    0x20a020, 0x00ff00,
    0xc80000, 0x00ff00, 0xc8c8ff
  }
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
OSystem::OSystem(const OSystem& osystem)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
OSystem& OSystem::operator = (const OSystem&)
{
  assert(false);

  return *this;
}
