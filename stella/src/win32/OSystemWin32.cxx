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
// Copyright (c) 1995-2006 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: OSystemWin32.cxx,v 1.14 2006-12-08 16:49:41 stephena Exp $
//============================================================================

#include <sstream>
#include <fstream>
#include <windows.h>

#include "bspf.hxx"
#include "OSystem.hxx"
#include "OSystemWin32.hxx"

/**
  Each derived class is responsible for calling the following methods
  in its constructor:

  setBaseDir()
  setStateDir()
  setPropertiesDir()
  setConfigFile()
  setCacheFile()

  See OSystem.hxx for a further explanation
*/

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
OSystemWin32::OSystemWin32(const string& path) : OSystem()
{
  // TODO - there really should be code here to determine which version
  // of Windows is being used.
  // If using a version which supports multiple users (NT and above),
  // the relevant directories should be created in per-user locations.
  // For now, we just put it in the same directory as the executable.
  const string& basedir = (path.length() > 0) ? path : ".";

  setBaseDir(basedir);

  setStateDir(basedir + "\\state\\");

  setPropertiesDir(basedir);
  setConfigFile(basedir + "\\stella.ini");

  setCacheFile(basedir + "\\stella.cache");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
OSystemWin32::~OSystemWin32()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OSystemWin32::mainLoop()
{
  // These variables are common to both timing options
  // and are needed to calculate the overall frames per second.
  uInt32 frameTime = 0, numberOfFrames = 0;

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
	myEventHandler->poll(startTime);
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

  // Only print console information if a console was actually created
  if(mySettings->getBool("showinfo") && myConsole)
  {
    double executionTime = (double) frameTime / 1000000.0;
    double framesPerSecond = (double) numberOfFrames / executionTime;

    cout << endl;
    cout << numberOfFrames << " total frames drawn\n";
    cout << framesPerSecond << " frames/second\n";
    cout << endl;
    cout << "Cartridge Name: " << myConsole->properties().get(Cartridge_Name);
    cout << endl;
    cout << "Cartridge MD5:  " << myConsole->properties().get(Cartridge_MD5);
    cout << endl << endl;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 OSystemWin32::getTicks()
{
  return (uInt32) SDL_GetTicks() * 1000;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OSystemWin32::getScreenDimensions(int& width, int& height)
{
  width  = (int) GetSystemMetrics(SM_CXSCREEN);
  height = (int) GetSystemMetrics(SM_CYSCREEN);
}
