/* Menus.m - Menus window 
   class and support functions for the
   Macintosh OS X SDL port of Stella
   Mark Grebe <atarimac@cox.net>
   
*/
/* $Id: Menus.m,v 1.2 2004-07-14 06:54:17 markgrebe Exp $ */

#import <Cocoa/Cocoa.h>
#import "Menus.h"
#import "SDL.h"

#define QZ_m			0x2E
#define QZ_o			0x1F
#define QZ_h			0x04
#define QZ_SLASH		0x2C
#define QZ_COMMA		0x2B

extern void setPaddleMode(int mode);
extern void getPrefsSettings(int *gl, int *volume, float *aspect);
extern void setPrefsSettings(int gl, int volume, float aspect);

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

/*------------------------------------------------------------------------------
*  browseFile - This allows the user to chose a file to read in.
*-----------------------------------------------------------------------------*/
char *browseFile(void) {
    NSOpenPanel *openPanel = nil;
	char *fileName;
	
	fileName = malloc(FILENAME_MAX);
	if (fileName == NULL)
	    return NULL;
	
    openPanel = [NSOpenPanel openPanel];
    [openPanel setCanChooseDirectories:NO];
    [openPanel setCanChooseFiles:YES];
    
    if ([openPanel runModalForDirectory:nil file:nil types:nil] == NSOKButton) {
		[[[openPanel filenames] objectAtIndex:0] getCString:fileName];
        releaseCmdKeys(@"o",QZ_o);
		return fileName;
		}
    else {
        releaseCmdKeys(@"o",QZ_o);
        return NULL;
		}
    }

void prefsStart(void)
{
	[[Menus sharedInstance] prefsStart];
    releaseCmdKeys(@",",QZ_m);
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

void initVideoMenu(int openGl)
{
	[[Menus sharedInstance] initVideoMenu:openGl];
}

void setPaddleMenu(int number)
{
    if (number < 4)
		[[Menus sharedInstance] setPaddleMenu:number];
}

void setSpeedLimitMenu(int limit)
{
	[[Menus sharedInstance] setSpeedLimitMenu:limit];
}

void enableGameMenus(void)
{
	[[Menus sharedInstance] enableGameMenus];
}

@implementation Menus

static Menus *sharedInstance = nil;

+ (Menus *)sharedInstance {
    return sharedInstance ? sharedInstance : [[self alloc] init];
}

- (id)init
{
	sharedInstance = self;
	gameMenusEnabled = 0;
	return(self);
}

- (void)setSpeedLimitMenu:(int)limit
{
	if (limit)
        [limitSpeedMenu setState:NSOnState];
	else
        [limitSpeedMenu setState:NSOffState];
}

- (void)setPaddleMenu:(int)number
{
	int i;
	
	for (i=0;i<4;i++)
	    [[paddlesMenu itemAtIndex:i] setState:NSOffState];
	if (number < 4)
		[[paddlesMenu itemAtIndex:number] setState:NSOnState];
}

- (void)initVideoMenu:(int)openGl
{
	openGlEnabled = openGl;
}

- (void)enableGameMenus
{
	gameMenusEnabled = 1;
}

- (IBAction) paddleChange:(id) sender
{
	setPaddleMode([sender tag]);
	[self setPaddleMenu:[sender tag]];
}

- (void) prefsStart
{
    int gl, volume;
	float aspectRatio;
	
	getPrefsSettings(&gl, &volume, &aspectRatio);
	
	[volumeSlider setIntValue:volume];
	[videoModeMatrix selectCellWithTag:gl];
	[aspectRatioField setFloatValue:aspectRatio];

    [NSApp runModalForWindow:[volumeSlider window]];
	
	gl = [[videoModeMatrix selectedCell] tag];
	volume = [volumeSlider intValue];
	aspectRatio = [aspectRatioField floatValue];
	
	setPrefsSettings(gl, volume, aspectRatio);
}

- (IBAction) prefsOK:(id) sender
{
    [NSApp stopModal];
    [[volumeSlider window] close];
}
- (IBAction)prefsMenu:(id)sender
{
	[[Menus sharedInstance] prefsStart];
}

-(void)pushKeyEvent:(int)key:(bool)shift
{
	SDL_Event theEvent;

	theEvent.key.type = SDL_KEYDOWN;
	theEvent.key.state = SDL_PRESSED;
	theEvent.key.keysym.scancode = 0;
	theEvent.key.keysym.sym = key;
	theEvent.key.keysym.mod = KMOD_LMETA;
	if (shift)
		theEvent.key.keysym.mod |= KMOD_LSHIFT;
	theEvent.key.keysym.unicode = 0;
	SDL_PushEvent(&theEvent);
}

- (IBAction)biggerScreen:(id)sender
{
	[self pushKeyEvent:SDLK_EQUALS:NO];
}

- (IBAction)smallerScreen:(id)sender
{
	[self pushKeyEvent:SDLK_MINUS:NO];
}

- (IBAction)fullScreen:(id)sender
{
	[self pushKeyEvent:SDLK_RETURN:NO];
}

- (IBAction)openCart:(id)sender
{
	[self pushKeyEvent:SDLK_o:NO];
}

- (IBAction)speedLimit:(id)sender
{
	[self pushKeyEvent:SDLK_l:NO];
}

- (IBAction)pauseGame:(id)sender
{
	[self pushKeyEvent:SDLK_p:NO];
}

- (IBAction)ntscPalMode:(id)sender
{
	[self pushKeyEvent:SDLK_f:YES];
}

- (IBAction)toggleGlFilter:(id)sender
{
	[self pushKeyEvent:SDLK_f:NO];
}

- (IBAction)togglePallette:(id)sender
{
	[self pushKeyEvent:SDLK_p:YES];
}

- (IBAction)grabMouse:(id)sender
{
	[self pushKeyEvent:SDLK_g:NO];
}

- (IBAction)xStartPlus:(id)sender
{
	[self pushKeyEvent:SDLK_HOME:NO];
}

- (IBAction)xStartMinus:(id)sender
{
	[self pushKeyEvent:SDLK_END:NO];
}

- (IBAction)yStartPlus:(id)sender
{
	[self pushKeyEvent:SDLK_PAGEUP:NO];
}

- (IBAction)yStartMinus:(id)sender
{
	[self pushKeyEvent:SDLK_PAGEDOWN:NO];
}

- (IBAction)widthPlus:(id)sender
{
	[self pushKeyEvent:SDLK_END:YES];
}

- (IBAction)widthMinus:(id)sender
{
	[self pushKeyEvent:SDLK_HOME:YES];
}

- (IBAction)heightPlus:(id)sender
{
	[self pushKeyEvent:SDLK_PAGEUP:YES];
}

- (IBAction)heightMinus:(id)sender
{
	[self pushKeyEvent:SDLK_PAGEDOWN:YES];
}

- (BOOL)validateMenuItem:(id <NSMenuItem>)menuItem
{
	if (gameMenusEnabled) {
	    if ([[menuItem title] isEqualToString:@"Toggle Open GL Filter"]) {
		    if (openGlEnabled)
			    return YES;
			else
			    return NO;
	        }
	    else
		    return YES;
		}
	else {
		if ([[menuItem title] isEqualToString:@"Open New Cartridge…"] ||
		    [[menuItem title] isEqualToString:@"Preferences..."])
		    return YES;
		else
			return NO;
		}
}
@end
