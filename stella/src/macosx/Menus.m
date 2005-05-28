/* Menus.m - Menus window 
   class and support functions for the
   Macintosh OS X SDL port of Stella
   Mark Grebe <atarimac@cox.net>
   
*/
/* $Id: Menus.m,v 1.6 2005-05-28 21:50:07 markgrebe Exp $ */

#import <Cocoa/Cocoa.h>
#import "SDL.h"

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
