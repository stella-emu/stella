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
// Copyright (c) 1995-2011 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

#ifndef OSYSTEM_UNIX_HXX
#define OSYSTEM_UNIX_HXX

#include "bspf.hxx"

/**
  This class defines UNIX-like OS's (Linux) system specific settings.

  @author  Stephen Anthony
  @version $Id$
*/
class OSystemUNIX : public OSystem
{
  public:
    /**
      Create a new UNIX-specific operating system object
    */
    OSystemUNIX();

    /**
      Destructor
    */
    virtual ~OSystemUNIX();

  public:
    /**
      Move X11 window to given position.  Width and height are not
      used (or modified).
    */
    void setAppWindowPos(int x, int y, /* not used*/ int, int);
};

#endif
