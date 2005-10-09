#include <windows.h>
#include <gx.h>
#include "EventHandler.hxx"
#include "OSystemWinCE.hxx"
#include "SettingsWinCE.hxx"
#include "PropsSet.hxx"
#include "FSNode.hxx"

#define KEYSCHECK_ASYNC

struct key2event
{
	UINT keycode;
	SDLKey sdlkey;
	uInt8 state;
	SDLKey launcherkey;
};
extern key2event keycodes[2][MAX_KEYS];
extern void KeySetup(void);

OSystemWinCE* theOSystem = (OSystemWinCE*) NULL;
HWND hWnd;

void KeyCheck(void)
{
#ifdef KEYSCHECK_ASYNC
	if (GetAsyncKeyState(keycodes[0][K_QUIT].keycode) & 0x8000)
	{
		PostQuitMessage(0);
		return;
	}
	for (int i=0; i<MAX_KEYS-1; i++)
		keycodes[0][i].state = ((uInt16) GetAsyncKeyState(keycodes[0][i].keycode)) >> 15;
#endif
}


void CleanUp(void)
{
	if(theOSystem) delete theOSystem;
	GXCloseDisplay();
	GXCloseInput();
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	static PAINTSTRUCT ps;

	switch (uMsg)
	{
	case WM_KEYDOWN:
#ifndef KEYSCHECK_ASYNC
		uInt8 i;
		if (wParam == keycodes[K_QUIT].keycode)
			PostQuitMessage(0);
		else
			for (i=0; i<MAX_KEYS-1; i++)
				if (wParam == keycodes[i].keycode)
				{
					theConsole->eventHandler().event()->set(keycodes[i].event,1);
					break;
				}
#endif
		return 0;

	case WM_KEYUP:
#ifndef KEYSCHECK_ASYNC
		for (i=0; i<MAX_KEYS-1; i++)
			if (wParam == keycodes[i].keycode)
			{
				theConsole->eventHandler().event()->set(keycodes[i].event,0);
				break;
			}
#endif
		return 0;

	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;

	case WM_CLOSE:
		PostQuitMessage(0);
		return 0;

	case WM_SETFOCUS:
	case WM_ACTIVATE:
		GXSuspend();
		return 0;

	case WM_KILLFOCUS:
	case WM_HIBERNATE:
		GXResume();
		return 0;

	case WM_PAINT:
		BeginPaint(hWnd, &ps);
		EndPaint(hWnd, &ps);
		return 0;

	}
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nShowCmd )
{
	LPTSTR wndname = _T("PocketStella");
	WNDCLASS wc = { CS_HREDRAW | CS_VREDRAW, WindowProc, 0, 0, hInstance, NULL, NULL, 
		(HBRUSH)GetStockObject(BLACK_BRUSH), NULL, wndname};
	RegisterClass(&wc);
	hWnd = CreateWindow(wndname, wndname, WS_VISIBLE, 0, 0, GetSystemMetrics(SM_CXSCREEN),
						GetSystemMetrics(SM_CYSCREEN), NULL, NULL, hInstance, NULL);
	if (!hWnd) return 1;
	SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE);
	
	MSG msg;
	while (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	theOSystem = new OSystemWinCE();
	SettingsWinCE theSettings(theOSystem);
	theOSystem->settings().loadConfig();
	theOSystem->settings().validate();
	EventHandler theEventHandler(theOSystem);
	PropertiesSet propertiesSet;
	propertiesSet.load(theOSystem->systemProperties(), false);
	theOSystem->attach(&propertiesSet);

	if ( !GXOpenDisplay(hWnd, GX_FULLSCREEN) || !GXOpenInput() )
	{
		CleanUp();
		return 1;
	}
	KeySetup();

	if(!theOSystem->createFrameBuffer())
	{
		CleanUp();
		return 1;
	}
	theOSystem->createSound();

	string romfile = ((string) getcwd()) + ((string) "\\") + theSettings.getString("GameFilename");
	if (!FilesystemNode::fileExists(romfile))
		theOSystem->createLauncher();
	else
	{
	/*TCHAR tmp[200];
	MultiByteToWideChar(CP_ACP, 0, romfile.c_str(), strlen(romfile.c_str()) + 1, tmp, 200);
	MessageBox(hWnd, tmp, _T("..."),0);*/
		theOSystem->createConsole(romfile);
	}

	theOSystem->mainLoop();

	CleanUp();

	return 0;
}
