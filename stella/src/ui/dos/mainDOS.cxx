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
// Copyright (c) 1995-2003 by Bradford W. Mott
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: mainDOS.cxx,v 1.10 2003-02-17 05:17:41 bwmott Exp $
//============================================================================

#include <go32.h>
#include <dpmi.h>
#include <sys/farptr.h>
#include <sys/nearptr.h>
#include <sys/stat.h>
#include <dos.h>
#include <pc.h>
#include <unistd.h>

#include <fstream>
#include <iostream>
#include <sstream>
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
#include "vga.hxx"

#define END_OF_FUNCTION(x) void x##_end(void) {}
#define END_OF_STATIC_FUNCTION(x) static void x##_end(void) {}
#define LOCK_VARIABLE(x) _go32_dpmi_lock_data((void*)&x, sizeof(x))
#define LOCK_FUNCTION(x) _go32_dpmi_lock_code((void*)x, (long)x##_end - (long)x)

// Pointer to the console object or the null pointer
Console* theConsole;

// Event objects to use
Event theEvent;
Event theKeyboardEvent;

// Array of flags for each keyboard key-code
volatile bool theKeyboardKeyState[128];

// Used to ignore some number of key codes
volatile uInt32 theNumOfKeyCodesToIgnore;

// An alternate properties file to use
string theAlternateProFile = "";

// Indicates if the entire frame should be redrawn
bool theRedrawEntireFrameFlag = true;

// Indicates if the user wants to quit
bool theQuitIndicator = false;

// Indicates if the emulator should be paused
bool thePauseIndicator = false;

// Indicates whether to show some game info on program exit
bool theShowInfoFlag = false;

// Indicates what the desired frame rate is
uInt32 theDesiredFrameRate = 60;

// Indicate which paddle mode we're using:
//   0 - Mouse emulates paddle 0
//   1 - Mouse emulates paddle 1
//   2 - Mouse emulates paddle 2
//   3 - Mouse emulates paddle 3
//   4 - Use real Atari 2600 paddles
uInt32 thePaddleMode = 0;

// Indicates if emulation should synchronize with video instead of system timer
bool theSynchronizeVideoFlag = false;

// Indicates if sound should be enabled or not
bool theSoundEnabledFlag = true;

// Indicates if the Mode X graphics should be used or not
bool theUseModeXFlag = false;

// Pointer to the joysticks object
PCJoysticks* theJoysticks;

#define MESSAGE_INTERVAL     2

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

// Indicates the current state to use for state saving
static uInt32 theCurrentState = 0;

// The locations for various required files
static string theHomeDir;
static string theStateDir;

/**
  Return true if the program is executing in a NT DOS virtual machine.
*/
bool isWindowsNt()
{
  const char* p = getenv("OS");

  if(((p) && (stricmp(p, "Windows_NT") == 0)) ||
      (_get_dos_version(1) == 0x0532))
  {
    return true;
  }
  else
  {
    return false;
  }
}

/**
  Changes the current state slot.
*/
void changeState(uInt32 change)
{
  // Calculate new state slot
  theCurrentState = (theCurrentState + ((change == 0) ? 9 : 1)) % 10;

  // Print appropriate message
  ostringstream buf;
  buf << "Changed to slot " << theCurrentState;
  string message = buf.str();
  theConsole->mediaSource().showMessage(message,
      MESSAGE_INTERVAL * theDesiredFrameRate);
}

