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
// $Id: OSystem.hxx,v 1.1 2005-02-21 02:23:49 stephena Exp $
//============================================================================

#ifndef OSYSTEM_HXX
#define OSYSTEM_HXX

class EventHandler;
class FrameBuffer;
class Sound;
class Settings;
class PropertiesSet;

#include "Console.hxx"
#include "bspf.hxx"


/**
  This class provides an interface for accessing operating system specific
  functions.  It also comprises an overall parent object, to which all the
  other objects belong.

  @author  Stephen Anthony
  @version $Id: OSystem.hxx,v 1.1 2005-02-21 02:23:49 stephena Exp $
*/
class OSystem
{
  public:
    /**
      Create a new OSystem abstract class
    */
    OSystem(FrameBuffer& framebuffer, Sound& sound,
            Settings& settings, PropertiesSet& propset);

    /**
      Destructor
    */
    virtual ~OSystem();

  public:
    /**
      Updates the osystem by one frame.  Determines which subsystem should
      be updated.  Generally will be called 'framerate' times per second.
    */
    void update();

    /**
      Adds the specified console to the system.

      @param console  The console (game emulation object) to add 
    */
    void addConsole(Console* console) { myConsole = console; }

    /**
      Removes the currently attached console from the system.
    */
    void removeConsole(void) { delete myConsole; myConsole = NULL; }

    /**
      Get the console of the system.

      @return The console object
    */
    Console& console(void) const { return *myConsole; }

    /**
      Get the event handler of the system

      @return The event handler
    */
    EventHandler& eventHandler() const { return *myEventHandler; }

    /**
      Get the frame buffer of the system

      @return The frame buffer
    */
    FrameBuffer& frameBuffer() const { return myFrameBuffer; }

    /**
      Get the sound object of the system

      @return The sound object
    */
    Sound& sound() const { return mySound; }

    /**
      Get the settings object of the system

      @return The settings object
    */
    Settings& settings() const { return mySettings; }

    /**
      Get the set of game properties for the system

      @return The properties set object
    */
    PropertiesSet& propSet() const { return myPropSet; }

  public:
    //////////////////////////////////////////////////////////////////////
    // The following methods are system-specific and must be implemented
    // in derived classes.
    //////////////////////////////////////////////////////////////////////

    /**
      This method should be called to get the filename of a state file
      given the state number.

      @param md5   The md5sum to use as part of the filename.
      @param state The state to use as part of the filename.

      @return      String representing the full path of the state filename.
    */
    virtual string stateFilename(const string& md5, uInt32 state) = 0;

    /**
      This method should be called to test whether the given file exists.

      @param filename The filename to test for existence.

      @return         boolean representing whether or not the file exists
    */
    virtual bool fileExists(const string& filename) = 0;

  protected:
    // Pointer to the EventHandler object
    EventHandler* myEventHandler;

    // Reference to the FrameBuffer object
    FrameBuffer& myFrameBuffer;

    // Reference to the Sound object
    Sound& mySound;

    // Reference to the Settings object
    Settings& mySettings;

    // Reference to the PropertiesSet object
    PropertiesSet& myPropSet;

    // Pointer to the (currently defined) Console object
    Console* myConsole;

  private:
    // Copy constructor isn't supported by this class so make it private
    OSystem(const OSystem&);

    // Assignment operator isn't supported by this class so make it private
    OSystem& operator = (const OSystem&);
};

#endif
