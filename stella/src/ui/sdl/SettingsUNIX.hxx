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
// $Id: SettingsUNIX.hxx,v 1.1 2003-10-26 19:40:39 stephena Exp $
//============================================================================

#ifndef SETTINGS_UNIX_HXX
#define SETTINGS_UNIX_HXX

#include "bspf.hxx"

class Console;


/**
  This class defines UNIX-like OS's (Linux) system specific settings.

  @author  Stephen Anthony
  @version $Id: SettingsUNIX.hxx,v 1.1 2003-10-26 19:40:39 stephena Exp $
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

    /**
      Display the commandline settings for this UNIX version of Stella.

      @param  message A short message about this version of Stella
    */
    virtual void usage(string& message);
};

#endif
