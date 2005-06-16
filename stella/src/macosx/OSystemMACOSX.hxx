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
// $Id: OSystemMACOSX.hxx,v 1.2 2005-06-16 01:11:29 stephena Exp $
//============================================================================

#ifndef OSYSTEM_MACOSX_HXX
#define OSYSTEM_MACOSX_HXX

#include "bspf.hxx"


/**
  This class defines UNIX-like OS's (Linux) system specific settings.

  @author  Mark Grebe
  @version $Id: OSystemMACOSX.hxx,v 1.2 2005-06-16 01:11:29 stephena Exp $
*/
class OSystemMACOSX : public OSystem
{
  public:
    /**
      Create a new UNIX-specific operating system object
    */
    OSystemMACOSX();

    /**
      Destructor
    */
    virtual ~OSystemMACOSX();

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
