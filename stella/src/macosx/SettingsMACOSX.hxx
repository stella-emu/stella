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
// $Id: SettingsMACOSX.hxx,v 1.1.1.1 2004-06-16 02:30:30 markgrebe Exp $
//============================================================================

#ifndef SETTINGS_MAC_OSX_HXX
#define SETTINGS_MAX_OSX_HXX

#include "bspf.hxx"

class Console;


/**
  This class defines Macintosh OSX system specific settings.

  @author  Mark Grebe
  @version $Id: SettingsMACOSX.hxx,v 1.1.1.1 2004-06-16 02:30:30 markgrebe Exp $
*/
class SettingsMACOSX : public Settings
{
  public:
    /**
      Create a new UNIX settings object
    */
    SettingsMACOSX();

    /**
      Destructor
    */
    virtual ~SettingsMACOSX();

  public:
    /**
      This method should be called to get the filename of a state file
      given the state number.

      @return String representing the full path of the state filename.
    */
    virtual string stateFilename(const string& md5, uInt32 state);

    /**
      This method should be called to test whether the given file exists.

      @param filename The filename to test for existence.

      @return boolean representing whether or not the file exists
    */
    virtual bool fileExists(const string& filename);

    /**
      Display the commandline settings for this UNIX version of Stella.

      @param  message A short message about this version of Stella
    */
    virtual void usage(string& message);
	
    /**
      This method should be called to load the current settings from the 
	  standard Mac preferences.
    */
    void loadConfig();

    /**
      This method should be called to save the current settings to the
	  standard Mac preferences.
    */
	
	void saveConfig();

};

#endif
