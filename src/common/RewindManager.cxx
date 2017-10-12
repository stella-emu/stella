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

#include <cmath>

#include "OSystem.hxx"
#include "Serializer.hxx"
#include "StateManager.hxx"
#include "TIA.hxx"

#include "RewindManager.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RewindManager::RewindManager(OSystem& system, StateManager& statemgr)
  : myOSystem(system),
    myStateManager(statemgr),
    myIsNTSC(true) // TODO
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool RewindManager::addState(const string& message)
{
  // TODO: remove following (preceding???) (all invalid) states
  // myStateList.removeToFront(); from current() + 1 (or - 1???)

  if(myStateList.full())
    compressStates();

  RewindState& state = myStateList.addFirst();
  Serializer& s = state.data;

  s.reset();  // rewind Serializer internal buffers
  if(myStateManager.saveState(s) && myOSystem.console().tia().saveDisplay(s))
  {
    state.message = message;
    state.cycle = myOSystem.console().tia().cycles();
    return true;
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool RewindManager::rewindState()
{
  if(!myStateList.empty())
  {
    // TODO: get state previous to the current state instead of first()
    RewindState& state = myStateList.first();
    Serializer& s = state.data;
    string message = getMessage(state);

    s.reset();  // rewind Serializer internal buffers
    myStateManager.loadState(s);
    myOSystem.console().tia().loadDisplay(s);

    // Show message indicating the rewind state
    myOSystem.frameBuffer().showMessage(message);

    // TODO: Do NOT remove state (TODO later somewhere else: stop emulation)
    myStateList.removeFirst();

    return true;
  }
  else
    return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool RewindManager::unwindState()
{
  if(!atFirst()) // or last???
  {
    // TODO: get state next to the current state
    /*RewindState& state = myStateList.???()
    Serializer& s = state.data;
    string message = getMessage(state);

    s.reset();  // rewind Serializer internal buffers
    myStateManager.loadState(s);
    myOSystem.console().tia().loadDisplay(s);

    // Show message indicating the rewind state
    myOSystem.frameBuffer().showMessage(message);*/
    return true;
  }    
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RewindManager::compressStates()
{
  myStateList.removeLast();  // remove the oldest state file
  // TODO: add smart state removal
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string RewindManager::getMessage(RewindState& state)
{
  const Int64 NTSC_FREQ = 1193182;
  const Int64 PAL_FREQ  = 1182298;
  Int64 diff = myOSystem.console().tia().cycles() - state.cycle,
    freq = myIsNTSC ? NTSC_FREQ : PAL_FREQ,
    diffUnit;
  stringstream message;
  string unit;

  message << (diff >= 0 ? "Rewind" : "Unwind");
  diff = abs(diff);

  if(diff < 76 * 2)
  {
    unit = "cycle";
    diffUnit = diff;
  }
  else if(diff < 76 * 262 * 2)
  {
    unit = "scanline";
    diffUnit = diff / 76;
  }
  else if(diff < NTSC_FREQ * 2)
  {
    unit = "frame";
    diffUnit = diff / (76 * 262);
  }
  else if(diff < NTSC_FREQ * 60 * 2)
  {
    unit = "second";
    diffUnit = diff / NTSC_FREQ;
  }
  else
  {
    unit = "minute";
    diffUnit = diff / (NTSC_FREQ * 60);
  }
  message << " " << diffUnit << " " << unit;
  if(diffUnit != 1)
    message << "s";

  // add optional message (TODO: when smart removal works, we have to do something smart with this part too)
  if(!state.message.empty())
    message << " (" << state.message << ")";
  return message.str();
}
