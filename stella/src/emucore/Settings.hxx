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
// $Id: Settings.hxx,v 1.20 2005-02-21 20:43:21 stephena Exp $
//============================================================================

#ifndef SETTINGS_HXX
#define SETTINGS_HXX

class OSystem;

#include "bspf.hxx"


/**
  This class provides an interface for accessing frontend specific settings.

  @author  Stephen Anthony
  @version $Id: Settings.hxx,v 1.20 2005-02-21 20:43:21 stephena Exp $
*/
class Settings
{
  public:
    /**
      Create a new settings abstract class
    */
    Settings(OSystem* osystem);

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

      @return False on any errors, otherwise true
    */
    bool loadCommandLine(Int32 argc, char** argv);

    /**
      This method should be called to display usage information.
    */
    void usage();

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
    void setInt(const string& key, const uInt32 value, bool save = true);

    /**
      Set the value associated with key to the given value.

      @param key   The key of the setting
      @param value The value to assign to the setting
      @param save  Whether this setting should be saved to the rc-file.
    */
    void setFloat(const string& key, const float value, bool save = true);

    /**
      Set the value associated with key to the given value.

      @param key   The key of the setting
      @param value The value to assign to the setting
      @param save  Whether this setting should be saved to the rc-file.
    */
    void setBool(const string& key, const bool value, bool save = true);

    /**
      Set the value associated with key to the given value.

      @param key   The key of the setting
      @param value The value to assign to the setting
      @param save  Whether this setting should be saved to the rc-file.
    */
    void setString(const string& key, const string& value, bool save = true);

  public:
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
      Return the default directory for storing data.
    */
    string baseDir() { return myBaseDir; }

    /**
      Set the OSystem object for this settings class
    */
    void setOSystem(OSystem* osystem) { myOSystem = osystem; }

  protected:
    void set(const string& key, const string& value, bool save = true);

    string myBaseDir;
    string myStateDir;

    string myPropertiesInputFile;
    string myPropertiesOutputFile;
    string myConfigInputFile;
    string myConfigOutputFile;

    OSystem* myOSystem;

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

    // Trim leading and following whitespace from a string
    string trim(string& str)
    {
      string::size_type first = str.find_first_not_of(' ');
      return (first == string::npos) ? string() :
              str.substr(first, str.find_last_not_of(' ')-first+1);
    }

    // Current capacity of the settings array
    unsigned int myCapacity;
};

#endif
