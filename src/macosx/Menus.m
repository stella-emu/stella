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
// $Id$
//============================================================================

#import <Cocoa/Cocoa.h>
#import "SDL.h"
#import "Menus.h"
#import "MenusEvents.h"

#define QZ_m      0x2E
#define QZ_o      0x1F
#define QZ_h      0x04
#define QZ_SLASH  0x2C

extern void macOSXSendMenuEvent(int event);

/*------------------------------------------------------------------------------
*  releaseCmdKeys - This method fixes an issue when modal windows are used with
*     the Mac OSX version of the SDL library.
*     As the SDL normally captures all keystrokes, but we need to type in some 
*     Mac windows, all of the control menu windows run in modal mode.  However, 
*     when this happens, the release of the command key and the shortcut key 
*     are not sent to SDL.  We have to manually cause these events to happen 
*     to keep the SDL library in a sane state, otherwise only everyother shortcut
*     keypress will work.
*-----------------------------------------------------------------------------*/
void releaseCmdKeys(NSString *character, int keyCode)
{
  NSEvent *event1, *event2;
  NSPoint point;
    
  event1 = [NSEvent keyEventWithType:NSKeyUp location:point modifierFlags:0
              timestamp:0.0 windowNumber:0 context:nil characters:character
              charactersIgnoringModifiers:character isARepeat:NO keyCode:keyCode];
  [NSApp postEvent:event1 atStart:NO];
    
  event2 = [NSEvent keyEventWithType:NSFlagsChanged location:point modifierFlags:0
              timestamp:0.0 windowNumber:0 context:nil characters:nil
              charactersIgnoringModifiers:nil isARepeat:NO keyCode:0];
  [NSApp postEvent:event2 atStart:NO];
}

void hideApp(void)
{
  [NSApp hide:nil];
  releaseCmdKeys(@"h",QZ_h);
}

void showHelp(void)
{
  [NSApp showHelp:nil];
  releaseCmdKeys(@"?",QZ_SLASH);
}

void miniturizeWindow(void)
{
  [[NSApp keyWindow] performMiniaturize:nil];
  releaseCmdKeys(@"m",QZ_m);
}

void handleMacOSXKeypress(int key)
{
  switch(key)
  {
    case SDLK_h:
      hideApp();
      break;
    case SDLK_m:
      miniturizeWindow();
      break;
    case SDLK_SLASH:
      showHelp();
      break;
  }
}

void setEmulationMenus(void)
{
  [[Menus sharedInstance] setEmulationMenus];
}

void setLauncherMenus(void)
{
  [[Menus sharedInstance] setLauncherMenus];
}

void setOptionsMenus(void)
{
  [[Menus sharedInstance] setOptionsMenus];
}

void setCommandMenus(void)
{
  [[Menus sharedInstance] setCommandMenus];
}

void setDebuggerMenus(void)
{
  [[Menus sharedInstance] setDebuggerMenus];
}


@implementation Menus

static Menus *sharedInstance = nil;

+ (Menus *)sharedInstance
{
  return sharedInstance ? sharedInstance : [[self alloc] init];
}

- (id)init
{
	sharedInstance = self;
	return(self);
}

-(void)pushKeyEvent:(int)key:(bool)shift:(bool)cmd
{
	SDL_Event theEvent;

	theEvent.key.type = SDL_KEYDOWN;
	theEvent.key.state = SDL_PRESSED;
	theEvent.key.keysym.scancode = 0;
	theEvent.key.keysym.sym = key;
	theEvent.key.keysym.mod = 0;
	if (cmd)
		theEvent.key.keysym.mod = KMOD_LMETA;
	if (shift)
		theEvent.key.keysym.mod |= KMOD_LSHIFT;
	theEvent.key.keysym.unicode = 0;
	SDL_PushEvent(&theEvent);
}

- (IBAction) paddleChange:(id) sender
{
  switch([sender tag])
  {
    case 0:
      [self pushKeyEvent:SDLK_0:NO:YES];
      break;
    case 1:
      [self pushKeyEvent:SDLK_1:NO:YES];
      break;
    case 2:
      [self pushKeyEvent:SDLK_2:NO:YES];
      break;
    case 3:
      [self pushKeyEvent:SDLK_3:NO:YES];
      break;
  }
}

- (IBAction)biggerScreen:(id)sender
{
  [self pushKeyEvent:SDLK_EQUALS:NO:YES];
}

- (IBAction)smallerScreen:(id)sender
{
  [self pushKeyEvent:SDLK_MINUS:NO:YES];
}

- (IBAction)fullScreen:(id)sender
{
  [self pushKeyEvent:SDLK_RETURN:NO:YES];
}

