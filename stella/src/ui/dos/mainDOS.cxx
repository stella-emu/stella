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
// Copyright (c) 1995-1999 by Bradford W. Mott
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: mainDOS.cxx,v 1.5 2002-04-10 04:09:59 bwmott Exp $
//============================================================================

#include <go32.h>
#include <dpmi.h>
#include <sys/farptr.h>
#include <sys/nearptr.h>
#include <dos.h>
#include <pc.h>

#include <fstream>
#include <iostream>
#include <strstream>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "bspf.hxx"
#include "Console.hxx"
#include "Event.hxx"
#include "MediaSrc.hxx"
#include "PropsSet.hxx"
#include "SndDOS.hxx"
#include "System.hxx"
#include "PCJoys.hxx"
#include "scandef.h"

// Pointer to the console object or the null pointer
Console* theConsole;

// Event objects to use
Event theEvent;
Event theKeyboardEvent;

// Indicates if the entire frame should be redrawn
bool theRedrawEntireFrameFlag = true;

// Indicates if the user wants to quit
bool theQuitIndicator = false;

// Indicates what the desired frame rate is
uInt32 theDesiredFrameRate = 60;

// Indicate which paddle mode we're using:
//   0 - Mouse emulates paddle 0
//   1 - Mouse emulates paddle 1
//   2 - Mouse emulates paddle 2
//   3 - Mouse emulates paddle 3
//   4 - Use real Atari 2600 paddles
uInt32 thePaddleMode = 0;

// Indicates if the Mode X graphics should be used or not
bool theUseModeXFlag = false;

// Pointer to the joysticks object
PCJoysticks* theJoysticks;

// Mouse IRQ
#define MOUSE_BIOS           0x33

// VGA Card definitions
#define VGA_BIOS             0x10
#define VGA_PEL_ADDRESS      0x03c8
#define VGA_PEL_DATA         0x03c9

// Remembers which video mode to restore when the program exits
static uInt16 theDefaultVideoMode;

static uInt16 thePixelDataTable[256];

// Indicates the width and height of the screen
static uInt32 theWidth;
static uInt32 theHeight;

// Keyboard Interrupt definitions
_go32_dpmi_seginfo theOldKeyboardHandler;
_go32_dpmi_seginfo theKeyboardHandler;
static void keyboardInterruptServiceRoutine(void);


