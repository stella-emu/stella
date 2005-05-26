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
// $Id: OSystemWin32.cxx,v 1.3 2005-05-26 18:56:58 stephena Exp $
//============================================================================

#include <sstream>
#include <fstream>
#include <direct.h>

#include "bspf.hxx"
#include "OSystem.hxx"
#include "OSystemWin32.hxx"

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
OSystemWin32::OSystemWin32()
{
  // TODO - there really should be code here to determine which version
  // of Windows is being used.
  // If using a version which supports multiple users (NT and above),
  // the relevant directories should be created in per-user locations.
  // For now, we just put it in the same directory as the executable.

  string basedir = ".\\";
  setBaseDir(basedir);

  string stateDir = basedir + "state\\";
  setStateDir(stateDir);

  string propsFile = basedir + "stella.pro";
  setPropertiesFiles(propsFile, propsFile);  // Input and output are the same

  string configFile = basedir + "stella.ini";
  setConfigFiles(configFile, configFile);  // Input and output are the same

  string cacheFile = basedir + "stella.cache";
  setCacheFile(cacheFile);

  myDriverList.push_back("windib");
  myDriverList.push_back("directx");
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
uInt32 OSystemWin32::getTicks()
{
  return (uInt32) SDL_GetTicks() * 1000;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool OSystemWin32::fileExists(const string& filename)
{
  // FIXME - Since I don't have time to figure out the correct
  // and fast 'Win32' way of doing this, I'll cheat a bit
  ifstream in(filename.c_str());
  if(in)
  {
    in.close();
    return true;
  }

  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool OSystemWin32::makeDir(const string& path)
{
  // FIXME
  _mkdir(path.c_str());
  return true;
}
