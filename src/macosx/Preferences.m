/* Preferences.m - Preferences 
   class and support functions for the
   Macintosh OS X SDL port of Stella
   Mark Grebe <atarimac@cox.net>
   
   Based on the Preferences pane of the
   TextEdit application.

*/
/* $Id: Preferences.m,v 1.3 2006-02-28 02:17:26 markgrebe Exp $ */

#import <Cocoa/Cocoa.h>
#import "Preferences.h"
#import "SDL.h"

void prefsSetString(const char* key, const char* value)
{
  [[Preferences sharedInstance] setString:key:value];
}

void prefsGetString(const char* key, char* value, int size)
{   
  [[Preferences sharedInstance] getString:key:value:size];
}

void prefsSave(void)
{
  [[Preferences sharedInstance] save];
}

@implementation Preferences

static Preferences *sharedInstance = nil;

+ (Preferences *)sharedInstance
{
  return sharedInstance ? sharedInstance : [[self alloc] init];
}

- (id)init
{
  defaults = [NSUserDefaults standardUserDefaults];
  sharedInstance = self;
  return(self);
}

- (void)setString:(const char *)key:(const char *)value
{
  NSString* theKey   = [NSString stringWithCString:key encoding:NSASCIIStringEncoding];
  NSString* theValue = [NSString stringWithCString:value encoding:NSASCIIStringEncoding];

  [defaults setObject:theValue forKey:theKey];
  [theKey release];
  [theValue release];
}

- (void)getString:(const char *)key:(char *)value:(int)size
{
  NSString* theKey   = [NSString stringWithCString:key encoding:NSASCIIStringEncoding];
  NSString* theValue = [defaults objectForKey:theKey];
  if (theValue != nil)
    strncpy(value, [theValue cStringUsingEncoding: NSASCIIStringEncoding], size);
  else
    value[0] = 0;

  [theKey release];
  [theValue release];
}

- (void)save
{
  [defaults synchronize];
}

@end
