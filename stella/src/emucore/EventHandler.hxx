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
// Copyright (c) 1995-1998 by Bradford W. Mott
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: EventHandler.hxx,v 1.18 2005-02-27 23:41:19 stephena Exp $
//============================================================================

#ifndef EVENTHANDLER_HXX
#define EVENTHANDLER_HXX

#include <SDL.h>

#include "bspf.hxx"
#include "Event.hxx"
#include "StellaEvent.hxx"

class Console;
class OSystem;

/**
  This class takes care of event remapping and dispatching for the
  Stella core, as well as keeping track of the current 'mode'.

  The frontend will send translated events here, and the handler will
  check to see what the current 'mode' is.

  If in emulation mode, events received from the frontend are remapped and
  sent to the emulation core.  If in menu mode, the events are sent
  unchanged to the menu class, where (among other things) changing key
  mapping can take place.

  @author  Stephen Anthony
  @version $Id: EventHandler.hxx,v 1.18 2005-02-27 23:41:19 stephena Exp $
*/
class EventHandler
{
  public:
    /**
      Create a new event handler object
    */
    EventHandler(OSystem* osystem);
 
    /**
      Destructor
    */
    virtual ~EventHandler();

    // Enumeration representing the different states of operation
    enum State { S_NONE, S_EMULATE, S_BROWSER, S_MENU, S_DEBUGGER };

    /**
      Returns the event object associated with this handler class.

      @return The event object
    */
    Event* event();

    /**
      Returns the current state of the EventHandler

      @return The State type
    */
    inline State state() { return myState; }

    /**
      Resets the state machine of the EventHandler to the defaults

      @param The current state to set
    */
    void reset(State state);

    /**
      Send an event directly to the event handler.
      These events cannot be remapped.

      @param type  The event
      @param value The value for the event
    */
    void handleEvent(Event::Type type, Int32 value);

    /**
      Send a keyboard event to the handler.

      @param key   keysym
      @param mod   modifiers
      @param state state of key
    */
    void handleKeyEvent(SDLKey key, SDLMod mod, uInt8 state);

    /**
      Send a joystick button event to the handler.

      @param stick The joystick activated
      @param code  The StellaEvent joystick code
      @param state The StellaEvent state
    */
    void sendJoyEvent(StellaEvent::JoyStick stick, StellaEvent::JoyCode code,
         Int32 state);
	
    /**
      This method indicates whether a pause event has been received.
    */
    inline bool doPause() { return myPauseFlag; }

    /**
      This method indicates whether a exit game event has been received.
    */
    inline bool doExitGame() { return myExitGameFlag; }

    /**
      This method indicates whether a quit event has been received.
    */
    inline bool doQuit() { return myQuitFlag; }

    void getKeymapArray(Event::Type** array, uInt32* size);
    void getJoymapArray(Event::Type** array, uInt32* size);

  private:
    void setKeymap();
    void setJoymap();
    void setDefaultKeymap();
    void setDefaultJoymap();

    bool isValidList(string list, uInt32 length);

    void saveState();
    void changeState();
    void loadState();
    void takeSnapshot();

  private:
    // Global OSystem object
    OSystem* myOSystem;

    // Array of key events, indexed by SDLKey
    Event::Type myKeyTable[256];

    // Array of alt-key events, indexed by SDLKey
    Event::Type myAltKeyTable[256];

    // Array of ctrl-key events, indexed by SDLKey
    Event::Type myCtrlKeyTable[256];

    // Array of joystick events
    Event::Type myJoyTable[StellaEvent::LastJSTICK*StellaEvent::LastJCODE];

    // Array of messages for each Event
    string ourMessageTable[Event::LastType];

    // Indicates the current state of the system (ie, which mode is current)
    State myState;

    // Global Event object
    Event* myEvent;

    // Indicates the current state to use for state loading/saving
    uInt32 myLSState;

    // Indicates the current pause status
    bool myPauseFlag;

    // Indicates whether to quit the current game
    bool myExitGameFlag;

    // Indicates whether to quit the emulator
    bool myQuitFlag;

    // The current keymap in string form
    string myKeymapString;

    // The current joymap in string form
    string myJoymapString;
};

#endif
