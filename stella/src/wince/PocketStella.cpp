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
// Copyright (c) 1995-2007 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// Windows CE Port by Kostas Nakos
// $Id: PocketStella.cpp,v 1.8 2007-01-21 20:10:50 knakos Exp $
//============================================================================

#include <queue>
#include "FSNode.hxx"
#include "EventHandler.hxx"
#include "OSystemWinCE.hxx"
#include "SettingsWinCE.hxx"
#include "PropsSet.hxx"
#include "FrameBufferWinCE.hxx"

extern void KeySetup(void);
extern void KeySetMode(int);
extern queue <SDL_Event> eventqueue;
extern int EventHandlerState;

bool RequestRefresh = false;
SDLKey VK_keymap[SDLK_LAST];

OSystemWinCE* theOSystem = (OSystemWinCE*) NULL;
HWND hWnd;
uInt16 rotkeystate = 0;

DWORD REG_bat, REG_ac, REG_disp, bat_timeout;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	static PAINTSTRUCT ps;
	SDL_Event e;
	memset(&e, 0, sizeof(SDL_Event));

	switch (uMsg)
	{
	case WM_KEYDOWN:
		if (wParam == VK_F3)
		{
			if (rotkeystate == 0 && theOSystem)
				if (theOSystem->eventHandler().state() == 1)
					KeySetMode( ((FrameBufferWinCE *) (&(theOSystem->frameBuffer())))->rotatedisplay() );
			rotkeystate = 1;
		}
		else
			rotkeystate = 0;
		e.key.type = SDL_KEYDOWN;
		e.key.keysym.sym = VK_keymap[wParam];
		e.key.keysym.mod = (SDLMod) 0;
		eventqueue.push(e);
		return 0;

	case WM_KEYUP:
		e.key.type = SDL_KEYUP;
		e.key.keysym.sym = VK_keymap[wParam];
		e.key.keysym.mod = (SDLMod) 0;
		eventqueue.push(e);
		return 0;

	case WM_MOUSEMOVE:
		e.type = SDL_MOUSEMOTION;
		e.motion.x = LOWORD(lParam);
		e.motion.y = HIWORD(lParam);
		eventqueue.push(e);
		return 0;

	case WM_LBUTTONDOWN:
		e.type = SDL_MOUSEBUTTONDOWN;
		e.motion.x = LOWORD(lParam);
		e.motion.y = HIWORD(lParam);
		e.button.button = SDL_BUTTON_LEFT;
		eventqueue.push(e);
		return 0;

	case WM_LBUTTONUP:
		e.type = SDL_MOUSEBUTTONUP;
		e.motion.x = LOWORD(lParam);
		e.motion.y = HIWORD(lParam);
		e.button.button = SDL_BUTTON_LEFT;
		eventqueue.push(e);
		return 0;

	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;

	case WM_CLOSE:
		PostQuitMessage(0);
		return 0;

	case WM_SETFOCUS:
	case WM_ACTIVATE:
		GXResume();
		if (theOSystem)
			theOSystem->frameBuffer().refresh();
		return 0;

	case WM_KILLFOCUS:
	case WM_HIBERNATE:
		GXSuspend();
		if (theOSystem)
			if (((FrameBufferWinCE &)theOSystem->frameBuffer()).IsSmartphoneLowRes())
				theOSystem->eventHandler().handleEvent(Event::LauncherMode, theOSystem->eventHandler().state());
			else
				theOSystem->eventHandler().enterMenuMode(EventHandler::S_MENU);
		return 0;

	case WM_PAINT:
		BeginPaint(hWnd, &ps);
		EndPaint(hWnd, &ps);
		RequestRefresh = true;
		return 0;
	}

	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

static DWORD reg_access(TCHAR *key, TCHAR *val, DWORD data)
{
	HKEY regkey;
	DWORD tmpval, cbdata;

	if (RegOpenKeyEx(HKEY_CURRENT_USER, key, 0, 0, &regkey) != ERROR_SUCCESS)
		return data;

	cbdata = sizeof(DWORD);
	if (RegQueryValueEx(regkey, val, NULL, NULL, (LPBYTE) &tmpval, &cbdata) != ERROR_SUCCESS)
	{
		RegCloseKey(regkey);
		return data;
	}

	cbdata = sizeof(DWORD);
	if (RegSetValueEx(regkey, val, 0, REG_DWORD, (LPBYTE) &data, cbdata) != ERROR_SUCCESS)
	{
		RegCloseKey(regkey);
		return data;
	}

	RegCloseKey(regkey);
	return tmpval;
}

