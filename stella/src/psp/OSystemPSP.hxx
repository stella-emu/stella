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
// Copyright (c) 1995-2005 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: OSystemPSP.hxx,v 1.2 2006-01-05 18:53:23 stephena Exp $
//============================================================================

#ifndef OSYSTEM_PSP_HXX
#define OSYSTEM_PSP_HXX

#include "bspf.hxx"


/**
  This class defines PSP-specific settings.

  @author  Stephen Anthony
  @version $Id: OSystemPSP.hxx,v 1.2 2006-01-05 18:53:23 stephena Exp $
*/
class OSystemPSP : public OSystem
{
  public:
    /**
      Create a new PSP-specific operating system object
    */
    OSystemPSP();

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
      This method gives joystick button numbers representing the 'up', 'down',
      'left' and 'right' directions for use in the internal GUI.
    */
    void getJoyButtonDirections(int& up, int& down, int& left, int& right);

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
};

#endif
