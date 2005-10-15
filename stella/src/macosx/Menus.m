/* Menus.m - Menus window 
   class and support functions for the
   Macintosh OS X SDL port of Stella
   Mark Grebe <atarimac@cox.net>
   
*/
/* $Id: Menus.m,v 1.10 2005-10-15 19:02:15 markgrebe Exp $ */

#import <Cocoa/Cocoa.h>
#import "SDL.h"
#import "Menus.h"

#define QZ_m			0x2E
#define QZ_o			0x1F
#define QZ_h			0x04
#define QZ_SLASH		0x2C

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
	[self pushKeyEvent:SDLK_EQUALS:YES:YES];
}

- (IBAction)smallerScreen:(id)sender
{
	[self pushKeyEvent:SDLK_MINUS:YES:YES];
}

- (IBAction)fullScreen:(id)sender
{
	[self pushKeyEvent:SDLK_RETURN:NO:YES];
}

- (IBAction)openCart:(id)sender
{
	[self pushKeyEvent:SDLK_ESCAPE:NO:NO];
}

- (IBAction)restartGame:(id)sender
{
	[self pushKeyEvent:SDLK_r:NO:YES];
}

- (IBAction)pauseGame:(id)sender
{
	[self pushKeyEvent:SDLK_PAUSE:NO:NO];
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
	[self pushKeyEvent:SDLK_RIGHTBRACKET:YES:YES];
}

- (IBAction)volumeMinus:(id)sender
{
	[self pushKeyEvent:SDLK_LEFTBRACKET:YES:YES];
}

- (IBAction)saveProps:(id)sender
{
	[self pushKeyEvent:SDLK_s:NO:YES];
}

- (IBAction)mergeProps:(id)sender
{
	[self pushKeyEvent:SDLK_s:YES:YES];
}

@end
