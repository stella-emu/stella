/** SDLMain.m - main entry point for our Cocoa-ized SDL app
    Initial Version: Darrell Walisser <dwaliss1@purdue.edu>
    Non-NIB-Code & other changes: Max Horn <max@quendi.de>
  */
// $Id$

#import "SDL.h"
#import "SDLMain.h"
#import "Preferences.h"

#define SDL_main stellaMain
extern int stellaMain(int argc, char* argv[]);

static int    gArgc;
static char** gArgv;
static BOOL   gFinderLaunch;
static BOOL   gCalledAppMainline = FALSE;


// The main class of the application, the application's delegate
@implementation SDLMain

// ----------------------------------------------------------------------------
static SDLMain* sharedInstance = nil;

+ (SDLMain*) sharedInstance {
    return sharedInstance;
}


// ----------------------------------------------------------------------------
// Set the working directory to the .app's parent directory
- (void) setupWorkingDirectory:(BOOL)shouldChdir
{
  if (shouldChdir)
  {
    char parentdir[MAXPATHLEN];
    CFURLRef url  = CFBundleCopyBundleURL(CFBundleGetMainBundle());
    CFURLRef url2 = CFURLCreateCopyDeletingLastPathComponent(0, url);
    if (CFURLGetFileSystemRepresentation(url2, 1, (UInt8*)parentdir, MAXPATHLEN))
    {
      chdir(parentdir);   /* chdir to the binary app's parent */
    }
    CFRelease(url);
    CFRelease(url2);
  }
}


// ----------------------------------------------------------------------------
/**
  * Catch document open requests...this lets us notice files when the app
  *  was launched by double-clicking a document, or when a document was
  *  dragged/dropped on the app's icon. You need to have a
  *  CFBundleDocumentsType section in your Info.plist to get this message,
  *  apparently.
  *
  * Files are added to gArgv, so to the app, they'll look like command line
  *  arguments. Previously, apps launched from the finder had nothing but
  *  an argv[0].
  *
  * This message may be received multiple times to open several docs on launch.
  *
  * This message is ignored once the app's mainline has been called.
  */
- (BOOL)application:(NSApplication*)theApplication openFile:(NSString*)filename
{
  const char* temparg;
  size_t arglen;
  char* arg;
  char** newargv;

  if (!gFinderLaunch)     // MacOS is passing command line args.
    return FALSE;
  if (gCalledAppMainline) // app has started, ignore this document.
    return FALSE;

  temparg = [filename UTF8String];
  arglen = SDL_strlen(temparg) + 1;
  arg = (char*) SDL_malloc(arglen);
  if (arg == NULL)
    return FALSE;

  newargv = (char **) realloc(gArgv, sizeof (char*) * (gArgc + 2));
  if (newargv == NULL)
  {
    SDL_free(arg);
    return FALSE;
  }
  gArgv = newargv;

  SDL_strlcpy(arg, temparg, arglen);
  gArgv[gArgc++] = arg;
  gArgv[gArgc] = NULL;
  return TRUE;
}


// ----------------------------------------------------------------------------
// Called when the internal event loop has just started running
- (void) applicationDidFinishLaunching: (NSNotification *) note
{
  int status;

  // Set the working directory to the .app's parent directory
  [self setupWorkingDirectory:gFinderLaunch];

  // Hand off to main application code
  gCalledAppMainline = TRUE;
  status = SDL_main (gArgc, gArgv);
	
  // We're done, thank you for playing
  exit(status);
}

@end


#ifdef main
#  undef main
#endif

// Main entry point to executable - should *not* be SDL_main!
int main (int argc, char* argv[])
{
	static Preferences *myPrefs;

  // Copy the arguments into a global variable
  // This is passed if we are launched by double-clicking
  if ( argc >= 2 && strncmp (argv[1], "-psn", 4) == 0 )
  {
    gArgv = (char**) SDL_malloc(sizeof(char*) * 2);
    gArgv[0] = argv[0];
    gArgv[1] = NULL;
    gArgc = 1;
    gFinderLaunch = YES;
  }
  else
  {
    gArgc = argc;
    gArgv = (char**) SDL_malloc(sizeof(char*) * (argc+1));
    for (int i = 0; i <= argc; i++)
      gArgv[i] = argv[i];
    gFinderLaunch = NO;
  }
	
	myPrefs = [Preferences sharedInstance];

  NSApplicationMain (argc, (const char**)argv);

  return 0;
}
