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
// Copyright (c) 1995-2019 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#ifndef PHYSICAL_KEYBOARD_HANDLER_HXX
#define PHYSICAL_KEYBOARD_HANDLER_HXX

#include <map>
#include <set>

class OSystem;
class EventHandler;

#include "bspf.hxx"
#include "EventHandlerConstants.hxx"
#include "KeyMap.hxx"

using EventSet = std::set<Event::Type>;

/**
  This class handles all physical keyboard-related operations in Stella.

  It is responsible for getting/setting events associated with keyboard
  actions.

  Essentially, this class is an extension of the EventHandler class, but
  handling only keyboard-specific functionality.

  @author  Stephen Anthony
*/
class PhysicalKeyboardHandler
{
  public:

    PhysicalKeyboardHandler(OSystem& system, EventHandler& handler);

    void setDefaultMapping(Event::Type type, EventMode mode, bool updateDefaults = false);

    /** define mappings for current controllers */
    void defineControllerMappings(const string& controllerName, Controller::Jack port);
    /** enable mappings for emulation mode */
    void enableEmulationMappings();

    void eraseMapping(Event::Type event, EventMode mode);
    void saveMapping();

    string getMappingDesc(Event::Type event, EventMode mode) const {
      return myKeyMap.getEventMappingDesc(event, getEventMode(event, mode));
    }

    /** Bind a physical keyboard event to a virtual event/action. */
    bool addMapping(Event::Type event, EventMode mode, StellaKey key, StellaMod mod);

    /** Handle a physical keyboard event. */
    void handleEvent(StellaKey key, StellaMod mod, bool pressed, bool repeated);

    Event::Type eventForKey(EventMode mode, StellaKey key, StellaMod mod) const {
      return myKeyMap.get(mode, key, mod);
    }

  #ifdef BSPF_UNIX
    /** See comments on 'myAltKeyCounter' for more information. */
    uInt8& altKeyCount() { return myAltKeyCounter; }
  #endif

    /** See comments on KeyMap.myModEnabled for more information. */
    bool& useModKeys() { return myKeyMap.enableMod(); }

  private:

    // Structure used for action menu items
    struct EventMapping {
      Event::Type event;
      StellaKey key;
      int mod = KBDM_NONE;
    };
    using EventMappingArray = std::vector<EventMapping>;

    void setDefaultKey(EventMapping map, Event::Type event = Event::NoType,
      EventMode mode = kEmulationMode, bool updateDefaults = false);

    /** returns the event's controller mode */
    EventMode getEventMode(const Event::Type event, const EventMode mode) const;
    /** Checks event type. */
    bool isJoystickEvent(const Event::Type event) const;
    bool isPaddleEvent(const Event::Type event) const;
    bool isKeypadEvent(const Event::Type event) const;
    bool isCommonEvent(const Event::Type event) const;

    void enableCommonMappings();

    void enableMappings(const EventSet events, EventMode mode);
    void enableMapping(const Event::Type event, EventMode mode);

    OSystem& myOSystem;
    EventHandler& myHandler;

    // Hashmap of key events
    KeyMap myKeyMap;

    EventMode myLeftMode;
    EventMode myRightMode;

  #ifdef BSPF_UNIX
    // Sometimes key combos with the Alt key become 'stuck' after the
    // window changes state, and we want to ignore that event
    // For example, press Alt-Tab and then upon re-entering the window,
    // the app receives 'tab'; obviously the 'tab' shouldn't be happening
    // So we keep track of the cases that matter (for now, Alt-Tab)
    // and swallow the event afterwards
    // Basically, the initial event sets the variable to 1, and upon
    // returning to the app (ie, receiving EVENT_WINDOW_FOCUS_GAINED),
    // the count is updated to 2, but only if it was already updated to 1
    // TODO - This may be a bug in SDL, and might be removed in the future
    //        It only seems to be an issue in Linux
    uInt8 myAltKeyCounter;
  #endif

    // Hold controller related events
    static EventSet LeftJoystickEvents;
    static EventSet RightJoystickEvents;
    static EventSet LeftPaddlesEvents;
    static EventSet RightPaddlesEvents;
    static EventSet LeftKeypadEvents;
    static EventSet RightKeypadEvents;

    static EventMappingArray DefaultMenuMapping;
    static EventMappingArray DefaultCommonMapping;
    static EventMappingArray DefaultJoystickMapping;
    static EventMappingArray DefaultPaddleMapping;
    static EventMappingArray DefaultKeypadMapping;
    static EventMappingArray CompuMateMapping;
};

#endif