static void backlight_xchg(void)
{
	HANDLE h;

	REG_bat = reg_access(_T("ControlPanel\\BackLight"), _T("BatteryTimeout"), REG_bat);
	REG_ac = reg_access(_T("ControlPanel\\BackLight"), _T("ACTimeout"), REG_ac);
	REG_disp = reg_access(_T("ControlPanel\\Power"), _T("Display"), REG_disp);

	h = CreateEvent(NULL, FALSE, FALSE, _T("BackLightChangeEvent"));
	if (h)
	{
		SetEvent(h);
		CloseHandle(h);
	}
}

void CleanUp(void)
{
	GXCloseDisplay();
	GXCloseInput();
	if(theOSystem) delete theOSystem;
	backlight_xchg();
	SystemParametersInfo(SPI_SETBATTERYIDLETIMEOUT, bat_timeout, NULL, SPIF_SENDCHANGE);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nShowCmd )
{
	LPTSTR wndname = _T("PocketStella");
	WNDCLASS wc = { CS_HREDRAW | CS_VREDRAW, WindowProc, 0, 0, hInstance, NULL, NULL, 
		(HBRUSH)GetStockObject(BLACK_BRUSH), NULL, wndname};
	RegisterClass(&wc);
	hWnd = CreateWindow(wndname, wndname, WS_VISIBLE | WS_POPUP, 0, 0, GetSystemMetrics(SM_CXSCREEN),
						GetSystemMetrics(SM_CYSCREEN), NULL, NULL, hInstance, NULL);
	if (!hWnd) return 1;
	SetWindowPos(hWnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE);

	/* backlight */
	REG_bat = REG_ac = REG_disp = 2 * 60 * 60 * 1000; /* 2hrs should do it */
	backlight_xchg();
	SystemParametersInfo(SPI_GETBATTERYIDLETIMEOUT, 0, (void *) &bat_timeout, 0);
	SystemParametersInfo(SPI_SETBATTERYIDLETIMEOUT, 60 * 60 * 2, NULL, SPIF_SENDCHANGE);
	
	// pump the messages to get the window up
	MSG msg;
	while (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	LOGFONT f = {11, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
				 OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, _T("")};
	HFONT hFnt = CreateFontIndirect(&f);
	HDC hDC = GetDC(hWnd);
	SelectObject(hDC, hFnt);
	RECT RWnd;
	GetClipBox(hDC, &RWnd);
	SetTextColor(hDC, 0xA0A0A0);
	SetBkColor(hDC, 0);
	DrawText(hDC, _T("PocketStella Initializing..."), -1, &RWnd, DT_CENTER | DT_VCENTER);
	ReleaseDC(hWnd, hDC);
	DeleteObject(hFnt);
	
	theOSystem = new OSystemWinCE(((string) getcwd()) + '\\');
	SettingsWinCE theSettings(theOSystem);
	theOSystem->settings().loadConfig();
	theOSystem->settings().validate();
	bool loaddefaultkeys = (theOSystem->settings().getString("keymap") == EmptyString);
	theOSystem->create();
	if (loaddefaultkeys)
	{
		// setup the default keybindings the first time we're run
		theOSystem->eventHandler().addKeyMapping(Event::JoystickZeroFire, kEmulationMode, SDLK_F1);
		theOSystem->eventHandler().addKeyMapping(Event::LauncherMode, kEmulationMode, SDLK_BACKSPACE);
		theOSystem->eventHandler().addKeyMapping(Event::ConsoleReset, kEmulationMode, SDLK_F6);
		theOSystem->eventHandler().addKeyMapping(Event::ConsoleSelect, kEmulationMode, SDLK_F5);

		theOSystem->eventHandler().addKeyMapping(Event::UIPgUp, kMenuMode, SDLK_LEFT);
		theOSystem->eventHandler().addKeyMapping(Event::UIPgDown, kMenuMode, SDLK_RIGHT);
		theOSystem->eventHandler().addKeyMapping(Event::UISelect, kMenuMode, SDLK_F1);
		theOSystem->eventHandler().addKeyMapping(Event::UICancel, kMenuMode, SDLK_BACKSPACE);
	}

	if ( !GXOpenDisplay(hWnd, GX_FULLSCREEN) || !GXOpenInput() )
	{
		CleanUp();
		return 1;
	}
	KeySetup();

	string romfile = ((string) getcwd()) + ((string) "\\") + theSettings.getString("GameFilename");
	if (!FilesystemNode::fileExists(romfile))
		theOSystem->createLauncher();
	else
		theOSystem->createConsole(romfile);

	theOSystem->mainLoop();

	CleanUp();

	return 0;
}
