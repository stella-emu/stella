/* Menus.m - Menus window 
   class and support functions for the
   Macintosh OS X SDL port of Stella
   Mark Grebe <atarimac@cox.net>
   
*/
/* $Id: Menus.m,v 1.14 2007-01-03 12:59:23 stephena Exp $ */

#import <Cocoa/Cocoa.h>
#import "SDL.h"
#import "Menus.h"
#import "MenusEvents.h"

#define QZ_m			0x2E
#define QZ_o			0x1F
#define QZ_h			0x04
#define QZ_SLASH		0x2C

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
                    timestamp:nil windowNumber:0 context:nil characters:character
                    charactersIgnoringModifiers:character isARepeat:NO keyCode:keyCode];
    [NSApp postEvent:event1 atStart:NO];
    
    event2 = [NSEvent keyEventWithType:NSFlagsChanged location:point modifierFlags:0
                    timestamp:nil windowNumber:0 context:nil characters:nil
                    charactersIgnoringModifiers:nil isARepeat:NO keyCode:0];
    [NSApp postEvent:event2 atStart:NO];
}

void hideApp(void) {
    [NSApp hide:nil];
    releaseCmdKeys(@"h",QZ_h);
}

void showHelp(void) {
    [NSApp showHelp:nil];
    releaseCmdKeys(@"?",QZ_SLASH);
}

void miniturizeWindow(void) {
    [[NSApp keyWindow] performMiniaturize:nil];
    releaseCmdKeys(@"m",QZ_m);
}

void handleMacOSXKeypress(int key) {
	switch(key) {
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

+ (Menus *)sharedInstance {
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

- (IBAction)ntscPalMode:(id)sender
{
	[self pushKeyEvent:SDLK_f:NO:YES];
}

- (IBAction)togglePallette:(id)sender
{
	[self pushKeyEvent:SDLK_p:NO:YES];
}

- (IBAction)grabMouse:(id)sender
{
	[self pushKeyEvent:SDLK_g:NO:YES];
}

- (IBAction)xStartPlus:(id)sender
{
	[self pushKeyEvent:SDLK_END:YES:YES];
}

- (IBAction)xStartMinus:(id)sender
{
	[self pushKeyEvent:SDLK_HOME:YES:YES];
}

- (IBAction)yStartPlus:(id)sender
{
	[self pushKeyEvent:SDLK_PAGEUP:YES:YES];
}

- (IBAction)yStartMinus:(id)sender
{
	[self pushKeyEvent:SDLK_PAGEDOWN:YES:YES];
}

- (IBAction)widthPlus:(id)sender
{
	[self pushKeyEvent:SDLK_END:NO:YES];
}

- (IBAction)widthMinus:(id)sender
{
	[self pushKeyEvent:SDLK_HOME:NO:YES];
}

- (IBAction)heightPlus:(id)sender
{
	[self pushKeyEvent:SDLK_PAGEUP:NO:YES];
}

- (IBAction)heightMinus:(id)sender
{
	[self pushKeyEvent:SDLK_PAGEDOWN:NO:YES];
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

- (IBAction)saveProps:(id)sender
{
	[self pushKeyEvent:SDLK_s:NO:YES];
}

- (void)setEmulationMenus
{
    [preferencesMenuItem setTarget:self];
    [openMenuItem setTarget:self];
    [restartMenuItem setTarget:self];
    [savePropsMenuItem setTarget:self];
    [screenBiggerMenuItem setTarget:self];
    [screenSmallerMenuItem setTarget:self];
    [fullScreenMenuItem setTarget:self];
    [togglePalletteMenuItem setTarget:self];
    [ntscPalMenuItem setTarget:self];
    [increaseXStartMenuItem setTarget:self];
    [decreaseXStartMenuItem setTarget:self];
    [increaseYStartMenuItem setTarget:self];
    [decreaseYStartMenuItem setTarget:self];
    [increaseWidthMenuItem setTarget:self];
    [decreaseWidthMenuItem setTarget:self];
    [increaseHeightMenuItem setTarget:self];
    [decreaseHeightMenuItem setTarget:self];
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
    [savePropsMenuItem setTarget:nil];
    [screenBiggerMenuItem setTarget:self];
    [screenSmallerMenuItem setTarget:self];
    [fullScreenMenuItem setTarget:self];
    [togglePalletteMenuItem setTarget:nil];
    [ntscPalMenuItem setTarget:nil];
    [increaseXStartMenuItem setTarget:nil];
    [decreaseXStartMenuItem setTarget:nil];
    [increaseYStartMenuItem setTarget:nil];
    [decreaseYStartMenuItem setTarget:nil];
    [increaseWidthMenuItem setTarget:nil];
    [decreaseWidthMenuItem setTarget:nil];
    [increaseHeightMenuItem setTarget:nil];
    [decreaseHeightMenuItem setTarget:nil];
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
    [savePropsMenuItem setTarget:nil];
    [screenBiggerMenuItem setTarget:self];
    [screenSmallerMenuItem setTarget:self];
    [fullScreenMenuItem setTarget:self];
    [togglePalletteMenuItem setTarget:nil];
    [ntscPalMenuItem setTarget:nil];
    [increaseXStartMenuItem setTarget:nil];
    [decreaseXStartMenuItem setTarget:nil];
    [increaseYStartMenuItem setTarget:nil];
    [decreaseYStartMenuItem setTarget:nil];
    [increaseWidthMenuItem setTarget:nil];
    [decreaseWidthMenuItem setTarget:nil];
    [increaseHeightMenuItem setTarget:nil];
    [decreaseHeightMenuItem setTarget:nil];
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
    [savePropsMenuItem setTarget:nil];
    [screenBiggerMenuItem setTarget:self];
    [screenSmallerMenuItem setTarget:self];
    [fullScreenMenuItem setTarget:self];
    [togglePalletteMenuItem setTarget:nil];
    [ntscPalMenuItem setTarget:nil];
    [increaseXStartMenuItem setTarget:nil];
    [decreaseXStartMenuItem setTarget:nil];
    [increaseYStartMenuItem setTarget:nil];
    [decreaseYStartMenuItem setTarget:nil];
    [increaseWidthMenuItem setTarget:nil];
    [decreaseWidthMenuItem setTarget:nil];
    [increaseHeightMenuItem setTarget:nil];
    [decreaseHeightMenuItem setTarget:nil];
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
    [savePropsMenuItem setTarget:nil];
    [screenBiggerMenuItem setTarget:self];
    [screenSmallerMenuItem setTarget:self];
    [fullScreenMenuItem setTarget:self];
    [togglePalletteMenuItem setTarget:nil];
    [ntscPalMenuItem setTarget:nil];
    [increaseXStartMenuItem setTarget:nil];
    [decreaseXStartMenuItem setTarget:nil];
    [increaseYStartMenuItem setTarget:nil];
    [decreaseYStartMenuItem setTarget:nil];
    [increaseWidthMenuItem setTarget:nil];
    [decreaseWidthMenuItem setTarget:nil];
    [increaseHeightMenuItem setTarget:nil];
    [decreaseHeightMenuItem setTarget:nil];
    [mousePaddle0MenuItem setTarget:nil];
    [mousePaddle1MenuItem setTarget:nil];
    [mousePaddle2MenuItem setTarget:nil];
    [mousePaddle3MenuItem setTarget:nil];
    [grabMouseMenuItem setTarget:nil];
    [increaseVolumeMenuItem setTarget:nil];
    [decreaseVolumeMenuItem setTarget:nil];
}

@end
