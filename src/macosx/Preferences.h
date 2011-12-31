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
// Copyright (c) 1995-2012 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

#import <Cocoa/Cocoa.h>

/**
  Preferences class and support functions for the Macintosh OS X
  SDL port of Stella.

  @author  Mark Grebe <atarimac@cox.net>
*/
@interface Preferences : NSObject
{
  NSUserDefaults *defaults;    /* Defaults pointer */
}

+ (Preferences *)sharedInstance;
- (void)setString:(const char *)key:(const char *)value;
- (void)getString:(const char *)key:(char *)value:(int)size;
- (void)save;

@end
