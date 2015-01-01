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
// Copyright (c) 1995-2015 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

#ifndef OSYSTEM_HXX
#define OSYSTEM_HXX

class Cartridge;
class CheatManager;
class CommandMenu;
class Console;
class Debugger;
class Launcher;
class Menu;
class Properties;
class PropertiesSet;
class Random;
class SerialPort;
class Settings;
class Sound;
class StateManager;
class VideoDialog;

#include "FSNode.hxx"
#include "FrameBuffer.hxx"
#include "PNGLibrary.hxx"
#include "bspf.hxx"

struct TimingInfo {
  uInt64 start;
  uInt64 current;
  uInt64 virt;
  uInt64 totalTime;
  uInt64 totalFrames;
};

/**
  This class provides an interface for accessing operating system specific
  functions.  It also comprises an overall parent object, to which all the
  other objects belong.

  @author  Stephen Anthony
  @version $Id$
*/
class OSystem
{
  friend class EventHandler;
  friend class VideoDialog;

  public:
    /**
      Create a new OSystem abstract class
    */
    OSystem();

    /**
      Destructor
    */
    virtual ~OSystem();

    /**
      Create all child objects which belong to this OSystem
    */
    virtual bool create();

  public:
    /**
      Get the event handler of the system.

      @return The event handler
    */
    EventHandler& eventHandler() const { return *myEventHandler; }

    /**
      Get the frame buffer of the system.

      @return The frame buffer
    */
    FrameBuffer& frameBuffer() const { return *myFrameBuffer; }

    /**
      Get the sound object of the system.

      @return The sound object
    */
    Sound& sound() const { return *mySound; }

    /**
      Get the settings object of the system.

      @return The settings object
    */
    Settings& settings() const { return *mySettings; }

    /**
      Get the random object of the system.

      @return The random object
    */
    Random& random() const { return *myRandom; }

    /**
      Get the set of game properties for the system.

      @return The properties set object
    */
    PropertiesSet& propSet() const { return *myPropSet; }

    /**
      Get the console of the system.  The console won't always exist,
      so we should test if it's available.

      @return The console object
    */
    Console& console() const { return *myConsole; }
    bool hasConsole() const { return myConsole != nullptr; }

    /**
      Get the serial port of the system.

      @return The serial port object
    */
    SerialPort& serialPort() const { return *mySerialPort; }

    /**
      Get the settings menu of the system.

      @return The settings menu object
    */
    Menu& menu() const { return *myMenu; }

    /**
      Get the command menu of the system.

      @return The command menu object
    */
    CommandMenu& commandMenu() const { return *myCommandMenu; }

    /**
      Get the ROM launcher of the system.

      @return The launcher object
    */
    Launcher& launcher() const { return *myLauncher; }

    /**
      Get the state manager of the system.

      @return The statemanager object
    */
    StateManager& state() const { return *myStateManager; }

    /**
      Get the PNG handler of the system.

      @return The PNGlib object
    */
    PNGLibrary& png() const { return *myPNGLib; }

    /**
      This method should be called to load the current settings from an rc file.
      It first loads the settings from the config file, then informs subsystems
      about the new settings.
    */
    void loadConfig();

    /**
      This method should be called to save the current settings to an rc file.
      It first asks each subsystem to update its settings, then it saves all
      settings to the config file.
    */
    void saveConfig();

#ifdef DEBUGGER_SUPPORT
    /**
      Get the ROM debugger of the system.

      @return The debugger object
    */
    Debugger& debugger() const { return *myDebugger; }
#endif

#ifdef CHEATCODE_SUPPORT
    /**
      Get the cheat manager of the system.

      @return The cheatmanager object
    */
    CheatManager& cheat() const { return *myCheatManager; }
#endif

    /**
      Set the framerate for the video system.  It's placed in this class since
      the mainLoop() method is defined here.

      @param framerate  The video framerate to use
    */
    virtual void setFramerate(float framerate);

    /**
      Set all config file paths for the OSystem.
    */
    void setConfigPaths();

    /**
      Get the current framerate for the video system.

      @return  The video framerate currently in use
    */
    float frameRate() const { return myDisplayFrameRate; }

    /**
      Return the default full/complete directory name for storing data.
    */
    const string& baseDir() const { return myBaseDir; }

    /**
      Return the full/complete directory name for storing state files.
    */
    const string& stateDir() const { return myStateDir; }

    /**
      Return the full/complete directory name for saving and loading
      PNG snapshots.
    */
    const string& snapshotSaveDir() const { return mySnapshotSaveDir; }
    const string& snapshotLoadDir() const { return mySnapshotLoadDir; }

