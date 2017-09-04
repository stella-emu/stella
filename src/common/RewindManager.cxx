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
  for(int i = 0; i < MAX_SIZE; ++i)
    myStateList[i] = nullptr;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RewindManager::~RewindManager()
{
  for(int i = 0; i < MAX_SIZE; ++i)
    delete myStateList[i];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool RewindManager::addState(const string& message)
{
  // Create a new Serializer object if we need one
  if(myStateList[myTop] == nullptr)
    myStateList[myTop] = new Serializer();
  Serializer& s = *(myStateList[myTop]);

  if(s)
  {
    s.reset();
    if(myStateManager.saveState(s) && myOSystem.console().tia().saveDisplay(s))
    {
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
    Serializer& s = *(myStateList[myTop]);

    s.reset();
    myStateManager.loadState(s);
    myOSystem.console().tia().loadDisplay(s);

    return true;
  }
  else
    return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RewindManager::clear()
{
  for(int i = 0; i < MAX_SIZE; ++i)
    if(myStateList[i] != nullptr)
      myStateList[i]->reset();

  myTop = mySize = 0;
}
