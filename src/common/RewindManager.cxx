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

//static int count = 1;
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RewindManager::RewindManager(OSystem& system, StateManager& statemgr)
  : myOSystem(system),
    myStateManager(statemgr)
{
  setup();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RewindManager::setup()
{
  string prefix = myOSystem.settings().getBool("dev.settings") ? "dev." : "plr.";

  mySize = myOSystem.settings().getInt(prefix + "rewind.size");
  if(mySize != myStateList.capacity())
    myStateList.resize(mySize);

  myUncompressed = myOSystem.settings().getInt(prefix + "rewind.uncompressed");

  myInterval = INTERVAL_CYCLES[0];
  for(int i = 0; i < NUM_INTERVALS; ++i)
    if(INT_SETTINGS[i] == myOSystem.settings().getString(prefix + "rewind.interval"))
      myInterval = INTERVAL_CYCLES[i];

  myHorizon = HORIZON_CYCLES[NUM_HORIZONS-1];
  for(int i = 0; i < NUM_HORIZONS; ++i)
    if(HOR_SETTINGS[i] == myOSystem.settings().getString(prefix + "rewind.horizon"))
      myHorizon = HORIZON_CYCLES[i];

  // calc interval growth factor
  const double MAX_FACTOR = 1E8;
  double minFactor = 0, maxFactor = MAX_FACTOR;

  while(true)
  {
    double interval = myInterval;
    double cycleSum = interval * myUncompressed;
    // calculate nextCycles factor
    myFactor = (minFactor + maxFactor) / 2;
    // horizon not reachable?
    if(myFactor == MAX_FACTOR)
      break;
    // sum up interval cycles
    for(uInt32 i = myUncompressed; i < mySize; ++i)
    {
      interval *= myFactor;
      cycleSum += interval;
    }
    double diff = cycleSum - myHorizon;

    // exit loop if result is close enough
    if(std::abs(diff) < myHorizon * 1E-5)
      break;
    // define new boundary
    if(cycleSum < myHorizon)
      minFactor = myFactor;
    else
      maxFactor = myFactor;
  }
//cerr << "factor " << myFactor << endl;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool RewindManager::addState(const string& message, bool continuous)
{
  // only check for continuous rewind states, ignore for debugger
  if(continuous && myStateList.currentIsValid())
  {
    // check if the current state has the right interval from the last state
    RewindState& lastState = myStateList.current();
    if(myOSystem.console().tia().cycles() - lastState.cycles < myInterval)
      return false;
  }

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
    state.cycles = myOSystem.console().tia().cycles();
    //state.count = count++;
//cerr << "add " << state.count << endl;
    return true;
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool RewindManager::rewindState()
{
  if(!atFirst())
  {
    RewindState& lastState = myStateList.current();

    // Set internal current iterator to previous state (back in time),
    // since we will now processed this state
    myStateList.moveToPrevious();

    RewindState& state = myStateList.current();
    Serializer& s = state.data;
    string message = getMessage(state, lastState);
//cerr << "rewind " << state.count << endl;

    s.rewind();  // rewind Serializer internal buffers
    myStateManager.loadState(s);
    myOSystem.console().tia().loadDisplay(s);

    // Show message indicating the rewind state
    myOSystem.frameBuffer().showMessage(message);
    return true;
  }
  myOSystem.frameBuffer().showMessage("Rewind not possible");
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool RewindManager::unwindState()
{
  if(!atLast())
  {
    // Set internal current iterator to nextCycles state (forward in time),
    // since we've now processed this state
    myStateList.moveToNext();

    RewindState& state = myStateList.current();
    Serializer& s = state.data;
    string message = getMessage(state, state);
//cerr << "unwind " << state.count << endl;

    s.rewind();  // rewind Serializer internal buffers
    myStateManager.loadState(s);
    myOSystem.console().tia().loadDisplay(s);

    // Show message indicating the rewind state
    myOSystem.frameBuffer().showMessage(message);
    return true;
  }
  myOSystem.frameBuffer().showMessage("Unwind not possible");
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RewindManager::compressStates()
{
  uInt64 currentCycles = myOSystem.console().tia().cycles();
  double expectedCycles = myInterval * (1*0 + myFactor);
  double maxError = 0;
  uInt32 removeIdx = 0;
  uInt32 idx = myStateList.size() - 2;

  //cerr << "idx: " << idx << endl;
  // iterate from last but one to first but one
  for(auto it = myStateList.previous(myStateList.last()); it != myStateList.first(); --it)
  {
    if(idx < mySize - myUncompressed)
    {
//cerr << *it << endl << endl;  // debug code
      expectedCycles *= myFactor;

      uInt64 prevCycles = myStateList.previous(it)->cycles;
      uInt64 nextCycles = myStateList.next(it)->cycles;
      double error = expectedCycles / (nextCycles - prevCycles);
//cerr << "prevCycles: " << prevCycles << ", nextCycles: " << nextCycles << ", error: " << error << endl;

      if(error > maxError)
      {
        maxError = error;
        removeIdx = idx;
      }
    }
    --idx;
  }
  if (maxError < 1)
  {
    // the horizon is getting too big (can happen after changing settings)
    myStateList.remove(1); // remove oldest but one
//cerr << "remove oldest + 1" << endl;
  }
  else
  {
    myStateList.remove(removeIdx); // remove
//cerr << "remove " << removeIdx << endl;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string RewindManager::getMessage(RewindState& state, RewindState& lastState)
{
  Int64 diff = myOSystem.console().tia().cycles() - state.cycles;
  stringstream message;

  message << (diff >= 0 ? "Rewind" : "Unwind") << " " << getUnitString(diff);

  // add optional message (TODO: when smart removal works, we have to do something smart with this part too)
  if(!lastState.message.empty())
    message << " (" << lastState.message << ")";
  message << " [" << myStateList.currentIdx() << "/" << myStateList.size() << "]";
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
  const Int64 UNIT_CYCLES[NUM_UNITS + 1] = { 1, 76, 76 * scanlines, freq, freq * 60, Int64(1) << 62 };

  stringstream result;
  Int32 i;

  cycles = abs(cycles);

  for(i = 0; i < NUM_UNITS - 1; ++i)
  {
    // use the lower unit up to twice the nextCycles unit, except for an exact match of the nextCycles unit
    // TODO: does the latter make sense, e.g. for ROMs with changing scanlines?
    if(cycles < UNIT_CYCLES[i + 1] * 2 && cycles % UNIT_CYCLES[i + 1] != 0)
      break;
  }
  result << cycles / UNIT_CYCLES[i] << " " << UNIT_NAMES[i];
  if(cycles / UNIT_CYCLES[i] != 1)
    result << "s";

  return result.str();
}
