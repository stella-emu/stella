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
    /**
      Add a new state file with the given message; this message will be
      displayed when the state is replayed.

      @param message  Message to display when replaying this state
    */
    bool addState(const string& message);

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

    bool atLast() const { return myStateList.empty(); }
    bool atFirst() const { return false; } // TODO
    void clear() { myStateList.clear(); }

    /**
      Convert the cycles into a unit string.
    */
    string getUnitString(Int64 cycles);

  private:
    // Maximum number of states to save
    static constexpr uInt32 MAX_SIZE = 10; // TODO: use a parameter here and allow user to define size in UI

    OSystem& myOSystem;
    StateManager& myStateManager;

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
