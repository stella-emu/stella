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
// Copyright (c) 1995-2019 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#ifndef SETTINGS_HXX
#define SETTINGS_HXX

#include <map>

#include "Variant.hxx"
#include "bspf.hxx"

/**
  This class provides an interface for accessing frontend specific settings.

  @author  Stephen Anthony
*/
class Settings
{
  public:
    /**
      Create a new settings abstract class
    */
    explicit Settings();
    virtual ~Settings() = default;

    using Options = std::map<string, Variant>;

  public:
    /**
      This method should be called to display usage information.
    */
    void usage() const;

    /**
      This method is called to load settings from the settings file,
      and apply commandline options specified by the given parameter.

      @param cfgfile  The full path to the configuration file
      @param options  A list of options that overrides ones in the
                      settings file
    */
    void load(const string& cfgfile, const Options& options);

    /**
      This method is called to save the current settings to the
      settings file.
    */
    void save(const string& cfgfile) const;

    /**
      Get the value assigned to the specified key.

      @param key The key of the setting to lookup
      @return The (variant) value of the setting
    */
    const Variant& value(const string& key) const;

    /**
      Set the value associated with the specified key.

      @param key   The key of the setting
      @param value The (variant) value to assign to the setting
    */
    void setValue(const string& key, const Variant& value);

    /**
      Convenience methods to return specific types.

      @param key The key of the setting to lookup
      @return The specific type value of the setting
    */
    int getInt(const string& key) const     { return value(key).toInt();   }
    float getFloat(const string& key) const { return value(key).toFloat(); }
    bool getBool(const string& key) const   { return value(key).toBool();  }
    const string& getString(const string& key) const { return value(key).toString(); }
    const GUI::Size getSize(const string& key) const { return value(key).toSize();   }

  protected:
    /**
      This method will be called to load the settings from the
      platform-specific settings file.  Since different ports can have
      different behaviour here, we mark it as virtual so derived
      classes can override as needed.

      @param cfgfile  The full path to the configuration file
      @return  False on any error, else true
    */
    virtual bool loadConfigFile(const string& cfgfile);

    /**
      This method will be called to save the current settings to the
      platform-specific settings file.  Since different ports can have
      different behaviour here, we mark it as virtual so derived
      classes can override as needed.

      @param cfgfile  The full path to the configuration file
      @return  False on any error, else true
    */
    virtual bool saveConfigFile(const string& cfgfile) const;

    // Trim leading and following whitespace from a string
    static string trim(const string& str)
    {
      string::size_type first = str.find_first_not_of(' ');
      return (first == string::npos) ? EmptyString :
              str.substr(first, str.find_last_not_of(' ')-first+1);
    }

  protected:
    // Structure used for storing settings
    struct Setting
    {
      string key;
      Variant value;
      Variant initialValue;

      Setting(const string& k, const Variant& v, const Variant& i = EmptyVariant)
        : key(k), value(v), initialValue(i) { }
    };
    using SettingsArray = vector<Setting>;

    const SettingsArray& getInternalSettings() const
      { return myInternalSettings; }
    const SettingsArray& getExternalSettings() const
      { return myExternalSettings; }

    /** Get position in specified array of 'key' */
    int getInternalPos(const string& key) const;
    int getExternalPos(const string& key) const;

    /** Add key,value pair to specified array at specified position */
    int setInternal(const string& key, const Variant& value,
                    int pos = -1, bool useAsInitial = false);
    int setExternal(const string& key, const Variant& value,
                    int pos = -1, bool useAsInitial = false);

  private:
    /**
      This method must be called *after* settings have been fully loaded
      to validate (and change, if necessary) any improper settings.
    */
    void validate();

  private:
    // Holds key,value pairs that are necessary for Stella to
    // function and must be saved on each program exit.
    SettingsArray myInternalSettings;

    // Holds auxiliary key,value pairs that shouldn't be saved on
    // program exit.
    SettingsArray myExternalSettings;

  private:
    // Following constructors and assignment operators not supported
    Settings(const Settings&) = delete;
    Settings(Settings&&) = delete;
    Settings& operator=(const Settings&) = delete;
    Settings& operator=(Settings&&) = delete;
};

#endif
