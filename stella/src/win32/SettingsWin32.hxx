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
// $Id: SettingsWin32.hxx,v 1.1 2004-05-24 17:18:23 stephena Exp $
//============================================================================

#ifndef SETTINGS_WIN32_HXX
#define SETTINGS_WIN32_HXX

#include "bspf.hxx"

class Console;


/**
  This class defines Windows system specific settings.

  @author  Stephen Anthony
  @version $Id: SettingsWin32.hxx,v 1.1 2004-05-24 17:18:23 stephena Exp $
*/
class SettingsWin32 : public Settings
{
  public:
    /**
      Create a new UNIX settings object
    */
    SettingsWin32();

    /**
      Destructor
    */
    virtual ~SettingsWin32();

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
