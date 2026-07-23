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
// Copyright (c) 1995-2026 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include <cmath>

#include "OSystem.hxx"
#include "Console.hxx"
#include "Serializer.hxx"
#include "StateManager.hxx"
#include "TIA.hxx"
#include "EventHandler.hxx"

#include "RewindManager.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RewindManager::RewindManager(OSystem& system, StateManager& statemgr)
  : myOSystem{system},
    myStateManager{statemgr}
{
  setup();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RewindManager::setup()
{
  myLastTimeMachineAdd = false;

  const string& prefix = myOSystem.settings().getBool("dev.settings") ? "dev." : "plr.";

  mySize = std::clamp<uInt32>(
      myOSystem.settings().getInt(prefix + "tm.size"), MIN_BUF_SIZE, MAX_BUF_SIZE);
  if(mySize != myStateList.capacity())
    resize(mySize);

  myUncompressed = std::clamp<uInt32>(
      myOSystem.settings().getInt(prefix + "tm.uncompressed"), 0, mySize);

  myInterval = INTERVAL_CYCLES[0];
  // NOLINTNEXTLINE(readability-qualified-auto)
  if(const auto it = std::ranges::find(INT_SETTINGS, myOSystem.settings().getString(prefix + "tm.interval"));
     it != INT_SETTINGS.end())
    myInterval = INTERVAL_CYCLES[std::distance(INT_SETTINGS.begin(), it)];

  myHorizon = HORIZON_CYCLES[NUM_HORIZONS-1];
  // NOLINTNEXTLINE(readability-qualified-auto)
  if(const auto it = std::ranges::find(HOR_SETTINGS, myOSystem.settings().getString(prefix + "tm.horizon"));
     it != HOR_SETTINGS.end())
    myHorizon = HORIZON_CYCLES[std::distance(HOR_SETTINGS.begin(), it)];

  // calc interval growth factor for compression
  // this factor defines the backward horizon
  constexpr double MAX_FACTOR = 1E8;
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
    // sum up interval cycles (first state is not compressed)
    for(uInt32 i = myUncompressed + 1; i < mySize; ++i)
    {
      interval *= myFactor;
      cycleSum += interval;
    }
    const double diff = cycleSum - myHorizon;

    // exit loop if result is close enough
    if(std::abs(diff) < myHorizon * 1E-5)
      break;
    // define new boundary
    if(cycleSum < myHorizon)
      minFactor = myFactor;
    else
      maxFactor = myFactor;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool RewindManager::addState(string_view message, bool timeMachine)
{
  // only check for Time Machine states, ignore for debugger
  if(timeMachine && myStateList.currentIsValid())
  {
    // check if the current state has the right interval from the last state
    const RewindState& lastState = myStateList.current();
    uInt32 interval = myInterval;

    // adjust frame timed intervals to actual scanlines (vs 262)
    if(interval >= 76 * 262 && interval <= 76 * 262 * 30)
    {
      const uInt32 scanlines = std::max<uInt32>
        (myOSystem.console().tia().scanlinesLastFrame(), 240);

      interval = interval * scanlines / 262;
    }

    if(myOSystem.console().system().cycles() - lastState.cycles < interval)
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
    state.cycles = myOSystem.console().system().cycles();
    myLastTimeMachineAdd = timeMachine;
    return true;
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 RewindManager::rewindStates(uInt32 numStates)
{
  return windStates(numStates, false);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 RewindManager::unwindStates(uInt32 numStates)
{
  return windStates(numStates, true);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 RewindManager::windStates(uInt32 numStates, bool unwind)
{
  const uInt64 startCycles = myOSystem.console().system().cycles();
  uInt32 i{0};

  for(i = 0; i < numStates; ++i)
  {
    if(unwind ? !atLast() : !atFirst())
    {
      if(unwind)
        myStateList.moveToNext();
      else if(!myLastTimeMachineAdd)
        myStateList.moveToPrevious();
      else
        // last state was added automatically one interval before, so skip the move
        myLastTimeMachineAdd = false;

      myStateList.current().data.rewind();
    }
    else
      break;
  }

  const string message = i
    ? loadState(startCycles, i)
    : (unwind ? "Unwind not possible" : "Rewind not possible");

  if(myOSystem.eventHandler().state() != EventHandlerState::TIMEMACHINE
     && myOSystem.eventHandler().state() != EventHandlerState::PLAYBACK)
    myOSystem.frameBuffer().showTextMessage(message);

  return i;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string RewindManager::saveAllStates()
{
  const uInt32 numStates = myStateList.size();
  if(numStates == 0)
    return "Nothing to save";

  try
  {
    const string path = std::format("{}{}.sta",
      myOSystem.stateDir().getPath(),
      myOSystem.console().properties().get(PropType::Cart_Name));

    Serializer out(path, Serializer::FileMode::ReadWriteTrunc);
    if(!out)
      return "Can't save to all states file";

    out.putString(StateManager::STATE_HEADER);
    out.putShort(numStates);

    ByteArray buffer;
    for(auto& state : myStateList)
    {
      Serializer& s = state.data;
      const auto stateSize = static_cast<uInt32>(s.size());

      s.rewind();
      buffer.resize(stateSize);
      s.getByteArray(buffer);

      out.putInt(stateSize);
      out.putByteArray(buffer);
      out.putString(state.message);
      out.putLong(state.cycles);
    }
    return std::format("Saved {} states", numStates);
  }
  catch(...)
  {
    return "Error saving all states";
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string RewindManager::loadAllStates()
{
  try
  {
    const string path = std::format("{}{}.sta",
      myOSystem.stateDir().getPath(),
      myOSystem.console().properties().get(PropType::Cart_Name));

    // Make sure the file can be opened for reading
    Serializer in(path, Serializer::FileMode::ReadOnly);
    if (!in)
      return "Can't load from all states file";

    clear();
    uInt32 numStates = 0;

    // Load header and check compatibility
    if (in.getString() != StateManager::STATE_HEADER)
      return "Incompatible all states file";

    numStates = in.getShort();
    ByteArray buffer;
    for(uInt32 i = 0; i < numStates; ++i)
    {
      if(myStateList.full())
        compressStates();

      const uInt32 stateSize = in.getInt();

      myStateList.addLast();
      RewindState& state = myStateList.current();
      Serializer& s = state.data;

      s.rewind();
      buffer.resize(stateSize);
      in.getByteArray(buffer);
      s.putByteArray(buffer);
      state.message = in.getString();
      state.cycles = in.getLong();
    }

    // initialize current state (parameters ignored)
    loadState(0, 0);

    return std::format("Loaded {} states", numStates);
  }
  catch (...)
  {
    return "Error loading all states";
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RewindManager::compressStates()
{
  double expectedCycles = myInterval * myFactor * (1 + myFactor);
  double maxError = 1.5;
  uInt32 idx = myStateList.size() - 2;
  // in case maxError is <= 1.5 remove first state by default:
  auto removeIter = myStateList.first();

  // iterate from last but one to first but one
  for(auto it = myStateList.previous(myStateList.last()); it != myStateList.first(); --it)
  {
    if(idx < mySize - myUncompressed)
    {
      expectedCycles *= myFactor;

      const uInt64 prevCycles = myStateList.previous(it)->cycles;
      const uInt64 nextCycles = myStateList.next(it)->cycles;
      const double error = expectedCycles / (nextCycles - prevCycles);

      if(error > maxError)
      {
        maxError = error;
        removeIter = it;
      }
    }
    --idx;
  }
   myStateList.remove(removeIter); // remove
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string RewindManager::loadState(Int64 startCycles, uInt32 numStates)
{
  RewindState& state = myStateList.current();
  Serializer& s = state.data;

  myStateManager.loadState(s);
  myOSystem.console().tia().loadDisplay(s);

  const Int64 diff = startCycles - state.cycles;

  string message = diff
    ? std::format("{} {}", diff > 0 ? "Rewind" : "Unwind", getUnitString(diff))
    : "No wind";

  message += std::format(" [{}/{}]", myStateList.currentIdx(), myStateList.size());

  // add optional message
  if(numStates == 1 && !state.message.empty())
    message += std::format(" ({})", state.message);

  return message;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string RewindManager::getUnitString(Int64 cycles)
{
  constexpr uInt64 NTSC_FREQ = 1193182; // ~76*262*60
  constexpr uInt64 PAL_FREQ  = 1182298; // ~76*312*50
  const uInt64 scanlines = std::max<uInt64>(
      myOSystem.console().tia().scanlinesLastFrame(), 240);
  const bool isNTSC = scanlines <= 287;
  const uInt64 freq = isNTSC ? NTSC_FREQ : PAL_FREQ; // = cycles/second

  static constexpr std::array<string_view, 5> UNIT_NAMES = {
    "cycle", "scanline", "frame", "second", "minute"
  };
  const std::array<uInt64, UNIT_NAMES.size() + 1> UNIT_CYCLES = {
    1, 76, 76 * scanlines, freq, freq * 60, uInt64{1} << 62
  };

  const uInt64 u_cycles = std::abs(cycles);

  auto i = 0UZ;
  for(i = 0; i < UNIT_NAMES.size() - 1; ++i)
  {
    // use the lower unit up to twice the nextCycles unit, except for an exact match of the nextCycles unit
    if(u_cycles == 0 || (u_cycles < UNIT_CYCLES[i + 1] * 2 && u_cycles % UNIT_CYCLES[i + 1] != 0))
      break;
  }

  const uInt64 count = u_cycles / UNIT_CYCLES[i];
  return std::format("{} {}{}", count, UNIT_NAMES[i], count != 1 ? "s" : "");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt64 RewindManager::getFirstCycles() const
{
  return !myStateList.empty() ? myStateList.first()->cycles : 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt64 RewindManager::getCurrentCycles() const
{
  if(myStateList.currentIsValid())
    return myStateList.current().cycles;
  else
    return 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt64 RewindManager::getLastCycles() const
{
  return !myStateList.empty() ? myStateList.last()->cycles : 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IntArray RewindManager::cyclesList() const
{
  IntArray arr;
  arr.reserve(myStateList.size());

  const uInt64 firstCycle = getFirstCycles();
  for(const auto& it: myStateList)
    arr.push_back(static_cast<uInt32>(it.cycles - firstCycle));

  return arr;
}
