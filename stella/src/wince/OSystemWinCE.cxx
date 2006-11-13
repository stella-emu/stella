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
// Windows CE Port by Kostas Nakos
//============================================================================

#include <sstream>
#include <fstream>

#include "bspf.hxx"
#include "OSystem.hxx"
#include "OSystemWinCE.hxx"
#include "SoundWinCE.hxx"
#include "FrameBufferWinCE.hxx"

extern void KeyCheck(void);
extern int EventHandlerState;
extern void KeySetMode(int);
extern bool RequestRefresh;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
OSystemWinCE::OSystemWinCE(const string& path) : OSystem()
{
  string basedir = ((string) getcwd()) + '\\';
  setBaseDir(basedir);

  setStateDir(basedir);

  setPropertiesDir(basedir);
  setConfigFile(basedir + "stella.ini");

  setCacheFile(basedir + "stella.cache");
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
  uInt8  lastkeyset;

  virtualTime = GetTickCount();
  frameTime = 0;

  // Main game loop
  MSG msg;
  int laststate = -1;
  if (!((FrameBufferWinCE *)myFrameBuffer)->IsSmartphoneLowRes())
  {
	  lastkeyset = 0;
	  KeySetMode(1);
  }
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

	if(myEventHandler->doQuit())
	  break;

	KeyCheck();

	if (RequestRefresh)
	{
		RequestRefresh = false;
		myEventHandler->refreshDisplay();
	}

	startTime = GetTickCount();

	EventHandlerState = (int) myEventHandler->state();
	if ((laststate != -1) && (laststate != EventHandlerState))
		if (EventHandlerState!=2 && EventHandlerState!=3)
		{
			((FrameBufferWinCE *)myFrameBuffer)->wipescreen();
			KeySetMode(lastkeyset);
		}
		else
		{
			if ( ((FrameBufferWinCE *)myFrameBuffer)->IsSmartphoneLowRes() )
			{
				KeySetMode(0);
				((FrameBufferWinCE *)myFrameBuffer)->setmode(0);
			}
			else
			{
				lastkeyset = ((FrameBufferWinCE *)myFrameBuffer)->getmode();
				if (lastkeyset == 0)
					KeySetMode(1);
			}
		}
	laststate = EventHandlerState;

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

  /* {
    double executionTime = (double) frameTime / 1000000.0;
    double framesPerSecond = (double) numberOfFrames / executionTime;
	ostringstream a;
    a << endl;
    a << numberOfFrames << " total frames drawn\n";
    a << framesPerSecond << " frames/second\n";
    a << endl;
  TCHAR unicodeString[MAX_PATH];
  MultiByteToWideChar(CP_ACP, 0, a.str().c_str(), strlen(a.str().c_str()) + 1, unicodeString, sizeof(unicodeString));
	MessageBox(GetDesktopWindow(),unicodeString, _T("..."),0);
   }
*/
}

uInt32 OSystemWinCE::getTicks(void)
{
	return GetTickCount();
}

inline const GUI::Font& OSystemWinCE::launcherFont() const
{
	if ( ((FrameBufferWinCE *)myFrameBuffer)->IsSmartphoneLowRes() )
		return consoleFont();
	else
		return OSystem::font();
}
