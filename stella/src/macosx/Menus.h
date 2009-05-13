/* Menus.h - Header for Menus 
   window class and support functions for the
   Macintosh OS X SDL port of Stella
   Mark Grebe <atarimac@cox.net>
   
*/
/* $Id$ */

#import <Cocoa/Cocoa.h>

@interface Menus : NSObject {
    IBOutlet id preferencesMenuItem;
    IBOutlet id openMenuItem;
    IBOutlet id restartMenuItem;
    IBOutlet id savePropsMenuItem;
    IBOutlet id screenBiggerMenuItem;
    IBOutlet id screenSmallerMenuItem;
    IBOutlet id fullScreenMenuItem;
    IBOutlet id togglePalletteMenuItem;
    IBOutlet id ntscPalMenuItem;
    IBOutlet id increaseXStartMenuItem;
    IBOutlet id decreaseXStartMenuItem;
    IBOutlet id increaseYStartMenuItem;
    IBOutlet id decreaseYStartMenuItem;
    IBOutlet id increaseWidthMenuItem;
    IBOutlet id decreaseWidthMenuItem;
    IBOutlet id increaseHeightMenuItem;
    IBOutlet id decreaseHeightMenuItem;
    IBOutlet id mousePaddle0MenuItem;
    IBOutlet id mousePaddle1MenuItem;
    IBOutlet id mousePaddle2MenuItem;
    IBOutlet id mousePaddle3MenuItem;
    IBOutlet id grabMouseMenuItem;
    IBOutlet id increaseVolumeMenuItem;
    IBOutlet id decreaseVolumeMenuItem;
}

+ (Menus *)sharedInstance;
- (void)pushKeyEvent:(int)key:(bool)shift:(bool)cmd;
- (IBAction)paddleChange:(id) sender;
- (IBAction)biggerScreen:(id)sender;
- (IBAction)smallerScreen:(id)sender;
- (IBAction)fullScreen:(id)sender;
- (IBAction)openCart:(id)sender;
- (IBAction)restartGame:(id)sender;
- (IBAction)ntscPalMode:(id)sender;
- (IBAction)togglePallette:(id)sender;
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
- (IBAction)saveProps:(id)sender;
- (void)setEmulationMenus;
- (void)setLauncherMenus;
- (void)setOptionsMenus;
- (void)setCommandMenus;
- (void)setDebuggerMenus;

@end
