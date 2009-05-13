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

#include <time.h>
#include "OSystem.hxx"
#include "Random.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Random::Random()
{
  if(ourSystem)
    myValue = ourSystem->getTicks();
  else
    myValue = (uInt32)time(0);
}
 
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 Random::next()
{
  return (myValue = (myValue * 2416 + 374441) % 1771875);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const OSystem* Random::ourSystem = NULL;
