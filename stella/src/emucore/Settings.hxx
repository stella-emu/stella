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
// $Id: Settings.hxx,v 1.13 2004-05-28 22:07:57 stephena Exp $
//============================================================================

#ifndef SETTINGS_HXX
#define SETTINGS_HXX

#ifdef DEVELOPER_SUPPORT
  #include "Props.hxx"
#endif

#include "bspf.hxx"


/**
  This class provides an interface for accessing frontend specific settings.

  @author  Stephen Anthony
  @version $Id: Settings.hxx,v 1.13 2004-05-28 22:07:57 stephena Exp $
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
    virtual void loadConfig();

    /**
      This method should be called to save the current settings to an rc file.
    */
    virtual void saveConfig();

    /**
      This method should be called to load the arguments from the commandline.
    */
    bool loadCommandLine(Int32 argc, char** argv);

    /**
      Get the value assigned to the specified key.  If the key does
      not exist then -1 is returned.

      @param key The key of the setting to lookup
      @return The integer value of the setting
    */
    Int32 getInt(const string& key) const;

    /**
      Get the value assigned to the specified key.  If the key does
      not exist then -1.0 is returned.

      @param key The key of the setting to lookup
      @return The floating point value of the setting
    */
    float getFloat(const string& key) const;

    /**
      Get the value assigned to the specified key.  If the key does
      not exist then false is returned.

      @param key The key of the setting to lookup
      @return The boolean value of the setting
    */
    bool getBool(const string& key) const;

    /**
      Get the value assigned to the specified key.  If the key does
      not exist then the empty string is returned.

      @param key The key of the setting to lookup
      @return The string value of the setting
    */
    string getString(const string& key) const;

    /**
      Set the value associated with key to the given value.

      @param key   The key of the setting
      @param value The value to assign to the setting
      @param save  Whether this setting should be saved to the rc-file.
    */
    void setInt(const string& key, const uInt32 value);

    /**
      Set the value associated with key to the given value.

      @param key   The key of the setting
      @param value The value to assign to the setting
      @param save  Whether this setting should be saved to the rc-file.
    */
    void setFloat(const string& key, const float value);

    /**
      Set the value associated with key to the given value.

      @param key   The key of the setting
      @param value The value to assign to the setting
      @param save  Whether this setting should be saved to the rc-file.
    */
    void setBool(const string& key, const bool value);

    /**
      Set the value associated with key to the given value.

      @param key   The key of the setting
      @param value The value to assign to the setting
      @param save  Whether this setting should be saved to the rc-file.
    */
    void setString(const string& key, const string& value);

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

      @return String representing the full path of the state filename.
    */
    virtual string stateFilename(const string& md5, uInt32 state) = 0;

    /**
      This method should be called to test whether the given file exists.

      @param filename The filename to test for existence.

      @return boolean representing whether or not the file exists
    */
    virtual bool fileExists(const string& filename) = 0;

    /**
      This method should be called to display usage information.

      @param  message A short message about this version of Stella
    */
    virtual void usage(string& message) = 0;

  public:
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

    /**
      Return the default directory for storing data.
    */
    string baseDir() { return myBaseDir; }

  public:
#ifdef DEVELOPER_SUPPORT
    // User-modified properties
    Properties userDefinedProperties;
#endif

  protected:
    void set(const string& key, const string& value, bool save = true);

    string myBaseDir;
    string myStateDir;
    string myStateFile;
    string mySnapshotFile;
    string myUserPropertiesFile;
    string mySystemPropertiesFile;
    string myUserConfigFile;
    string mySystemConfigFile;

    // The full pathname of the settings file for input
    string mySettingsInputFilename;

    // The full pathname of the settings file for output
    string mySettingsOutputFilename;
	
    // Structure used for storing settings
    struct Setting
    {
      string key;
      string value;
      bool save;
    };

    // Pointer to a dynamically allocated array of settings
    Setting* mySettings;

    // Size of the settings array (i.e. the number of <key,value> pairs)
    unsigned int mySize;
	
    // Test whether the given setting is present in the array
    bool contains(const string& key);

	
  private:
    // Copy constructor isn't supported by this class so make it private
    Settings(const Settings&);

    // Assignment operator isn't supported by this class so make it private
    Settings& operator = (const Settings&);

    // Current capacity of the settings array
    unsigned int myCapacity;
};

#endif
