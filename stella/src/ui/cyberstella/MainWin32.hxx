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
// Copyright (c) 1995-2002 by Bradford W. Mott
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: MainWin32.hxx,v 1.4 2003-11-19 21:06:27 stephena Exp $
//============================================================================

#ifndef MAIN_WIN32_HXX
#define MAIN_WIN32_HXX

class Console;
class FrameBufferWin32;
class MediaSource;
class PropertiesSet;
class Sound;
class Settings;
class DirectInput;

#include "GlobalData.hxx"
#include "FrameBuffer.hxx"
#include "Event.hxx"
#include "StellaEvent.hxx"

/**
  This class implements a main-like method where all per-game
  instantiation is done.  Event gathering is also done here.

  This class is meant to be quite similar to the mainDOS or mainSDL
  classes so that all platforms have a main-like method as described
  in the Porting.txt document

  @author  Stephen Anthony
  @version $Id: MainWin32.hxx,v 1.4 2003-11-19 21:06:27 stephena Exp $
*/
class MainWin32
{
  public:
    /**
	  Create a new instance of the emulation core for the specified
      rom image.

      @param image       The ROM image of the game to emulate 
      @param size        The size of the ROM image
      @param filename    The name of the file that contained the ROM image
      @param settings    The settings object to use
      @param properties  The game profiles object to use
     */
    MainWin32(const uInt8* image, uInt32 size, const char* filename,
              Settings& settings, PropertiesSet& properties);
    /**
      Destructor
    */ 
    virtual ~MainWin32();

    // Start the main emulation loop
    DWORD run();

  private:
    void UpdateEvents();
    void cleanup();

  private:
    // Pointer to the console object
    Console* theConsole;

    // Reference to the settings object
    Settings& theSettings;

    // Reference to the properties set object
    PropertiesSet& thePropertiesSet;

    // Pointer to the display object
    FrameBufferWin32* theDisplay;

    // Pointer to the sound object
    Sound* theSound;

    // Pointer to the input object
    DirectInput* theInput;

    struct Switches
    {
      uInt32 nVirtKey;
      StellaEvent::KeyCode keyCode;
    };
    static Switches keyList[StellaEvent::LastKCODE];

    // Lookup tables for joystick numbers and events
    static StellaEvent::JoyStick joyList[StellaEvent::LastJSTICK];
    static StellaEvent::JoyCode  joyButtonList[StellaEvent::LastJCODE-4];

    // Indicates the current mouse position in the X direction
    Int32 theMouseX;

    // Indicates the current paddle mode
    uInt32 thePaddleMode;

    // Indicates that all subsystems were initialized
    bool myIsInitialized;
};

#endif
