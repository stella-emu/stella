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

#include <map>

#include "OSystem.hxx"
#include "Settings.hxx"
#include "Vec.hxx"
#include "bspf.hxx"
#include "PhysicalJoystick.hxx"
#include "jsonDefinitions.hxx"

using json = nlohmann::json;

namespace {
  string jsonName(EventMode eventMode) {
    json serializedName = eventMode;

    return serializedName.get<string>();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PhysicalJoystick::initialize(int index, const string& desc,
            int axes, int buttons, int hats, int /*balls*/)
{
  ID = index;
  name = desc;

  numAxes    = axes;
  numButtons = buttons;
  numHats    = hats;
  axisLastValue.resize(numAxes, 0);
  buttonLast.resize(numButtons, JOY_CTRL_NONE);

  // Erase the mappings
  eraseMap(EventMode::kMenuMode);
  eraseMap(EventMode::kJoystickMode);
  eraseMap(EventMode::kPaddlesMode);
  eraseMap(EventMode::kKeypadMode);
  eraseMap(EventMode::kCommonMode);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
json PhysicalJoystick::getMap() const
{
  json mapping = json::object();

  mapping["name"] = name;

  for (auto& mode: {
    EventMode::kMenuMode, EventMode::kJoystickMode, EventMode::kPaddlesMode, EventMode::kKeypadMode, EventMode::kCommonMode
  })
    mapping[jsonName(mode)] = joyMap.saveMapping(mode);

  return mapping;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool PhysicalJoystick::setMap(const string& mapString)
{
  istringstream buf(mapString);
  string map;
  int i = 0;

  // Skip joystick name
  getline(buf, map, MODE_DELIM);

  while (getline(buf, map, MODE_DELIM))
  {
    int mode;

    // Get event mode
    std::replace(map.begin(), map.end(), '|', ' ');
    istringstream modeBuf(map);
    modeBuf >> mode;

    // Remove leading "<mode>|" string
    map.erase(0, 2);

    joyMap.loadMapping(map, EventMode(mode));
    i++;
  }
  // Brief error checking
  if(i != 5)
  {
    cerr << "ERROR: Invalid controller mappings found" << endl;
    return false;
  }

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
  buf << "'" << name << "' with: " << numAxes << " axes, " << numButtons << " buttons, "
    << numHats << " hats";

  return buf.str();
}