/**
  Saves state of the current game in the current slot.
*/
void saveState()
{
  ostringstream buf;
  string md5 = theConsole->properties().get("Cartridge.MD5");

  string sub1 = theStateDir + '\\' + md5.substr(0, 8);
  string sub2 = sub1 + '\\' + md5.substr(8, 8);
  string sub3 = sub2 + '\\' + md5.substr(16, 8);
  buf << sub3 << '\\' << md5.substr(24, 8) << ".st" << theCurrentState;
  string filename = buf.str();

  // If needed create the sub-directory corresponding to the characters
  // of the MD5 checksum
  if(access(sub1.c_str(), F_OK) != 0)
  {
    if(mkdir(sub1.c_str(), S_IWUSR) != 0)
    {
      theConsole->mediaSource().showMessage(string() = "Error Saving State",
          MESSAGE_INTERVAL * theDesiredFrameRate);
      return;
    }
  }

  if(access(sub2.c_str(), F_OK) != 0)
  {
    if(mkdir(sub2.c_str(), S_IWUSR) != 0)
    {
      theConsole->mediaSource().showMessage(string() = "Error Saving State",
          MESSAGE_INTERVAL * theDesiredFrameRate);
      return;
    }
  }

  if(access(sub3.c_str(), F_OK) != 0)
  {
    if(mkdir(sub3.c_str(), S_IWUSR) != 0)
    {
      theConsole->mediaSource().showMessage(string() = "Error Saving State",
          MESSAGE_INTERVAL * theDesiredFrameRate);
      return;
    }
  }

  // Do a state save using the System
  int result = theConsole->system().saveState(filename, md5);

  // Print appropriate message
  buf.str("");
  if(result == 1)
    buf << "State " << theCurrentState << " saved";
  else if(result == 2)
    buf << "Error saving state " << theCurrentState;
  else if(result == 3)
    buf << "Invalid state " << theCurrentState << " file";

  string message = buf.str();
  theConsole->mediaSource().showMessage(message, MESSAGE_INTERVAL *
      theDesiredFrameRate);
}

/**
  Loads state from the current slot for the current game.
*/
void loadState()
{
  ostringstream buf;
  string md5 = theConsole->properties().get("Cartridge.MD5");

  string sub1 = theStateDir + '\\' + md5.substr(0, 8);
  string sub2 = sub1 + '\\' + md5.substr(8, 8);
  string sub3 = sub2 + '\\' + md5.substr(16, 8);
  buf << sub3 << '\\' << md5.substr(24, 8) << ".st" << theCurrentState;
  string filename = buf.str();

  // Do a state load using the System
  int result = theConsole->system().loadState(filename, md5);

  // Print appropriate message
  buf.str("");
  if(result == 1)
    buf << "State " << theCurrentState << " loaded";
  else if(result == 2)
    buf << "Error loading state " << theCurrentState;
  else if(result == 3)
    buf << "Invalid state " << theCurrentState << " file";

  string message = buf.str();
  theConsole->mediaSource().showMessage(message, MESSAGE_INTERVAL *
      theDesiredFrameRate);
}

/**
  This is the keyboard interrupt service routine.  It's called 
  whenever a key is pressed or released on the keyboard.
*/
static void keyboardInterruptServiceRoutine(void)
{
  // Get the scan code of the key
  uInt8 code = inportb(0x60);

  // Are we ignoring some key codes?
  if(theNumOfKeyCodesToIgnore > 0)
  {
    --theNumOfKeyCodesToIgnore;
  }
  // Handle the pause key
  else if(code == 0xE1)
  {
    // Toggle the state of the pause key.  The pause key only sends a "make"
    // code it does not send a "break" code.  Also the "make" code is the
    // sequence 0xE1, 0x1D, 0x45, 0xE1, 0x9D, 0xC5 so we'll need to skip the
    // remaining 5 values in the sequence.
    theKeyboardKeyState[SCAN_PAUSE] = !theKeyboardKeyState[SCAN_PAUSE];
    theNumOfKeyCodesToIgnore = 5;
  }
  // Handle the "extended" and the "error" key codes
  else if((code == 0xE0) || (code == 0x00))
  {
    // Currently, we ignore the "extended" and "error" key codes.  We should
    // probably modify the "extended" key code support so that we can identify
    // the extended keys...
  }
  else
  {
    // Update the state of the key
    theKeyboardKeyState[code & 0x7F] = !(code & 0x80);
  }

  // Ack the interrupt
  outp(0x20, 0x20);
}
END_OF_STATIC_FUNCTION(keyboardInterruptServiceRoutine);

