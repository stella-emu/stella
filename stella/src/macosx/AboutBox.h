/* AboutBox.h - Header for About Box 
   window class and support functions for the
   Macintosh OS X SDL port of Stella
   Mark Grebe <atarimac@cox.net>
   
*/
/* $Id: AboutBox.h,v 1.2 2005-06-04 02:04:06 markgrebe Exp $ */


#import <Cocoa/Cocoa.h>

@interface AboutBox : NSObject
{
    IBOutlet id appNameField;
    IBOutlet id creditsField;
    IBOutlet id versionField;
    NSTimer *scrollTimer;
    float currentPosition;
    float maxScrollHeight;
    NSTimeInterval startTime;
    BOOL restartAtTop;
}

+ (AboutBox *)sharedInstance;
- (IBAction)showPanel:(id)sender;
- (void)OK:(id)sender;

@end
