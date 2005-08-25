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
// $Id: OSystemPSP.hxx,v 1.1 2005-08-25 15:19:17 stephena Exp $
//============================================================================

#ifndef OSYSTEM_PSP_HXX
#define OSYSTEM_PSP_HXX

#include "bspf.hxx"


/**
  This class defines PSP-like OS's (Linux) system specific settings.

  @author  Stephen Anthony
  @version $Id: OSystemPSP.hxx,v 1.1 2005-08-25 15:19:17 stephena Exp $
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
    virtual void mainLoop();

    /**
      This method returns number of ticks in microseconds.

      @return Current time in microseconds.
    */
    virtual uInt32 getTicks();
};

#endif