/**
  This routine should be called once the console is created to setup
  the graphics mode 
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

  // Setup VGA graphics mode
  if(theUseModeXFlag)
  {
    VgaSetMode(VGA_320_240_60HZ);

    // Clear the screen
    outp(0x3C4, 0x02);
    outp(0x3C5, 0x0F);
    for(uInt32 i = 0; i < 240 * 80; ++i)
    {
      _farpokeb(_dos_ds, 0xA0000 + i, 0);
    }
  }
  else
  {
    VgaSetMode(VGA_320_200_60HZ);

    // Clear the screen
    for(uInt32 i = 0; i < 320 * 200; ++i)
    {
      _farpokew(_dos_ds, 0xA0000 + i, 0);
    }
  }

  // Setup color palette for the video card
  const uInt32* palette = theConsole->mediaSource().palette();
  outp(VGA_PEL_ADDRESS, 0);
  for(int index = 0; index < 256; index++)
  {
    outp(VGA_PEL_DATA, (palette[index] & 0x00ff0000) >> 18);
    outp(VGA_PEL_DATA, (palette[index] & 0x0000ff00) >> 10);
    outp(VGA_PEL_DATA, (palette[index] & 0x000000ff) >> 2);
  }

  // Install keyboard interrupt handler
  LOCK_VARIABLE(theKeyboardKeyState);
  LOCK_VARIABLE(theNumOfKeyCodesToIgnore);
  LOCK_FUNCTION(keyboardInterruptServiceRoutine);
  for(uInt32 k = 0; k < 128; ++k)
  {
    theKeyboardKeyState[k] = false;
  }
  theNumOfKeyCodesToIgnore = 0;
  disable();
  _go32_dpmi_get_protected_mode_interrupt_vector(0x09, &theOldKeyboardHandler);
  theKeyboardHandler.pm_selector = _go32_my_cs();
  theKeyboardHandler.pm_offset = (long)(&keyboardInterruptServiceRoutine);
  _go32_dpmi_allocate_iret_wrapper(&theKeyboardHandler);
  _go32_dpmi_set_protected_mode_interrupt_vector(0x09, &theKeyboardHandler);
  enable();

  // Initialize mouse handler via DOS interrupt
  regs.w.ax = 0x0000;
  int86(MOUSE_BIOS, &regs, &regs);

  if(regs.w.ax != 0x0000)
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
    else
    {
      // Counldn't enable near pointers so we'll use a slower methods :-(
      uInt8* data = (uInt8*)(0xA0000 + offset) 
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
            _farpokeb(_dos_ds, (uInt32)screen, *frame);
            _farpokeb(_dos_ds, (uInt32)(screen + 1), *(frame + 2));

            outp(0x3C4, 0x02);
            outp(0x3C5, 0x0C);
            _farpokeb(_dos_ds, (uInt32)screen, *(frame + 1));
            _farpokeb(_dos_ds, (uInt32)(screen + 1), *(frame + 3));
          }
          screen += 2;
          current++;
          previous++;
        }
        data += 80;
      }
    }
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
  This routine is called by the updateEvents routine to handle updated
  the events based on the current keyboard state.
*/
void updateEventsUsingKeyboardState()
{
  struct Switches
  {
    uInt16 scanCode;
    Event::Type eventCode;
    string message;
  };

  static Switches list[] = {
    { SCAN_1,         Event::KeyboardZero1,             "" },
    { SCAN_2,         Event::KeyboardZero2,             "" },
    { SCAN_3,         Event::KeyboardZero3,             "" },
    { SCAN_Q,         Event::KeyboardZero4,             "" },
    { SCAN_W,         Event::KeyboardZero5,             "" },
    { SCAN_E,         Event::KeyboardZero6,             "" },
    { SCAN_A,         Event::KeyboardZero7,             "" },
    { SCAN_S,         Event::KeyboardZero8,             "" },
    { SCAN_D,         Event::KeyboardZero9,             "" },
    { SCAN_Z,         Event::KeyboardZeroStar,          "" },
    { SCAN_X,         Event::KeyboardZero0,             "" },
    { SCAN_C,         Event::KeyboardZeroPound,         "" },

    { SCAN_8,         Event::KeyboardOne1,              "" },
    { SCAN_9,         Event::KeyboardOne2,              "" },
    { SCAN_0,         Event::KeyboardOne3,              "" },
    { SCAN_I,         Event::KeyboardOne4,              "" },
    { SCAN_O,         Event::KeyboardOne5,              "" },
    { SCAN_P,         Event::KeyboardOne6,              "" },
    { SCAN_K,         Event::KeyboardOne7,              "" },
    { SCAN_L,         Event::KeyboardOne8,              "" },
    { SCAN_SCOLON,    Event::KeyboardOne9,              "" },
    { SCAN_COMMA,     Event::KeyboardOneStar,           "" },
    { SCAN_STOP,      Event::KeyboardOne0,              "" },
    { SCAN_FSLASH,    Event::KeyboardOnePound,          "" },

    { SCAN_DOWN,      Event::JoystickZeroDown,          "" },
    { SCAN_UP,        Event::JoystickZeroUp,            "" },
    { SCAN_LEFT,      Event::JoystickZeroLeft,          "" },
    { SCAN_RIGHT,     Event::JoystickZeroRight,         "" },
    { SCAN_SPACE,     Event::JoystickZeroFire,          "" },
    { SCAN_Z,         Event::BoosterGripZeroTrigger,    "" },
    { SCAN_X,         Event::BoosterGripZeroBooster,    "" },
   
    { SCAN_W,         Event::JoystickZeroUp,            "" },
    { SCAN_S,         Event::JoystickZeroDown,          "" },
    { SCAN_A,         Event::JoystickZeroLeft,          "" },
    { SCAN_D,         Event::JoystickZeroRight,         "" },
    { SCAN_TAB,       Event::JoystickZeroFire,          "" },
    { SCAN_1,         Event::BoosterGripZeroTrigger,    "" },
    { SCAN_2,         Event::BoosterGripZeroBooster,    "" },
   
    { SCAN_L,         Event::JoystickOneDown,           "" },
    { SCAN_O,         Event::JoystickOneUp,             "" },
    { SCAN_K,         Event::JoystickOneLeft,           "" },
    { SCAN_SCOLON,    Event::JoystickOneRight,          "" },
    { SCAN_J,         Event::JoystickOneFire,           "" },
    { SCAN_N,         Event::BoosterGripOneTrigger,     "" },
    { SCAN_M,         Event::BoosterGripOneBooster,     "" },
   
    { SCAN_F1,        Event::ConsoleSelect,             "" },
    { SCAN_F2,        Event::ConsoleReset,              "" },
    { SCAN_F3,        Event::ConsoleColor,              "Color Mode" },
    { SCAN_F4,        Event::ConsoleBlackWhite,         "BW Mode" },
    { SCAN_F5,        Event::ConsoleLeftDifficultyA,    "Left Difficulty A" },
    { SCAN_F6,        Event::ConsoleLeftDifficultyB,    "Left Difficulty B" },
    { SCAN_F7,        Event::ConsoleRightDifficultyA,   "Right Difficulty A" },
    { SCAN_F8,        Event::ConsoleRightDifficultyB,   "Right Difficulty B" }
  };

  // Handle pausing the emulator
  if((!thePauseIndicator) && (theKeyboardKeyState[SCAN_PAUSE]))
  {
    thePauseIndicator = true;
    theConsole->mediaSource().pause(true);
  }
  else if(thePauseIndicator && (!theKeyboardKeyState[SCAN_PAUSE]))
  {
    thePauseIndicator = false;
    theConsole->mediaSource().pause(false);
  }

  // Handle quiting the emulator
  if(theKeyboardKeyState[SCAN_ESC])
  {
    theQuitIndicator = true;
  }

  // Handle switching save state slots
  static bool changedState = false;
  if((theKeyboardKeyState[SCAN_F10]) && !changedState)
  {
    if(theKeyboardKeyState[SCAN_LSHIFT] || theKeyboardKeyState[SCAN_RSHIFT])
    {
      changeState(0);
    }
    else
    {
      changeState(1);
    }
    changedState = true;
  }
  else if(!theKeyboardKeyState[SCAN_F10])
  {
    changedState = false;
  }
 
  static bool savedState = false;
  if(!savedState && theKeyboardKeyState[SCAN_F9])
  {
    saveState();
    savedState = true;
  }
  else if(!theKeyboardKeyState[SCAN_F9])
  {
    savedState = false;
  }
 
  static bool loadedState = false;
  if(!loadedState && theKeyboardKeyState[SCAN_F11])
  {
    loadState();
    loadedState = true;
  }
  else if(!theKeyboardKeyState[SCAN_F11])
  {
    loadedState = false;
  }

  // First we clear all of the keyboard events
  for(unsigned int k = 0; k < sizeof(list) / sizeof(Switches); ++k)
  {
    theKeyboardEvent.set(list[k].eventCode, 0);
  }

  // Now, change the event state if needed for each event
  for(unsigned int i = 0; i < sizeof(list) / sizeof(Switches); ++i)
  {
    if(theKeyboardKeyState[list[i].scanCode])
    {
      if(theKeyboardEvent.get(list[i].eventCode) == 0)
      {
        theEvent.set(list[i].eventCode, 1);
        theKeyboardEvent.set(list[i].eventCode, 1);
        if(list[i].message != "")
        {
          theConsole->mediaSource().showMessage(list[i].message,
              2 * theDesiredFrameRate);
        } 
      }
    }
    else
    {
      if(theKeyboardEvent.get(list[i].eventCode) == 0)
      {
        theEvent.set(list[i].eventCode, 0);
        theKeyboardEvent.set(list[i].eventCode, 0);
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

  // Update events based on keyboard state
  updateEventsUsingKeyboardState();

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
  Ensure that the necessary directories are created for Stella under
  STELLA_HOME or the current working directory if STELLA_HOME is not
  defined.

  Required directories are $STELLA_HOME/state.
  This must be called before any other function.
*/
bool setupDirs()
{
  // Get the directory to use
  theHomeDir = getenv("STELLA_HOME");
  if(theHomeDir == "")
  {
    theHomeDir = ".";
  }

  // Remove any trailing backslashes
  while((theHomeDir.length() >= 1) &&
      (theHomeDir[theHomeDir.length() - 1] == '\\'))
  {
    theHomeDir = theHomeDir.substr(0, theHomeDir.length() - 1);
  }

  // Create state saving directory if needed
  theStateDir = theHomeDir + "\\state";
  if(access(theStateDir.c_str(), F_OK) != 0)
  {
    if(mkdir(theStateDir.c_str(), S_IWUSR) != 0)
    {
      cerr << "ERROR: Creating state save directory " 
          << theStateDir << endl;
      return false;
    }
  }

  if(access(theStateDir.c_str(), R_OK | W_OK | X_OK | D_OK) != 0)
  {
    cerr << "ERROR: Access not allowed to the state save directory "
          << theStateDir << endl;
    return false;
  }

  return true;
}

/**
  Display a usage message and exit the program
*/
void usage()
{
  static const char* message[] = {
    "",
    "Stella for DOS version 1.3",
    "",
    "Usage: stella [option ...] file",
    "",
    "Valid options are:",
    "",
    "  -fps <number>           Display the given number of frames per second",
    "  -modex                  Use 320x240 video mode instead of 320x200",
    "  -nosound                Disables audio output",
    "  -paddle <0|1|2|3|real>  Indicates which paddle the mouse should emulate",
    "                          or that real Atari 2600 paddles are being used",
    "  -pro <props file>       Use given properties file instead of stella.pro",
    "  -showinfo               Show some game info on exit",
    "  -vsync                  Synchronize with video instead of system timer",
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
bool setupProperties(PropertiesSet& set)
{
  // Try to load the properties file from either the current working
  // directory or the $STELLA_HOME directory
  string filename1 = (theAlternateProFile != "") ? theAlternateProFile :
      "stella.pro";
  string filename2 = theHomeDir + '\\' + filename1;

  if(access(filename1.c_str(), R_OK | F_OK) == 0)
  {
    // File is accessible so load properties from it
    set.load(filename1, &Console::defaultProperties(), false);
    return true;
  }
  else if(access(filename2.c_str(), R_OK | F_OK) == 0)
  {
    // File is accessible so load properties from it
    set.load(filename2, &Console::defaultProperties(), false);
    return true;
  }
  else
  {
    set.load("", &Console::defaultProperties(), false);
    return true;
  }
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
    else if(string(argv[i]) == "-nosound")
    {
      theSoundEnabledFlag = false;
    }
    else if(string(argv[i]) == "-modex")
    {
      theUseModeXFlag = true;
    }
    else if(string(argv[i]) == "-showinfo")
    {
      theShowInfoFlag = true;
    }
    else if(string(argv[i]) == "-pro")
    {
      theAlternateProFile = argv[++i];
    }
    else if(string(argv[i]) == "-vsync")
    {
      theSynchronizeVideoFlag = true;
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
  // Find out if we're running in an NT DOS virtual machine
  bool windowsNtFlag = isWindowsNt();

  // First set up the directories where Stella will find RC and state files
  if(!setupDirs())
  {
    return 0;
  }

  // Handle the command line arguments
  handleCommandLineArguments(argc, argv);

  // Get a pointer to the file which contains the cartridge ROM
  const char* file = argv[argc - 1];

  // Open the cartridge image and read it in.  The cartridge image is
  // searched for in the current working directory, the $STELLA_HOME\ROMS
  // directory, and finally the $STELLA_HOME directory.
  string file1(file);
  string file2(theHomeDir + "\\ROMS\\" + file1);
  string file3(theHomeDir + '\\' + file1);

  ifstream in; 
  in.open(file1.c_str(), ios::in | ios::binary); 
  if(!in)
  {
    in.close();
    in.clear();
    in.open(file2.c_str(), ios::in | ios::binary);
    if(!in)
    {
      in.close();
      in.clear();
      in.open(file3.c_str(), ios::in | ios::binary);
      if(!in)
      {
        cerr << "ERROR: Couldn't locate " << file << "..." << endl;
        exit(1);
      }
    }
  }

  uInt8* image = new uInt8[512 * 1024];
  in.read((char*)image, 512 * 1024);
  uInt32 size = in.gcount();
  in.close();

  // Create a properties set for us to use and set it up
  PropertiesSet propertiesSet;
  if(!setupProperties(propertiesSet))
  {
    delete[] image;
    exit(1);
  }

  // Get just the filename of the file containing the ROM image
  const char* filename = (!strrchr(file, '\\')) ? 
      file : strrchr(file, '\\') + 1;

  // Create a sound object for use with the console
  SoundDOS sound(theSoundEnabledFlag);
//  sound.setSoundVolume(settings->theDesiredVolume);

  // Create the 2600 game console
  theConsole = new Console(image, size, filename, 
      theEvent, propertiesSet, sound.getSampleRate());

  // Free the image since we don't need it any longer
  delete[] image;

  startup();

  // Get the starting time in case we need to print statistics
  clock_t startingTime = clock();
 
  uInt32 numberOfFrames = 0;
  for( ; !theQuitIndicator ; ++numberOfFrames)
  {
    // Remember the current time before we start drawing the frame
    uclock_t startTimeStamp = uclock();

    // Ask the media source to prepare the next frame
    if(!thePauseIndicator)
    {
      theConsole->mediaSource().update();
      sound.mute(false);
      sound.updateSound(theConsole->mediaSource());
    }
    else
    {
      sound.mute(true);
    }

    // If vsync is selected or we're running under NT then wait for VSYNC
    if(windowsNtFlag || theSynchronizeVideoFlag)
    { 
      // Wait until previous retrace has ended
      while(inp(0x3DA) & 0x08);

      // Wait until next retrace has begun
      while(!(inp(0x3DA) & 0x08));
    }

    // Update the display and handle events
    updateDisplay(theConsole->mediaSource());
    handleEvents();
 
    // Waste time if we need to so that we are at the desired frame rate
    if(!(windowsNtFlag || theSynchronizeVideoFlag))
    {
      for(;;)
      {
        uclock_t endTimeStamp = uclock();
        long long delta = endTimeStamp - startTimeStamp;
        if(delta >= (UCLOCKS_PER_SEC / theDesiredFrameRate))
        {
          break;
        }
      }
    }
  }

  // Get the ending time in case we need to print statistics
  clock_t endingTime = clock();

  // Close the sound device
  sound.close();

  uInt32 scanlines = theConsole->mediaSource().scanlines();
  string cartName = theConsole->properties().get("Cartridge.Name");
  string cartMD5 = theConsole->properties().get("Cartridge.MD5");
  delete theConsole;
  shutdown();

  if(theShowInfoFlag)
  {
    double executionTime = (endingTime - startingTime) / (double)CLOCKS_PER_SEC;
    double framesPerSecond = numberOfFrames / executionTime;

    cout << endl;
    cout << numberOfFrames << " total frames drawn\n";
    cout << framesPerSecond << " frames/second\n";
    cout << scanlines << " scanlines in last frame\n";
    cout << endl;
    cout << "Cartridge Name: " << cartName << endl;
    cout << "Cartridge MD5:  " << cartMD5 << endl;
    cout << endl;
    cout << endl;
  }
}

