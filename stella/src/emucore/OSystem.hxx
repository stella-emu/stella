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
// $Id: OSystem.hxx,v 1.12 2005-05-11 19:36:00 stephena Exp $
//============================================================================

#ifndef OSYSTEM_HXX
#define OSYSTEM_HXX

class PropertiesSet;

class Menu;
class Launcher;

#include "EventHandler.hxx"
#include "FrameBuffer.hxx"
#include "Sound.hxx"
#include "Settings.hxx"
#include "Console.hxx"
#include "bspf.hxx"


/**
  This class provides an interface for accessing operating system specific
  functions.  It also comprises an overall parent object, to which all the
  other objects belong.

  @author  Stephen Anthony
  @version $Id: OSystem.hxx,v 1.12 2005-05-11 19:36:00 stephena Exp $
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
      Get the ROM launcher of the system.

      @return The launcher object
    */
    Launcher& launcher(void) const { return *myLauncher; }

    /**
      Set the framerate for the video system.  It's placed in this class since
      the mainLoop() method is defined here.

      @param framerate  The video framerate to use
    */
    void setFramerate(uInt32 framerate)
         { myTimePerFrame = (uInt32)(1000000.0 / (double) framerate); }

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
      Set the location of the gamelist cache file
    */
    void setCacheFile(const string& cachefile) { myGameListCacheFile = cachefile; }

    /**
      Return the default directory for storing data.
    */
    const string& baseDir() { return myBaseDir; }

    /**
      Return the directory for storing state files.
    */
    const string& stateDir() { return myStateDir; }

    /**
      This method should be called to get the filename of the
      properties (stella.pro) file for the purpose of loading.

      @return String representing the full path of the properties filename.
    */
    const string& propertiesInputFilename() { return myPropertiesInputFile; }

    /**
      This method should be called to get the filename of the
      properties (stella.pro) file for the purpose of saving.

      @return String representing the full path of the properties filename.
    */
    const string& propertiesOutputFilename() { return myPropertiesOutputFile; }

    /**
      This method should be called to get the filename of the config file
      for the purpose of loading.

      @return String representing the full path of the config filename.
    */
    const string& configInputFilename() { return myConfigInputFile; }

    /**
      This method should be called to get the filename of the config file
      for the purpose of saving.

      @return String representing the full path of the config filename.
    */
    const string& configOutputFilename() { return myConfigOutputFile; }

    /**
      This method should be called to get the filename of the gamelist
      cache file (used by the Launcher to show a listing of available games).

      @return String representing the full path of the gamelist cache file.
    */
    const string& cacheFile() { return myGameListCacheFile; }

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

    /**
      Creates a new game console from the specified romfile.

      @param romfile  The full pathname of the ROM to use
      @return  True on successful creation, otherwise false
    */
    bool createConsole(const string& romfile = "");

    /**
      Creates a new ROM launcher, to select a new ROM to emulate.
    */
    void createLauncher();

  public:
    //////////////////////////////////////////////////////////////////////
    // The following methods are system-specific and must be implemented
    // in derived classes.
    //////////////////////////////////////////////////////////////////////
    /**
      This method runs the main loop.  Since different platforms
      may use different timing methods and/or algorithms, this method has
      been abstracted to each platform.
    */
    virtual void mainLoop() = 0;

    /**
      This method returns number of ticks in microseconds.

      @return Current time in microseconds.
    */
    virtual uInt32 getTicks() = 0;

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

    // Pointer to the Launcher object
    Launcher* myLauncher;

    // Time per frame for a video update, based on the current framerate
    uInt32 myTimePerFrame;

  private:
    string myBaseDir;
    string myStateDir;

    string myPropertiesInputFile;
    string myPropertiesOutputFile;
    string myConfigInputFile;
    string myConfigOutputFile;

    string myGameListCacheFile;
    string myRomFile;

  private:
    // Copy constructor isn't supported by this class so make it private
    OSystem(const OSystem&);

    // Assignment operator isn't supported by this class so make it private
    OSystem& operator = (const OSystem&);
};

#endif
