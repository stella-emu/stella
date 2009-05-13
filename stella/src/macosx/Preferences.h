/* Preferences.h - Header for Preferences 
   class and support functions for the
   Macintosh OS X SDL port of Stella
   Mark Grebe <atarimac@cox.net>
   
   Based on the Preferences pane of the
   TextEdit application.

*/
/* $Id$ */
#import <Cocoa/Cocoa.h>

@interface Preferences : NSObject {
    NSUserDefaults *defaults;    /* Defaults pointer */
}

+ (Preferences *)sharedInstance;
- (void)setString:(char *)key:(char *)value;
- (void)getString:(char *)key:(char *)value;
- (void)save;

@end
