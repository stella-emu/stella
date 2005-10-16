#include "bspf.hxx"
#include "SDL.h"
#include "gx.h"

char *msg = NULL;
int EventHandlerState;

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
	uInt8 state;
	SDLKey launcherkey;
};
key2event keycodes[2][MAX_KEYS];

void KeySetup(void)
{
	GXKeyList klist = GXGetDefaultKeys(GX_NORMALKEYS);

	for (int i=0; i<2; i++)
	{
		for (int j=0; j<MAX_KEYS; j++,	keycodes[i][j].state = 0,
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
	for (int i=0; i<MAX_KEYS; i++)
	{
		if (keycodes[0][i].state != keycodes[1][i].state)
		{
			keycodes[1][i].state = keycodes[0][i].state;
			if (i!=K_QUIT || EventHandlerState!=2)
			{
				if (keycodes[1][i].state == 1)
					event->type = event->key.type = SDL_KEYDOWN;
				else
					event->type = event->key.type = SDL_KEYUP;
			}
			else if (keycodes[1][i].state == 1)
				event->type = SDL_QUIT;
			if (EventHandlerState != 2)
				event->key.keysym.sym = keycodes[0][i].sdlkey;
			else
				event->key.keysym.sym = keycodes[0][i].launcherkey;
			event->key.keysym.mod = (SDLMod) 0;
			event->key.keysym.unicode = '\n';  // hack
			return 1;
		}
	}
	event->type = SDL_NOEVENT;
	return 0;
}