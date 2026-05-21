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

#ifndef PHYSICAL_KEYBOARD_HANDLER_HXX
#define PHYSICAL_KEYBOARD_HANDLER_HXX

class OSystem;
class EventHandler;

#include "bspf.hxx"
#include "Control.hxx"
#include "Event.hxx"
#include "EventHandlerConstants.hxx"
#include "Props.hxx"
#include "KeyMap.hxx"
#include "StellaKeys.hxx"

/**
  This class handles all physical keyboard-related operations in Stella.

  It is responsible for getting/setting events associated with keyboard
  actions.

  Essentially, this class is an extension of the EventHandler class, but
  handling only keyboard-specific functionality.

  @author  Stephen Anthony, Thomas Jentzsch
*/
class PhysicalKeyboardHandler
{
  public:

    PhysicalKeyboardHandler(OSystem& system, EventHandler& handler);

    void loadSerializedMappings(string_view serializedMappings, EventMode mode);

    void setDefaultMapping(Event::Type event, EventMode mode,
                           bool updateDefaults = false);

    /** define mappings for current controllers */
    void defineControllerMappings(Controller::Type type,
                                  Controller::Jack port,
                                  const Properties& properties,
                                  Controller::Type qtType1 = Controller::Type::Unknown,
                                  Controller::Type qtType2 = Controller::Type::Unknown);
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

    bool checkEventForKey(EventMode mode, StellaKey key, StellaMod mod) const {
      return myKeyMap.check(mode, key, mod);
    }

    /** See comments on KeyMap.myModEnabled for more information. */
    bool& useModKeys() { return myKeyMap.enableMod(); }

    void toggleModKeys(bool toggle = true);

  private:

    // Structure used for action menu items
    struct EventMapping {
      Event::Type event{Event::NoType};
      StellaKey key{StellaKey::UNKNOWN};
      StellaMod mod{StellaMod::NONE};
    };
    using EventMappingSpan = std::span<const EventMapping>;

    // Checks if the given mapping is used by any event mode
    bool isMappingUsed(EventMode mode, const EventMapping& map) const;

    void setDefaultKey(EventMapping map, Event::Type event = Event::NoType,
      EventMode mode = EventMode::kEmulationMode, bool updateDefaults = false);

    /** returns the event's controller mode */
    static EventMode getEventMode(Event::Type event, EventMode mode);
    /** Checks event type. */
    static bool isJoystickEvent(Event::Type event);
    static bool isPaddleEvent(Event::Type event);
    static bool isKeyboardEvent(Event::Type event);
    static bool isDrivingEvent(Event::Type event);
    static bool isCommonEvent(Event::Type event);

    void enableCommonMappings();

    void enableMappings(const Event::EventSet& events, EventMode mode);
    void enableMapping(Event::Type event, EventMode mode);

    void applyDefaultMappings(EventMappingSpan mappings,
      Event::Type event, EventMode mode, bool updateDefaults);

    /** return event mode for given property */
    static EventMode getMode(const Properties& properties, PropType propType);
    /** return event mode for given controller type */
    static EventMode getMode(Controller::Type type);

#ifdef DEBUG_BUILD
    void verifyDefaultMapping(EventMappingSpan mapping,
      EventMode mode, string_view name);
#endif

  private:
    OSystem& myOSystem;
    EventHandler& myHandler;

    // Hashmap of key events
    KeyMap myKeyMap;

    EventMode myLeftMode{EventMode::kEmulationMode};
    EventMode myRightMode{EventMode::kEmulationMode};
    // Additional modes for QuadTari controller
    EventMode myLeft2ndMode{EventMode::kEmulationMode};
    EventMode myRight2ndMode{EventMode::kEmulationMode};

    // Controller menu and common emulation mappings
    static const EventMappingSpan DefaultMenuMapping;
  #ifdef GUI_SUPPORT
    static const EventMappingSpan FixedEditMapping;
  #endif
  #ifdef DEBUGGER_SUPPORT
    static const EventMappingSpan FixedPromptMapping;
  #endif
    static const EventMappingSpan DefaultCommonMapping;
    // Controller specific mappings
    static const EventMappingSpan DefaultJoystickMapping;
    static const EventMappingSpan DefaultPaddleMapping;
    static const EventMappingSpan DefaultKeyboardMapping;
    static const EventMappingSpan DefaultDrivingMapping;
    static const EventMappingSpan CompuMateMapping;
};

#endif  // PHYSICAL_KEYBOARD_HANDLER_HXX
