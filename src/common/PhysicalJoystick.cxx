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

#include <map>

#include "OSystem.hxx"
#include "Settings.hxx"
#include "Vec.hxx"
#include "bspf.hxx"
#include "PhysicalJoystick.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PhysicalJoystick::PhysicalJoystick()
  : type(JT_NONE),
    ID(-1),
    name("None"),
    numAxes(0),
    numButtons(0),
    numHats(0),
    axisLastValue(nullptr),
    buttonLast(nullptr)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PhysicalJoystick::~PhysicalJoystick()
{
  delete[] axisLastValue;
  delete[] buttonLast;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PhysicalJoystick::initialize(int index, const string& desc,
            int axes, int buttons, int hats, int /*balls*/)
{
  ID = index;
  name = desc;

  // Dynamically create the various mapping arrays for this joystick,
  // based on its specific attributes
  numAxes    = axes;
  numButtons = buttons;
  numHats    = hats;
  axisLastValue = new int[numAxes];
  buttonLast = new int[numButtons];

  // Erase the last button
  for (int b = 0; b < numButtons; ++b)
    buttonLast[b] = JOY_CTRL_NONE;

  // Erase the last axis value
  for(int a = 0; a < numAxes; ++a)
    axisLastValue[a] = 0;

  // Erase the mappings
  eraseMap(kJoystickMode);
  eraseMap(kPaddlesMode);
  eraseMap(kKeypadMode);
  eraseMap(kMenuMode);

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string PhysicalJoystick::getMap() const
{
  // The mapping structure (for remappable devices) is defined as follows:
  // <NAME>'$'<MODE>['|'(<EVENT>':'<BUTTON>','<AXIS>','<VALUE>)|(<EVENT>':'<BUTTON>','<HAT>','<HATDIR>)]

  ostringstream joybuf;

  joybuf << name;
  joybuf << MODE_DELIM << int(kMenuMode) << "|" << joyMap.saveMapping(kMenuMode);
  joybuf << MODE_DELIM << int(kJoystickMode) << "|" << joyMap.saveMapping(kJoystickMode);
  joybuf << MODE_DELIM << int(kPaddlesMode) << "|" << joyMap.saveMapping(kPaddlesMode);
  joybuf << MODE_DELIM << int(kKeypadMode) << "|" << joyMap.saveMapping(kKeypadMode);
  joybuf << MODE_DELIM << int(kCommonMode) << "|" << joyMap.saveMapping(kCommonMode);


  return joybuf.str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool PhysicalJoystick::setMap(const string& mapString)
{
  istringstream buf(mapString);
  StringList mappings;
  string map;

  while (getline(buf, map, MODE_DELIM))
  {
    // remove leading "<mode>|" string
    map.erase(0, 2);
    mappings.push_back(map);
  }
  // Error checking
  if(mappings.size() != 1 + 5)
    return false;

  joyMap.loadMapping(mappings[1], kMenuMode);
  joyMap.loadMapping(mappings[2], kJoystickMode);
  joyMap.loadMapping(mappings[3], kPaddlesMode);
  joyMap.loadMapping(mappings[4], kKeypadMode);
  joyMap.loadMapping(mappings[5], kCommonMode);

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PhysicalJoystick::eraseMap(EventMode mode)
{
  joyMap.eraseMode(mode);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PhysicalJoystick::eraseEvent(Event::Type event, EventMode mode)
{
  joyMap.eraseEvent(event, mode);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PhysicalJoystick::getValues(const string& list, IntArray& map) const
{
  map.clear();
  istringstream buf(list);

  int value;
  buf >> value;  // we don't need to know the # of items at this point
  while(buf >> value)
    map.push_back(value);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string PhysicalJoystick::about() const
{
  ostringstream buf;
  buf << " with: " << numAxes << " axes, " << numButtons << " buttons, "
    << numHats << " hats";

  return buf.str();
}
