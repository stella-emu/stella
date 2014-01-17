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
// Copyright (c) 1995-2014 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

#include <cassert>
#include <sstream>
#include <fstream>

#include <ctime>
#ifdef HAVE_GETTIMEOFDAY
  #include <sys/time.h>
#endif

#include "bspf.hxx"

#include "MediaFactory.hxx"
#include "Sound.hxx"

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
#include "MD5.hxx"
#include "Cart.hxx"
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
#include "Version.hxx"

#include "OSystem.hxx"

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
    myLauncherUsed(false),
    myDebugger(NULL),
    myCheatManager(NULL),
    myStateManager(NULL),
    myPNGLib(NULL),
    myQuitLoop(false),
    myRomFile(""),
    myRomMD5(""),
    myFeatures(""),
    myBuildInfo(""),
    myFont(NULL)
{
  // Calculate startup time
  myMillisAtStart = (uInt32)(time(NULL) * 1000);

  // Get built-in features
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

  // Get build info
  ostringstream info;
  const SDL_version* ver = SDL_Linked_Version();

  info << "Build " << STELLA_BUILD << ", using SDL " << (int)ver->major
       << "." << (int)ver->minor << "."<< (int)ver->patch
       << " [" << BSPF_ARCH << "]";
  myBuildInfo = info.str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
OSystem::~OSystem()
{
  delete myMenu;
  delete myCommandMenu;
  delete myLauncher;
  delete myFont;
  delete myInfoFont;
  delete mySmallFont;
  delete myLauncherFont;

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
  delete myPNGLib;
  delete myZipHandler;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool OSystem::create()
{
  // Get updated paths for all configuration files
  setConfigPaths();
  ostringstream buf;
  buf << "Stella " << STELLA_VERSION << endl
      << "  Features: " << myFeatures << endl
      << "  " << myBuildInfo << endl << endl
      << "Base directory:       '"
      << FilesystemNode(myBaseDir).getShortPath() << "'" << endl
      << "Configuration file:   '"
      << FilesystemNode(myConfigFile).getShortPath() << "'" << endl
      << "User game properties: '"
      << FilesystemNode(myPropertiesFile).getShortPath() << "'" << endl;
  logMessage(buf.str(), 1);

  // Get relevant information about the video hardware
  // This must be done before any graphics context is created, since
  // it may be needed to initialize the size of graphical objects
  if(!queryVideoHardware())
    return false;

  ////////////////////////////////////////////////////////////////////
  // Create fonts to draw text
  // NOTE: the logic determining appropriate font sizes is done here,
  //       so that the UI classes can just use the font they expect,
  //       and not worry about it
  //       This logic should also take into account the size of the
  //       framebuffer, and try to be intelligent about font sizes
  //       We can probably add ifdefs to take care of corner cases,
  //       but that means we've failed to abstract it enough ...
  ////////////////////////////////////////////////////////////////////
  bool smallScreen = myDesktopWidth < 640 || myDesktopHeight < 480;

  // This font is used in a variety of situations when a really small
  // font is needed; we let the specific widget/dialog decide when to
  // use it
  mySmallFont = new GUI::Font(GUI::stellaDesc);

  // The general font used in all UI elements
  // This is determined by the size of the framebuffer
  myFont = new GUI::Font(smallScreen ? GUI::stellaDesc : GUI::stellaMediumDesc);

  // The info font used in all UI elements
  // This is determined by the size of the framebuffer
  myInfoFont = new GUI::Font(smallScreen ? GUI::stellaDesc : GUI::consoleDesc);

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

  // Create PNG handler
  myPNGLib = new PNGLibrary();

  // Create ZIP handler
  myZipHandler = new ZipHandler();

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OSystem::loadConfig()
{
  mySettings->loadConfig();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OSystem::saveConfig()
{
  // Ask all subsystems to save their settings
  if(myFrameBuffer)
    myFrameBuffer->ntsc().saveConfig(*mySettings);

  mySettings->saveConfig();
}

#ifdef DEBUGGER_SUPPORT
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OSystem::createDebugger(Console& console)
{
  delete myDebugger;  myDebugger = NULL;
  myDebugger = new Debugger(*this, console);
  myDebugger->initialize();
}
#endif

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OSystem::setConfigPaths()
{
  // Paths are saved with special characters preserved ('~' or '.')
  // We do some error checking here, so the rest of the codebase doesn't
  // have to worry about it
  FilesystemNode node;
  string s;

  validatePath(myStateDir, "statedir", myBaseDir + "state");
  validatePath(mySnapshotSaveDir, "snapsavedir", defaultSnapSaveDir());
  validatePath(mySnapshotLoadDir, "snaploaddir", defaultSnapLoadDir());
  validatePath(myNVRamDir, "nvramdir", myBaseDir + "nvram");
  validatePath(myCfgDir, "cfgdir", myBaseDir + "cfg");

  s = mySettings->getString("cheatfile");
  if(s == "") s = myBaseDir + "stella.cht";
  node = FilesystemNode(s);
  myCheatFile = node.getPath();
  mySettings->setValue("cheatfile", node.getShortPath());

  s = mySettings->getString("palettefile");
  if(s == "") s = myBaseDir + "stella.pal";
  node = FilesystemNode(s);
  myPaletteFile = node.getPath();
  mySettings->setValue("palettefile", node.getShortPath());

  s = mySettings->getString("propsfile");
  if(s == "") s = myBaseDir + "stella.pro";
  node = FilesystemNode(s);
  myPropertiesFile = node.getPath();
  mySettings->setValue("propsfile", node.getShortPath());
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
  FilesystemNode node(basedir);
  if(!node.isDirectory())
    node.makeDir();

  myBaseDir = node.getPath();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OSystem::setConfigFile(const string& file)
{
  FilesystemNode node(file);
  myConfigFile = node.getPath();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OSystem::setFramerate(float framerate)
{
  if(framerate > 0.0)
  {
    myDisplayFrameRate = framerate;
    myTimePerFrame = (uInt32)(1000000.0 / myDisplayFrameRate);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FBInitStatus OSystem::createFrameBuffer()
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
  FBInitStatus fbstatus = kFailComplete;
  switch(myEventHandler->state())
  {
    case EventHandler::S_EMULATE:
    case EventHandler::S_PAUSE:
    case EventHandler::S_MENU:
    case EventHandler::S_CMDMENU:
      fbstatus = myConsole->initializeVideo();
      if(fbstatus != kSuccess)
        goto fallback;
      break;  // S_EMULATE, S_PAUSE, S_MENU, S_CMDMENU

    case EventHandler::S_LAUNCHER:
      fbstatus = myLauncher->initializeVideo();
      if(fbstatus != kSuccess)
        goto fallback;
      break;  // S_LAUNCHER

#ifdef DEBUGGER_SUPPORT
    case EventHandler::S_DEBUGGER:
      fbstatus = myDebugger->initializeVideo();
      if(fbstatus != kSuccess)
        goto fallback;
      break;  // S_DEBUGGER
#endif

    default:  // Should never happen
      logMessage("ERROR: Unknown emulation state in createFrameBuffer()", 0);
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

  return fbstatus;

  // GOTO are normally considered evil, unless well documented :)
  // If initialization of video system fails while in OpenGL mode
  // because OpenGL is unavailable, attempt to fallback to software mode
  // Otherwise, pass the error to the parent
fallback:
  if(fbstatus == kFailNotSupported && myFrameBuffer &&
     myFrameBuffer->type() == kDoubleBuffer)
  {
    logMessage("ERROR: OpenGL mode failed, fallback to software", 0);
    delete myFrameBuffer; myFrameBuffer = NULL;
    mySettings->setValue("video", "soft");
    FBInitStatus newstatus = createFrameBuffer();
    if(newstatus == kSuccess)
    {
      setFramerate(60);
      myFrameBuffer->showMessage("OpenGL mode failed, fallback to software",
                                 kMiddleCenter, true);
    }
    return newstatus;
  }
  else
    return fbstatus;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OSystem::createSound()
{
  if(!mySound)
    mySound = MediaFactory::createAudio(this);
#ifndef SOUND_SUPPORT
  mySettings->setValue("sound", false);
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string OSystem::createConsole(const FilesystemNode& rom, const string& md5sum,
                              bool newrom)
{
  // Do a little error checking; it shouldn't be necessary
  if(myConsole) deleteConsole();

  bool showmessage = false;

  // If same ROM has been given, we reload the current one (assuming one exists)
  if(!newrom && rom == myRomFile)
  {
    showmessage = true;  // we show a message if a ROM is being reloaded
  }
  else
  {
    myRomFile = rom;
    myRomMD5  = md5sum;

    // Each time a new console is loaded, we simulate a cart removal
    // Some carts need knowledge of this, as they behave differently
    // based on how many power-cycles they've been through since plugged in
    mySettings->setValue("romloadcount", 0);
  }

  // Create an instance of the 2600 game console
  ostringstream buf;
  string type, id;
  try
  {
    myConsole = openConsole(myRomFile, myRomMD5, type, id);
  }
  catch(const char* err_msg)
  {
    myConsole = 0;
    buf << "ERROR: Couldn't create console (" << err_msg << ")";
    logMessage(buf.str(), 0);
    return buf.str();
  }

  if(myConsole)
  {
  #ifdef DEBUGGER_SUPPORT
    myConsole->addDebugger();
  #endif
  #ifdef CHEATCODE_SUPPORT
    myCheatManager->loadCheats(myRomMD5);
  #endif
    //////////////////////////////////////////////////////////////////////////
    // For some reason, ATI video drivers for OpenGL in Win32 cause problems
    // if the sound isn't initialized before the video
    // According to the SDL documentation, it shouldn't matter what order the
    // systems are initialized, but apparently it *does* matter
    // For now, I'll just reverse the ordering, as suggested by 'zagon' at
    // http://www.atariage.com/forums/index.php?showtopic=126090&view=findpost&p=1648693
    // Hopefully it won't break anything else
    //////////////////////////////////////////////////////////////////////////
    myConsole->initializeAudio();
    myEventHandler->reset(EventHandler::S_EMULATE);
    myEventHandler->setMouseControllerMode(mySettings->getString("usemouse"));
    if(createFrameBuffer() != kSuccess)  // Takes care of initializeVideo()
    {
      logMessage("ERROR: Couldn't create framebuffer for console", 0);
      myEventHandler->reset(EventHandler::S_LAUNCHER);
      return "ERROR: Couldn't create framebuffer for console";
    }

    if(showmessage)
    {
      if(id == "")
        myFrameBuffer->showMessage("New console created");
      else
        myFrameBuffer->showMessage("Multicart " + type + ", loading ROM" + id);
    }
    buf << "Game console created:" << endl
        << "  ROM file: " << myRomFile.getShortPath() << endl << endl
        << getROMInfo(myConsole) << endl;
    logMessage(buf.str(), 1);

    // Update the timing info for a new console run
    resetLoopTiming();

    myFrameBuffer->setCursorState();

    // Also check if certain virtual buttons should be held down
    // These must be checked each time a new console is being created
    if(mySettings->getBool("holdreset"))
      myEventHandler->handleEvent(Event::ConsoleReset, 1);
    if(mySettings->getBool("holdselect"))
      myEventHandler->handleEvent(Event::ConsoleSelect, 1);

    const string& holdjoy0 = mySettings->getString("holdjoy0");
    if(BSPF_containsIgnoreCase(holdjoy0, "U"))
      myEventHandler->handleEvent(Event::JoystickZeroUp, 1);
    if(BSPF_containsIgnoreCase(holdjoy0, "D"))
      myEventHandler->handleEvent(Event::JoystickZeroDown, 1);
    if(BSPF_containsIgnoreCase(holdjoy0, "L"))
      myEventHandler->handleEvent(Event::JoystickZeroLeft, 1);
    if(BSPF_containsIgnoreCase(holdjoy0, "R"))
      myEventHandler->handleEvent(Event::JoystickZeroRight, 1);
    if(BSPF_containsIgnoreCase(holdjoy0, "F"))
      myEventHandler->handleEvent(Event::JoystickZeroFire, 1);

    const string& holdjoy1 = mySettings->getString("holdjoy1");
    if(BSPF_containsIgnoreCase(holdjoy1, "U"))
      myEventHandler->handleEvent(Event::JoystickOneUp, 1);
    if(BSPF_containsIgnoreCase(holdjoy1, "D"))
      myEventHandler->handleEvent(Event::JoystickOneDown, 1);
    if(BSPF_containsIgnoreCase(holdjoy1, "L"))
      myEventHandler->handleEvent(Event::JoystickOneLeft, 1);
    if(BSPF_containsIgnoreCase(holdjoy1, "R"))
      myEventHandler->handleEvent(Event::JoystickOneRight, 1);
    if(BSPF_containsIgnoreCase(holdjoy1, "F"))
      myEventHandler->handleEvent(Event::JoystickOneFire, 1);
  #ifdef DEBUGGER_SUPPORT
    if(mySettings->getBool("debug"))
      myEventHandler->enterDebugMode();
  #endif
  }
  return EmptyString;
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
    ostringstream buf;
    double executionTime   = (double) myTimingInfo.totalTime / 1000000.0;
    double framesPerSecond = (double) myTimingInfo.totalFrames / executionTime;
    buf << "Game console stats:" << endl
        << "  Total frames drawn: " << myTimingInfo.totalFrames << endl
        << "  Total time (sec):   " << executionTime << endl
        << "  Frames per second:  " << framesPerSecond << endl
        << endl;
    logMessage(buf.str(), 1);

    delete myConsole;  myConsole = NULL;
  #ifdef DEBUGGER_SUPPORT
    delete myDebugger; myDebugger = NULL;
  #endif
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool OSystem::reloadConsole()
{
  deleteConsole();
  return createConsole(myRomFile, myRomMD5, false) == EmptyString;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool OSystem::createLauncher(const string& startdir)
{
  mySettings->setValue("tmpromdir", startdir);
  bool status = false;

  myEventHandler->reset(EventHandler::S_LAUNCHER);
  if(createFrameBuffer() == kSuccess)
  {
    myLauncher->reStack();
    myFrameBuffer->setCursorState();
    myFrameBuffer->refresh();

    setFramerate(60);
    resetLoopTiming();
    status = true;
  }
  else
    logMessage("ERROR: Couldn't create launcher", 0);

  myLauncherUsed = myLauncherUsed || status;
  return status;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string OSystem::getROMInfo(const FilesystemNode& romfile)
{
  string md5, type, id, result = "";
  Console* console = 0;
  try
  {
    console = openConsole(romfile, md5, type, id);
  }
  catch(const char* err_msg)
  {
    ostringstream buf;
    buf << "ERROR: Couldn't get ROM info (" << err_msg << ")";
    return buf.str();
  }

  result = getROMInfo(console);
  delete console;
  return result;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OSystem::logMessage(const string& message, uInt8 level)
{
  if(level == 0)
  {
    cout << message << endl << flush;
    myLogMessages += message + "\n";
  }
  else if(level <= (uInt8)mySettings->getInt("loglevel"))
  {
    if(mySettings->getBool("logtoconsole"))
      cout << message << endl << flush;
    myLogMessages += message + "\n";
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Console* OSystem::openConsole(const FilesystemNode& romfile, string& md5,
                              string& type, string& id)
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
    // For initial creation of the Cart, we're only concerned with the BS type
    Properties props;
    myPropSet->getMD5(md5, props);
    string s = "";
    CMDLINE_PROPS_UPDATE("bs", Cartridge_Type);
    CMDLINE_PROPS_UPDATE("type", Cartridge_Type);

    // Now create the cartridge
    string cartmd5 = md5;
    type = props.get(Cartridge_Type);
    Cartridge* cart =
      Cartridge::create(image, size, cartmd5, type, id, *this, *mySettings);

    // It's possible that the cart created was from a piece of the image,
    // and that the md5 (and hence the cart) has changed
    if(props.get(Cartridge_MD5) != cartmd5)
    {
      if(!myPropSet->getMD5(cartmd5, props))
      {
        // Cart md5 wasn't found, so we create a new props for it
        props.set(Cartridge_MD5, cartmd5);
        props.set(Cartridge_Name, props.get(Cartridge_Name)+id);
        myPropSet->insert(props, false);
      }
    }

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
    CMDLINE_PROPS_UPDATE("ma", Controller_MouseAxis);
    CMDLINE_PROPS_UPDATE("format", Display_Format);
    CMDLINE_PROPS_UPDATE("ystart", Display_YStart);
    CMDLINE_PROPS_UPDATE("height", Display_Height);
    CMDLINE_PROPS_UPDATE("pp", Display_Phosphor);
    CMDLINE_PROPS_UPDATE("ppblend", Display_PPBlend);

    // Finally, create the cart with the correct properties
    if(cart)
      console = new Console(this, cart, props);
  }

  // Free the image since we don't need it any longer
  if(image != 0 && size > 0)
    delete[] image;

  return console;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8* OSystem::openROM(const FilesystemNode& rom, string& md5, uInt32& size)
{
  // This method has a documented side-effect:
  // It not only loads a ROM and creates an array with its contents,
  // but also adds a properties entry if the one for the ROM doesn't
  // contain a valid name

  uInt8* image = 0;
  if((size = rom.read(image)) == 0)
  {
    delete[] image;
    return (uInt8*) 0;
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
  myPropSet->getMD5WithInsert(rom, md5, props);

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
  myTimingInfo.start = myTimingInfo.virt = getTicks();
  myTimingInfo.current = 0;
  myTimingInfo.totalTime = 0;
  myTimingInfo.totalFrames = 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OSystem::validatePath(string& path, const string& setting,
                           const string& defaultpath)
{
  const string& s = mySettings->getString(setting) == "" ? defaultpath :
                    mySettings->getString(setting);
  FilesystemNode node(s);
  if(!node.isDirectory())
    node.makeDir();

  path = node.getPath();
  mySettings->setValue(setting, node.getShortPath());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OSystem::setDefaultJoymap(Event::Type event, EventMode mode)
{
#define SET_DEFAULT_AXIS(sda_event, sda_mode, sda_stick, sda_axis, sda_val, sda_cmp_event) \
  if(eraseAll || sda_cmp_event == sda_event) \
    myEventHandler->addJoyAxisMapping(sda_event, sda_mode, sda_stick, sda_axis, sda_val, false);

#define SET_DEFAULT_BTN(sdb_event, sdb_mode, sdb_stick, sdb_button, sdb_cmp_event) \
  if(eraseAll || sdb_cmp_event == sdb_event) \
    myEventHandler->addJoyButtonMapping(sdb_event, sdb_mode, sdb_stick, sdb_button, false);

#define SET_DEFAULT_HAT(sdh_event, sdh_mode, sdh_stick, sdh_hat, sdh_dir, sdh_cmp_event) \
  if(eraseAll || sdh_cmp_event == sdh_event) \
    myEventHandler->addJoyHatMapping(sdh_event, sdh_mode, sdh_stick, sdh_hat, sdh_dir, false);

  bool eraseAll = (event == Event::NoType);
  switch(mode)
  {
    case kEmulationMode:  // Default emulation events
      // Left joystick left/right directions (assume joystick zero)
      SET_DEFAULT_AXIS(Event::JoystickZeroLeft, mode, 0, 0, 0, event);
      SET_DEFAULT_AXIS(Event::JoystickZeroRight, mode, 0, 0, 1, event);
      // Left joystick up/down directions (assume joystick zero)
      SET_DEFAULT_AXIS(Event::JoystickZeroUp, mode, 0, 1, 0, event);
      SET_DEFAULT_AXIS(Event::JoystickZeroDown, mode, 0, 1, 1, event);
      // Right joystick left/right directions (assume joystick one)
      SET_DEFAULT_AXIS(Event::JoystickOneLeft, mode, 1, 0, 0, event);
      SET_DEFAULT_AXIS(Event::JoystickOneRight, mode, 1, 0, 1, event);
      // Right joystick left/right directions (assume joystick one)
      SET_DEFAULT_AXIS(Event::JoystickOneUp, mode, 1, 1, 0, event);
      SET_DEFAULT_AXIS(Event::JoystickOneDown, mode, 1, 1, 1, event);

      // Left joystick (assume joystick zero, button zero)
      SET_DEFAULT_BTN(Event::JoystickZeroFire, mode, 0, 0, event);
      // Right joystick (assume joystick one, button zero)
      SET_DEFAULT_BTN(Event::JoystickOneFire, mode, 1, 0, event);

      // Left joystick left/right directions (assume joystick zero and hat 0)
      SET_DEFAULT_HAT(Event::JoystickZeroLeft, mode, 0, 0, EVENT_HATLEFT, event);
      SET_DEFAULT_HAT(Event::JoystickZeroRight, mode, 0, 0, EVENT_HATRIGHT, event);
      // Left joystick up/down directions (assume joystick zero and hat 0)
      SET_DEFAULT_HAT(Event::JoystickZeroUp, mode, 0, 0, EVENT_HATUP, event);
      SET_DEFAULT_HAT(Event::JoystickZeroDown, mode, 0, 0, EVENT_HATDOWN, event);

      break;

    case kMenuMode:  // Default menu/UI events
      SET_DEFAULT_AXIS(Event::UILeft, mode, 0, 0, 0, event);
      SET_DEFAULT_AXIS(Event::UIRight, mode, 0, 0, 1, event);
      SET_DEFAULT_AXIS(Event::UIUp, mode, 0, 1, 0, event);
      SET_DEFAULT_AXIS(Event::UIDown, mode, 0, 1, 1, event);

      // Left joystick (assume joystick zero, button zero)
      SET_DEFAULT_BTN(Event::UISelect, mode, 0, 0, event);
      // Right joystick (assume joystick one, button zero)
      SET_DEFAULT_BTN(Event::UISelect, mode, 1, 0, event);

      SET_DEFAULT_HAT(Event::UILeft, mode, 0, 0, EVENT_HATLEFT, event);
      SET_DEFAULT_HAT(Event::UIRight, mode, 0, 0, EVENT_HATRIGHT, event);
      SET_DEFAULT_HAT(Event::UIUp, mode, 0, 0, EVENT_HATUP, event);
      SET_DEFAULT_HAT(Event::UIDown, mode, 0, 0, EVENT_HATDOWN, event);

      break;

    default:
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt64 OSystem::getTicks() const
{
#ifdef HAVE_GETTIMEOFDAY
  // Gettimeofday natively refers to the UNIX epoch (a set time in the past)
  timeval now;
  gettimeofday(&now, 0);

  return uInt64(now.tv_sec) * 1000000 + now.tv_usec;
#else
  // We use SDL_GetTicks, but add in the time when the application was
  // initialized.  This is necessary, since SDL_GetTicks only measures how
  // long SDL has been running, which can be the same between multiple runs
  // of the application.
  return uInt64(SDL_GetTicks() + myMillisAtStart) * 1000;
#endif
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

      // Timestamps may periodically go out of sync, particularly on systems
      // that can have 'negative time' (ie, when the time seems to go backwards)
      // This normally results in having a very large delay time, so we check
      // for that and reset the timers when appropriate
      if((myTimingInfo.virt - myTimingInfo.current) > (myTimePerFrame << 1))
      {
        myTimingInfo.start = myTimingInfo.current = myTimingInfo.virt = getTicks();
      }

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
bool OSystem::queryVideoHardware()
{
  // Go ahead and open the video hardware; we're going to need it eventually
  if(SDL_WasInit(SDL_INIT_VIDEO) == 0)
    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0)
      return false;

  // First get the maximum windowed desktop resolution
  // Check the 'maxres' setting, which is an undocumented developer feature
  // that specifies the desktop size
  // Normally, this wouldn't be set, and we ask SDL directly
  const GUI::Size& s = mySettings->getSize("maxres");
  if(s.w <= 0 || s.h <= 0)
  {
    const SDL_VideoInfo* info = SDL_GetVideoInfo();
    myDesktopWidth  = info->current_w;
    myDesktopHeight = info->current_h;
  }
  else
  {
    myDesktopWidth  = BSPF_max(s.w, 320);
    myDesktopHeight = BSPF_max(s.h, 240);
  }

  // Various parts of the codebase assume a minimum screen size of 320x240
  if(!(myDesktopWidth >= 320 && myDesktopHeight >= 240))
  {
    logMessage("ERROR: queryVideoHardware failed, "
               "window 320x240 or larger required", 0);
    return false;
  }

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
    // All modes must fit between the lower and upper limits of the desktop
    // For 'small' desktop, this means larger than 320x240
    // For 'large'/normal desktop, exclude all those less than 640x480
    bool largeDesktop = myDesktopWidth >= 640 && myDesktopHeight >= 480;
    uInt32 lowerWidth  = largeDesktop ? 640 : 320,
           lowerHeight = largeDesktop ? 480 : 240;
    for(uInt32 i = 0; modes[i]; ++i)
    {
      if(modes[i]->w >= lowerWidth && modes[i]->w <= myDesktopWidth &&
         modes[i]->h >= lowerHeight && modes[i]->h <= myDesktopHeight)
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
    // If no modes were valid, use the desktop dimensions
    if(myResolutions.size() == 0)
    {
      Resolution r;
      r.width  = myDesktopWidth;
      r.height = myDesktopHeight;
      buf << r.width << "x" << r.height;
      r.name = buf.str();
      myResolutions.push_back(r);
    }
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/*
  Palette is defined as follows:
    // Base colors
    kColor            Normal foreground color (non-text)
    kBGColor          Normal background color (non-text)
    kShadowColor      Item is disabled
    kTextColor        Normal text color
    kTextColorHi      Highlighted text color
    kTextColorEm      Emphasized text color

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

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ZipHandler* OSystem::myZipHandler = 0;
