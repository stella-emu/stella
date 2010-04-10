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
// Copyright (c) 1995-2010 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// Windows CE Port by Kostas Nakos
// $Id$
//============================================================================

#include <sstream>
#include <fstream>

#include "bspf.hxx"
#include "OSystem.hxx"
#include "OSystemWinCE.hxx"
#include "SoundWinCE.hxx"
#include "FrameBufferWinCE.hxx"

extern bool RequestRefresh;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
OSystemWinCE::OSystemWinCE(const string& path) : OSystem()
{
  setBaseDir(path);
  setStateDir(path);
  setPropertiesDir(path);
  setConfigFile(path + "stella.ini");
  setCacheFile(path + "stella.cache");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
OSystemWinCE::~OSystemWinCE()
{
}

void OSystemWinCE::setFramerate(uInt32 framerate)
{
	myDisplayFrameRate = framerate;
	myTimePerFrame = (uInt32)(1000.0 / (double)myDisplayFrameRate);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OSystemWinCE::mainLoop()
{

  uInt32 frameTime = 0, numberOfFrames = 0;
  uInt32 startTime, virtualTime, currentTime;

  virtualTime = GetTickCount();
  frameTime = 0;

  // Main game loop
  MSG msg;

  for(;;)
  {
	while (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
	{
		if (msg.message == WM_QUIT)
			break;
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	if (msg.message == WM_QUIT)
		break;

	if (myQuitLoop)
	  break;

	if (RequestRefresh)
	{
		RequestRefresh = false;
		myEventHandler->refreshDisplay();
	}

	startTime = GetTickCount();

	myEventHandler->poll(startTime);
	myFrameBuffer->update();
	((SoundWinCE *)mySound)->update();

	currentTime = GetTickCount();
	virtualTime += myTimePerFrame;
	if(currentTime < virtualTime)
	  Sleep(virtualTime - currentTime);
	else
		virtualTime = currentTime;

	currentTime = GetTickCount() - startTime;
	frameTime += currentTime;
	++numberOfFrames;
  }
}

uInt32 OSystemWinCE::getTicks(void) const
{
	return GetTickCount();
}

void OSystemWinCE::getScreenDimensions(int& width, int& height)
{
	// do not use the framebuffer issmartphonelowres method as the framebuffer has not been created yet
	if ((unsigned int) GetSystemMetrics(SM_CXSCREEN) != 176 )
	{
		if (GetSystemMetrics(SM_CXSCREEN) != GetSystemMetrics(SM_CYSCREEN) )
		{
			width = 320;
			height = 240;
		}
		else
		{
			width = height = 240;
		}
	}
	else
	{
		width = 220;
		height = 176;
	}
}
