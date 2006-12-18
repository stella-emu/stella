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
// Copyright (c) 1995-2006 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: Console.hxx,v 1.51 2006-12-18 16:44:39 stephena Exp $
//============================================================================

#ifndef CONSOLE_HXX
#define CONSOLE_HXX

class Console;
class Controller;
class Event;
class MediaSource;
class Switches;
class System;

#include "bspf.hxx"
#include "Control.hxx"
#include "Props.hxx"
#include "TIA.hxx"
#include "Cart.hxx"
#include "M6532.hxx"
#include "AtariVox.hxx"

/**
  This class represents the entire game console.

  @author  Bradford W. Mott
  @version $Id: Console.hxx,v 1.51 2006-12-18 16:44:39 stephena Exp $
*/
class Console
{
  public:
    /**
      Create a new console for emulating the specified game using the
      given game image and operating system.

      @param image       The ROM image of the game to emulate
      @param size        The size of the ROM image  
      @param md5         The md5 of the ROM image
      @param osystem     The OSystem object to use
    */
    Console(const uInt8* image, uInt32 size, const string& md5,
            OSystem* osystem);

    /**
      Create a new console object by copying another one

      @param console The object to copy
    */
    Console(const Console& console);
 
    /**
      Destructor
    */
    virtual ~Console();

  public:
    /**
      Get the controller plugged into the specified jack

      @return The specified controller
    */
    Controller& controller(Controller::Jack jack) const
    {
      return (jack == Controller::Left) ? *myControllers[0] : *myControllers[1];
    }

    /**
      Get the MediaSource for this console

      @return The mediasource
    */
    MediaSource& mediaSource() const { return *myMediaSource; }

    /**
      Get the properties being used by the game

      @return The properties being used by the game
    */
    const Properties& properties() const;

    /**
      Get the console switches

      @return The console switches
    */
    Switches& switches() const { return *mySwitches; }

    /**
      Get the 6502 based system used by the console to emulate the game

      @return The 6502 based system
    */
    System& system() const { return *mySystem; }

    /**
      Get the cartridge used by the console which contains the ROM code

      @return The cartridge for this console
    */
    Cartridge& cartridge() const { return *myCart; }

    /**
      Get the 6532 used by the console

      @return The 6532 for this console
    */
    M6532& riot() const { return *myRiot; }

    /**
      Set the properties to those given

      @param The properties to use for the current game
    */
    void setProperties(const Properties& props);

  public:
    /**
      Overloaded assignment operator

      @param console The console object to set myself equal to
      @return Myself after assignment has taken place
    */
    Console& operator = (const Console& console);

  public:
    /**
      Toggle between NTSC and PAL mode.
    */
    void toggleFormat();

    /**
      Toggle between the available palettes.
    */
    void togglePalette();

    /**
      Sets the palette according to the given palette name.

      @param palette  The palette to switch to.
    */
    void setPalette(const string& palette);

    /**
      Toggles phosphor effect.
    */
    void togglePhosphor();

    /**
      Initialize the basic properties of the console.
      TODO - This is a workaround for a bug in the TIA rendering, whereby
      XStart/YStart values cause incorrect coordinates to be passed to the
      in-game GUI rendering.
    */
    void initialize();

    /**
      Determine whether the console was successfully initialized
    */
    bool isInitialized() { return myIsInitializedFlag; }

    /**
      Initialize the video subsystem wrt this class.
    */
    void initializeVideo();

    /**
      Initialize the audio subsystem wrt this class.
    */
    void initializeAudio();

    /**
      Sets the number of sound channels

      @param channels  Number of channels (indicates stereo or mono)
    */
    void setChannels(int channels);

    /**
      "Fry" the Atari (mangle memory/TIA contents)
    */
    void fry();

    /**
      Change the "Display.XStart" variable.  Currently, a system reset is issued
      after the change.  GUI's may need to resize their viewports.

      @param direction +1 indicates increase, -1 indicates decrease.
    */
    void changeXStart(int direction);

    /**
      Change the "Display.XStart" variable.  Currently, a system reset is issued
      after the change.  GUI's may need to resize their viewports.

      @param direction +1 indicates increase, -1 indicates decrease.
    */
    void changeYStart(int direction);

    /**
      Toggles the TIA bit specified in the method name.
    */
    void toggleP0Bit() { toggleTIABit(TIA::P0, "P0"); }
    void toggleP1Bit() { toggleTIABit(TIA::P1, "P1"); }
    void toggleM0Bit() { toggleTIABit(TIA::M0, "M0"); }
    void toggleM1Bit() { toggleTIABit(TIA::M1, "M1"); }
    void toggleBLBit() { toggleTIABit(TIA::BL, "BL"); }
    void togglePFBit() { toggleTIABit(TIA::PF, "PF"); }
    void enableBits(bool enable);

#ifdef ATARIVOX_SUPPORT
    AtariVox *atariVox() { return vox; }
#endif

  private:
    void toggleTIABit(TIA::TIABit bit, const string& bitname, bool show = true);
    void setDeveloperProperties();

    /**
      Loads a user-defined palette file from 'stella.pal', filling the
      appropriate user-defined palette arrays.
    */
    void loadUserPalette();

    /**
      Returns a pointer to the palette data for the palette currently defined
      by the ROM properties.
    */
    const uInt32* getPalette(int direction) const;

  private:
    // Pointer to the osystem object
    OSystem* myOSystem;

    // Pointers to the left and right controllers
    Controller* myControllers[2];

    // Pointer to the event object to use
    Event* myEvent;

    // Pointer to the media source object 
    MediaSource* myMediaSource;

    // Properties for the game
    Properties myProperties; 

    // Pointer to the switches on the front of the console
    Switches* mySwitches;
 
    // Pointer to the 6502 based system being emulated 
    System* mySystem;

    // Pointer to the Cartridge (the debugger needs it)
    Cartridge *myCart;

    // Pointer to the 6532 (aka RIOT) (the debugger needs it)
    // A RIOT of my own! (...with apologies to The Clash...)
    M6532 *myRiot;

#ifdef ATARIVOX_SUPPORT
    AtariVox *vox;
#endif

    // Indicates whether the console was correctly initialized
    // We don't really care why it wasn't initialized ...
    bool myIsInitializedFlag;

    // Indicates whether an external palette was found and
    // successfully loaded
    bool myUserPaletteDefined;

    // User-defined NTSC and PAL RGB values
    uInt32* ourUserNTSCPalette;
    uInt32* ourUserPALPalette;

    // Table of RGB values for NTSC
    static const uInt32 ourNTSCPalette[256];

    // Table of RGB values for PAL.  NOTE: The odd numbered entries in
    // this array are always shades of grey.  This is used to implement
    // the PAL color loss effect.
    static const uInt32 ourPALPalette[256];

    // Table of RGB values for NTSC - Stella 1.1 version
    static const uInt32 ourNTSCPalette11[256];

    // Table of RGB values for PAL - Stella 1.1 version
    static const uInt32 ourPALPalette11[256];

    // Table of RGB values for NTSC - Z26 version
    static const uInt32 ourNTSCPaletteZ26[256];

    // Table of RGB values for PAL - Z26 version
    static const uInt32 ourPALPaletteZ26[256];
};

#endif
