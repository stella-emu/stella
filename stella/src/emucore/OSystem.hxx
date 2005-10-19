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
// Copyright (c) 1995-2005 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: OSystem.hxx,v 1.30 2005-10-19 00:59:51 stephena Exp $
//============================================================================

#ifndef OSYSTEM_HXX
#define OSYSTEM_HXX

class PropertiesSet;

class Menu;
class CommandMenu;
class Launcher;
class Debugger;

#include "EventHandler.hxx"
#include "FrameBuffer.hxx"
#include "Sound.hxx"
#include "Settings.hxx"
#include "Console.hxx"
#include "StringList.hxx"
#include "Font.hxx"

#include "bspf.hxx"


/**
  This class provides an interface for accessing operating system specific
  functions.  It also comprises an overall parent object, to which all the
  other objects belong.

  @author  Stephen Anthony
  @version $Id: OSystem.hxx,v 1.30 2005-10-19 00:59:51 stephena Exp $
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
      Get the command menu of the system.

      @return The command menu object
    */
    CommandMenu& commandMenu(void) const { return *myCommandMenu; }

    /**
      Get the ROM launcher of the system.

      @return The launcher object
    */
    Launcher& launcher(void) const { return *myLauncher; }

#ifdef DEVELOPER_SUPPORT
    /**
      Get the ROM debugger of the system.

      @return The debugger object
    */
    Debugger& debugger(void) const { return *myDebugger; }
#endif

    /**
      Get the font object of the system

      @return The font reference
    */
    inline const GUI::Font& font() const { return *myFont; }

    /**
      Get the console font object of the system

      @return The console font reference
    */
    inline const GUI::Font& consoleFont() const { return *myConsoleFont; }

    /**
      Set the framerate for the video system.  It's placed in this class since
      the mainLoop() method is defined here.

      @param framerate  The video framerate to use
    */
    void setFramerate(uInt32 framerate)
         { myDisplayFrameRate = framerate;
           myTimePerFrame = (uInt32)(1000000.0 / (double)myDisplayFrameRate); }

    /**
      Get the current framerate for the video system.

      @return  The video framerate currently in use
    */
    uInt32 frameRate() { return myDisplayFrameRate; }

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
      system-wide properties file (stella.pro).

      @return String representing the full path of the properties filename.
    */
    const string& systemProperties() { return mySystemPropertiesFile; }

    /**
      This method should be called to get the filename of the
      user-specific properties file (user.pro).

      @return String representing the full path of the properties filename.
    */
    const string& userProperties() { return myUserPropertiesFile; }

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

    /**
      The features which are conditionally compiled into Stella.

      @return  The supported features
    */
    const string& features() { return myFeatures; }

    /**
      The features which are conditionally compiled into Stella.

      @return  The supported features
    */
    const StringList& driverList() { return myDriverList; }

    /**
      Open the given ROM and return an array containing its contents.

      @param rom    The absolute pathname of the ROM file
      @param md5    The md5 calculated from the ROM file
      @param image  A pointer to store the ROM data
                    Note, the calling method is responsible for deleting this
      @param size   The amount of data read into the image array
      @return  False on any errors, else true
    */
    bool openROM(const string& rom, string& md5, uInt8** image, int* size);

    const string& romFile() { return myRomFile; }

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

  protected:
    /**
      Set the base directory for all Stella files
    */
    void setBaseDir(const string& basedir);

    /**
      Set the directory where state files are stored
    */
    void setStateDir(const string& statedir);

    /**
      Set the locations of game properties files
    */
    void setPropertiesDir(const string& userpath, const string& systempath);

    /**
      Set the locations of config files
    */
    void setConfigFiles(const string& userconfig, const string& systemconfig);

    /**
      Set the location of the gamelist cache file
    */
    void setCacheFile(const string& cachefile) { myGameListCacheFile = cachefile; }

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

    // Pointer to the CommandMenu object
    CommandMenu* myCommandMenu;

    // Pointer to the Launcher object
    Launcher* myLauncher;

    // Pointer to the Debugger object
    Debugger* myDebugger;

    // Number of times per second to iterate through the main loop
    uInt32 myDisplayFrameRate;

    // Time per frame for a video update, based on the current framerate
    uInt32 myTimePerFrame;

    // Holds the types of SDL video driver supported by this OSystem
    StringList myDriverList;

  private:
    string myBaseDir;
    string myStateDir;

    string mySystemPropertiesFile;
    string myUserPropertiesFile;
    string myConfigInputFile;
    string myConfigOutputFile;

    string myGameListCacheFile;
    string myRomFile;

    string myFeatures;

    // The normal GUI font object to use
    GUI::Font* myFont;

    // The console font object to use
    GUI::Font* myConsoleFont;

  private:
    // Copy constructor isn't supported by this class so make it private
    OSystem(const OSystem&);

    // Assignment operator isn't supported by this class so make it private
    OSystem& operator = (const OSystem&);
};

#endif
