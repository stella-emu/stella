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
// $Id: OSystem.hxx,v 1.6 2005-05-01 18:57:21 stephena Exp $
//============================================================================

#ifndef OSYSTEM_HXX
#define OSYSTEM_HXX

class EventHandler;
class FrameBuffer;
class Sound;
class Settings;
class PropertiesSet;
class Menu;
class Browser;

#include "Console.hxx"
#include "bspf.hxx"


/**
  This class provides an interface for accessing operating system specific
  functions.  It also comprises an overall parent object, to which all the
  other objects belong.

  @author  Stephen Anthony
  @version $Id: OSystem.hxx,v 1.6 2005-05-01 18:57:21 stephena Exp $
*/
class OSystem
{
  public:
    /**
      Create a new OSystem abstract class
    */
    OSystem();

    /**
      Destructor
    */
    virtual ~OSystem();

  public:
    /**
      Adds the specified eventhandler to the system.

      @param eventhandler The eventhandler to add 
    */
    void attach(EventHandler* eventhandler) { myEventHandler = eventhandler; }

    /**
      Adds the specified framebuffer to the system.

      @param framebuffer The framebuffer to add 
    */
    void attach(FrameBuffer* framebuffer) { myFrameBuffer = framebuffer; }

    /**
      Adds the specified sound device to the system.

      @param sound The sound device to add 
    */
    void attach(Sound* sound) { mySound = sound; }

    /**
      Adds the specified settings object to the system.

      @param settings The settings object to add 
    */
    void attach(Settings* settings) { mySettings = settings; }

    /**
      Adds the specified game properties set to the system.

      @param propset The properties set to add 
    */
    void attach(PropertiesSet* propset) { myPropSet = propset; }

    /**
      Adds the specified console to the system.

      @param console  The console (game emulation object) to add 
    */
    void attach(Console* console) { myConsole = console; }

    /**
      Removes the currently attached console from the system.
    */
    void detachConsole(void) { delete myConsole; myConsole = NULL; }

    /**
      Adds the specified settings menu yo the system.

      @param menu The menu object to add 
    */
    void attach(Menu* menu) { myMenu = menu; }

    /**
      Adds the specified ROM browser to the system.

      @param browser The browser object to add 
    */
    void attach(Browser* browser) { myBrowser = browser; }

    /**
      Get the event handler of the system

      @return The event handler
    */
    EventHandler& eventHandler() const { return *myEventHandler; }

    /**
      Get the frame buffer of the system

      @return The frame buffer
    */
    FrameBuffer& frameBuffer() const { return *myFrameBuffer; }

    /**
      Get the sound object of the system

      @return The sound object
    */
    Sound& sound() const { return *mySound; }

    /**
      Get the settings object of the system

      @return The settings object
    */
    Settings& settings() const { return *mySettings; }

    /**
      Get the set of game properties for the system

      @return The properties set object
    */
    PropertiesSet& propSet() const { return *myPropSet; }

    /**
      Get the console of the system.

      @return The console object
    */
    Console& console(void) const { return *myConsole; }

    /**
      Get the settings menu of the system.

      @return The settings menu object
    */
    Menu& menu(void) const { return *myMenu; }

    /**
      Get the ROM browser of the system.

      @return The browser object
    */
    Browser& browser(void) const { return *myBrowser; }

    /**
      Set the base directory for all configuration files
    */
    void setBaseDir(const string& basedir) { myBaseDir = basedir; }

    /**
      Set the directory where state files are stored
    */
    void setStateDir(const string& statedir) { myStateDir = statedir; }

    /**
      Set the locations of game properties files
    */
    void setPropertiesFiles(const string& userprops, const string& systemprops);

    /**
      Set the locations of config files
    */
    void setConfigFiles(const string& userconfig, const string& systemconfig);

    /**
      Return the default directory for storing data.
    */
    string baseDir() { return myBaseDir; }

    /**
      Return the directory for storing state files.
    */
    string stateDir() { return myStateDir; }

    /**
      This method should be called to get the filename of the
      properties (stella.pro) file for the purpose of loading.

      @return String representing the full path of the properties filename.
    */
    string propertiesInputFilename() { return myPropertiesInputFile; }

    /**
      This method should be called to get the filename of the
      properties (stella.pro) file for the purpose of saving.

      @return String representing the full path of the properties filename.
    */
    string propertiesOutputFilename() { return myPropertiesOutputFile; }

    /**
      This method should be called to get the filename of the config file
      for the purpose of loading.

      @return String representing the full path of the config filename.
    */
    string configInputFilename() { return myConfigInputFile; }

    /**
      This method should be called to get the filename of the config file
      for the purpose of saving.

      @return String representing the full path of the config filename.
    */
    string configOutputFilename() { return myConfigOutputFile; }

    /**
      Creates the various framebuffers/renderers available in this system
      (for now, that means either 'software' or 'opengl').

      @return Success or failure of the framebuffer creation
    */
    bool createFrameBuffer(bool showmessage = false);

    /**
      Switches between software and OpenGL framebuffer modes.
    */
    void toggleFrameBuffer();

    /**
      Creates the various sound devices available in this system
      (for now, that means either 'SDL' or 'Null').
    */
    void createSound();

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

    /**
      This method should be called to create the specified directory.

      @param path   The directory to create

      @return       boolean representing whether or not the directory was created
    */
    virtual bool makeDir(const string& path) = 0;

  protected:
    // Pointer to the EventHandler object
    EventHandler* myEventHandler;

    // Pointer to the FrameBuffer object
    FrameBuffer* myFrameBuffer;

    // Pointer to the Sound object
    Sound* mySound;

    // Pointer to the Settings object
    Settings* mySettings;

    // Pointer to the PropertiesSet object
    PropertiesSet* myPropSet;

    // Pointer to the (currently defined) Console object
    Console* myConsole;

    // Pointer to the Menu object
    Menu* myMenu;

    // Pointer to the Browser object
    Browser* myBrowser;

  private:
    string myBaseDir;
    string myStateDir;

    string myPropertiesInputFile;
    string myPropertiesOutputFile;
    string myConfigInputFile;
    string myConfigOutputFile;

  private:
    // Copy constructor isn't supported by this class so make it private
    OSystem(const OSystem&);

    // Assignment operator isn't supported by this class so make it private
    OSystem& operator = (const OSystem&);
};

#endif
