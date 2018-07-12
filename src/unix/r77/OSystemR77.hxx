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
// Copyright (c) 1995-2014 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

#ifndef OSYSTEM_R77_HXX
#define OSYSTEM_R77_HXX

#include "bspf.hxx"

/**
  This class is used for the Retron77 system.
  The Retron77 system is based on an embedded Linux platform.

  @author  Stephen Anthony
*/
class OSystemR77 : public OSystem
{
  public:
    /**
      Create a new R77-specific operating system object
    */
    OSystemR77();

    /**
      Destructor
    */
    virtual ~OSystemR77();
};

#endif