/**
  This routine should be called once the console is create to setup
  to graphics mode 
*/
void startup()
{
  union REGS regs;

  // Get the desired width and height of the display
  theWidth = theConsole->mediaSource().width();
  theHeight = theConsole->mediaSource().height();

  // Initialize the pixel data table
  for(uInt32 j = 0; j < 256; ++j)
  {
    thePixelDataTable[j] = j | (j << 8);
  }

  // Lets save the current video mode
  regs.h.ah = 0x0f;
  int86(VGA_BIOS, &regs, &regs);
  theDefaultVideoMode = regs.h.al;

  // Plop into 320x200x256 mode 13
  regs.w.ax = 0x0013;
  regs.h.ah = 0x00;
  int86(VGA_BIOS, &regs, &regs);

  // Enable Mode X if we're using it
  if(theUseModeXFlag)
  {
    outpw(0x3C4, 0x604);     // Unchain mode
    outpw(0x3C4, 0xF02);
    outpw(0x3D4, 0x0014);
    outpw(0x3D4, 0xE317);

    outpw(0x3C2, 0xE3);      // Create square pixel aspect ratio
    outpw(0x3D4, 0x2C11);    // Turn off write protect
    outpw(0x3D4, 0x0D06);    // Vertical total
    outpw(0x3D4, 0x3E07);    // Overflow register
    outpw(0x3D4, 0xEA10);    // Vertical retrace start
    outpw(0x3D4, 0xAC11);    // Vertical retrace end and write protect
    outpw(0x3D4, 0xDF12);    // Vertical display enable end
    outpw(0x3D4, 0xE715);    // Start vertical blanking
    outpw(0x3D4, 0x0616);    // End vertical blanking

    // Clear the screen now that Mode X is enabled
    for(uInt32 i = 0; i < 240 * 80; ++i)
    {
      outp(0x3C4, 0x02);
      outp(0x3C5, 0x0F);
      _farpokeb(_dos_ds, 0xA0000 + i, 0);
    }
  }
    
  // Setup to color palette for the video card
  const uInt32* palette = theConsole->mediaSource().palette();
  outp(VGA_PEL_ADDRESS, 0);
  for(int index = 0; index < 256; index++)
  {
    outp(VGA_PEL_DATA, (palette[index] & 0x00ff0000) >> 18);
    outp(VGA_PEL_DATA, (palette[index] & 0x0000ff00) >> 10);
    outp(VGA_PEL_DATA, (palette[index] & 0x000000ff) >> 2);
  }

  // Install keyboard interrupt handler
  disable();
  _go32_dpmi_get_protected_mode_interrupt_vector(0x09, &theOldKeyboardHandler);
  theKeyboardHandler.pm_selector = _my_cs();
  theKeyboardHandler.pm_offset = (int)keyboardInterruptServiceRoutine;
  _go32_dpmi_allocate_iret_wrapper(&theKeyboardHandler);
  _go32_dpmi_set_protected_mode_interrupt_vector(0x09, &theKeyboardHandler);
  enable();

  // Initialize mouse handler via DOS interrupt
  regs.w.ax = 0x0000;
  int86(MOUSE_BIOS, &regs, &regs);

  if(regs.w.ax == 0x0000)
  {
    cerr << "WARNING: Mouse initialization failed..." << endl;
  }
  else
  {
    // Set mouse bounding box to 0,0 to 511,511
    regs.w.ax = 0x0007;
    regs.w.cx = 0;
    regs.w.dx = 511;
    int86(MOUSE_BIOS, &regs, &regs);

    regs.w.ax = 0x0008;
    regs.w.cx = 0;
    regs.w.dx = 511;
    int86(MOUSE_BIOS, &regs, &regs);
  }

  // Set joystick pointer to null
  theJoysticks = 0;

  // Register function to remove interrupts when program exits
  void shutdownInterrupts();
  atexit(shutdownInterrupts);
}

/**
  This function should be registered with the atexit function so that it's
  automatically called when the program terminates.  It's responsible for 
  removing any interrupts we're using.
*/
void shutdownInterrupts()
{
  // Restore the keyboard interrupt routine
  disable();
  _go32_dpmi_set_protected_mode_interrupt_vector(0x09, &theOldKeyboardHandler);
  _go32_dpmi_free_iret_wrapper(&theKeyboardHandler);
  enable();
}

/**
  This function should be called right before the program exists to 
  clean up things and reset the graphics mode.
*/
void shutdown()
{
  union REGS regs;

  // Restore previous display mode
  regs.h.ah = 0x00;
  regs.h.al = theDefaultVideoMode;
  int86(VGA_BIOS, &regs, &regs);

  // Delete the joystick object
  delete theJoysticks;
  theJoysticks = 0;
}