    /**
      Return the full/complete directory name for storing nvram
      (flash/EEPROM) files.
    */
    const string& nvramDir() const { return myNVRamDir; }

    /**
      Return the full/complete directory name for storing Distella cfg files.
    */
    const string& cfgDir() const { return myCfgDir; }

    /**
      This method should be called to get the full path of the cheat file.

      @return String representing the full path of the cheat filename.
    */
    const string& cheatFile() const { return myCheatFile; }

    /**
      This method should be called to get the full path of the config file.

      @return String representing the full path of the config filename.
    */
    const string& configFile() const { return myConfigFile; }

    /**
      This method should be called to get the full path of the
      (optional) palette file.

      @return String representing the full path of the properties filename.
    */
    const string& paletteFile() const { return myPaletteFile; }

    /**
      This method should be called to get the full path of the
      properties file (stella.pro).

      @return String representing the full path of the properties filename.
    */
    const string& propertiesFile() const { return myPropertiesFile; }

    /**
      This method should be called to get the full path of the currently
      loaded ROM.

      @return FSNode object representing the ROM file.
    */
    const FilesystemNode& romFile() const { return myRomFile; }

    /**
      Creates a new game console from the specified romfile, and correctly
      initializes the system state to start emulation of the Console.

      @param rom     The FSNode of the ROM to use (contains path, etc)
      @param md5     The MD5sum of the ROM
      @param newrom  Whether this is a new ROM, or a reload of current one

      @return  String indicating any error message (EmptyString for no errors)
    */
    string createConsole(const FilesystemNode& rom, const string& md5 = "",
                         bool newrom = true);

    /**
      Reloads the current console (essentially deletes and re-creates it).
      This can be thought of as a real console off/on toggle.

      @return  True on successful creation, otherwise false
    */
    bool reloadConsole();

    /**
      Creates a new ROM launcher, to select a new ROM to emulate.

      @param startdir  The directory to use when opening the launcher;
                       if blank, use 'romdir' setting.

      @return  True on successful creation, otherwise false
    */
    bool createLauncher(const string& startdir = "");

    /**
      Answers whether the ROM launcher was actually successfully used
      at some point since the app started.

      @return  True if launcher was ever used, otherwise false
    */
    bool launcherUsed() const { return myLauncherUsed; }

    /**
      Gets all possible info about the ROM by creating a temporary
      Console object and querying it.

      @param romfile  The file node of the ROM to use
      @return  Some information about this ROM
    */
    string getROMInfo(const FilesystemNode& romfile);

    /**
      The features which are conditionally compiled into Stella.

      @return  The supported features
    */
    const string& features() const { return myFeatures; }

    /**
      The build information for Stella (toolkit version, architecture, etc).

      @return  The build info
    */
    const string& buildInfo() const { return myBuildInfo; }

    /**
      Issue a quit event to the OSystem.
    */
    void quit() { myQuitLoop = true; }

    /**
      Append a message to the internal log
      (a newline is automatically added).

      @param message  The message to be appended
      @param level    If 0, always output the message, only append when
                      level is less than or equal to that in 'loglevel'
    */
    void logMessage(const string& message, uInt8 level);

    /**
      Get the system messages logged up to this point.

      @return The list of log messages
    */
    const string& logMessages() const { return myLogMessages; }

    /**
      Return timing information (start time of console, current
      number of frames rendered, etc.
    */
    const TimingInfo& timingInfo() const { return myTimingInfo; }

  public:
    //////////////////////////////////////////////////////////////////////
    // The following methods are system-specific and can be overrided in
    // derived classes.  Otherwise, the base methods will be used.
    //////////////////////////////////////////////////////////////////////
    /**
      This method returns number of ticks in microseconds since some
      pre-defined time in the past.  *NOTE*: it is necessary that this
      pre-defined time exists between runs of the application, and must
      be (relatively) unique.  For example, the time since the system
      started running is not a good choice, since it can be duplicated.
      The current implementation uses time since the UNIX epoch.

      @return Current time in microseconds.
    */
    virtual uInt64 getTicks() const;

    /**
      This method runs the main loop.  Since different platforms
      may use different timing methods and/or algorithms, this method can
      be overrided.  However, the port then takes all responsibility for
      running the emulation and taking care of timing.
    */
    virtual void mainLoop();

    /**
      Informs the OSystem of a change in EventHandler state.
    */
    virtual void stateChanged(EventHandler::State state) { }

    /**
      Returns the default save and load paths for the snapshot directory.
      Since this varies greatly among different systems and is the one
      directory that most end-users care about (vs. config file stuff
      that usually isn't user-modifiable), we create a special method
      for it.
    */
    virtual string defaultSnapSaveDir() { return "~" BSPF_PATH_SEPARATOR; }
    virtual string defaultSnapLoadDir() { return "~" BSPF_PATH_SEPARATOR; }

