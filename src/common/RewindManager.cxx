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

  s.rewind();  // rewind Serializer internal buffers
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

    s.rewind();  // rewind Serializer internal buffers
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
  //bool debugMode = myOSystem.eventHandler().state() == EventHandler::S_DEBUGGER;
  // TODO: let user control these:
  const double DENSITY = 1.15; // exponential growth of cycle intervals
  const uInt32 STEP_STATES = 60; // single step rewind length
  //const uInt32 SECONDS_STATES = 10; // TODO: one second rewind length

  uInt64 currentCycle = myOSystem.console().tia().cycles();
  uInt64 lastCycle = currentCycle;
  double expectedCycles = 76 * 262.0; // == cycles of 1 frame, TODO: use actual number of scanlines
  double maxDelta = 0;
  size_type removeIdx = 0;

  size_type idx = myStateList.size() - 1;
  for(auto it = myStateList.last(); it != myStateList.first(); --it)
  {
cerr << *it << endl << endl;  // debug code
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
    --idx;
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
  const Int32 scanlines = std::max(myOSystem.console().tia().scanlinesLastFrame(), 240u);
  const bool isNTSC = scanlines <= 285; // TODO: replace magic number
  const Int32 NTSC_FREQ = 1193182; // ~76*262*60
  const Int32 PAL_FREQ = 1182298; // ~76*312*50
  const Int32 freq = isNTSC ? NTSC_FREQ : PAL_FREQ; // = cycles/second

  // TODO: do we need hours here? don't think so
  const Int32 NUM_UNITS = 5;
  const string UNIT_NAMES[NUM_UNITS] = { "cycle", "scanline", "frame", "second", "minute" };
  const Int64 UNIT_CYCLES[NUM_UNITS + 1] = { 1, 76, 76 * scanlines, freq,
      freq * 60, Int64(1) << 62 };

  stringstream result;
  Int32 i;

  cycles = abs(cycles);

  for(i = 0; i < NUM_UNITS - 1; ++i)
  {
    // use the lower unit up to twice the next unit, except for an exact match of the next unit
    // TODO: does the latter make sense, e.g. for ROMs with changing scanlines?
    if(cycles < UNIT_CYCLES[i + 1] * 2 && cycles % UNIT_CYCLES[i + 1] != 0)
      break;
  }
  result << cycles / UNIT_CYCLES[i] << " " << UNIT_NAMES[i];
  if(cycles / UNIT_CYCLES[i] != 1)
    result << "s";

  return result.str();
}

