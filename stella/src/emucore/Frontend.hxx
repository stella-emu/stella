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
// Copyright (c) 1995-1998 by Bradford W. Mott
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: Frontend.hxx,v 1.1 2003-09-05 18:02:58 stephena Exp $
//============================================================================

#ifndef FRONTEND_HXX
#define FRONTEND_HXX

class Console;

#include "bspf.hxx"

/**
  This class provides an interface for accessing frontend specific data.

  @author  Stephen Anthony
  @version $Id: Frontend.hxx,v 1.1 2003-09-05 18:02:58 stephena Exp $
*/
class Frontend
{
  public:
    /**
      Create a new frontend
    */
    Frontend();
 
    /**
      Destructor
    */
    virtual ~Frontend();

  public:
    /**
      This method should be called when the emulation core sets
      the console object.
    */
        virtual void setConsole(Console* console) = 0;

    /**
      This method should be called when the emulation core receives
      a QUIT event.
    */
    virtual void quit() = 0;

    /**
      This method should be called at when the emulation core receives
      a PAUSE event.
    */
    virtual void pause(bool status) = 0;

    /**
      This method should be called to get the filename of a state file
      given the md5 and state number.

      @return String representing the full path of the state filename.
    */
    virtual string& stateFilename(string& md5, uInt32 state) = 0;

    /**
      This method should be called to get the filename of a snapshot
      file given the md5 and state number.

      @return String representing the full path of the snapshot filename.
    */
    virtual string& snapshotFilename(string& md5, uInt32 state) = 0;

    /**
      This method should be called to get the filename of the users
      properties (stella.pro) file.

      @return String representing the full path of the user properties filename.
    */
    virtual string& userPropertiesFilename() = 0;

    /**
      This method should be called to get the filename of the system
      properties (stella.pro) file.

      @return String representing the full path of the system properties filename.
    */
    virtual string& systemPropertiesFilename() = 0;

    /**
      This method should be called to get the filename of the users
      config (stellarc) file.

      @return String representing the full path of the users config filename.
    */
    virtual string& userConfigFilename() = 0;

    /**
      This method should be called to get the filename of the system
      config (stellarc) file.

      @return String representing the full path of the system config filename.
    */
    virtual string& systemConfigFilename() = 0;


  private:
    // Copy constructor isn't supported by this class so make it private
    Frontend(const Frontend&);

    // Assignment operator isn't supported by this class so make it private
    Frontend& operator = (const Frontend&);
};
#endif
