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
// $Id: OSystemUNIX.hxx,v 1.14 2007-01-01 18:04:55 stephena Exp $
//============================================================================

#ifndef OSYSTEM_UNIX_HXX
#define OSYSTEM_UNIX_HXX

#include "bspf.hxx"


/**
  This class defines UNIX-like OS's (Linux) system specific settings.

  @author  Stephen Anthony
  @version $Id: OSystemUNIX.hxx,v 1.14 2007-01-01 18:04:55 stephena Exp $
*/
class OSystemUNIX : public OSystem
{
  public:
    /**
      Create a new UNIX-specific operating system object
    */
    OSystemUNIX(const string& path);

    /**
      Destructor
    */
    virtual ~OSystemUNIX();

  public:
    /**
      This method returns number of ticks in microseconds.

      @return Current time in microseconds.
    */
    uInt32 getTicks();

    /**
      This method queries the dimensions of the screen for this hardware.
      It is assumed that a UNIX SDL framebuffer is using X11.
    */
    void getScreenDimensions(int& width, int& height);
};

#endif