/**
*/
void updateDisplay(MediaSource& mediaSource)
{
  uInt32* current = (uInt32*)mediaSource.currentFrameBuffer();
  uInt32* previous = (uInt32*)mediaSource.previousFrameBuffer();

  // Are we updating a Mode X display?
  if(theUseModeXFlag)
  {
    uInt32 width = theWidth / 4;
    uInt32 height = (theHeight > 240) ? 240 : theHeight;
    int offset = ((240 - height) / 2) * 80;

    // See if we can enable near pointers for updating the screen
    if(__djgpp_nearptr_enable())
    {
      // We've got near pointers enabled so let's use them
      uInt8* data = (uInt8*)(0xA0000 + __djgpp_conventional_base + offset)
          + (((160 - theWidth) / 2) * 2) / 4;

      // TODO: Rearrange this loop so we don't have to do as many calls to 
      // outp().  This is rather slow when the entire screen changes.
      for(uInt32 y = 0; y < height; ++y)
      {
        uInt8* screen = data;

        for(uInt32 x = 0; x < width; ++x)
        {
          if(*current != *previous)
          {
            uInt8* frame = (uInt8*)current;

            outp(0x3C4, 0x02);
            outp(0x3C5, 0x03);
            *screen = *frame;
            *(screen + 1) = *(frame + 2);

            outp(0x3C4, 0x02);
            outp(0x3C5, 0x0C);
            *screen = *(frame + 1);
            *(screen + 1) = *(frame + 3);
          }
          screen += 2;
          current++;
          previous++;
        }
        data += 80;
      }

      // Disable the near pointers
      __djgpp_nearptr_disable();
    }

    // TODO: I should handle the case where near pointers aren't available
  }
  else
  {
    uInt32 width = theWidth / 4;
    uInt32 height = (theHeight > 200) ? 200 : theHeight;
    int offset = ((200 - height) / 2) * 320;

    // See if we can enable near pointers for updating the screen
    if(__djgpp_nearptr_enable())
    {
      // We've got near pointers enabled so let's use them
      uInt16* data = (uInt16*)(0xA0000 + __djgpp_conventional_base + offset)
          + ((160 - theWidth) / 2);
 
      for(uInt32 y = 0; y < height; ++y)
      {
        uInt16* screen = data;
        data += 160;

        for(uInt32 x = 0; x < width; ++x)
        {
          if(*current != *previous)
          {
            uInt8* frame = (uInt8*)current;

            *screen++ = thePixelDataTable[*frame++];
            *screen++ = thePixelDataTable[*frame++];
            *screen++ = thePixelDataTable[*frame++];
            *screen++ = thePixelDataTable[*frame];
          }
          else
          {
            screen += 4;
          }
          current++;
          previous++;
        }
      }

      // Disable the near pointers
      __djgpp_nearptr_disable();
    }
    else
    {
      // Counldn't enable near pointers so we'll use a slower methods :-(
      uInt16* data = (uInt16*)(0xA0000 + offset) + ((160 - theWidth) / 2);
 
      for(uInt32 y = 0; y < height; ++y)
      {
        uInt16* screen = data;
        data += 160;

        for(uInt32 x = 0; x < width; ++x)
        {
          if(*current != *previous)
          {
            uInt8* frame = (uInt8*)current;

            _farpokew(_dos_ds, (uInt32)screen++, thePixelDataTable[*frame++]);
            _farpokew(_dos_ds, (uInt32)screen++, thePixelDataTable[*frame++]);
            _farpokew(_dos_ds, (uInt32)screen++, thePixelDataTable[*frame++]);
            _farpokew(_dos_ds, (uInt32)screen++, thePixelDataTable[*frame++]);
          }
          else
          {
            screen += 4;
          }
          current++;
          previous++;
        }
      }
    }
  }
}

