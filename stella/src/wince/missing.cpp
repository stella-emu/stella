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
// Windows CE Port by Kostas Nakos
//============================================================================

#include "bspf.hxx"
#include "SDL.h"
//#include "gx.h"
#include "OSystemWinCE.hxx"
#include "FrameBufferWinCE.hxx"
#include "EventHandler.hxx"

char *msg = NULL;
int EventHandlerState;
extern OSystemWinCE *theOSystem;
extern int paddlespeed;

int time(int dummy)
{
	return GetTickCount();
}

char *getcwd(void)
{
	TCHAR fileUnc[MAX_PATH+1];
	static char cwd[MAX_PATH+1] = "";
	char *plast;
	
	GetModuleFileName(NULL, fileUnc, MAX_PATH);
	WideCharToMultiByte(CP_ACP, 0, fileUnc, -1, cwd, MAX_PATH, NULL, NULL);
	plast = strrchr(cwd, '\\');
	if(plast)
		*plast = 0;

	return cwd;
}


struct key2event
{
	UINT keycode;
	SDLKey sdlkey;
	uInt32 state;
	SDLKey launcherkey;
};
key2event keycodes[2][MAX_KEYS+NUM_MOUSEKEYS];

void KeySetup(void)
{
	GXKeyList klist = GXGetDefaultKeys(GX_NORMALKEYS);

	for (int i=0; i<2; i++)
	{
		for (int j=0; j<MAX_KEYS+NUM_MOUSEKEYS; j++,	keycodes[i][j].state = 0,
										keycodes[i][j].launcherkey = SDLK_UNKNOWN);

		keycodes[i][K_UP].keycode = klist.vkUp;
		keycodes[i][K_DOWN].keycode = klist.vkDown;
		keycodes[i][K_LEFT].keycode = klist.vkLeft;
		keycodes[i][K_RIGHT].keycode = klist.vkRight;
		keycodes[i][K_FIRE].keycode = klist.vkA;
		keycodes[i][K_RESET].keycode = klist.vkStart;
		keycodes[i][K_SELECT].keycode = klist.vkB;
		keycodes[i][K_QUIT].keycode = klist.vkC;

		keycodes[i][K_UP].sdlkey = SDLK_UP;
		keycodes[i][K_DOWN].sdlkey = SDLK_DOWN;
		keycodes[i][K_LEFT].sdlkey = SDLK_LEFT;
		keycodes[i][K_RIGHT].sdlkey = SDLK_RIGHT;
		keycodes[i][K_FIRE].sdlkey = SDLK_SPACE;
		keycodes[i][K_RESET].sdlkey = SDLK_F2;
		keycodes[i][K_SELECT].sdlkey = SDLK_F1;
		keycodes[i][K_QUIT].sdlkey = SDLK_ESCAPE;

		keycodes[i][K_UP].launcherkey = SDLK_UP;
		keycodes[i][K_DOWN].launcherkey = SDLK_DOWN;
		keycodes[i][K_LEFT].launcherkey = SDLK_PAGEUP;
		keycodes[i][K_RIGHT].launcherkey = SDLK_PAGEDOWN;
		keycodes[i][K_RESET].launcherkey = SDLK_RETURN;
		keycodes[i][K_QUIT].launcherkey = SDLK_ESCAPE;
	}
}

void KeySetMode(int mode)
{
	GXKeyList klist = GXGetDefaultKeys(GX_NORMALKEYS);

	for (int i=0; i<2; i++)
	{
		switch (mode)
		{
		case 0:
			keycodes[i][K_UP].keycode = klist.vkUp;
			keycodes[i][K_DOWN].keycode = klist.vkDown;
			keycodes[i][K_LEFT].keycode = klist.vkLeft;
			keycodes[i][K_RIGHT].keycode = klist.vkRight;
			break;

		case 2:
			keycodes[i][K_UP].keycode = klist.vkRight;
			keycodes[i][K_DOWN].keycode = klist.vkLeft;
			keycodes[i][K_LEFT].keycode = klist.vkUp;
			keycodes[i][K_RIGHT].keycode = klist.vkDown;
			break;

		case 1:
			keycodes[i][K_UP].keycode = klist.vkLeft;
			keycodes[i][K_DOWN].keycode = klist.vkRight;
			keycodes[i][K_LEFT].keycode = klist.vkDown;
			keycodes[i][K_RIGHT].keycode = klist.vkUp;
			break;

		}
	}
}

