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
// $Id: OSystemPSP.cxx,v 1.3 2005-09-18 14:31:36 optixx Exp $
//============================================================================

#include <cstdlib>
#include <sstream>
#include <fstream>

#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <pspkernel.h>
#include <psppower.h>

#include "bspf.hxx"
#include "OSystem.hxx"
#include "OSystemPSP.hxx"

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

  And for initializing the following variables:

  myDriverList (a StringList)

  See OSystem.hxx for a further explanation
*/

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
OSystemPSP::OSystemPSP()
{
  // First set variables that the OSystem needs
  string basedir = string("ms0:/stella");
  setBaseDir(basedir);

  string statedir = basedir + "/state";
  setStateDir(statedir);

  string userPropertiesFile   = basedir + "/stella.pro";
  string systemPropertiesFile = "/etc/stella.pro";
  setConfigFiles(userPropertiesFile, systemPropertiesFile);

  string userConfigFile   = basedir + "/stellarc";
  string systemConfigFile = "/etc/stellarc";
  setConfigFiles(userConfigFile, systemConfigFile);

  string cacheFile = basedir + "/stella.cache";
  setCacheFile(cacheFile);

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
OSystemPSP::~OSystemPSP()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OSystemPSP::mainLoop()
{
  // These variables are common to both timing options
  // and are needed to calculate the overall frames per second.
  uInt32 frameTime = 0, numberOfFrames = 0;

  // Set up less accurate timing stuff
  uInt32 startTime, virtualTime, currentTime;

  // Set the base for the timers
  virtualTime = getTicks();
  frameTime = 0;

  // Overclock CPU to 333MHz
  if (settings().getBool("pspoverclock"))
  {
    scePowerSetClockFrequency(333,333,166);
    fprintf(stderr,"OSystemPSP::mainLoop overclock to 333\n");
  } else
  {
    fprintf(stderr,"OSystemPSP::mainLoop NOT overclock\n");
  }
  // Main game loop
  for(;;)
  {
    // Exit if the user wants to quit
    if(myEventHandler->doQuit())
      break;

    startTime = getTicks();
    myEventHandler->poll(startTime);
    myFrameBuffer->update();
    currentTime = getTicks();
    virtualTime += myTimePerFrame;
    if(currentTime < virtualTime)
      SDL_Delay((virtualTime - currentTime)/1000);

    currentTime = getTicks() - startTime;
    frameTime += currentTime;
    ++numberOfFrames;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 OSystemPSP::getTicks()
{
#if defined(HAVE_GETTIMEOFDAY)
  timeval now;
  gettimeofday(&now, 0);
  return (uInt32) (now.tv_sec * 1000000 + now.tv_usec);
#else
  return (uInt32) SDL_GetTicks() * 1000;
#endif
}

