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
// $Id: Settings.hxx,v 1.5 2003-09-19 15:45:01 stephena Exp $
//============================================================================

#ifndef SETTINGS_HXX
#define SETTINGS_HXX

#ifdef DEVELOPER_SUPPORT
  #include "Props.hxx"
#endif

#include "bspf.hxx"

class Console;


/**
  This class provides an interface for accessing frontend specific settings.

  @author  Stephen Anthony
  @version $Id: Settings.hxx,v 1.5 2003-09-19 15:45:01 stephena Exp $
*/
class Settings
{
  public:
    /**
      Create a new settings abstract class
    */
    Settings();

    /**
      Destructor
    */
    virtual ~Settings();

  public:
    /**
      This method should be called to load the current settings from an rc file.
    */
    void loadConfig();

    /**
      This method should be called to save the current settings to an rc file.
    */
    void saveConfig();

    /**
      This method should be called to load the arguments from the commandline.
    */
    bool loadCommandLine(Int32 argc, char** argv);

  public:
    //////////////////////////////////////////////////////////////////////
    // The following methods are system-specific and must be implemented
    // in derived classes.
    //////////////////////////////////////////////////////////////////////

    /**
      This method should be called to set arguments.

      @param key   The variable to be set
      @param value The value for the variable to hold
    */
    virtual void setArgument(string& key, string& value) = 0;

    /**
      This method should be called to get system-specific settings.

      @return  A string representing all the key/value pairs.
    */
    virtual string getArguments() = 0;

    /**
      This method should be called to get the filename of a state file
      given the state number.

      @return String representing the full path of the state filename.
    */
    virtual string stateFilename(uInt32 state) = 0;

    /**
      This method should be called to get the filename of a snapshot.

      @return String representing the full path of the snapshot filename.
    */
    virtual string snapshotFilename() = 0;

    //////////////////////////////////////////////////////////////////////
  public:
    /**
      This method should be called when the emulation core sets
      the console object.
    */
    void setConsole(Console* console) { myConsole = console; } 

    /**
      This method should be called when the emulation core receives
      a QUIT event.
    */
    void setQuitEvent() { myQuitIndicator = true; }

    /**
      This method determines whether the QUIT event has been received.

      @return Boolean representing whether a QUIT event has been received
    */
    bool quit() { return myQuitIndicator; }

    /**
      This method should be called at when the emulation core receives
      a PAUSE event.
    */
    void setPauseEvent() { myPauseIndicator = !myPauseIndicator; }

    /**
      This method determines whether the PAUSE event has been received.

      @return Boolean representing whether a PAUSE event has been received
    */
    bool pause() { return myPauseIndicator; }

    /**
      This method should be called to get the filename of the users'
      properties (stella.pro) file.

      @return String representing the full path of the user properties filename.
    */
    string userPropertiesFilename() { return myUserPropertiesFile; }

    /**
      This method should be called to get the filename of the system
      properties (stella.pro) file.

      @return String representing the full path of the system properties filename.
    */
    string systemPropertiesFilename() { return mySystemPropertiesFile; }

    /**
      This method should be called to get the filename of the users'
      config (stellarc) file.

      @return String representing the full path of the user config filename.
    */
    string userConfigFilename() { return myUserConfigFile; }

    /**
      This method should be called to get the filename of the system
      config (stellarc) file.

      @return String representing the full path of the system config filename.
    */
    string systemConfigFilename() { return mySystemConfigFile; }

  public:
    // The following settings are needed by the emulation core and are
    // common among all settings objects

    // Indicates what the desired frame rate is
    uInt32 theDesiredFrameRate;

    // The keymap to use
    string theKeymapList;

    // The joymap to use
    string theJoymapList;

    // The scale factor for the window/screen
    uInt32 theZoomLevel;

    // The path to save snapshot files
    string theSnapshotDir;

    // What the snapshot should be called (romname or md5sum)
    string theSnapshotName;

    // Indicates whether to generate multiple snapshots or keep
    // overwriting the same file.
    bool theMultipleSnapshotFlag;

#ifdef DEVELOPER_SUPPORT
    // User-modified properties
    Properties userDefinedProperties;

    // Whether to save user-defined properties to a file or
    // merge into the propertiesset file for future use
    bool theMergePropertiesFlag;
#endif

  protected:
    bool myPauseIndicator;
    bool myQuitIndicator;

    string myStateDir;
    string myStateFile;
    string mySnapshotFile;
    string myUserPropertiesFile;
    string mySystemPropertiesFile;
    string myUserConfigFile;
    string mySystemConfigFile;

    // The global Console object
    Console* myConsole;

    // The full pathname of the settings file for input
    string mySettingsInputFilename;

    // The full pathname of the settings file for output
    string mySettingsOutputFilename;

  private:
    // Copy constructor isn't supported by this class so make it private
    Settings(const Settings&);

    // Assignment operator isn't supported by this class so make it private
    Settings& operator = (const Settings&);

    void set(string& key, string& value);
};

#endif
