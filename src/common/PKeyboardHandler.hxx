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
// Copyright (c) 1995-2018 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#ifndef PHYSICAL_KEYBOARD_HANDLER_HXX
#define PHYSICAL_KEYBOARD_HANDLER_HXX

#include <map>

class OSystem;
class EventHandler;
class Event;

#include "bspf.hxx"
#include "EventHandlerConstants.hxx"

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
    PhysicalKeyboardHandler(OSystem& system, EventHandler& handler, Event& event);

    void setDefaultMapping(Event::Type type, EventMode mode);
    void eraseMapping(Event::Type event, EventMode mode);
    void saveMapping();
    string getMappingDesc(Event::Type, EventMode mode) const;

    /** Bind a physical keyboard event to a virtual event/action. */
    bool addMapping(Event::Type event, EventMode mode, StellaKey key);

    /** Handle a physical keyboard event. */
    void handleEvent(StellaKey key, StellaMod mod, bool state);

    Event::Type eventForKey(StellaKey key, EventMode mode) const {
      return myKeyTable[key][mode];
    }

    /** See comments on 'myAltKeyCounter' for more information. */
    uInt8& altKeyCount() { return myAltKeyCounter; }

    /** See comments on 'myUseCtrlKeyFlag' for more information. */
    bool& useCtrlKey() { return myUseCtrlKeyFlag; }

  private:
    OSystem& myOSystem;
    EventHandler& myHandler;
    Event& myEvent;

    // Array of key events, indexed by StellaKey
    Event::Type myKeyTable[KBDK_LAST][kNumModes];

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

    // Indicates whether the key-combos tied to the Control key are
    // being used or not (since Ctrl by default is the fire button,
    // pressing it with a movement key could inadvertantly activate
    // a Ctrl combo when it isn't wanted)
    bool myUseCtrlKeyFlag;
};

#endif
