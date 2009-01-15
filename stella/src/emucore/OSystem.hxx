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
// Copyright (c) 1995-2009 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: OSystem.hxx,v 1.75 2009-01-15 01:31:26 stephena Exp $
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
class SerialPort;
class Settings;
class Sound;
class StateManager;
class VideoDialog;

namespace GUI {
  class Font;
}

#include "Array.hxx"
#include "EventHandler.hxx"
#include "FrameBuffer.hxx"
#include "bspf.hxx"

struct Resolution {
  uInt32 width;
  uInt32 height;
  string name;
};
typedef Common::Array<Resolution> ResolutionList;

/**
  This class provides an interface for accessing operating system specific
  functions.  It also comprises an overall parent object, to which all the
  other objects belong.

  @author  Stephen Anthony
  @version $Id: OSystem.hxx,v 1.75 2009-01-15 01:31:26 stephena Exp $
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
      Adds the specified settings object to the system.

      @param settings The settings object to add 
    */
    void attach(Settings* settings) { mySettings = settings; }

    /**
      Get the event handler of the system

      @return The event handler
    */
    inline EventHandler& eventHandler() const { return *myEventHandler; }

    /**
      Get the frame buffer of the system

      @return The frame buffer
    */
    inline FrameBuffer& frameBuffer() const { return *myFrameBuffer; }

    /**
      Get the sound object of the system

      @return The sound object
    */
    inline Sound& sound() const { return *mySound; }

    /**
      Get the settings object of the system

      @return The settings object
    */
    inline Settings& settings() const { return *mySettings; }

    /**
      Get the set of game properties for the system

      @return The properties set object
    */
    inline PropertiesSet& propSet() const { return *myPropSet; }

    /**
      Get the console of the system.

      @return The console object
    */
    inline Console& console() const { return *myConsole; }

    /**
      Get the serial port of the system.

      @return The serial port object
    */
    inline SerialPort& serialPort() const { return *mySerialPort; }

    /**
      Get the settings menu of the system.

      @return The settings menu object
    */
    inline Menu& menu() const { return *myMenu; }

    /**
      Get the command menu of the system.

      @return The command menu object
    */
    inline CommandMenu& commandMenu() const { return *myCommandMenu; }

    /**
      Get the ROM launcher of the system.

      @return The launcher object
    */
    inline Launcher& launcher() const { return *myLauncher; }

    /**
      Get the state manager of the system.

      @return The statemanager object
    */
    inline StateManager& state() const { return *myStateManager; }

#ifdef DEBUGGER_SUPPORT
    /**
      Get the ROM debugger of the system.

      @return The debugger object
    */
    inline Debugger& debugger() const { return *myDebugger; }
#endif

#ifdef CHEATCODE_SUPPORT
    /**
      Get the cheat manager of the system.

      @return The cheatmanager object
    */
    inline CheatManager& cheat() const { return *myCheatManager; }
