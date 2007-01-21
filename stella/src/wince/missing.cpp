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
// $Id: missing.cpp,v 1.8 2007-01-21 20:10:50 knakos Exp $
//============================================================================

#include <queue>
#include "bspf.hxx"
#include "SDL.h"
#include "OSystemWinCE.hxx"
#include "FrameBufferWinCE.hxx"
#include "EventHandler.hxx"

char *msg = NULL;
int EventHandlerState;
extern OSystemWinCE *theOSystem;
extern SDLKey VK_keymap[SDLK_LAST];

queue <SDL_Event> eventqueue;

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

void KeySetMode(int mode)
{
/*	GXKeyList klist = GXGetDefaultKeys(GX_NORMALKEYS);

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
	}*/
}

void KeySetup(void)
{
	// Map the VK keysyms
	// stolen from SDL_dibevents.c :-)
	for (int i=0; i<SDLK_LAST; ++i )
		VK_keymap[i] = SDLK_UNKNOWN;

	VK_keymap[VK_BACK] = SDLK_BACKSPACE;
	VK_keymap[VK_TAB] = SDLK_TAB;
	VK_keymap[VK_CLEAR] = SDLK_CLEAR;
	VK_keymap[VK_RETURN] = SDLK_RETURN;
	VK_keymap[VK_PAUSE] = SDLK_PAUSE;
	VK_keymap[VK_ESCAPE] = SDLK_ESCAPE;
	VK_keymap[VK_SPACE] = SDLK_SPACE;
	VK_keymap[VK_APOSTROPHE] = SDLK_QUOTE;
	VK_keymap[VK_COMMA] = SDLK_COMMA;
	VK_keymap[VK_PERIOD] = SDLK_PERIOD;
	VK_keymap[VK_SLASH] = SDLK_SLASH;
	VK_keymap['0'] = SDLK_0;
	VK_keymap['1'] = SDLK_1;
	VK_keymap['2'] = SDLK_2;
	VK_keymap['3'] = SDLK_3;
	VK_keymap['4'] = SDLK_4;
	VK_keymap['5'] = SDLK_5;
	VK_keymap['6'] = SDLK_6;
	VK_keymap['7'] = SDLK_7;
	VK_keymap['8'] = SDLK_8;
	VK_keymap['9'] = SDLK_9;
	VK_keymap[VK_SEMICOLON] = SDLK_SEMICOLON;
	VK_keymap[VK_EQUAL] = SDLK_EQUALS;
	VK_keymap[VK_LBRACKET] = SDLK_LEFTBRACKET;
	VK_keymap[VK_BACKSLASH] = SDLK_BACKSLASH;
	VK_keymap[VK_RBRACKET] = SDLK_RIGHTBRACKET;
	VK_keymap[VK_BACKQUOTE] = SDLK_BACKQUOTE;
	VK_keymap['A'] = SDLK_a;
	VK_keymap['B'] = SDLK_b;
	VK_keymap['C'] = SDLK_c;
	VK_keymap['D'] = SDLK_d;
	VK_keymap['E'] = SDLK_e;
	VK_keymap['F'] = SDLK_f;
	VK_keymap['G'] = SDLK_g;
	VK_keymap['H'] = SDLK_h;
	VK_keymap['I'] = SDLK_i;
	VK_keymap['J'] = SDLK_j;
	VK_keymap['K'] = SDLK_k;
	VK_keymap['L'] = SDLK_l;
	VK_keymap['M'] = SDLK_m;
	VK_keymap['N'] = SDLK_n;
	VK_keymap['O'] = SDLK_o;
	VK_keymap['P'] = SDLK_p;
	VK_keymap['Q'] = SDLK_q;
	VK_keymap['R'] = SDLK_r;
	VK_keymap['S'] = SDLK_s;
	VK_keymap['T'] = SDLK_t;
	VK_keymap['U'] = SDLK_u;
	VK_keymap['V'] = SDLK_v;
	VK_keymap['W'] = SDLK_w;
	VK_keymap['X'] = SDLK_x;
	VK_keymap['Y'] = SDLK_y;
	VK_keymap['Z'] = SDLK_z;
	VK_keymap[VK_DELETE] = SDLK_DELETE;

	VK_keymap[VK_NUMPAD0] = SDLK_KP0;
	VK_keymap[VK_NUMPAD1] = SDLK_KP1;
	VK_keymap[VK_NUMPAD2] = SDLK_KP2;
	VK_keymap[VK_NUMPAD3] = SDLK_KP3;
	VK_keymap[VK_NUMPAD4] = SDLK_KP4;
	VK_keymap[VK_NUMPAD5] = SDLK_KP5;
	VK_keymap[VK_NUMPAD6] = SDLK_KP6;
	VK_keymap[VK_NUMPAD7] = SDLK_KP7;
	VK_keymap[VK_NUMPAD8] = SDLK_KP8;
	VK_keymap[VK_NUMPAD9] = SDLK_KP9;
	VK_keymap[VK_DECIMAL] = SDLK_KP_PERIOD;
	VK_keymap[VK_DIVIDE] = SDLK_KP_DIVIDE;
	VK_keymap[VK_MULTIPLY] = SDLK_KP_MULTIPLY;
	VK_keymap[VK_SUBTRACT] = SDLK_KP_MINUS;
	VK_keymap[VK_ADD] = SDLK_KP_PLUS;

	VK_keymap[VK_UP] = SDLK_UP;
	VK_keymap[VK_DOWN] = SDLK_DOWN;
	VK_keymap[VK_RIGHT] = SDLK_RIGHT;
	VK_keymap[VK_LEFT] = SDLK_LEFT;
	VK_keymap[VK_INSERT] = SDLK_INSERT;
	VK_keymap[VK_HOME] = SDLK_HOME;
	VK_keymap[VK_END] = SDLK_END;
	VK_keymap[VK_PRIOR] = SDLK_PAGEUP;
	VK_keymap[VK_NEXT] = SDLK_PAGEDOWN;

	VK_keymap[VK_F1] = SDLK_F1;
	VK_keymap[VK_F2] = SDLK_F2;
	VK_keymap[VK_F3] = SDLK_F3;
	VK_keymap[VK_F4] = SDLK_F4;
	VK_keymap[VK_F5] = SDLK_F5;
	VK_keymap[VK_F6] = SDLK_F6;
	VK_keymap[VK_F7] = SDLK_F7;
	VK_keymap[VK_F8] = SDLK_F8;
	VK_keymap[VK_F9] = SDLK_F9;
	VK_keymap[VK_F10] = SDLK_F10;
	VK_keymap[VK_F11] = SDLK_F11;
	VK_keymap[VK_F12] = SDLK_F12;
	VK_keymap[VK_F13] = SDLK_F13;
	VK_keymap[VK_F14] = SDLK_F14;
	VK_keymap[VK_F15] = SDLK_F15;

	VK_keymap[VK_NUMLOCK] = SDLK_NUMLOCK;
	VK_keymap[VK_CAPITAL] = SDLK_CAPSLOCK;
	VK_keymap[VK_SCROLL] = SDLK_SCROLLOCK;
	VK_keymap[VK_RSHIFT] = SDLK_RSHIFT;
	VK_keymap[VK_LSHIFT] = SDLK_LSHIFT;
	VK_keymap[VK_RCONTROL] = SDLK_RCTRL;
	VK_keymap[VK_LCONTROL] = SDLK_LCTRL;
	VK_keymap[VK_RMENU] = SDLK_RALT;
	VK_keymap[VK_LMENU] = SDLK_LALT;
	VK_keymap[VK_RWIN] = SDLK_RSUPER;
	VK_keymap[VK_LWIN] = SDLK_LSUPER;

	VK_keymap[VK_HELP] = SDLK_HELP;
	VK_keymap[VK_PRINT] = SDLK_PRINT;
	VK_keymap[VK_SNAPSHOT] = SDLK_PRINT;
	VK_keymap[VK_CANCEL] = SDLK_BREAK;
	VK_keymap[VK_APPS] = SDLK_MENU;

	// fix wince keys
	GXKeyList klist = GXGetDefaultKeys(GX_NORMALKEYS);

	VK_keymap[klist.vkUp] = SDLK_UP;
	VK_keymap[klist.vkDown] = SDLK_DOWN;
	VK_keymap[klist.vkLeft] = SDLK_LEFT;
	VK_keymap[klist.vkRight] = SDLK_RIGHT;
	VK_keymap[klist.vkA] = SDLK_F1;
	VK_keymap[klist.vkB] = SDLK_TAB;	// harwire softkey2 to tab
	// VK_F3 => SDLK_F3 is call button, VK_F4 => SDLK_F4 is end call button
	VK_keymap[klist.vkC] = SDLK_F5;
	VK_keymap[klist.vkStart] = SDLK_F6;
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
DECLSPEC void SDL_Delay(Uint32 ms) { Sleep(ms); }


DECLSPEC int SDLCALL SDL_PollEvent(SDL_Event *event)
{
	while (!eventqueue.empty())
	{
		memcpy(event, &(eventqueue.front()), sizeof(SDL_Event));
		eventqueue.pop();
		return 1;
	}
	event->type = SDL_NOEVENT;
	return 0;
}