- (IBAction)openCart:(id)sender
{
  [self pushKeyEvent:SDLK_ESCAPE:NO:NO];
//  Fixme - This should work like the other keys, but instead
//   if you send the LauncherOpen event, it crashes SDL in
//    the poll loop.    
//    macOSXSendMenuEvent(MENU_OPEN);
}

- (IBAction)restartGame:(id)sender
{
  [self pushKeyEvent:SDLK_r:NO:YES];
}

- (IBAction)grabMouse:(id)sender
{
  [self pushKeyEvent:SDLK_g:NO:YES];
}

- (IBAction)doPrefs:(id)sender
{
  [self pushKeyEvent:SDLK_TAB:NO:NO];
}

- (IBAction)volumePlus:(id)sender
{
  macOSXSendMenuEvent(MENU_VOLUME_INCREASE);
}

- (IBAction)volumeMinus:(id)sender
{
  macOSXSendMenuEvent(MENU_VOLUME_DECREASE);
}

- (void)setEmulationMenus
{
  [preferencesMenuItem setTarget:self];
  [openMenuItem setTarget:self];
  [restartMenuItem setTarget:self];
  [screenBiggerMenuItem setTarget:self];
  [screenSmallerMenuItem setTarget:self];
  [fullScreenMenuItem setTarget:self];
  [mousePaddle0MenuItem setTarget:self];
  [mousePaddle1MenuItem setTarget:self];
  [mousePaddle2MenuItem setTarget:self];
  [mousePaddle3MenuItem setTarget:self];
  [grabMouseMenuItem setTarget:self];
  [increaseVolumeMenuItem setTarget:self];
  [decreaseVolumeMenuItem setTarget:self];
}

- (void)setLauncherMenus
{
  [preferencesMenuItem setTarget:nil];
  [openMenuItem setTarget:nil];
  [restartMenuItem setTarget:nil];
  [screenBiggerMenuItem setTarget:self];
  [screenSmallerMenuItem setTarget:self];
  [fullScreenMenuItem setTarget:self];
  [mousePaddle0MenuItem setTarget:nil];
  [mousePaddle1MenuItem setTarget:nil];
  [mousePaddle2MenuItem setTarget:nil];
  [mousePaddle3MenuItem setTarget:nil];
  [grabMouseMenuItem setTarget:nil];
  [increaseVolumeMenuItem setTarget:nil];
  [decreaseVolumeMenuItem setTarget:nil];
}

- (void)setOptionsMenus
{
  [preferencesMenuItem setTarget:nil];
  [openMenuItem setTarget:nil];
  [restartMenuItem setTarget:nil];
  [screenBiggerMenuItem setTarget:self];
  [screenSmallerMenuItem setTarget:self];
  [fullScreenMenuItem setTarget:self];
  [mousePaddle0MenuItem setTarget:nil];
  [mousePaddle1MenuItem setTarget:nil];
  [mousePaddle2MenuItem setTarget:nil];
  [mousePaddle3MenuItem setTarget:nil];
  [grabMouseMenuItem setTarget:nil];
  [increaseVolumeMenuItem setTarget:nil];
  [decreaseVolumeMenuItem setTarget:nil];
}

- (void)setCommandMenus
{
  [preferencesMenuItem setTarget:nil];
  [openMenuItem setTarget:nil];
  [restartMenuItem setTarget:nil];
  [screenBiggerMenuItem setTarget:self];
  [screenSmallerMenuItem setTarget:self];
  [fullScreenMenuItem setTarget:self];
  [mousePaddle0MenuItem setTarget:nil];
  [mousePaddle1MenuItem setTarget:nil];
  [mousePaddle2MenuItem setTarget:nil];
  [mousePaddle3MenuItem setTarget:nil];
  [grabMouseMenuItem setTarget:nil];
  [increaseVolumeMenuItem setTarget:nil];
  [decreaseVolumeMenuItem setTarget:nil];
}

- (void)setDebuggerMenus
{
  [preferencesMenuItem setTarget:nil];
  [openMenuItem setTarget:nil];
  [restartMenuItem setTarget:nil];
  [screenBiggerMenuItem setTarget:self];
  [screenSmallerMenuItem setTarget:self];
  [fullScreenMenuItem setTarget:self];
  [mousePaddle0MenuItem setTarget:nil];
  [mousePaddle1MenuItem setTarget:nil];
  [mousePaddle2MenuItem setTarget:nil];
  [mousePaddle3MenuItem setTarget:nil];
  [grabMouseMenuItem setTarget:nil];
  [increaseVolumeMenuItem setTarget:nil];
  [decreaseVolumeMenuItem setTarget:nil];
}

@end
