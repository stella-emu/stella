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
// Copyright (c) 1995-2017 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include "OSystem.hxx"
#include "Serializer.hxx"
#include "StateManager.hxx"
#include "TIA.hxx"

#include "RewindManager.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RewindManager::RewindManager(OSystem& system, StateManager& statemgr)
  : myOSystem(system),
    myStateManager(statemgr),
    mySize(0),
    myTop(0)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RewindManager::~RewindManager()
{
  for(uInt8 i = 0; i < MAX_SIZE; ++i)
    delete myStateList[i].data;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool RewindManager::addState(const string& message)
{
  // Create a new Serializer object if we need one
  if(myStateList[myTop].data == nullptr)
    myStateList[myTop].data = new Serializer();

  // And use it to store the serialized data and text message
  if(myStateList[myTop].data != nullptr)
  {
    Serializer& s = *(myStateList[myTop].data);
    s.reset();
    if(myStateManager.saveState(s) && myOSystem.console().tia().saveDisplay(s))
    {
      myStateList[myTop].message = "Rewind " + message;

      // Are we still within the allowable size, or are we overwriting an item?
      mySize++; if(mySize > MAX_SIZE) mySize = MAX_SIZE;
      myTop = (myTop + 1) % MAX_SIZE;

      return true;
    }
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool RewindManager::rewindState()
{
  if(mySize > 0)
  {
    mySize--;
    myTop = myTop == 0 ? MAX_SIZE - 1 : myTop - 1;
    Serializer& s = *(myStateList[myTop].data);

    s.reset();
    myStateManager.loadState(s);
    myOSystem.console().tia().loadDisplay(s);

    // Show message indicating the rewind state
    myOSystem.frameBuffer().showMessage(myStateList[myTop].message);

    return true;
  }
  else
    return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RewindManager::clear()
{
  for(uInt8 i = 0; i < MAX_SIZE; ++i)
    if(myStateList[i].data != nullptr)
      myStateList[i].data->reset();

  myTop = mySize = 0;
}
