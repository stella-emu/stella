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
// $Id: EventHandler.hxx,v 1.10 2003-09-30 01:22:45 stephena Exp $
//============================================================================

#ifndef EVENTHANDLER_HXX
#define EVENTHANDLER_HXX

#include "bspf.hxx"
#include "Event.hxx"
#include "StellaEvent.hxx"

class Console;
class MediaSource;


/**
  This class takes care of event remapping and dispatching for the
  Stella core, as well as keeping track of the current 'mode'.

  The frontends will send translated events here, and the handler will
  check to see what the current 'mode' is.  For now, the modes can be
  normal and menu mode.

  If in normal mode, events received from the frontends are remapped and
  sent to the emulation core.  If in menu mode, the events are sent
  unchanged to the user interface, where (among other things) changing key
  mapping can take place.

  @author  Stephen Anthony
  @version $Id: EventHandler.hxx,v 1.10 2003-09-30 01:22:45 stephena Exp $
*/
class EventHandler
{
  public:
    /**
      Create a new event handler object
    */
    EventHandler(Console* console);
 
    /**
      Destructor
    */
    virtual ~EventHandler();

    /**
      Returns the event object associated with this handler class.

      @return The event object
    */
    Event* event();

    /**
      Send a keyboard event to the handler.

      @param code  The StellaEvent code
      @param state The StellaEvent state
    */
    void sendKeyEvent(StellaEvent::KeyCode code, Int32 state);

    /**
      Send a joystick button event to the handler.

      @param stick The joystick activated
      @param code  The StellaEvent joystick code
      @param state The StellaEvent state
    */
    void sendJoyEvent(StellaEvent::JoyStick stick, StellaEvent::JoyCode code,
         Int32 state);

    /**
      Send an event directly to the event handler.
      These events cannot be remapped.

      @param type  The event
      @param value The value for the event
    */
    void sendEvent(Event::Type type, Int32 value);
	
    /**
      Set the mediasource.

      @param mediaSource   The mediasource
    */
    void setMediaSource(MediaSource* mediaSource);

    /**
      Get the current keymapping being used in string form

      @return  The keymap list in string form
    */
    string getKeymap();

    /**
      Get the current joymapping being used in string form

      @return  The joymap list in string form
    */
    string getJoymap();

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
    // Array of key events
    Event::Type myKeyTable[StellaEvent::LastKCODE];

    // Array of joystick events
    Event::Type myJoyTable[StellaEvent::LastJSTICK*StellaEvent::LastJCODE];

    // Array of messages for each Event
    string ourMessageTable[Event::LastType];

    // Global Console object
    Console* myConsole;

    // Global Event object
    Event* myEvent;

    // Global mediasource object
    MediaSource* myMediaSource;

    // Indicates the current state to use for state loading/saving
    uInt32 myCurrentState;

    // Indicates the current pause status
    bool myPauseStatus;

    // The current keymap in string form
    string myKeymapString;

    // The current joymap in string form
    string myJoymapString;

    // Indicates that the main menu is being entered
    bool myMenuStatus;
};

#endif