/**
  This routine should be called regularly to handle events
*/
void handleEvents()
{
  union REGS regs;

  // Update paddles if we're using the mouse to emulate one
  if(thePaddleMode < 4)
  {
    // Update the paddle resistance and fire button based on the mouse settings
    regs.w.ax = 0x0003;
    int86(MOUSE_BIOS, &regs, &regs);
    Int32 resistance = (uInt32)((1000000.0 * (512 - regs.w.cx)) / 512);
  
    if(thePaddleMode == 0)
    {
      theEvent.set(Event::PaddleZeroResistance, resistance);
      theEvent.set(Event::PaddleZeroFire, (regs.w.bx & 0x07) ? 1 : 0);
    }
    else if(thePaddleMode == 1)
    {
      theEvent.set(Event::PaddleOneResistance, resistance);
      theEvent.set(Event::PaddleOneFire, (regs.w.bx & 0x07) ? 1 : 0);
    }
    else if(thePaddleMode == 2)
    {
      theEvent.set(Event::PaddleTwoResistance, resistance);
      theEvent.set(Event::PaddleTwoFire, (regs.w.bx & 0x07) ? 1 : 0);
    }
    else if(thePaddleMode == 3)
    {
      theEvent.set(Event::PaddleThreeResistance, resistance);
      theEvent.set(Event::PaddleThreeFire, (regs.w.bx & 0x07) ? 1 : 0);
    }
  }

  // If no joystick object is available create one
  if(theJoysticks == 0)
  {
    theJoysticks = new PCJoysticks(thePaddleMode != 4);
  }

  if(theJoysticks->present())
  {
    bool buttons[4];
    Int16 axes[4];

    theJoysticks->read(buttons, axes);

    theEvent.set(Event::JoystickZeroFire, buttons[0] ?
        1 : theKeyboardEvent.get(Event::JoystickZeroFire));

    theEvent.set(Event::BoosterGripZeroTrigger, buttons[1] ?
        1 : theKeyboardEvent.get(Event::BoosterGripZeroTrigger));

    theEvent.set(Event::JoystickZeroLeft, (axes[0] < -16384)  ?
        1 : theKeyboardEvent.get(Event::JoystickZeroLeft));

    theEvent.set(Event::JoystickZeroRight, (axes[0] > 16384) ?
        1 : theKeyboardEvent.get(Event::JoystickZeroRight));

    theEvent.set(Event::JoystickZeroUp, (axes[1] < -16384) ?
        1 : theKeyboardEvent.get(Event::JoystickZeroUp));

    theEvent.set(Event::JoystickZeroDown, (axes[1] > 16384) ?
        1 : theKeyboardEvent.get(Event::JoystickZeroDown));

    theEvent.set(Event::JoystickOneFire, buttons[2] ?
        1 : theKeyboardEvent.get(Event::JoystickOneFire));

    theEvent.set(Event::BoosterGripOneTrigger, buttons[3] ?
        1 : theKeyboardEvent.get(Event::BoosterGripOneTrigger));

    theEvent.set(Event::JoystickOneLeft, (axes[2] < -16384) ?
        1 : theKeyboardEvent.get(Event::JoystickOneLeft));

    theEvent.set(Event::JoystickOneRight, (axes[2] > 16384) ?
        1 : theKeyboardEvent.get(Event::JoystickOneRight));

    theEvent.set(Event::JoystickOneUp, (axes[3] < -16384) ?
        1 : theKeyboardEvent.get(Event::JoystickOneUp));

    theEvent.set(Event::JoystickOneDown, (axes[3] > 16384) ?
        1 : theKeyboardEvent.get(Event::JoystickOneDown));

    // If we're using real paddles then set paddle events as well
    if(thePaddleMode == 4)
    {
      uInt32 r;

      theEvent.set(Event::PaddleZeroFire, buttons[0]);
      r = (uInt32)((1.0E6L * (axes[0] + 32767L)) / 65536);
      theEvent.set(Event::PaddleZeroResistance, r);

      theEvent.set(Event::PaddleOneFire, buttons[1]);
      r = (uInt32)((1.0E6L * (axes[1] + 32767L)) / 65536);
      theEvent.set(Event::PaddleOneResistance, r);

      theEvent.set(Event::PaddleTwoFire, buttons[2]);
      r = (uInt32)((1.0E6L * (axes[2] + 32767L)) / 65536);
      theEvent.set(Event::PaddleTwoResistance, r);

      theEvent.set(Event::PaddleThreeFire, buttons[3]);
      r = (uInt32)((1.0E6L * (axes[3] + 32767L)) / 65536);
      theEvent.set(Event::PaddleThreeResistance, r);
    }
  }
}

