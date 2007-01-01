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
// $Id: OSystemGP2X.hxx,v 1.10 2007-01-01 18:04:51 stephena Exp $
// Modified by Alex Zaballa on 2006/01/04 for use on GP2X
//============================================================================

#ifndef OSYSTEM_GP2X_HXX
#define OSYSTEM_GP2X_HXX

#include "bspf.hxx"


/**
  This class defines GP2X system specific settings.
*/
class OSystemGP2X : public OSystem
{
  public:
    /**
      Create a new GP2X-specific operating system object
    */
    OSystemGP2X(const string& path);

    /**
      Destructor
    */
    virtual ~OSystemGP2X();

  public:
    /**
      This method returns number of ticks in microseconds.

      @return Current time in microseconds.
    */
    uInt32 getTicks();

    /**
      This method queries the dimensions of the screen for this hardware.
    */
    void getScreenDimensions(int& width, int& height);

    /**
      This method determines the default mapping of joystick buttons to
      Stella events for the PSP device.
    */
    void setDefaultJoymap();

    /**
      This method creates events from platform-specific hardware.
    */
    void pollEvent();

    /**
      This method answers whether the given button as already been
      handled by the pollEvent() method, and as such should be ignored
      in the main event handler.
    */
    bool joyButtonHandled(int button);

  private:
    enum {
      kJDirUp    =  0,  kJDirUpLeft    =  1,
      kJDirLeft  =  2,  kJDirDownLeft  =  3,
      kJDirDown  =  4,  kJDirDownRight =  5,
      kJDirRight =  6,  kJDirUpRight   =  7
    };

    uInt8* myPreviousEvents;
    uInt8* myCurrentEvents;
    bool   myActiveEvents[8];
};

#endif