#endif

    /**
      Get the small font object of the system

      @return The font reference
    */
    inline const GUI::Font& smallFont() const { return *mySmallFont; }

    /**
      Get the font object of the system

      @return The font reference
    */
    inline const GUI::Font& font() const { return *myFont; }

    /**
      Get the launcher font object of the system

      @return The font reference
    */
    inline const GUI::Font& launcherFont() const { return *myLauncherFont; }

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
    virtual void setFramerate(float framerate);

    /**
      Set all config file paths for the OSystem.
    */
    void setConfigPaths();

    /**
      Set the user-interface palette which is specified in current settings.
    */
    void setUIPalette();

    /**
      Get the current framerate for the video system.

      @return  The video framerate currently in use
    */
    inline float frameRate() const { return myDisplayFrameRate; }

    /**
      Get the maximum dimensions of a window for the video hardware.
    */
    inline uInt32 desktopWidth() const  { return myDesktopWidth; }
    inline uInt32 desktopHeight() const { return myDesktopHeight; }

    /**
      Get the supported fullscreen resolutions for the video hardware.

      @return  An array of supported resolutions
    */
    const ResolutionList& supportedResolutions() const { return myResolutions; }

    /**
      Return the default directory for storing data.
    */
    const string& baseDir() const { return myBaseDir; }

    /**
      Return the directory for storing state files.
    */
    const string& stateDir() const { return myStateDir; }

    /**
      Return the directory for storing PNG snapshots.
    */
    const string& snapshotDir() const { return mySnapshotDir; }

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

      @return String representing the full path of the ROM file.
    */
    const string& romFile() const { return myRomFile; }

    /**
      Creates a new game console from the specified romfile, and correctly
      initializes the system state to start emulation of the Console.

      @param romfile  The full pathname of the ROM to use
      @param md5      The MD5sum of the ROM

      @return  True on successful creation, otherwise false
    */
    bool createConsole(const string& romfile = "", const string& md5 = "");

    /**
      Deletes the currently defined console, if it exists.
      Also prints some statistics (fps, total frames, etc).
    */
    void deleteConsole();

    /**
      Creates a new ROM launcher, to select a new ROM to emulate.

      @return  True on successful creation, otherwise false
    */
    bool createLauncher();

    /**
      Gets all possible info about the ROM by creating a temporary
      Console object and querying it.

      @param romfile  The full pathname of the ROM to use
      @return  Some information about this ROM
    */
    string getROMInfo(const string& romfile);

    /**
      The features which are conditionally compiled into Stella.

      @return  The supported features
    */
    const string& features() const { return myFeatures; }

    /**
      Calculate the MD5sum of the given file.

      @param filename  Filename of potential ROM file
     */
    string MD5FromFile(const string& filename);

    /**
      Issue a quit event to the OSystem.
    */
    void quit() { myQuitLoop = true; }

  public:
    //////////////////////////////////////////////////////////////////////
    // The following methods are system-specific and must be implemented
    // in derived classes.
    //////////////////////////////////////////////////////////////////////
    /**
      This method returns number of ticks in microseconds.

      @return Current time in microseconds.
    */
    virtual uInt32 getTicks() const = 0;

    //////////////////////////////////////////////////////////////////////
    // The following methods are system-specific and can be overrided in
    // derived classes.  Otherwise, the base methods will be used.
    //////////////////////////////////////////////////////////////////////
    /**
      This method runs the main loop.  Since different platforms
      may use different timing methods and/or algorithms, this method can
      be overrided.  However, the port then takes all responsibility for
      running the emulation and taking care of timing.
    */
    virtual void mainLoop();

    /**
      This method determines the default mapping of joystick buttons to
      Stella events for a specific system/platform.
    */
    virtual void setDefaultJoymap();

    /**
      This method determines the default mapping of joystick axis to
      Stella events for a specific system/platform.
    */
    virtual void setDefaultJoyAxisMap();

    /**
      This method determines the default mapping of joystick hats to
      Stella events for a specific system/platform.
    */
    virtual void setDefaultJoyHatMap();

    /**
      This method creates events from platform-specific hardware.
    */
    virtual void pollEvent();

    /**
      This method answers whether the given button as already been
      handled by the pollEvent() method, and as such should be ignored
      in the main event handler.
    */
    virtual bool joyButtonHandled(int button);

    /**
      Informs the OSystem of a change in EventHandler state.
    */
    virtual void stateChanged(EventHandler::State state);

  protected:
    /**
      Query the OSystem video hardware for resolution information.
    */
    virtual void queryVideoHardware();

    /**
      Set the base directory for all Stella files (these files may be
      located in other places through settings).
    */
    void setBaseDir(const string& basedir);

    /**
      Set the locations of config file
    */
    void setConfigFile(const string& file) { myConfigFile = file; }

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

    // Pointer to the serial port object
    SerialPort* mySerialPort;

    // Pointer to the Menu object
    Menu* myMenu;

    // Pointer to the CommandMenu object
    CommandMenu* myCommandMenu;

    // Pointer to the Launcher object
    Launcher* myLauncher;

    // Pointer to the Debugger object
    Debugger* myDebugger;

    // Pointer to the CheatManager object
    CheatManager* myCheatManager;

    // Pointer to the StateManager object
    StateManager* myStateManager;

    // Maximum dimensions of the desktop area
    uInt32 myDesktopWidth, myDesktopHeight;

    // Supported fullscreen resolutions
    ResolutionList myResolutions;

    // Number of times per second to iterate through the main loop
    float myDisplayFrameRate;

    // Time per frame for a video update, based on the current framerate
    uInt32 myTimePerFrame;

    // Indicates whether to stop the main loop
    bool myQuitLoop;

  private:
    enum { kNumUIPalettes = 2 };
    string myBaseDir;
    string myStateDir;
    string mySnapshotDir;

    string myCheatFile;
    string myConfigFile;
    string myPaletteFile;
    string myPropertiesFile;

    string myRomFile;
    string myRomMD5;

    string myFeatures;

    // The font object to use when space is very limited
    GUI::Font* mySmallFont;

    // The font object to use for the normal in-game GUI
    GUI::Font* myFont;

    // The font object to use for the ROM launcher
    GUI::Font* myLauncherFont;

    // The font object to use for the console/debugger 
    GUI::Font* myConsoleFont;

    // Indicates whether the main processing loop should proceed
    struct TimingInfo {
      uInt32 start;
      uInt32 current;
      uInt32 virt;
      uInt32 totalTime;
      uInt32 totalFrames;
    };
    TimingInfo myTimingInfo;

    // Table of RGB values for GUI elements
    static uInt32 ourGUIColors[kNumUIPalettes][kNumColors-256];

  private:
    /**
      Creates the various framebuffers/renderers available in this system
      (for now, that means either 'software' or 'opengl').  Note that
      it will only create one type per run of Stella.

      @return Success or failure of the framebuffer creation
    */
    bool createFrameBuffer();

    /**
      Creates the various sound devices available in this system
      (for now, that means either 'SDL' or 'Null').
    */
    void createSound();

    /**
      Creates an actual Console object based on the given info.

      @param romfile  The full pathname of the ROM to use
      @param md5      The MD5sum of the ROM

      @return  The actual Console object, otherwise NULL
               (calling method is responsible for deleting it)
    */
    Console* openConsole(const string& romfile, string& md5);

    /**
      Open the given ROM and return an array containing its contents.
      Also, the properties database is updated with a valid ROM name
      for this ROM (if necessary).

      @param rom    The absolute pathname of the ROM file
      @param md5    The md5 calculated from the ROM file
                    (will be recalculated if necessary)
      @param size   The amount of data read into the image array

      @return  Pointer to the array, with size >=0 indicating valid data
               (calling method is responsible for deleting it)
    */
    uInt8* openROM(const string& rom, string& md5, uInt32& size);

    /**
      Gets all possible info about the given console.

      @param console  The console to use
      @return  Some information about this console
    */
    string getROMInfo(const Console* console);

    /**
      Initializes the timing so that the mainloop is reset to its
      initial values.
    */
    void resetLoopTiming();

    // Copy constructor isn't supported by this class so make it private
    OSystem(const OSystem&);

    // Assignment operator isn't supported by this class so make it private
    OSystem& operator = (const OSystem&);
};

#endif