/**
  This is the keyboard interrupt service routine.  It's called 
  whenever a key is pressed or released on the keyboard.
*/
static void keyboardInterruptServiceRoutine(void)
{
  struct Switches
  {
    uInt16 scanCode;
    Event::Type eventCode;
  };

  static Switches list[] = {
    { SCAN_1,         Event::KeyboardZero1 },
    { SCAN_2,         Event::KeyboardZero2 },
    { SCAN_3,         Event::KeyboardZero3 },
    { SCAN_Q,         Event::KeyboardZero4 },
    { SCAN_W,         Event::KeyboardZero5 },
    { SCAN_E,         Event::KeyboardZero6 },
    { SCAN_A,         Event::KeyboardZero7 },
    { SCAN_S,         Event::KeyboardZero8 },
    { SCAN_D,         Event::KeyboardZero9 },
    { SCAN_Z,         Event::KeyboardZeroStar },
    { SCAN_X,         Event::KeyboardZero0 },
    { SCAN_C,         Event::KeyboardZeroPound },

    { SCAN_8,         Event::KeyboardOne1 },
    { SCAN_9,         Event::KeyboardOne2 },
    { SCAN_0,         Event::KeyboardOne3 },
    { SCAN_I,         Event::KeyboardOne4 },
    { SCAN_O,         Event::KeyboardOne5 },
    { SCAN_P,         Event::KeyboardOne6 },
    { SCAN_K,         Event::KeyboardOne7 },
    { SCAN_L,         Event::KeyboardOne8 },
    { SCAN_SCOLON,    Event::KeyboardOne9 },
    { SCAN_COMMA,     Event::KeyboardOneStar },
    { SCAN_STOP,      Event::KeyboardOne0 },
    { SCAN_FSLASH,    Event::KeyboardOnePound },

    { SCAN_DOWN,      Event::JoystickZeroDown },
    { SCAN_UP,        Event::JoystickZeroUp },
    { SCAN_LEFT,      Event::JoystickZeroLeft },
    { SCAN_RIGHT,     Event::JoystickZeroRight },
    { SCAN_SPACE,     Event::JoystickZeroFire },
    { SCAN_Z,         Event::BoosterGripZeroTrigger },
    { SCAN_X,         Event::BoosterGripZeroBooster },
   
    { SCAN_W,         Event::JoystickZeroUp },
    { SCAN_S,         Event::JoystickZeroDown },
    { SCAN_A,         Event::JoystickZeroLeft },
    { SCAN_D,         Event::JoystickZeroRight },
    { SCAN_TAB,       Event::JoystickZeroFire },
    { SCAN_1,         Event::BoosterGripZeroTrigger },
    { SCAN_2,         Event::BoosterGripZeroBooster },
   
    { SCAN_L,         Event::JoystickOneDown },
    { SCAN_O,         Event::JoystickOneUp },
    { SCAN_K,         Event::JoystickOneLeft },
    { SCAN_SCOLON,    Event::JoystickOneRight },
    { SCAN_J,         Event::JoystickOneFire },
    { SCAN_N,         Event::BoosterGripOneTrigger },
    { SCAN_M,         Event::BoosterGripOneBooster },
   
    { SCAN_F1,        Event::ConsoleSelect },
    { SCAN_F2,        Event::ConsoleReset },
    { SCAN_F3,        Event::ConsoleColor },
    { SCAN_F4,        Event::ConsoleBlackWhite },
    { SCAN_F5,        Event::ConsoleLeftDifficultyA },
    { SCAN_F6,        Event::ConsoleLeftDifficultyB },
    { SCAN_F7,        Event::ConsoleRightDifficultyA },
    { SCAN_F8,        Event::ConsoleRightDifficultyB }
  };

  // Get the scan code of the key
  uInt8 scanCode = inportb(0x60);
  
  // If it is an 0xE0 or 0 then ignore it
  if((scanCode != 0xE0) && (scanCode != 0))
  {
    // See if the escape key has been pressed
    if((scanCode & 0x7f) == SCAN_ESC)
    {
      theQuitIndicator = true;
    }
    else
    {
      // Change the event state if needed
      for(unsigned int i = 0; i < sizeof(list) / sizeof(Switches); ++i)
      {
        if(list[i].scanCode == (scanCode & 0x7f))
        {
          theEvent.set(list[i].eventCode, (scanCode & 0x80) ? 0 : 1);
          theKeyboardEvent.set(list[i].eventCode, (scanCode & 0x80) ? 0 : 1);
        }
      }
    }
  }

  // Ack the interrupt
  outp(0x20, 0x20);
}

/**
  Display a usage message and exit the program
*/
void usage()
{
  static const char* message[] = {
    "",
    "Usage: stella [option ...] file",
    "",
    "Valid options are:",
    "",
    "  -fps <number>           Display the given number of frames per second",
    "  -modex                  Use 320x240 video mode instead of 320x200",
    "  -paddle <0|1|2|3|real>  Indicates which paddle the mouse should emulate",
    "                          or that real Atari 2600 paddles are being used",
    0
  };

  for(uInt32 i = 0; message[i]; ++i)
  {
    cerr << message[i] << endl;
  }
  exit(1);
}

/**
  Setup the properties set by loading from the file stella.pro

  @param set The properties set to setup
*/
void setupProperties(PropertiesSet& set)
{
  // Try to load the file stella.pro file
  string filename = "stella.pro";

  // File was opened so load properties from it
  set.load(filename, &Console::defaultProperties(), false);
}

