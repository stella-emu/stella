//============================================================================
//
//   SSSS    tt          lll  lll
//  SS  SS   tt           ll   ll
//  SS     tttttt  eeee   ll   ll   aaaa
//   SSSS    tt   ee  ee  ll   ll      aa
//      SS   tt   eeeeee  ll   ll   aaaaa  --  "An Atari 2600 VCS Emulator"
//  SS  SS   tt   ee      ll   ll  aa  aa
//   SSSS     ttt  eeeee llll llll  aaaaa
//
// Copyright (c) 1995-2017 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#import <Cocoa/Cocoa.h>

#import "Preferences.h"

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
  if (self = [super init]) {
    defaults = [[NSUserDefaults standardUserDefaults] retain];
    sharedInstance = self;
  }
  return(self);
}

- (void)dealloc
{
  [defaults release];
  if (self == sharedInstance) {
    sharedInstance = nil;
  }
  
  [super dealloc];
}

- (void)setString:(const char *)key : (const char *)value
{
  NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
  NSString* theKey   = [NSString stringWithCString:key encoding:NSUTF8StringEncoding];
  NSString* theValue = [NSString stringWithCString:value encoding:NSUTF8StringEncoding];

  [defaults setObject:theValue forKey:theKey];
  [pool release];
}

- (void)getString:(const char *)key : (char *)value : (int)size
{
  NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
  NSString* theKey   = [NSString stringWithCString:key encoding:NSUTF8StringEncoding];
  NSString* theValue = [defaults objectForKey:theKey];
  if (theValue != nil)
    strncpy(value, [theValue cStringUsingEncoding: NSUTF8StringEncoding], size);
  else
    value[0] = 0;

  [pool release];
}

- (void)save
{
  NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
  [defaults synchronize];
  [pool release];
}

@end
