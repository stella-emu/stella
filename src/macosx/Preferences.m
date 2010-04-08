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
// Copyright (c) 1995-2010 by Bradford W. Mott and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
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
