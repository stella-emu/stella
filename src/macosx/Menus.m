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
// Copyright (c) 1995-2011 by Bradford W. Mott, Stephen Anthony
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

-(void)pushKeyEvent:(int)key:(bool)shift:(bool)cmd:(bool)control
{
	SDL_Event theEvent;

	theEvent.key.type = SDL_KEYDOWN;
	theEvent.key.state = SDL_PRESSED;
	theEvent.key.keysym.scancode = 0;
	theEvent.key.keysym.sym = key;
	theEvent.key.keysym.mod = 0;
	if (cmd)
		theEvent.key.keysym.mod = KMOD_META;
	else if (control)
		theEvent.key.keysym.mod = KMOD_CTRL;
	if (shift)
		theEvent.key.keysym.mod |= KMOD_SHIFT;
	theEvent.key.keysym.unicode = 0;
	SDL_PushEvent(&theEvent);
}

- (IBAction)biggerScreen:(id)sender
{
  [self pushKeyEvent:SDLK_EQUALS:NO:YES:NO];
}

- (IBAction)smallerScreen:(id)sender
{
  [self pushKeyEvent:SDLK_MINUS:NO:YES:NO];
}

- (IBAction)fullScreen:(id)sender
{
  [self pushKeyEvent:SDLK_RETURN:NO:YES:NO];
}

- (IBAction)restartGame:(id)sender
{
  [self pushKeyEvent:SDLK_r:NO:NO:YES];
}

- (IBAction)doPrefs:(id)sender
{
  [self pushKeyEvent:SDLK_TAB:NO:NO:NO];
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
  [increaseVolumeMenuItem setTarget:self];
  [decreaseVolumeMenuItem setTarget:self];
}

- (void)setLauncherMenus
{
  [preferencesMenuItem setTarget:nil];
  [openMenuItem setTarget:nil];
  [restartMenuItem setTarget:nil];
  [screenBiggerMenuItem setTarget:nil];
  [screenSmallerMenuItem setTarget:nil];
  [fullScreenMenuItem setTarget:self];
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
  [increaseVolumeMenuItem setTarget:nil];
  [decreaseVolumeMenuItem setTarget:nil];
}

@end
