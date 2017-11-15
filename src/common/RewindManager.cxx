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

static int count = 1;
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RewindManager::RewindManager(OSystem& system, StateManager& statemgr)
  : myOSystem(system),
    myStateManager(statemgr)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool RewindManager::addState(const string& message)
{
  // Remove all future states
  myStateList.removeToLast();

  // Make sure we never run out of space
  if(myStateList.full())
    compressStates();

  // Add new state at the end of the list (queue adds at end)
  // This updates the 'current' iterator inside the list
  myStateList.addLast();
  RewindState& state = myStateList.current();
  Serializer& s = state.data;

  s.reset();  // rewind Serializer internal buffers
  if(myStateManager.saveState(s) && myOSystem.console().tia().saveDisplay(s))
  {
    state.message = message;
    state.cycle = myOSystem.console().tia().cycles();
    state.count = count++;
cerr << "add " << state.count << endl;
    return true;
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool RewindManager::rewindState()
{
  if(myStateList.currentIsValid())
  {
    RewindState& state = myStateList.current();
    Serializer& s = state.data;
    string message = getMessage(state);
cerr << "rewind " << state.count << endl;

    s.reset();  // rewind Serializer internal buffers
    myStateManager.loadState(s);
    myOSystem.console().tia().loadDisplay(s);

    // Show message indicating the rewind state
    myOSystem.frameBuffer().showMessage(message);

    // Set internal current iterator to previous state (back in time),
    // since we've now processed this state
    myStateList.moveToPrevious();

    return true;
  }
  else
    return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool RewindManager::unwindState()
{
#if 0
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
#endif
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RewindManager::compressStates()
{
#if 0
  myStateList.removeFirst();  // remove the oldest state file
#else
  bool debugMode = myOSystem.eventHandler().state() == EventHandler::S_DEBUGGER;
  // TODO: let user control these:
  const double DENSITY = 1.15; // exponential growth of cycle intervals
  const uInt32 STEP_STATES = 60; // single step rewind length
  //const uInt32 SECONDS_STATES = 10; // TODO: one second rewind length

  uInt64 currentCycle = myOSystem.console().tia().cycles();
  uInt64 lastCycle = currentCycle;
  double expectedCycles = 76 * 262.0; // == cycles of 1 frame, TODO: use actual number of scanlines
  double maxDelta = 0;
  uInt32 removeIdx = 0;

  uInt32 idx = 0;
  for(auto it = myStateList.begin(); it != myStateList.end(); ++it)
  {
    // test and never remove the very first saved state
    if(++it == myStateList.end())
    {
      break;
    }
    --it; // UGLY!

    if(idx >= STEP_STATES)
    {
      expectedCycles *= DENSITY;

      double expected = expectedCycles * (1 + DENSITY);
      uInt64 prev = (--it)->cycle; ++it; // UGLY!
      uInt64 next = (++it)->cycle; --it; // UGLY!
      double delta = expected / (prev - next);

      if(delta > maxDelta)
      {
        maxDelta = delta;
        removeIdx = idx;
      }
    }
    lastCycle = it->cycle;
    idx++;
  }
  if (maxDelta < 1)
  {
    // the horizon is getting too big
    //myStateList.remove(idx - 1); // remove oldest but one
  }
  else
  {
    //myStateList.remove(removeIdx); // remove
  }
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string RewindManager::getMessage(RewindState& state)
{
  Int64 diff = myOSystem.console().tia().cycles() - state.cycle;
  stringstream message;

  message << (diff >= 0 ? "Rewind" : "Unwind") << " " << getUnitString(diff);

  // add optional message (TODO: when smart removal works, we have to do something smart with this part too)
  if(!state.message.empty())
    message << " (" << state.message << ")";
  return message.str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string RewindManager::getUnitString(Int64 cycles)
{
  const uInt64 scanlines = myOSystem.console().tia().scanlinesLastFrame();
  const bool isNTSC = scanlines <= 285; // TODO: replace magic number
  const uInt64 NTSC_FREQ = 1193182; // ~76*262*60
  const uInt64 PAL_FREQ = 1182298; // ~76*312*50
  const uInt64 freq = isNTSC ? NTSC_FREQ : PAL_FREQ; // = cycles/second

  string unit;
  Int64 diffUnit;
  stringstream result;

  cycles = abs(cycles);
  // use the lower unit up to twice the next unit, except for an exact match of the next unit
  // TODO: does the latter make sense, e.g. for ROMs with changing scanlines?
  if(cycles < 76 * 2 && cycles % 76 != 0)
  {
    unit = "cycle";
    diffUnit = cycles;
  }
  else if(cycles < 76 * scanlines * 2 && cycles % (76 * scanlines) != 0)
  {
    unit = "scanline";
    diffUnit = cycles / 76;
  }
  else if(cycles < freq * 2 && cycles % freq != 0)
  {
    unit = "frame";
    diffUnit = cycles / (76 * scanlines);
  }
  else if(cycles < freq * 60 * 2 && cycles % (freq * 60) != 0)
  {
    unit = "second";
    diffUnit = cycles / freq;
  }
  else
  {
    unit = "minute";
    diffUnit = cycles / (freq * 60);
  } // TODO: do we need hours here? don't think so

  result << diffUnit << " " << unit;
  if(diffUnit != 1)
    result << "s";

  return result.str();
}

