/* AboutBoxTextView.m - 
   AboutBoxTextView view class for the
   Macintosh OS X SDL port of Stella
   Mark Grebe <atarimac@cox.net>
*/
/* $Id: AboutBoxTextView.m,v 1.1.1.1 2004-06-16 02:30:30 markgrebe Exp $ */

#import "AboutBoxTextView.h"
#import "AboutBox.h"

@implementation AboutBoxTextView
/*------------------------------------------------------------------------------
*  mouseDown - This method notifies the AboutBox class of a mouse click, then
*    calls the normal text view mouseDown.
*-----------------------------------------------------------------------------*/
- (void)mouseDown:(NSEvent *)theEvent
{
	if ([theEvent clickCount] >= 2)
		[[AboutBox sharedInstance] doubleClicked];
	else
		[[AboutBox sharedInstance] clicked];
	[super mouseDown:theEvent];
}


@end
