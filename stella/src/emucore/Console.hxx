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
// $Id: Console.hxx,v 1.7 2003-09-04 16:50:48 stephena Exp $
//============================================================================

#ifndef CONSOLE_HXX
#define CONSOLE_HXX

class Console;
class Controller;
class Event;
class EventHandler;
class MediaSource;
class PropertiesSet;
class Settings;
class Sound;
class Switches;
class System;

#include "bspf.hxx"
#include "Control.hxx"
#include "Props.hxx"

/**
  This class represents the entire game console.

  @author  Bradford W. Mott
  @version $Id: Console.hxx,v 1.7 2003-09-04 16:50:48 stephena Exp $
*/
class Console
{
  public:
    /**
      Create a new console for emulating the specified game using the
      given event object and game profiles.

      @param image The ROM image of the game to emulate
      @param size The size of the ROM image  
      @param filename The name of the file that contained the ROM image
      @param rcsettings The settings object to use
      @param profiles The game profiles object to use
      @param sampleRate The rate to create audio samples at
      @param userDefinedProperties User properties that should override the defaults
    */
    Console(const uInt8* image, uInt32 size, const char* filename,
        Settings& rcsettings, PropertiesSet& propertiesSet, uInt32 sampleRate,
        const Properties* userDefinedProperties = 0);

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
      Get the media source of the console

      @return The media source
    */
    MediaSource& mediaSource() const
    {
      return *myMediaSource;
    }

    /**
      Get the properties being used by the game

      @return The properties being used by the game
    */
    const Properties& properties() const;

    /**
      Get the settings of the console

      @return The settings for this console
    */
    const Settings& settings() const;

    /**
      Get the console switches

      @return The console switches
    */
    Switches& switches() const
    {
      return *mySwitches;
    }

    /**
      Get the 6502 based system used by the console to emulate the game

      @return The 6502 based system
    */
    System& system() const
    {
      return *mySystem;
    }

    /**
      Get the event handler of the console

      @return The event handler
    */
    EventHandler& eventHandler() const
    {
      return *myEventHandler;
    }

  public:
    /**
      Overloaded assignment operator

      @param console The console object to set myself equal to
      @return Myself after assignment has taken place
    */
    Console& operator = (const Console& console);

  public:
    /**
      Get the default properties object to use for other properties objects

      @return The default properties object
    */
    static const Properties& defaultProperties();

  public:
    /**
      Toggle between NTSC and PAL mode.  The GUI's may need to reload their palette.
    */
    void toggleFormat();

    /**
      Change the "Display.XStart" variable.  Currently, a system reset is issued
      after the change.  GUI's may need to resize their viewports.

      @param direction A 1 indicates increase, 0 indicates decrease.
    */
    void changeXStart(const uInt32 direction);

    /**
      Change the "Display.XStart" variable.  Currently, a system reset is issued
      after the change.  GUI's may need to resize their viewports.

      @param direction A 1 indicates increase, 0 indicates decrease.
    */
    void changeYStart(const uInt32 direction);

    /**
      Change the "Display.XStart" variable.  Currently, a system reset is issued
      after the change.  GUI's may need to resize their viewports.

      @param direction A 1 indicates increase, 0 indicates decrease.
    */
    void changeWidth(const uInt32 direction);

    /**
      Change the "Display.XStart" variable.  Currently, a system reset is issued
      after the change.  GUI's may need to resize their viewports.

      @param direction A 1 indicates increase, 0 indicates decrease.
    */
    void changeHeight(const uInt32 direction);

    /**
      Save a copy of the current properties after any changes.

      @param filename Filename to save the properties into.
      @param merge Whether or not to merge the changes into the
                   main properties file.
    */
    void saveProperties(string& filename, bool merge = false);

  private:
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

    // Reference to the Settings object
    Settings& mySettings;

    // Reference to the PropertiesSet object
    PropertiesSet& myPropSet;

    // Pointer to the EventHandler object
    EventHandler* myEventHandler;

  private:
    // Default properties to use for properties objects
    static Properties ourDefaultProperties;
};
#endif
