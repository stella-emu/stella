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
// Copyright (c) 1995-2005 by Bradford W. Mott
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: OSystem.cxx,v 1.17 2005-05-18 16:02:53 stephena Exp $
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

#include "Settings.hxx"
#include "PropsSet.hxx"
#include "EventHandler.hxx"
#include "Menu.hxx"
#include "Launcher.hxx"
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
    myLauncher(NULL),
    myRomFile(""),
    myFeatures("")
{
  // Create menu and launcher GUI objects
  myMenu = new Menu(this);
  myLauncher = new Launcher(this);

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
  delete myMenu;
  delete myLauncher;

  // Remove any game console that is currently attached
  delete myConsole;

  // OSystem takes responsibility for framebuffer and sound,
  // since it created them
  delete myFrameBuffer;
  delete mySound;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OSystem::setPropertiesFiles(const string& userprops,
                                 const string& systemprops)
{
  // Set up the input and output properties files
  myPropertiesOutputFile = userprops;

  if(fileExists(userprops))
    myPropertiesInputFile = userprops;
  else if(fileExists(systemprops))
    myPropertiesInputFile = systemprops;
  else
    myPropertiesInputFile = "";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OSystem::setConfigFiles(const string& userconfig,
                             const string& systemconfig)
{
  // Set up the names of the input and output config files
  myConfigOutputFile = userconfig;

  if(fileExists(userconfig))
    myConfigInputFile = userconfig;
  else if(fileExists(systemconfig))
    myConfigInputFile = systemconfig;
  else
    myConfigInputFile = "";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool OSystem::createFrameBuffer(bool showmessage)
{
  // Set the SDL_VIDEODRIVER environment variable, if possible
  string videodriver = mySettings->getString("video_driver");
  if(videodriver != "")
  {
    string buf = "SDL_VIDEODRIVER=" + videodriver;
    putenv((char*) buf.c_str());

    if(mySettings->getBool("showinfo"))
    {
      buf = "Video driver: " + videodriver;
      cout << buf << endl;
    }
  }

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
      break;  // S_EMULATE, S_MENU

    case EventHandler::S_LAUNCHER:
      myLauncher->initializeVideo();
      break;  // S_LAUNCHER

    case EventHandler::S_DEBUGGER:
      break;

    case EventHandler::S_NONE:
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

  // Re-initialize the framebuffer to current settings
  switch(myEventHandler->state())
  {
    case EventHandler::S_EMULATE:
    case EventHandler::S_MENU:
      myConsole->initializeAudio();
      break;  // S_EMULATE, S_MENU

    case EventHandler::S_LAUNCHER:
      break;  // S_LAUNCHER

    case EventHandler::S_DEBUGGER:
      break;

    case EventHandler::S_NONE:
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
  int size;
  if(openROM(myRomFile, &image, &size))
  {
    delete myConsole;  myConsole = NULL;

    // Create an instance of the 2600 game console
    // The Console c'tor takes care of updating the eventhandler state
    myConsole = new Console(image, size, this);

    // Free the image since we don't need it any longer
    delete[] image;

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

  return retval;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OSystem::createLauncher()
{
  setFramerate(20);  // We don't need a large framerate for the launcher
  myEventHandler->reset(EventHandler::S_LAUNCHER);

  // Create the window
  myLauncher->initializeVideo();

  // And start the base dialog
  myLauncher->initialize();
  myLauncher->reStack();
  myFrameBuffer->refresh();
  myFrameBuffer->setCursorState();
  mySound->mute(true);
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
