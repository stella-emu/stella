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

#ifndef REWIND_MANAGER_HXX
#define REWIND_MANAGER_HXX

class OSystem;
class StateManager;

#include "LinkedObjectPool.hxx"
#include "bspf.hxx"

/**
  This class is used to save (and later 'replay') system save states.
  In this implementation, we assume states are added at the end of the list.

  Rewinding involves moving the internal iterator backwards in time (towards
  the beginning of the list).

  Unwinding involves moving the internal iterator forwards in time (towards
  the end of the list).

  Any time a new state is added, all states from the current iterator position
  to the end of the list (aka, all future states) are removed, and the internal
  iterator moves to the insertion point of the data (the end of the list).

  @author  Stephen Anthony
*/
class RewindManager
{
  public:
    RewindManager(OSystem& system, StateManager& statemgr);

  public:
    static const int NUM_INTERVALS = 7;
    const uInt32 INTERVAL_CYCLES[NUM_INTERVALS] = {
      76 * 262,
      76 * 262 * 3,
      76 * 262 * 10,
      76 * 262 * 30,
      76 * 262 * 60,
      76 * 262 * 60 * 3,
      76 * 262 * 60 * 10
    };
    /*static const int NUM_INTERVALS = 6;
    const string INTERVALS[NUM_INTERVALS] = { "1 scanline", "50 scanlines", "1 frame", "10 frames",
    "1 second", "10 seconds" };
    const uInt32 INTERVAL_CYCLES[NUM_INTERVALS] = { 76, 76 * 50, 76 * 262, 76 * 262 * 10,
    76 * 262 * 60, 76 * 262 * 60 * 10 };*/
    const string INT_SETTINGS[NUM_INTERVALS] = {
      "1f",
      "3f",
      "10f",
      "30f",
      "1s",
      "3s",
      "10s"
    };

    static const int NUM_HORIZONS = 8;
    const uInt64 HORIZON_CYCLES[NUM_HORIZONS] = {
      76 * 262 * 60 * 3,
      76 * 262 * 60 * 10,
      76 * 262 * 60 * 30,
      76 * 262 * 60 * 60,
      76 * 262 * 60 * 60 * 3,
      76 * 262 * 60 * 60 * 10,
      uInt64(76) * 262 * 60 * 60 * 30,
      uInt64(76) * 262 * 60 * 60 * 60
    };
    /*static const int NUM_HORIZONS = 7;
    const string HORIZONS[NUM_HORIZONS] = { "~1 frame", "~10 frames", "~1 second", "~10 seconds",
    "~1 minute", "~10 minutes", "~60 minutes" };
    const uInt64 HORIZON_CYCLES[NUM_HORIZONS] = { 76 * 262, 76 * 262 * 10, 76 * 262 * 60, 76 * 262 * 60 * 10,
    76 * 262 * 60 * 60, 76 * 262 * 60 * 60 * 10, uInt64(76) * 262 * 60 * 60 * 60 };*/
    const string HOR_SETTINGS[NUM_HORIZONS] = {
      "3s",
      "10s",
      "30s",
      "1m",
      "3m",
      "10m",
      "30m",
      "60m"
    };

    /**
    */
    void setup();

    /**
      Add a new state file with the given message; this message will be
      displayed when the state is replayed.

      @param message  Message to display when replaying this state
    */
    bool addState(const string& message, bool continuous = false);

    /**
      Rewind one level of the state list, and display the message associated
      with that state.
    */
    bool rewindState();

    /**
      Unwind one level of the state list, and display the message associated
      with that state.
    */
    bool unwindState();

    bool atFirst() const { return myStateList.atFirst(); }
    bool atLast() const  { return myStateList.atLast();  }
    void clear() { myStateList.clear(); }

    /**
      Convert the cycles into a unit string.
    */
    string getUnitString(Int64 cycles);

  private:
    // Maximum number of states to save
    static constexpr uInt32 MAX_SIZE = 20; // TODO: use a parameter here and allow user to define size in UI

    OSystem& myOSystem;
    StateManager& myStateManager;

    uInt32 mySize;
    uInt32 myUncompressed;
    uInt32 myInterval;
    uInt64 myHorizon;
    double myFactor;

    struct RewindState {
      Serializer data;
      string message;
      uInt64 cycle;
      int count; //  TODO - remove this

      // We do nothing on object instantiation or copy
      // The goal of LinkedObjectPool is to not do any allocations at all
      RewindState() { }
      RewindState(const RewindState&) { }

      // Output object info; used for debugging only
      friend ostream& operator<<(ostream& os, const RewindState& s) {
        return os << "msg: " << s.message << "   cycle: " << s.cycle << "   count: " << s.count;
      }
    };

    // The linked-list to store states (internally it takes care of reducing
    // frequent (de)-allocations)
    Common::LinkedObjectPool<RewindState, MAX_SIZE> myStateList;

    void compressStates();

    string getMessage(RewindState& state);

  private:
    // Following constructors and assignment operators not supported
    RewindManager() = delete;
    RewindManager(const RewindManager&) = delete;
    RewindManager(RewindManager&&) = delete;
    RewindManager& operator=(const RewindManager&) = delete;
    RewindManager& operator=(RewindManager&&) = delete;
};

#endif
