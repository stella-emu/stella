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
// Copyright (c) 1995-2020 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#ifndef PHYSICAL_JOYSTICK_HXX
#define PHYSICAL_JOYSTICK_HXX

#include "Event.hxx"
#include "EventHandlerConstants.hxx"
#include "JoyMap.hxx"

/**
  An abstraction of a physical (real) joystick in Stella.

  A PhysicalJoystick holds its own event mapping information, space for
  which is dynamically allocated based on the actual number of buttons,
  axes, etc that the device contains.

  Specific backend class(es) will inherit from this class, and implement
  functionality specific to the device.

  @author  Stephen Anthony, Thomas Jentzsch
*/

class PhysicalJoystick
{
  friend class PhysicalJoystickHandler;

  static constexpr char MODE_DELIM = '>'; // must not be '^', '|' or '#'

  public:
    PhysicalJoystick() = default;

    string getMap() const;
    bool setMap(const string& map);
    void eraseMap(EventMode mode);
    void eraseEvent(Event::Type event, EventMode mode);
    string about() const;

  protected:
    void initialize(int index, const string& desc,
                    int axes, int buttons, int hats, int balls);

  private:
    // TODO: these are not required anymore, delete or keep for future usage?
    enum JoyType {
      JT_NONE               = 0,
      JT_REGULAR            = 1,
      JT_STELLADAPTOR_LEFT  = 2,
      JT_STELLADAPTOR_RIGHT = 3,
      JT_2600DAPTOR_LEFT    = 4,
      JT_2600DAPTOR_RIGHT   = 5
    };

    JoyType type{JT_NONE};
    int ID{-1};
    string name{"None"};
    int numAxes{0}, numButtons{0}, numHats{0};
    IntArray axisLastValue;
    IntArray buttonLast;

    // Hashmaps of controller events
    JoyMap joyMap;

  private:
    void getValues(const string& list, IntArray& map) const;

    friend ostream& operator<<(ostream& os, const PhysicalJoystick& s) {
      os << "  ID: " << s.ID << ", name: " << s.name << ", numaxis: " << s.numAxes
         << ", numbtns: " << s.numButtons << ", numhats: " << s.numHats;
      return os;
    }

    // Following constructors and assignment operators not supported
    PhysicalJoystick(const PhysicalJoystick&) = delete;
    PhysicalJoystick(PhysicalJoystick&&) = delete;
    PhysicalJoystick& operator=(const PhysicalJoystick&) = delete;
    PhysicalJoystick& operator=(PhysicalJoystick&&) = delete;
};

#endif
