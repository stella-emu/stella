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
// Copyright (c) 1995-2012 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

#include "Control.hxx"
#include "System.hxx"
#include "CompuMate.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CompuMate::CompuMate(const Event& event, const System& system)
  : mySystem(system),
    myLeftController(0),
    myRightController(0),
    myCycleAtLastUpdate(0),
    myIOPort(0xff)
{
  myLeftController = new CMControl(*this, Controller::Left, event, system);
  myRightController = new CMControl(*this, Controller::Right, event, system);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CompuMate::update()
{
  uInt32 cycle = mySystem.cycles();

  // Only perform update once for both ports in the same cycle
  if(myCycleAtLastUpdate != cycle)
  {
    myCycleAtLastUpdate = cycle;
    return;
  }
  myCycleAtLastUpdate = cycle;

  // TODO - handle SWCHA changes
}
