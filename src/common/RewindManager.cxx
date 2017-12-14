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
  myLastContinuousAdd = false;

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
  // this factor defines the backward horizon
  const double MAX_FACTOR = 1E8;
  double minFactor = 0, maxFactor = MAX_FACTOR;
  myFactor = 1;

  while(myUncompressed < mySize)
  {
    double interval = myInterval;
    double cycleSum = interval * (myUncompressed + 1);
    // calculate nextCycles factor
    myFactor = (minFactor + maxFactor) / 2;
    // horizon not reachable?
    if(myFactor == MAX_FACTOR)
      break;
    // sum up interval cycles (first and last state are not compressed)
    for(uInt32 i = myUncompressed + 1; i < mySize - 1; ++i)
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
    myLastContinuousAdd = continuous;
    return true;
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 RewindManager::rewindState(uInt32 numStates)
{
  uInt64 startCycles = myOSystem.console().tia().cycles();
  uInt32 i;
  string message;

  for(i = 0; i < numStates; ++i)
  {
    if(!atFirst())
    {
      if(!myLastContinuousAdd)
        // Set internal current iterator to previous state (back in time),
        // since we will now processed this state
        myStateList.moveToPrevious();
      myLastContinuousAdd = false;

      RewindState& state = myStateList.current();
      Serializer& s = state.data;
//cerr << "rewind " << state.count << endl;
      s.rewind();  // rewind Serializer internal buffers
    }
    else
      break;
  }

  if(i)
  {
    RewindState& state = myStateList.current();
    Serializer& s = state.data;

    myStateManager.loadState(s);
    myOSystem.console().tia().loadDisplay(s);

    // Get message indicating the rewind state
    message = getMessage(startCycles, i);
  }
  else
    message = "Rewind not possible";

  myOSystem.frameBuffer().showMessage(message);
  return i;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 RewindManager::unwindState(uInt32 numStates)
{
  uInt64 startCycles = myOSystem.console().tia().cycles();
  uInt32 i;
  string message;

  for(i = 0; i < numStates; ++i)
  {
    if(!atLast())
    {
      // Set internal current iterator to nextCycles state (forward in time),
      // since we've now processed this state
      myStateList.moveToNext();

      RewindState& state = myStateList.current();
      Serializer& s = state.data;
//cerr << "unwind " << state.count << endl;
      s.rewind();  // rewind Serializer internal buffers
    }
    else
      break;
  }

  if(i)
  {
    RewindState& state = myStateList.current();
    Serializer& s = state.data;

    myStateManager.loadState(s);
    myOSystem.console().tia().loadDisplay(s);

    // Get message indicating the rewind state
    message = getMessage(startCycles, i);
    myOSystem.frameBuffer().showMessage(message);
  }
  else
    message = "Unwind not possible";

  myOSystem.frameBuffer().showMessage(message);
  return i;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RewindManager::compressStates()
{
  uInt64 currentCycles = myOSystem.console().tia().cycles();
  double expectedCycles = myInterval * myFactor * (1 + myFactor);
  double maxError = 1;
  uInt32 idx = myStateList.size() - 2;
  Common::LinkedObjectPool<RewindState>::const_iter removeIter = myStateList.first();

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
        removeIter = it;
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
    myStateList.remove(removeIter); // remove
//cerr << "remove " << removeIdx << endl;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string RewindManager::getMessage(Int64 startCycles, uInt32 numStates)
{
  RewindState& state = myStateList.current();
  Int64 diff = startCycles - state.cycles;
  stringstream message;

  message << (diff >= 0 ? "Rewind" : "Unwind") << " " << getUnitString(diff);
  message << " [" << myStateList.currentIdx() << "/" << myStateList.size() << "]";

  // add optional message
  if(numStates == 1 && !state.message.empty())
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
