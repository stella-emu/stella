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
  This class is used to save (and later 'rewind') system save states.

  @author  Stephen Anthony
*/
class RewindManager
{
  public:
    RewindManager(OSystem& system, StateManager& statemgr);

  public:
    /**
      Add a new state file with the given message; this message will be
      displayed when the state is rewound.

      @param message  Message to display when rewinding to this state
    */
    bool addState(const string& message);

    /**
      Rewind one level of the state list, and display the message associated
      with that state.
    */
    bool rewindState();

    bool empty() const { return myStateList.empty(); }
    void clear() { myStateList.clear(); }

  private:
    // Maximum number of states to save
    static constexpr uInt32 MAX_SIZE = 100;

    OSystem& myOSystem;
    StateManager& myStateManager;

    struct RewindState {
      Serializer data;
      string message;

      // We do nothing on object instantiation or copy
      RewindState() { }
      RewindState(const RewindState&) { }
    };

    Common::LinkedObjectPool<RewindState, MAX_SIZE> myStateList;

  private:
    // Following constructors and assignment operators not supported
    RewindManager() = delete;
    RewindManager(const RewindManager&) = delete;
    RewindManager(RewindManager&&) = delete;
    RewindManager& operator=(const RewindManager&) = delete;
    RewindManager& operator=(RewindManager&&) = delete;
};

#endif
