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
// Copyright (c) 1995-2002 by Bradford W. Mott
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: FrontendUNIX.hxx,v 1.3 2003-09-07 18:30:28 stephena Exp $
//============================================================================

#ifndef FRONTEND_UNIX_HXX
#define FRONTEND_UNIX_HXX

class Console;

#include "bspf.hxx"
#include "Frontend.hxx"

/**
  This class defines UNIX-like OS's (Linux) system specific file locations
  and events.

  @author  Stephen Anthony
  @version $Id: FrontendUNIX.hxx,v 1.3 2003-09-07 18:30:28 stephena Exp $
*/
class FrontendUNIX : public Frontend
{
  public:
    /**
      Create a new UNIX frontend
    */
    FrontendUNIX();
 
    /**
      Destructor
    */
    virtual ~FrontendUNIX();

  public:
    /**
      Let the frontend know about the console object
    */
    virtual void setConsole(Console* console);

    /**
      Called when the emulation core receives a QUIT event.
    */
    virtual void setQuitEvent();

    /**
      Check whether a QUIT event has been received.
    */
    virtual bool quit();

    /**
      Called when the emulation core receives a PAUSE event.
    */
    virtual void setPauseEvent();

    /**
      Check whether a PAUSE event has been received.
    */
    virtual bool pause();

    /**
      Returns the UNIX filename representing a state file.

      @param md5   The md5 string to use as part of the filename.
      @param state The state number to use as part of the filename.
      @return      The full path and filename of the state file.
    */
    virtual string stateFilename(string& md5, uInt32 state);

    /**
      Returns the UNIX filename representing a state file.

      @param md5   The md5 string to use as part of the filename.
      @param state The state number to use as part of the filename.
      @return      The full path and filename of the snapshot file.
    */
    virtual string snapshotFilename(string& md5, uInt32 state);

    /**
      Returns the UNIX filename representing a users properties file.

      @return      The full path and filename of the user properties file.
    */
    virtual string userPropertiesFilename();

    /**
      Returns the UNIX filename representing a system properties file.

      @return      The full path and filename of the system properties file.
    */
    virtual string systemPropertiesFilename();

    /**
      Returns the UNIX filename representing a users config file.

      @return      The full path and filename of the user config file.
    */
    virtual string userConfigFilename();

    /**
      Returns the UNIX filename representing a system config file.

      @return      The full path and filename of the system config file.
    */
    virtual string systemConfigFilename();

    /**
      Returns the filename representing the users home directory.

      @return      The full path and filename of the home directory.
    */
    virtual string userHomeDir() { return myHomeDir; }

  private:
    bool myPauseIndicator;
    bool myQuitIndicator;

    string myHomeDir;
    string myStateDir;

    string mySnapshotFile;
    string myStateFile;
    string myHomePropertiesFile;
    string mySystemPropertiesFile;
    string myHomeRCFile;
    string mySystemRCFile;

    // The global Console object
    Console* myConsole;
};

#endif
