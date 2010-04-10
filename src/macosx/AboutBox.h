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
// Copyright (c) 1995-2010 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

#import <Cocoa/Cocoa.h>

/**
  AboutBox window class and support functions for the Macintosh OS X
  SDL port of Stella.

  @author  Mark Grebe <atarimac@cox.net>
*/
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
