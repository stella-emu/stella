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
// $Id: SettingsUNIX.hxx,v 1.3 2003-09-19 15:45:01 stephena Exp $
//============================================================================

#ifndef SETTINGS_UNIX_HXX
#define SETTINGS_UNIX_HXX

#include "bspf.hxx"

class Console;


/**
  This class defines UNIX-like OS's (Linux) system specific settings.

  @author  Stephen Anthony
  @version $Id: SettingsUNIX.hxx,v 1.3 2003-09-19 15:45:01 stephena Exp $
*/
class SettingsUNIX : public Settings
{
  public:
    /**
      Create a new UNIX settings object
    */
    SettingsUNIX();

    /**
      Destructor
    */
    virtual ~SettingsUNIX();

  public:
    /**
      This method should be called to set arguments.

      @param key   The variable to be set
      @param value The value for the variable to hold
    */
    virtual void setArgument(string& key, string& value);

    /**
      This method should be called to get system-specific settings.

      @return  A string representing all the key/value pairs.
    */
    virtual string getArguments();

    /**
      This method should be called to get the filename of a state file
      given the state number.

      @return String representing the full path of the state filename.
    */
    virtual string stateFilename(uInt32 state);

    /**
      This method should be called to get the filename of a snapshot.

      @return String representing the full path of the snapshot filename.
    */
    virtual string snapshotFilename();

  public:
    /**
      Display the commandline settings for this UNIX version of Stella.

      @param  message A short message about this version of Stella
    */
    void usage(string& message);

    /**
      Return the users UNIX home directory

      @param  message A short message about this version of Stella
    */
    string userHomeDir();

  public:
    // Indicates whether to use fullscreen
    bool theUseFullScreenFlag;

    // Indicates whether mouse can leave the game window
    bool theGrabMouseFlag;

    // Indicates whether to center the game window
    bool theCenterWindowFlag;

    // Indicates whether to show some game info on program exit
    bool theShowInfoFlag;

    // Indicates whether to show cursor in the game window
    bool theHideCursorFlag;

    // Indicates whether to allocate colors from a private color map
    bool theUsePrivateColormapFlag;

    // Indicates whether to use more/less accurate emulation,
    // resulting in more/less CPU usage.
    bool theAccurateTimingFlag;

    // Indicates what the desired volume is
    Int32 theDesiredVolume;

    // Indicate which paddle mode we're using:
    //   0 - Mouse emulates paddle 0
    //   1 - Mouse emulates paddle 1
    //   2 - Mouse emulates paddle 2
    //   3 - Mouse emulates paddle 3
    //   4 - Use real Atari 2600 paddles
    uInt32 thePaddleMode;

    // An alternate properties file to use
    string theAlternateProFile;

    // Indicates which sound driver to use at run-time
    string theSoundDriver;

    // The left joystick number (0 .. StellaEvent::LastJSTICK)
    Int32 theLeftJoystickNumber;

    // The right joystick number (0 .. StellaEvent::LastJSTICK)
    Int32 theRightJoystickNumber;

  private:
    // The UNIX home directory
    string myHomeDir;
};

#endif
