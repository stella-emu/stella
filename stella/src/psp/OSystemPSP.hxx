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
// $Id: OSystemPSP.hxx,v 1.7 2007-01-01 18:04:55 stephena Exp $
//============================================================================

#ifndef OSYSTEM_PSP_HXX
#define OSYSTEM_PSP_HXX

#include "bspf.hxx"


/**
  This class defines PSP-specific settings.

  @author  Stephen Anthony
  @version $Id: OSystemPSP.hxx,v 1.7 2007-01-01 18:04:55 stephena Exp $
*/
class OSystemPSP : public OSystem
{
  public:
    /**
      Create a new PSP-specific operating system object
    */
    OSystemPSP(const string& path);

    /**
      Destructor
    */
    virtual ~OSystemPSP();

  public:
    /**
      This method runs the main loop.  Since different platforms
      may use different timing methods and/or algorithms, this method has
      been abstracted to each platform.
    */
    void mainLoop();

    /**
      This method returns number of ticks in microseconds.

      @return Current time in microseconds.
    */
    uInt32 getTicks();

    /**
      This method determines the default mapping of joystick buttons to
      Stella events for the PSP device.
    */
    void setDefaultJoymap();

    /**
      This method determines the default mapping of joystick axis to
      Stella events for for the PSP device.
    */
    void setDefaultJoyAxisMap();

    /**
      This method queries the dimensions of the screen for this hardware.
    */
    virtual void getScreenDimensions(int& width, int& height);
};

// FIXME - this doesn't even compile any more ...

/*
  kJDirUp    =  8,  kJDirUpLeft    = -1,
  kJDirLeft  =  7,  kJDirDownLeft  = -2,
  kJDirDown  =  6,  kJDirDownRight = -3,
  kJDirRight =  9,  kJDirUpRight   = -4
*/

#endif
