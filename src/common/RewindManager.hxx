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

/**
  This class is used to save (and later 'rewind') system save states.
  Essentially, it's a modified circular array-based stack that cleverly deals
  with allocation/deallocation of memory.

  Since the stack is circular, the oldest states are automatically overwritten
  by new ones (up to MAX_SIZE, defined below).

  @author  Stephen Anthony
*/
class RewindManager
{
  public:
    RewindManager(OSystem& system, StateManager& statemgr);
    virtual ~RewindManager();

  public:
    /**
      Add a new state file with the given message; this message will be
      displayed when the state is rewound.

      @param message  Message to display when rewinding to this state
    */
    bool addState(const string& message = "");

    /**
      Rewind one level of the state list, and display the message associated
      with that state.
    */
    bool rewindState();

    bool empty() const { return mySize == 0; }
    void clear();

  private:
    // Maximum number of states to save
    static constexpr uInt8 MAX_SIZE = 100;

    OSystem& myOSystem;
    StateManager& myStateManager;

    struct SerialData {
      Serializer* data;
      string message;

      SerialData(Serializer* d = nullptr) : data(d) { }
    };

    SerialData myStateList[MAX_SIZE];
    uInt8 mySize, myTop;

  private:
    // Following constructors and assignment operators not supported
    RewindManager() = delete;
    RewindManager(const RewindManager&) = delete;
    RewindManager(RewindManager&&) = delete;
    RewindManager& operator=(const RewindManager&) = delete;
    RewindManager& operator=(RewindManager&&) = delete;
};

#endif
