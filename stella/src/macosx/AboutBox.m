/* AboutBox.m - AboutBox window 
   class and support functions for the
   Macintosh OS X SDL port of Stella
   Mark Grebe <atarimac@cox.net>
*/
/* $Id: AboutBox.m,v 1.1.1.1 2004-06-16 02:30:30 markgrebe Exp $ */

#import "AboutBox.h";

static int boxDisplayed = FALSE;
static int shouldScroll = TRUE;

/*------------------------------------------------------------------------------
*  AboutBoxScroll - Function call which is called from main emulator loop
*      If About Box is key window, and scrolling hasn't been stop, it will
*      call the scrollCredits method to advance the scroll.
*-----------------------------------------------------------------------------*/
void AboutBoxScroll(void) 
{
	if (boxDisplayed && shouldScroll)
		[[AboutBox sharedInstance] scrollCredits];
}

@implementation AboutBox

static AboutBox *sharedInstance = nil;

+ (AboutBox *)sharedInstance
{
    return sharedInstance ? sharedInstance : [[self alloc] init];
}

- (id)init 
{
    if (sharedInstance) {
        [self dealloc];
    } else {
        sharedInstance = [super init];
    }
    
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
        localInfoDict = (NSDictionary *)
                        CFBundleGetLocalInfoDictionary( localInfoBundle );

        // Setup the app name field
        appName = @"StellaOSX";
        [appNameField setStringValue:appName];

        // Set the about box window title
        [theWindow setTitle:[NSString stringWithFormat:@"About %@", appName]];

        // Setup the version field
        versionString = [infoDictionary objectForKey:@"CFBundleVersion"];
        [versionField setStringValue:[NSString stringWithFormat:@"Version %@", 
                                                          versionString]];

        // Setup our credits
        creditsPath = [[NSBundle mainBundle] pathForResource:@"Credits" 
                                             ofType:@"html"];

        creditsString = [[NSAttributedString alloc] initWithPath:creditsPath 
                                                    documentAttributes:nil];

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
    [[appNameField window] makeKeyAndOrderFront:nil];
	
}

/*------------------------------------------------------------------------------
*  windowDidBecomeKey - Start the scrolling when the about box is displayed.
*-----------------------------------------------------------------------------*/
- (void)windowDidBecomeKey:(NSNotification *)notification
{
	boxDisplayed = TRUE;
	shouldScroll = TRUE;
}

/*------------------------------------------------------------------------------
*  windowDidResignKey - Stop the scrolling when the about box is gone.
*-----------------------------------------------------------------------------*/
- (void)windowDidResignKey:(NSNotification *)notification
{
	boxDisplayed = FALSE;
}

/*------------------------------------------------------------------------------
*  scrollCredits - Perform the scrolling.
*-----------------------------------------------------------------------------*/
- (void)scrollCredits
{
        if (restartAtTop)
        {
            restartAtTop = NO;
            
            // Set the position
            [creditsField scrollPoint:NSMakePoint( 0, 0 )];
            
            return;
        }
        if (currentPosition >= maxScrollHeight) 
        {
            // Reset the position
            currentPosition = 0;
            restartAtTop = YES;
        }
        else
        {
            // Scroll to the position
            [creditsField scrollPoint:NSMakePoint( 0, currentPosition )];
            
            // Increment the scroll position
            currentPosition += 0.25;
        }
}

/*------------------------------------------------------------------------------
*  clicked - Starts/stops scrolling on mouse click in about box text view.
*-----------------------------------------------------------------------------*/
- (void)clicked
{
	shouldScroll = !shouldScroll;
}

/*------------------------------------------------------------------------------
*  clicked - Restarts at start of about box on double mouse click in about box 
*      text view.
*-----------------------------------------------------------------------------*/
- (void)doubleClicked
{
	shouldScroll = !shouldScroll;
    currentPosition = 0;
	restartAtTop = YES;
}

@end

