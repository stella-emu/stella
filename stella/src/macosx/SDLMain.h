/*   SDLMain.h - main entry point for our Cocoa-ized SDL app
       Initial Version: Darrell Walisser <dwaliss1@purdue.edu>
       Non-NIB-Code & other changes: Max Horn <max@quendi.de>

    Feel free to customize this file to suit your needs
*/
/* $Id: SDLMain.h,v 1.1.1.1 2004-06-16 02:30:30 markgrebe Exp $ */

#import <Cocoa/Cocoa.h>

@interface SDLMain : NSObject
{
}
+ (SDLMain *)sharedInstance;

@end
