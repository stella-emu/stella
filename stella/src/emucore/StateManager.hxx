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
// Copyright (c) 1995-2007 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: StateManager.hxx,v 1.1 2007-09-23 17:04:17 stephena Exp $
//============================================================================

#ifndef STATE_MANAGER_HXX
#define STATE_MANAGER_HXX

class OSystem;

/**
  This class provides an interface to all things related to emulation state.
  States can be loaded or saved here, as well as recorded, rewound, and later
  played back.

  @author  Stephen Anthony
  @version $Id: StateManager.hxx,v 1.1 2007-09-23 17:04:17 stephena Exp $
*/
class StateManager
{
  public:
    /**
      Create a new statemananger class
    */
    StateManager(OSystem* osystem);

    /**
      Destructor
    */
    virtual ~StateManager();

  public:
    /**
      Load a state into the current system

      @param slot  The state 'slot' to load state from
    */
    void loadState(int slot = -1);

    /**
      Save the current state from the system

      @param slot  The state 'slot' to save into
    */
    void saveState(int slot = -1);

    /**
      Switches to the next higher state slot (circular queue style)
    */
    void changeState();

    /**
      Resets manager to defaults
    */
    void reset();

  private:
    // Copy constructor isn't supported by this class so make it private
    StateManager(const StateManager&);

    // Assignment operator isn't supported by this class so make it private
    StateManager& operator = (const StateManager&);

  private:
    // The parent OSystem object
    OSystem* myOSystem;

    // The current slot for load/save states
    int myCurrentSlot;
};

#endif
