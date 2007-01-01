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
// Copyright (c) 1995-2007 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: Settings.hxx,v 1.31 2007-01-01 18:04:49 stephena Exp $
//============================================================================

#ifndef SETTINGS_HXX
#define SETTINGS_HXX

class OSystem;

#include "Array.hxx"
#include "bspf.hxx"

/**
  This class provides an interface for accessing frontend specific settings.

  @author  Stephen Anthony
  @version $Id: Settings.hxx,v 1.31 2007-01-01 18:04:49 stephena Exp $
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
    bool loadCommandLine(int argc, char** argv);

    /**
      This method should be called *after* settings have been read,
      to validate (and change, if necessary) any improper settings.
    */
    void validate();

    /**
      This method should be called to display usage information.
    */
    void usage();

    /**
      Get the value assigned to the specified key.  If the key does
      not exist then 0 is returned.

      @param key The key of the setting to lookup
      @return The integer value of the setting
    */
    int getInt(const string& key) const;

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
    const string& getString(const string& key) const;

    /**
      Set the value associated with key to the given value.

      @param key   The key of the setting
      @param value The value to assign to the setting
    */
    void setInt(const string& key, const int value);

    /**
      Set the value associated with key to the given value.

      @param key   The key of the setting
      @param value The value to assign to the setting
    */
    void setFloat(const string& key, const float value);

    /**
      Set the value associated with key to the given value.

      @param key   The key of the setting
      @param value The value to assign to the setting
    */
    void setBool(const string& key, const bool value);

    /**
      Set the value associated with key to the given value.

      @param key   The key of the setting
      @param value The value to assign to the setting
    */
    void setString(const string& key, const string& value);

  private:
    // Copy constructor isn't supported by this class so make it private
    Settings(const Settings&);

    // Assignment operator isn't supported by this class so make it private
    Settings& operator = (const Settings&);

    // Trim leading and following whitespace from a string
    static string trim(string& str)
    {
      string::size_type first = str.find_first_not_of(' ');
      return (first == string::npos) ? string() :
              str.substr(first, str.find_last_not_of(' ')-first+1);
    }

  protected:
    // The parent OSystem object
    OSystem* myOSystem;

    // Structure used for storing settings
    struct Setting
    {
      string key;
      string value;
      string initialValue;
    };
    typedef Common::Array<Setting> SettingsArray;

    const SettingsArray& getInternalSettings() const
      { return myInternalSettings; }
    const SettingsArray& getExternalSettings() const
      { return myExternalSettings; }

    /** Get position in specified array of 'key' */
    int getInternalPos(const string& key) const;
    int getExternalPos(const string& key) const;

    /** Add key,value pair to specified array at specified position */
    int setInternal(const string& key, const string& value,
                    int pos = -1, bool useAsInitial = false);
    int setExternal(const string& key, const string& value,
                    int pos = -1, bool useAsInitial = false);

  private:
    // Holds key,value pairs that are necessary for Stella to
    // function and must be saved on each program exit.
    SettingsArray myInternalSettings;

    // Holds auxiliary key,value pairs that shouldn't be saved on
    // program exit.
    SettingsArray myExternalSettings;
};

#endif
