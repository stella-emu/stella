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

/**
  This class handles all physical joystick-related operations in Stella.

  It is responsible for adding/accessing/removing PhysicalJoystick objects,
  and getting/setting events associated with joystick actions (button presses,
  axis/hat actions, etc).

  Essentially, this class is an extension of the EventHandler class, but
  handling only joystick-specific functionality.

  @author  Stephen Anthony
*/
class PhysicalJoystickHandler
{
  private:
    struct StickInfo
    {
      StickInfo(const string& map = EmptyString, PhysicalJoystick* stick = nullptr)
        : mapping(map), joy(stick) {}

      string mapping;
      PhysicalJoystick* joy;

      friend ostream& operator<<(ostream& os, const StickInfo& si) {
        os << "  joy: " << si.joy << endl << "  map: " << si.mapping;
        return os;
      }
    };

  public:
    using StickDatabase = std::map<string,StickInfo>;
    using StickList = std::map<int, PhysicalJoystick*>;

    PhysicalJoystickHandler(OSystem& system, EventHandler& handler, Event& event);
    ~PhysicalJoystickHandler();

    /** Return stick ID on success, -1 on failure. */
    int add(PhysicalJoystick* stick);
    bool remove(int id);
    bool remove(const string& name);
    void mapStelladaptors(const string& saport);
    void setDefaultMapping(Event::Type type, EventMode mode);
    void eraseMapping(Event::Type event, EventMode mode);
    void saveMapping();
    string getMappingDesc(Event::Type, EventMode mode) const;

    /** Bind a physical joystick event to a virtual event/action. */
    bool addAxisMapping(Event::Type event, EventMode mode, int stick, int axis, int value);
    bool addBtnMapping(Event::Type event, EventMode mode, int stick, int button);
    bool addHatMapping(Event::Type event, EventMode mode, int stick, int hat, JoyHat value);

    /** Handle a physical joystick event. */
    void handleAxisEvent(int stick, int axis, int value);
    void handleBtnEvent(int stick, int button, uInt8 state);
    void handleHatEvent(int stick, int hat, int value);

    Event::Type eventForAxis(int stick, int axis, int value, EventMode mode) const {
      const PhysicalJoystick* j = joy(stick);
      return j ? j->axisTable[axis][(value > 0)][mode] : Event::NoType;
    }
    Event::Type eventForButton(int stick, int button, EventMode mode) const {
      const PhysicalJoystick* j = joy(stick);
      return j ? j->btnTable[button][mode] : Event::NoType;
    }
    Event::Type eventForHat(int stick, int hat, JoyHat value, EventMode mode) const {
      const PhysicalJoystick* j = joy(stick);
      return j ? j->hatTable[hat][int(value)][mode] : Event::NoType;
    }

    VariantList database() const;

  private:
    OSystem& myOSystem;
    EventHandler& myHandler;
    Event& myEvent;

    // Contains all joysticks that Stella knows about, indexed by name
    StickDatabase myDatabase;

    // Contains only joysticks that are currently available, indexed by id
    StickList mySticks;

    // Get joystick corresponding to given id (or nullptr if it doesn't exist)
    // Make this inline so it's as fast as possible
    const PhysicalJoystick* joy(int id) const {
      const auto& i = mySticks.find(id);
      return i != mySticks.cend() ? i->second : nullptr;
    }

    void setStickDefaultMapping(int stick, Event::Type type, EventMode mode);
    void printDatabase() const;

    // Static lookup tables for Stelladaptor/2600-daptor axis/button support
    static const Event::Type SA_Axis[2][2];
    static const Event::Type SA_Button[2][4];
    static const Event::Type SA_Key[2][12];
};

#endif
