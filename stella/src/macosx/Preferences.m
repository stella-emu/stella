/* Preferences.m - Preferences 
   class and support functions for the
   Macintosh OS X SDL port of Stella
   Mark Grebe <atarimac@cox.net>
   
   Based on the Preferences pane of the
   TextEdit application.

*/
/* $Id: Preferences.m,v 1.1.1.1 2004-06-16 02:30:30 markgrebe Exp $ */

#import <Cocoa/Cocoa.h>
#import "Preferences.h"
#import "SDL.h"

void prefsSetString(char *key, char *value)
{
	[[Preferences sharedInstance] setString:key:value];
}

void prefsGetString(char *key, char *value)
{   
	[[Preferences sharedInstance] getString:key:value];
}

void prefsSave(void)
{
	[[Preferences sharedInstance] save];
}

@implementation Preferences

static Preferences *sharedInstance = nil;

+ (Preferences *)sharedInstance {
    return sharedInstance ? sharedInstance : [[self alloc] init];
}

- (id)init
{
    defaults = [NSUserDefaults standardUserDefaults];
	sharedInstance = self;
	return(self);
}

- (void)setString:(char *)key:(char *)value
{
	NSNumber *theValue;
	NSString *theKey;
	
	theKey = [NSString stringWithCString:key];
	theValue = [NSString stringWithCString:value];
	[defaults setObject:theValue forKey:theKey];
	[theKey release];
	[theValue release];
}

- (void)getString:(char *)key:(char *)value
{
	NSString *theKey;
	NSString *theValue;
	
	theKey = [NSString stringWithCString:key];
	theValue = [defaults objectForKey:theKey];
	if (theValue == nil)
		value[0] = 0;
	else {
		[theValue getCString:value maxLength:1023];
		[theKey release];
		}
}

- (void)save
{
	[defaults synchronize];
}



@end