// SDL
DECLSPEC Uint32 SDLCALL SDL_WasInit(Uint32 flags) { return 0xFFFFFFFF; }
DECLSPEC int SDLCALL SDL_EnableUNICODE(int enable) { return 1; }
DECLSPEC int SDLCALL SDL_Init(Uint32 flags) { return 1; }
DECLSPEC int SDLCALL SDL_ShowCursor(int toggle) { return 1; }
DECLSPEC SDL_GrabMode SDLCALL SDL_WM_GrabInput(SDL_GrabMode mode) { return SDL_GRAB_ON; }
DECLSPEC void SDLCALL SDL_WM_SetCaption(const char *title, const char *icon) { return; }
DECLSPEC void SDLCALL SDL_FreeSurface(SDL_Surface *surface) { return; }
DECLSPEC void SDLCALL SDL_WM_SetIcon(SDL_Surface *icon, Uint8 *mask) { return; }
DECLSPEC SDL_Surface * SDLCALL SDL_CreateRGBSurfaceFrom(void *pixels,	int width, int height, int depth, int pitch, Uint32 Rmask, Uint32 Gmask, Uint32 Bmask, Uint32 Amask) { return NULL; }
DECLSPEC void SDLCALL SDL_WarpMouse(Uint16 x, Uint16 y) { return; }

DECLSPEC int SDLCALL SDL_PollEvent(SDL_Event *event)
{
	static int paddleres = 300000;

	for (int i=0; i<MAX_KEYS+NUM_MOUSEKEYS; i++)
	{
		if (keycodes[0][i].state != keycodes[1][i].state)
		{
			keycodes[1][i].state = keycodes[0][i].state;
			if (i!=K_QUIT || EventHandlerState!=2)
			{
				if (i < MAX_KEYS)
				{
					if (keycodes[1][i].state == 1)
						event->type = event->key.type = SDL_KEYDOWN;
					else
						event->type = event->key.type = SDL_KEYUP;

					if (EventHandlerState != 2)
						event->key.keysym.sym = keycodes[0][i].sdlkey;
					else
						event->key.keysym.sym = keycodes[0][i].launcherkey;
					event->key.keysym.mod = (SDLMod) 0;
					event->key.keysym.unicode = '\n';  // hack
				}
				else if (i == M_POS)
				{
					event->type = SDL_MOUSEMOTION;
					event->motion.x = LOWORD(keycodes[0][M_POS].state);
					event->motion.y = HIWORD(keycodes[0][M_POS].state);
				}
				else
				{
					if (keycodes[0][M_BUT].state & 0x80000000)
						event->type = event->button.type = SDL_MOUSEBUTTONDOWN;
					else
						event->type = SDL_MOUSEBUTTONUP;
					event->motion.x = LOWORD(keycodes[0][M_BUT].state);
					event->motion.y = HIWORD(keycodes[0][M_BUT].state & 0x7FFFFFFF);
					event->button.button = SDL_BUTTON_LEFT;

					if (event->type==SDL_MOUSEBUTTONDOWN && event->motion.x>220 && event->motion.y>300 && EventHandlerState!=2)
					{
						// bottom right corner for rotate
						KeySetMode( ((FrameBufferWinCE *) (&(theOSystem->frameBuffer())))->rotatedisplay() );
						event->type = SDL_NOEVENT;
					}
					else if (event->type==SDL_MOUSEBUTTONDOWN && event->motion.x<20 && event->motion.y>300 && EventHandlerState!=2)
					{
						// bottom left corner for launcher
						keycodes[0][K_QUIT].state = 1;
						event->type = SDL_NOEVENT;
					}
					else if (event->type==SDL_MOUSEBUTTONDOWN && event->motion.x<20 && event->motion.y<20 && EventHandlerState!=2 && EventHandlerState!=3)
					{
						// top left for menu
						theOSystem->eventHandler().enterMenuMode((enum EventHandler::State)3); //S_MENU
					}
				}
			}
			else if (keycodes[1][i].state == 1)
				event->type = SDL_QUIT;

			return 1;
		}

		if ( ((FrameBufferWinCE *) (&(theOSystem->frameBuffer())))->IsSmartphone() )
		{
			if (keycodes[0][K_RIGHT].state == 1 && paddleres > 200000)
				paddleres -= paddlespeed;
			else if (keycodes[0][K_LEFT].state == 1 && paddleres < 900000)
				paddleres += paddlespeed;

			theOSystem->eventHandler().event()->set(Event::PaddleZeroResistance, paddleres);
			theOSystem->eventHandler().event()->set(Event::PaddleZeroFire, keycodes[0][K_FIRE].state);
		}

	}
	event->type = SDL_NOEVENT;
	return 0;
}