/**
  Should be called to parse the command line arguments

  @param argc The count of command line arguments
  @param argv The command line arguments
*/
void handleCommandLineArguments(int argc, char* argv[])
{
  // Make sure we have the correct number of command line arguments
  if((argc < 2) || (argc > 7))
  {
    usage();
  }

  for(Int32 i = 1; i < (argc - 1); ++i)
  {
    // See which command line switch they're using
    if(string(argv[i]) == "-fps")
    {
      // They're setting the desired frame rate
      Int32 rate = atoi(argv[++i]);
      if((rate < 1) || (rate > 300))
      {
        rate = 60;
      }

      theDesiredFrameRate = rate;
    }
    else if(string(argv[i]) == "-paddle")
    {
      // They're setting the desired frame rate
      if(string(argv[i + 1]) == "real")
      {
        thePaddleMode = 4;
      }
      else
      {
        thePaddleMode = atoi(argv[i + 1]);
        if((thePaddleMode < 0) || (thePaddleMode > 3))
        {
          usage();
        }
      }
      ++i;
    }
    else if(string(argv[i]) == "-modex")
    {
      theUseModeXFlag = true;
    }
    else
    {
      usage();
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int main(int argc, char* argv[])
{
  // Handle the command line arguments
  handleCommandLineArguments(argc, argv);

  // Get a pointer to the file which contains the cartridge ROM
  const char* file = argv[argc - 1];

  // Open the cartridge image and read it in
  ifstream in;
  in.open(file, ios::in | ios::binary); 
  if(!in)
  {
    cerr << "ERROR: Couldn't open " << file << "..." << endl;
    exit(1);
  }

  uInt8* image = new uInt8[512 * 1024];
  in.read((char*)image, 512 * 1024);
  uInt32 size = in.gcount();
  in.close();

  // Create a properties set for us to use and set it up
  PropertiesSet propertiesSet;
  setupProperties(propertiesSet);

  // Create a sound object for use with the console
  SoundDOS sound;

  // Get just the filename of the file containing the ROM image
  const char* filename = (!strrchr(file, '\\')) ? 
      file : strrchr(file, '\\') + 1;

  // Create the 2600 game console
  theConsole = new Console(image, size, filename, 
      theEvent, propertiesSet, sound);

  // Free the image since we don't need it any longer
  delete[] image;

  startup();

  // Get the starting time in case we need to print statistics
  uclock_t startingTime = uclock();

  uInt32 numberOfFrames = 0;
  for( ; !theQuitIndicator ; ++numberOfFrames)
  {
    // Remember the current time before we start drawing the frame
    uclock_t before = uclock();

    // Ask the media source to prepare the next frame
    theConsole->mediaSource().update();

/*
    TODO: This code seems to work fine under mode 13, however, it slows
    the frame rate down under Mode X.  At any point it needs to be 
    tested on more video cards before it's ready for production :-)

    // If we're not behind schedule then let's wait for the VSYNC!
    static uclock_t endOfLastVsync = 0;
    if((theDesiredFrameRate <= 60) && 
        (uclock() - endOfLastVsync < (1000000 / theDesiredFrameRate)))
    {
      while(inp(0x3DA) & 0x08);
      while(!(inp(0x3DA) & 0x08));
    }
    endOfLastVsync = uclock();
*/
 
    // Update the display and handle events
    updateDisplay(theConsole->mediaSource());
    handleEvents();
    
    // Now, waste time if we need to so that we are at the desired frame rate
    for(;;)
    {
      uInt32 delta = uclock() - before;

      if(delta > (1000000 / theDesiredFrameRate))
      {
        break;
      }
    }
  }

  // Get the ending time in case we need to print statistics
  uclock_t endingTime = uclock();

  uInt32 scanlines = theConsole->mediaSource().scanlines();
  delete theConsole;
  shutdown();

  double executionTime = (endingTime - startingTime) / 1000000.0;
  double framesPerSecond = numberOfFrames / executionTime;

  cout << endl;
  cout << numberOfFrames << " total frames drawn\n";
  cout << framesPerSecond << " frames/second\n";
  cout << scanlines << " scanlines in last frame\n";
  cout << endl;
}

