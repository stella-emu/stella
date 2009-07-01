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
// Copyright (c) 1995-2009 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

#ifndef OSYSTEM_MACOSX_HXX
#define OSYSTEM_MACOSX_HXX

#include "bspf.hxx"


/**
  This class defines UNIX-like OS's (Linux) system specific settings.

  @author  Mark Grebe
  @version $Id$
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
      This method returns number of ticks in microseconds.

      @return Current time in microseconds.
    */
    virtual uInt64 getTicks() const;
	
    /**
      This method queries the dimensions of the screen for this hardware.
    */
    virtual void getScreenDimensions(int& width, int& height);
    
    /**
      Informs the OSystem of a change in EventHandler state.
    */
    virtual void stateChanged(EventHandler::State state);
};

#endif
