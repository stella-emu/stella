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
// $Id: OSystemUNIX.cxx,v 1.9 2005-05-25 17:17:38 stephena Exp $
//============================================================================

#include <cstdlib>
#include <sstream>
#include <fstream>

#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "bspf.hxx"
#include "OSystem.hxx"
#include "OSystemUNIX.hxx"

#ifdef HAVE_GETTIMEOFDAY
  #include <time.h>
  #include <sys/time.h>
#endif

/**
  Each derived class is responsible for calling the following methods
  in its constructor:

  setBaseDir()
  setStateDir()
  setPropertiesFiles()
  setConfigFiles()
  setCacheFile()

  See OSystem.hxx for a further explanation
*/

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
OSystemUNIX::OSystemUNIX()
{
  // First set variables that the OSystem needs
  string basedir = string(getenv("HOME")) + "/.stella";
  setBaseDir(basedir);

  string statedir = basedir + "/state";
  setStateDir(statedir);

  string userPropertiesFile   = basedir + "/stella.pro";
  string systemPropertiesFile = "/etc/stella.pro";
  setPropertiesFiles(userPropertiesFile, systemPropertiesFile);

  string userConfigFile   = basedir + "/stellarc";
  string systemConfigFile = "/etc/stellarc";
  setConfigFiles(userConfigFile, systemConfigFile);

  string cacheFile = basedir + "/stella.cache";
  setCacheFile(cacheFile);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
OSystemUNIX::~OSystemUNIX()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OSystemUNIX::mainLoop()
{
  // These variables are common to both timing options
  // and are needed to calculate the overall frames per second.
  uInt32 frameTime = 0, numberOfFrames = 0;

  if(mySettings->getBool("accurate"))   // normal, CPU-intensive timing
  {
    // Set up accurate timing stuff
    uInt32 startTime, delta;

    // Set the base for the timers
    frameTime = 0;

    // Main game loop
    for(;;)
    {
      // Exit if the user wants to quit
      if(myEventHandler->doQuit())
        break;

      startTime = getTicks();
      myEventHandler->poll();
      myFrameBuffer->update();

      // Now, waste time if we need to so that we are at the desired frame rate
      for(;;)
      {
        delta = getTicks() - startTime;

        if(delta >= myTimePerFrame)
          break;
      }

      frameTime += getTicks() - startTime;
      ++numberOfFrames;
    }
  }
  else    // less accurate, less CPU-intensive timing
  {
    // Set up less accurate timing stuff
    uInt32 startTime, virtualTime, currentTime;

    // Set the base for the timers
    virtualTime = getTicks();
    frameTime = 0;

    // Main game loop
    for(;;)
    {
      // Exit if the user wants to quit
      if(myEventHandler->doQuit())
        break;

      startTime = getTicks();
      myEventHandler->poll();
      myFrameBuffer->update();

      currentTime = getTicks();
      virtualTime += myTimePerFrame;
      if(currentTime < virtualTime)
      {
        SDL_Delay((virtualTime - currentTime)/1000);
      }

      currentTime = getTicks() - startTime;
      frameTime += currentTime;
      ++numberOfFrames;
    }
  }

  // Only print console information if a console was actually created
  if(mySettings->getBool("showinfo") && myConsole)
  {
    double executionTime = (double) frameTime / 1000000.0;
    double framesPerSecond = (double) numberOfFrames / executionTime;

    cout << endl;
    cout << numberOfFrames << " total frames drawn\n";
    cout << framesPerSecond << " frames/second\n";
    cout << endl;
    cout << "Cartridge Name: " << myConsole->properties().get("Cartridge.Name");
    cout << endl;
    cout << "Cartridge MD5:  " << myConsole->properties().get("Cartridge.MD5");
    cout << endl << endl;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 OSystemUNIX::getTicks()
{
#ifdef HAVE_GETTIMEOFDAY
  timeval now;
  gettimeofday(&now, 0);

  return (uInt32) (now.tv_sec * 1000000 + now.tv_usec);
#else
  return (uInt32) SDL_GetTicks() * 1000;
#endif
}