  protected:
    /**
      Set the base directory for all Stella files (these files may be
      located in other places through settings).
    */
    void setBaseDir(const string& basedir);

    /**
      Set the locations of config file
    */
    void setConfigFile(const string& file);

  protected:
    // Pointer to the EventHandler object
    unique_ptr<EventHandler> myEventHandler;

    // Pointer to the FrameBuffer object
    unique_ptr<FrameBuffer> myFrameBuffer;

    // Pointer to the Sound object
    unique_ptr<Sound> mySound;

    // Pointer to the Settings object
    unique_ptr<Settings> mySettings;

    // Pointer to the Random object
    unique_ptr<Random> myRandom;

    // Pointer to the PropertiesSet object
    unique_ptr<PropertiesSet> myPropSet;

    // Pointer to the (currently defined) Console object
    unique_ptr<Console> myConsole;

    // Pointer to the serial port object
    unique_ptr<SerialPort> mySerialPort;

    // Pointer to the Menu object
    unique_ptr<Menu> myMenu;

    // Pointer to the CommandMenu object
    unique_ptr<CommandMenu> myCommandMenu;

    // Pointer to the Launcher object
    unique_ptr<Launcher> myLauncher;
    bool myLauncherUsed;

    // Pointer to the Debugger object
    unique_ptr<Debugger> myDebugger;

    // Pointer to the CheatManager object
    unique_ptr<CheatManager> myCheatManager;

    // Pointer to the StateManager object
    unique_ptr<StateManager> myStateManager;

    // PNG object responsible for loading/saving PNG images
    unique_ptr<PNGLibrary> myPNGLib;

    // The list of log messages
    string myLogMessages;

    // Number of times per second to iterate through the main loop
    float myDisplayFrameRate;

    // Time per frame for a video update, based on the current framerate
    uInt32 myTimePerFrame;

    // The time (in milliseconds) from the UNIX epoch when the application starts
    uInt32 myMillisAtStart;

    // Indicates whether to stop the main loop
    bool myQuitLoop;

  private:
    string myBaseDir;
    string myStateDir;
    string mySnapshotSaveDir;
    string mySnapshotLoadDir;
    string myNVRamDir;
    string myCfgDir;

    string myCheatFile;
    string myConfigFile;
    string myPaletteFile;
    string myPropertiesFile;

    FilesystemNode myRomFile;
    string myRomMD5;

    string myFeatures;
    string myBuildInfo;

    // Indicates whether the main processing loop should proceed
    TimingInfo myTimingInfo;

  private:
    /**
      Creates the various framebuffers/renderers available in this system.
      Note that it will only create one type per run of Stella.

      @return  Success or failure of the framebuffer creation
    */
    FBInitStatus createFrameBuffer();

    /**
      Creates the various sound devices available in this system
    */
    void createSound();

    /**
      Creates an actual Console object based on the given info.

      @param romfile  The file node of the ROM to use (contains path)
      @param md5      The MD5sum of the ROM
      @param type     The bankswitch type of the ROM
      @param id       The additional id (if any) used by the ROM

      @return  The actual Console object, otherwise nullptr.
    */
    unique_ptr<Console> openConsole(const FilesystemNode& romfile, string& md5,
                                    string& type, string& id);

    /**
      Close and finalize any currently open console.
    */
    void closeConsole();

    /**
      Open the given ROM and return an array containing its contents.
      Also, the properties database is updated with a valid ROM name
      for this ROM (if necessary).

      @param rom    The file node of the ROM to open (contains path)
      @param md5    The md5 calculated from the ROM file
                    (will be recalculated if necessary)
      @param size   The amount of data read into the image array

      @return  Pointer to the array, with size >=0 indicating valid data
               (calling method is responsible for deleting it)
    */
    uInt8* openROM(const FilesystemNode& rom, string& md5, uInt32& size);

    /**
      Gets all possible info about the given console.

      @param console  The console to use
      @return  Some information about this console
    */
    string getROMInfo(const Console& console);

    /**
      Initializes the timing so that the mainloop is reset to its
      initial values.
    */
    void resetLoopTiming();

    /**
      Validate the directory name, and create it if necessary.
      Also, update the settings with the new name.  For now, validation
      means that the path must always end with the appropriate separator.

      @param path     The actual path being accessed and created
      @param setting  The setting corresponding to the path being considered
      @param defaultpath  The default path to use if the settings don't exist
    */
    void validatePath(string& path, const string& setting,
                      const string& defaultpath);

    // Copy constructor and assignment operator not supported
    OSystem(const OSystem&);
    OSystem& operator = (const OSystem&);
};

#endif
