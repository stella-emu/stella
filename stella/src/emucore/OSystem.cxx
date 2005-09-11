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
// $Id: OSystem.cxx,v 1.39 2005-09-11 22:55:51 stephena Exp $
//============================================================================

#include <cassert>
#include <sstream>
#include <fstream>

#include "unzip.h"

#include "FrameBuffer.hxx"
#include "FrameBufferSoft.hxx"
#ifdef DISPLAY_OPENGL
  #include "FrameBufferGL.hxx"
#endif

#include "Sound.hxx"
#include "SoundNull.hxx"
#ifdef SOUND_SUPPORT
  #include "SoundSDL.hxx"
#endif

#ifdef DEVELOPER_SUPPORT
  #include "Debugger.hxx"
#endif

#include "FSNode.hxx"
#include "Settings.hxx"
#include "PropsSet.hxx"
#include "EventHandler.hxx"
#include "Menu.hxx"
#include "CommandMenu.hxx"
#include "Launcher.hxx"
#include "Font.hxx"
#include "StellaFont.hxx"
#include "ConsoleFont.hxx"
#include "bspf.hxx"
#include "OSystem.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
OSystem::OSystem()
  : myEventHandler(NULL),
    myFrameBuffer(NULL),
    mySound(NULL),
    mySettings(NULL),
    myPropSet(NULL),
    myConsole(NULL),
    myMenu(NULL),
    myCommandMenu(NULL),
    myLauncher(NULL),
    myDebugger(NULL),
    myRomFile(""),
    myFeatures(""),
    myFont(NULL),
    myConsoleFont(NULL)
{
  // Create menu and launcher GUI objects
  myMenu = new Menu(this);
  myCommandMenu = new CommandMenu(this);
  myLauncher = new Launcher(this);
#ifdef DEVELOPER_SUPPORT
  myDebugger = new Debugger(this);
#endif

  // Create fonts to draw text
  myFont        = new GUI::Font(GUI::stellaDesc);
  myConsoleFont = new GUI::Font(GUI::consoleDesc);

  // Determine which features were conditionally compiled into Stella
#ifdef DISPLAY_OPENGL
  myFeatures += "OpenGL ";
#endif
#ifdef SOUND_SUPPORT 
  myFeatures += "Sound ";
#endif
#ifdef JOYSTICK_SUPPORT
  myFeatures += "Joystick ";
#endif
#ifdef SNAPSHOT_SUPPORT
  myFeatures += "Snapshot ";
#endif
#ifdef DEVELOPER_SUPPORT
  myFeatures += "Debugger";
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
OSystem::~OSystem()
{
  myDriverList.clear();

  delete myMenu;
  delete myCommandMenu;
  delete myLauncher;
  delete myFont;
  delete myConsoleFont;

#ifdef DEVELOPER_SUPPORT
  delete myDebugger;
#endif

  // Remove any game console that is currently attached
  delete myConsole;

  // OSystem takes responsibility for framebuffer and sound,
  // since it created them
  delete myFrameBuffer;
  delete mySound;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OSystem::setBaseDir(const string& basedir)
{
  myBaseDir = basedir;
  if(!FilesystemNode::dirExists(myBaseDir))
    FilesystemNode::makeDir(myBaseDir);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OSystem::setStateDir(const string& statedir)
{
  myStateDir = statedir;
  if(!FilesystemNode::dirExists(myStateDir))
    FilesystemNode::makeDir(myStateDir);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OSystem::setPropertiesDir(const string& userpath,
                               const string& systempath)
{
  // Set up the input and output properties files
  myUserPropertiesFile   = userpath + BSPF_PATH_SEPARATOR + "user.pro";
  mySystemPropertiesFile = systempath + BSPF_PATH_SEPARATOR + "stella.pro";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OSystem::setConfigFiles(const string& userconfig,
                             const string& systemconfig)
{
  // Set up the names of the input and output config files
  myConfigOutputFile = userconfig;

  if(FilesystemNode::fileExists(userconfig))
    myConfigInputFile = userconfig;
  else if(FilesystemNode::fileExists(systemconfig))
    myConfigInputFile = systemconfig;
  else
    myConfigInputFile = "";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool OSystem::createFrameBuffer(bool showmessage)
{
  // Delete the old framebuffer
  delete myFrameBuffer;  myFrameBuffer = NULL;

  // And recreate a new one
  string video = mySettings->getString("video");

  if(video == "soft")
    myFrameBuffer = new FrameBufferSoft(this);
#ifdef DISPLAY_OPENGL
  else if(video == "gl")
    myFrameBuffer = new FrameBufferGL(this);
#endif
  else   // a driver that doesn't exist was requested, so use software mode
    myFrameBuffer = new FrameBufferSoft(this);

  if(!myFrameBuffer)
    return false;

  // Re-initialize the framebuffer to current settings
  switch(myEventHandler->state())
  {
    case EventHandler::S_EMULATE:
    case EventHandler::S_MENU:
    case EventHandler::S_CMDMENU:
      myConsole->initializeVideo();
      if(showmessage)
      {
        if(video == "soft")
          myFrameBuffer->showMessage("Software mode");
      #ifdef DISPLAY_OPENGL
        else if(video == "gl")
          myFrameBuffer->showMessage("OpenGL mode");
      #endif
        else   // a driver that doesn't exist was requested, so use software mode
          myFrameBuffer->showMessage("Software mode");
      }
      break;  // S_EMULATE, S_MENU, S_CMDMENU

    case EventHandler::S_LAUNCHER:
      myLauncher->initializeVideo();
      break;  // S_LAUNCHER

#ifdef DEVELOPER_SUPPORT
    case EventHandler::S_DEBUGGER:
      myDebugger->initializeVideo();
      break;  // S_DEBUGGER
#endif

    default:
      break;
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OSystem::toggleFrameBuffer()
{
  // First figure out which mode to switch to
  string video = mySettings->getString("video");
  if(video == "soft")
    video = "gl";
  else if(video == "gl")
    video = "soft";
  else   // a driver that doesn't exist was requested, so use software mode
    video = "soft";

  // Update the settings and create the framebuffer
  mySettings->setString("video", video);
  createFrameBuffer(true);  // show onscreen message
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OSystem::createSound()
{
  // Delete the old sound device
  delete mySound;  mySound = NULL;

  // And recreate a new sound device
#ifdef SOUND_SUPPORT
  mySound = new SoundSDL(this);
#else
  mySettings->setBool("sound", false);
  mySound = new SoundNull(this);
#endif

  // Re-initialize the sound object to current settings
  switch(myEventHandler->state())
  {
    case EventHandler::S_EMULATE:
    case EventHandler::S_MENU:
    case EventHandler::S_CMDMENU:
    case EventHandler::S_DEBUGGER:
      myConsole->initializeAudio();
      break;  // S_EMULATE, S_MENU, S_DEBUGGER

    default:
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool OSystem::createConsole(const string& romfile)
{
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
    myRomFile = romfile;

  // Open the cartridge image and read it in
  uInt8* image;
  int size = -1;
  if(openROM(myRomFile, &image, &size))
  {
    delete myConsole;  myConsole = NULL;

    // Create an instance of the 2600 game console
    // The Console c'tor takes care of updating the eventhandler state
    myConsole = new Console(image, size, this);

    if(showmessage)
      myFrameBuffer->showMessage("New console created");
    if(mySettings->getBool("showinfo"))
      cout << "Game console created: " << myRomFile << endl;

    retval = true;
    myEventHandler->reset(EventHandler::S_EMULATE);
    myFrameBuffer->setCursorState();
  }
  else
  {
    cerr << "ERROR: Couldn't open " << myRomFile << "..." << endl;
//    myEventHandler->quit();
    retval = false;
  }

  // Free the image since we don't need it any longer
  if(size != -1)
    delete[] image;

  return retval;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OSystem::createLauncher()
{
  mySound->close();
  setFramerate(60);
  myEventHandler->reset(EventHandler::S_LAUNCHER);

  // Create the window
  myLauncher->initializeVideo();

  // And start the base dialog
  myLauncher->initialize();
  myLauncher->reStack();

  myEventHandler->refreshDisplay();

  myFrameBuffer->setCursorState();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool OSystem::openROM(const string& rom, uInt8** image, int* size)
{
  // Try to open the file as a zipped archive
  // If that fails, we assume it's just a normal data file
  unzFile tz;
  if((tz = unzOpen(rom.c_str())) != NULL)
  {
    if(unzGoToFirstFile(tz) == UNZ_OK)
    {
      unz_file_info ufo; 
      unzGetCurrentFileInfo(tz, &ufo, 0, 0, 0, 0, 0, 0);

      if(ufo.uncompressed_size <= 0)
      {
        unzClose(tz);
        return false;
      }

      *size = ufo.uncompressed_size;
      *image = new uInt8[*size];

      // We don't have to check for any return errors from these functions,
      // since if there are, 'image' will not contain a valid ROM and the
      // calling method can take of it
      unzOpenCurrentFile(tz);
      unzReadCurrentFile(tz, *image, *size);
      unzCloseCurrentFile(tz);
      unzClose(tz);
    }
    else
    {
      unzClose(tz);
      return false;
    }
  }
  else
  {
    ifstream in(rom.c_str(), ios_base::binary);
    if(!in)
      return false;

    *image = new uInt8[512 * 1024];
    in.read((char*)(*image), 512 * 1024);
    *size = in.gcount();
    in.close();
  }

  return true;
}

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
