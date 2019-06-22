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

#ifndef PHYSICAL_JOYSTICK_HANDLER_HXX
#define PHYSICAL_JOYSTICK_HANDLER_HXX

#include <map>

class OSystem;
class EventHandler;
class Event;

#include "bspf.hxx"
#include "EventHandlerConstants.hxx"
#include "PhysicalJoystick.hxx"
#include "Variant.hxx"

using PhysicalJoystickPtr = shared_ptr<PhysicalJoystick>;

/**
  This class handles all physical joystick-related operations in Stella.

  It is responsible for adding/accessing/removing PhysicalJoystick objects,
  and getting/setting events associated with joystick actions (button presses,
  axis/hat actions, etc).

  Essentially, this class is an extension of the EventHandler class, but
  handling only joystick-specific functionality.

  @author  Stephen Anthony, Thomas Jentzsch
*/
class PhysicalJoystickHandler
{
  private:
    struct StickInfo
    {
      StickInfo(const string& map = EmptyString, PhysicalJoystickPtr stick = nullptr)
        : mapping(map), joy(stick) {}

      string mapping;
      PhysicalJoystickPtr joy;

      friend ostream& operator<<(ostream& os, const StickInfo& si) {
        os << "  joy: " << si.joy << endl << "  map: " << si.mapping;
        return os;
      }
    };

  public:
    PhysicalJoystickHandler(OSystem& system, EventHandler& handler, Event& event);

    /** Return stick ID on success, -1 on failure. */
    int add(PhysicalJoystickPtr stick);
    bool remove(int id);
    bool remove(const string& name);
    void mapStelladaptors(const string& saport);
    void setDefaultMapping(Event::Type type, EventMode mode);
    void eraseMapping(Event::Type event, EventMode mode);
    void saveMapping();
    string getMappingDesc(Event::Type, EventMode mode) const;

    /** Bind a physical joystick event to a virtual event/action. */
    bool addJoyMapping(Event::Type event, EventMode mode, int stick,
                       int button, JoyAxis axis, int value);
    bool addJoyHatMapping(Event::Type event, EventMode mode, int stick,
                          int button, int hat, JoyHat hdir);

    /** Handle a physical joystick event. */
    void handleAxisEvent(int stick, int axis, int value);
    void handleBtnEvent(int stick, int button, bool pressed);
    void handleHatEvent(int stick, int hat, int value);

    Event::Type eventForAxis(EventMode mode, int stick, int axis, int value, int button) const {
      const PhysicalJoystickPtr j = joy(stick);
      return j->joyMap.get(mode, button, JoyAxis(axis),
                           value == int(JoyDir::NONE) ? JoyDir::NONE : value > 0 ? JoyDir::POS : JoyDir::NEG);
    }
    Event::Type eventForButton(EventMode mode, int stick, int button) const {
      const PhysicalJoystickPtr j = joy(stick);
      return j->joyMap.get(mode, button);
    }
    Event::Type eventForHat(EventMode mode, int stick, int hat, JoyHat hatDir, int button) const {
      const PhysicalJoystickPtr j = joy(stick);
      return j->joyHatMap.get(mode, button, hat, hatDir);
    }

    /** Returns a list of pairs consisting of joystick name and associated ID. */
    VariantList database() const;

  private:
    using StickDatabase = std::map<string,StickInfo>;
    using StickList = std::map<int, PhysicalJoystickPtr>;

    OSystem& myOSystem;
    EventHandler& myHandler;
    Event& myEvent;

    // Contains all joysticks that Stella knows about, indexed by name
    StickDatabase myDatabase;

    // Contains only joysticks that are currently available, indexed by id
    StickList mySticks;

    // Get joystick corresponding to given id (or nullptr if it doesn't exist)
    // Make this inline so it's as fast as possible
    const PhysicalJoystickPtr joy(int id) const {
      const auto& i = mySticks.find(id);
      return i != mySticks.cend() ? i->second : nullptr;
    }

    // Set default mapping for given joystick when no mappings already exist
    void setStickDefaultMapping(int stick, Event::Type type, EventMode mode);

    friend ostream& operator<<(ostream& os, const PhysicalJoystickHandler& jh);

    // Static lookup tables for Stelladaptor/2600-daptor axis/button support
    static const int NUM_JOY_BTN = 4;
    static const int NUM_KEY_BTN = 12;

    static const Event::Type SA_Axis[NUM_PORTS][NUM_JOY_AXIS];
    static const Event::Type SA_Button[NUM_PORTS][NUM_JOY_BTN];
    static const Event::Type SA_Key[NUM_PORTS][NUM_KEY_BTN];
};

#endif
