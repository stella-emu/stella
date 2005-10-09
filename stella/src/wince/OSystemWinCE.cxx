#include <sstream>
#include <fstream>

#include "bspf.hxx"
#include "OSystem.hxx"
#include "OSystemWinCE.hxx"
#include "SoundWinCE.hxx"
//#include <sstream>
extern void KeyCheck(void);
int EventHandlerState;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
OSystemWinCE::OSystemWinCE()
{
  string basedir = ((string) getcwd()) + '\\';
  setBaseDir(basedir);

  string stateDir = basedir + "state\\";
  setStateDir(stateDir);

  setPropertiesDir(basedir, basedir);

  string configFile = basedir + "stella.ini";
  setConfigFiles(configFile, configFile);  // Input and output are the same

  string cacheFile = basedir + "stella.cache";
  setCacheFile(cacheFile);
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

  // These variables are common to both timing options
  // and are needed to calculate the overall frames per second.
  uInt32 frameTime = 0, numberOfFrames = 0;

  // Set up less accurate timing stuff
  uInt32 startTime, virtualTime, currentTime;

  // Set the base for the timers
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

	if(myEventHandler->doQuit())
	  break;

	KeyCheck();

	startTime = GetTickCount();

	EventHandlerState = (int) myEventHandler->state();
	myEventHandler->poll(startTime);
	myFrameBuffer->update();
	((SoundWinCE *)mySound)->update();

	currentTime = GetTickCount();
	virtualTime += myTimePerFrame;
	if(currentTime < virtualTime)
	  Sleep(virtualTime - currentTime);

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
