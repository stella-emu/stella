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
// $Id: Settings.hxx,v 1.3 2003-09-11 20:53:51 stephena Exp $
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
  @version $Id: Settings.hxx,v 1.3 2003-09-11 20:53:51 stephena Exp $
*/
class Settings
{
  public:
    /**
      Create a new settings abstract class
    */
    Settings();
    Settings(const string& infile, const string& outfile);

    /**
      Destructor
    */
    virtual ~Settings();

  public:
    /**
      This method should be called to display the supported settings.

      @param  message A short message about this version of Stella
    */
    virtual void usage(string& message) = 0;

    /**
      This method should be called to save the current settings to an rc file.
    */
    virtual void save() = 0;

    /**
      This method should be called when the emulation core sets
      the console object.
    */
    virtual void setConsole(Console* console) = 0;

  public:
    // The following settings are used by the emulation core and are
    // common among all settings objects

    // The keymap to use
    string theKeymapList;

    // The joymap to use
    string theJoymapList;

#ifdef DEVELOPER_SUPPORT
    // User-modified properties
    Properties userDefinedProperties;

    // Whether to save user-defined properties to a file or
    // merge into the propertiesset file for future use
    bool theMergePropertiesFlag;
#endif

  private:
    // Copy constructor isn't supported by this class so make it private
    Settings(const Settings&);

    // Assignment operator isn't supported by this class so make it private
    Settings& operator = (const Settings&);
};

#endif
