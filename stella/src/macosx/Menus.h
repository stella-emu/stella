/* Menus.h - Header for Menus 
   window class and support functions for the
   Macintosh OS X SDL port of Stella
   Mark Grebe <atarimac@cox.net>
   
*/
/* $Id: Menus.h,v 1.1.1.1 2004-06-16 02:30:30 markgrebe Exp $ */

#import <Cocoa/Cocoa.h>

@interface Menus : NSObject {
    IBOutlet id limitSpeedMenu;
    IBOutlet id paddlesMenu;
	IBOutlet id filterMenu;
	IBOutlet id videoModeMatrix;
	IBOutlet id volumeSlider;
	IBOutlet id aspectRatioField;
	int openGlEnabled;
	int gameMenusEnabled;
}

+ (Menus *)sharedInstance;
- (void)setSpeedLimitMenu:(int)limit;
- (void)initVideoMenu:(int)openGl;
- (void)setPaddleMenu:(int)number;
- (void)prefsStart;
- (void)enableGameMenus;
- (void)pushKeyEvent:(int)key:(bool)shift;
- (IBAction) paddleChange:(id) sender;
- (IBAction) prefsOK:(id) sender;
- (IBAction)prefsMenu:(id)sender;
- (IBAction)biggerScreen:(id)sender;
- (IBAction)smallerScreen:(id)sender;
- (IBAction)fullScreen:(id)sender;
- (IBAction)openCart:(id)sender;
- (IBAction)speedLimit:(id)sender;
- (IBAction)pauseGame:(id)sender;
- (IBAction)ntscPalMode:(id)sender;
- (IBAction)togglePallette:(id)sender;
- (IBAction)toggleGlFilter:(id)sender;
- (IBAction)grabMouse:(id)sender;
- (IBAction)xStartPlus:(id)sender;
- (IBAction)xStartMinus:(id)sender;
- (IBAction)yStartPlus:(id)sender;
- (IBAction)yStartMinus:(id)sender;

@end
