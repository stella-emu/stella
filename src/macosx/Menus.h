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
// Copyright (c) 1995-2011 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

#import <Cocoa/Cocoa.h>

/**
  Menus window class and support functions for the Macintosh OS X
  SDL port of Stella.

  @author  Mark Grebe <atarimac@cox.net>
*/
@interface Menus : NSObject
{
  IBOutlet id preferencesMenuItem;
  IBOutlet id openMenuItem;
  IBOutlet id restartMenuItem;
  IBOutlet id screenBiggerMenuItem;
  IBOutlet id screenSmallerMenuItem;
  IBOutlet id fullScreenMenuItem;
  IBOutlet id increaseVolumeMenuItem;
  IBOutlet id decreaseVolumeMenuItem;
}

+ (Menus *)sharedInstance;
- (void)pushKeyEvent:(int)key:(bool)shift:(bool)cmd:(bool)control;
- (IBAction)biggerScreen:(id)sender;
- (IBAction)smallerScreen:(id)sender;
- (IBAction)fullScreen:(id)sender;
- (IBAction)restartGame:(id)sender;
- (IBAction)doPrefs:(id)sender;
- (IBAction)volumePlus:(id)sender;
- (IBAction)volumeMinus:(id)sender;
- (void)setEmulationMenus;
- (void)setLauncherMenus;
- (void)setOptionsMenus;
- (void)setCommandMenus;
- (void)setDebuggerMenus;

@end
