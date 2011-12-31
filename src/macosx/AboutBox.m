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

#import "AboutBox.h"

@implementation AboutBox

static AboutBox *sharedInstance = nil;

+ (AboutBox *)sharedInstance
{
  return sharedInstance ? sharedInstance : [[self alloc] init];
}

- (id)init 
{
  if (sharedInstance)
    [self dealloc];
  else
    sharedInstance = [super init];
    
  return sharedInstance;
}

/*------------------------------------------------------------------------------
*  showPanel - Display the About Box.
*-----------------------------------------------------------------------------*/
- (IBAction)showPanel:(id)sender
{
  NSRect creditsBounds;

  if (!appNameField)
  {
    NSWindow *theWindow;
    NSString *creditsPath;
    NSAttributedString *creditsString;
    NSString *appName;
    NSString *versionString;
    NSDictionary *infoDictionary;
    CFBundleRef localInfoBundle;
    NSDictionary *localInfoDict;

    if (![NSBundle loadNibNamed:@"AboutBox" owner:self])
    {
      NSLog( @"Failed to load AboutBox.nib" );
      NSBeep();
      return;
    }
    theWindow = [appNameField window];

    // Get the info dictionary (Info.plist)
    infoDictionary = [[NSBundle mainBundle] infoDictionary];

    // Get the localized info dictionary (InfoPlist.strings)
    localInfoBundle = CFBundleGetMainBundle();
    localInfoDict = (NSDictionary *) CFBundleGetLocalInfoDictionary( localInfoBundle );

    // Setup the app name field
    appName = @"Stella";
    [appNameField setStringValue:appName];

    // Set the about box window title
    [theWindow setTitle:[NSString stringWithFormat:@"About %@", appName]];

    // Setup the version field
    versionString = [infoDictionary objectForKey:@"CFBundleVersion"];
    [versionField setStringValue:[NSString stringWithFormat:@"Version %@", versionString]];

    // Setup our credits
    creditsPath = [[NSBundle mainBundle] pathForResource:@"Credits" ofType:@"html"];
    creditsString = [[NSAttributedString alloc] initWithPath:creditsPath documentAttributes:nil];

    [creditsField replaceCharactersInRange:NSMakeRange( 0, 0 ) 
                    withRTF:[creditsString RTFFromRange:
                    NSMakeRange( 0, [creditsString length] ) 
                    documentAttributes:nil]];

    // Prepare some scroll info
    creditsBounds = [creditsField bounds];
    maxScrollHeight = creditsBounds.size.height*2.75;

    // Setup the window
    [theWindow setExcludedFromWindowsMenu:YES];
    [theWindow setMenu:nil];
    [theWindow center];
  }
    
  if (![[appNameField window] isVisible])
  {
    currentPosition = 0;
    restartAtTop = NO;
    [creditsField scrollPoint:NSMakePoint( 0, 0 )];
  }
    
  // Show the window
  [NSApp runModalForWindow:[appNameField window]];
  [[appNameField window] close];
}

- (void)OK:(id)sender
{
  [NSApp stopModal];
}

@end
