/* Menus.h - Header for Menus 
   window class and support functions for the
   Macintosh OS X SDL port of Stella
   Mark Grebe <atarimac@cox.net>
   
*/
/* $Id: Menus.h,v 1.5 2005-06-03 05:04:56 markgrebe Exp $ */

#import <Cocoa/Cocoa.h>

@interface Menus : NSObject {
}

+ (Menus *)sharedInstance;
- (void)pushKeyEvent:(int)key:(bool)shift:(bool)cmd;
- (IBAction)paddleChange:(id) sender;
- (IBAction)biggerScreen:(id)sender;
- (IBAction)smallerScreen:(id)sender;
- (IBAction)fullScreen:(id)sender;
- (IBAction)openCart:(id)sender;
- (IBAction)restartGame:(id)sender;
- (IBAction)pauseGame:(id)sender;
- (IBAction)ntscPalMode:(id)sender;
- (IBAction)togglePallette:(id)sender;
- (IBAction)toggleGlFilter:(id)sender;
- (IBAction)grabMouse:(id)sender;
- (IBAction)xStartPlus:(id)sender;
- (IBAction)xStartMinus:(id)sender;
- (IBAction)yStartPlus:(id)sender;
- (IBAction)yStartMinus:(id)sender;
- (IBAction)widthPlus:(id)sender;
- (IBAction)widthMinus:(id)sender;
- (IBAction)heightPlus:(id)sender;
- (IBAction)heightMinus:(id)sender;
- (IBAction)doPrefs:(id)sender;
- (IBAction)volumePlus:(id)sender;
- (IBAction)volumeMinus:(id)sender;

@end
