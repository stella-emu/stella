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
    axisTable(nullptr),
    btnTable(nullptr),
    hatTable(nullptr),
    axisLastValue(nullptr)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PhysicalJoystick::~PhysicalJoystick()
{
  delete[] axisTable;
  delete[] btnTable;
  delete[] hatTable;
  delete[] axisLastValue;
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
  if(numAxes)
    axisTable = new Event::Type[numAxes][NUM_JOY_DIRS][kNumModes];
  if(numButtons)
    btnTable = new Event::Type[numButtons][kNumModes];
  if(numHats)
    hatTable = new Event::Type[numHats][NUM_JOY_HAT_DIRS][kNumModes];
  axisLastValue = new int[numAxes];

  // Erase the joystick axis mapping array and last axis value
  for(int a = 0; a < numAxes; ++a)
  {
    axisLastValue[a] = 0;
    for(int m = 0; m < kNumModes; ++m)
      for (int d = 0; d < NUM_JOY_DIRS; ++d)
        axisTable[a][d][m] = Event::NoType;
  }

  // Erase the joystick button mapping array
  for(int b = 0; b < numButtons; ++b)
    for(int m = 0; m < kNumModes; ++m)
      btnTable[b][m] = Event::NoType;

  // Erase the joystick hat mapping array
  for(int h = 0; h < numHats; ++h)
    for(int m = 0; m < kNumModes; ++m)
      for (int d = 0; d < NUM_JOY_HAT_DIRS; ++d)
        hatTable[h][d][m] = Event::NoType;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string PhysicalJoystick::getMap() const
{
  // The mapping structure (for remappable devices) is defined as follows:
  // NAME | AXIS # + values | BUTTON # + values | HAT # + values,
  // where each subsection of values is separated by ':'
  if(type == JT_REGULAR)
  {
    ostringstream joybuf;
    joybuf << name << "|" << numAxes;
    for(int m = 0; m < kNumModes; ++m)
      for(int a = 0; a < numAxes; ++a)
        for (int d = 0; d < NUM_JOY_DIRS; ++d)
          joybuf << " " << axisTable[a][d][m];
    joybuf << "|" << numButtons;
    for(int m = 0; m < kNumModes; ++m)
      for(int b = 0; b < numButtons; ++b)
        joybuf << " " << btnTable[b][m];
    joybuf << "|" << numHats;
    for(int m = 0; m < kNumModes; ++m)
      for(int h = 0; h < numHats; ++h)
        for (int d = 0; d < NUM_JOY_HAT_DIRS; ++d)
          joybuf << " " << hatTable[h][d][m];

    return joybuf.str();
  }
  return EmptyString;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool PhysicalJoystick::setMap(const string& mapString)
{
  istringstream buf(mapString);
  StringList items;
  string item;
  while(getline(buf, item, '|'))
    items.push_back(item);

  // Error checking
  if(items.size() != 4)
    return false;

  IntArray map;

  // Parse axis/button/hat values
  getValues(items[1], map);
  if(int(map.size()) == numAxes * NUM_JOY_DIRS * kNumModes)
  {
    // Fill the axes table with events
    auto event = map.cbegin();
    for(int m = 0; m < kNumModes; ++m)
      for(int a = 0; a < numAxes; ++a)
        for (int d = 0; d < NUM_JOY_DIRS; ++d)
          axisTable[a][d][m] = Event::Type(*event++);
  }

  getValues(items[2], map);
  if(int(map.size()) == numButtons * kNumModes)
  {
    auto event = map.cbegin();
    for(int m = 0; m < kNumModes; ++m)
      for(int b = 0; b < numButtons; ++b)
        btnTable[b][m] = Event::Type(*event++);
  }

  getValues(items[3], map);
  if(int(map.size()) == numHats * NUM_JOY_HAT_DIRS * kNumModes)
  {
    auto event = map.cbegin();
    for(int m = 0; m < kNumModes; ++m)
      for(int h = 0; h < numHats; ++h)
        for (int d = 0; d < NUM_JOY_HAT_DIRS; ++d)
          hatTable[h][d][m] = Event::Type(*event++);
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PhysicalJoystick::eraseMap(EventMode mode)
{
  // Erase axis mappings
  for(int a = 0; a < numAxes; ++a)
    for (int d = 0; d < NUM_JOY_DIRS; ++d)
      axisTable[a][d][mode] = Event::NoType;

  // Erase button mappings
  for(int b = 0; b < numButtons; ++b)
    btnTable[b][mode] = Event::NoType;

  // Erase hat mappings
  for(int h = 0; h < numHats; ++h)
    for (int d = 0; d < NUM_JOY_HAT_DIRS; ++d)
      hatTable[h][d][mode] = Event::NoType;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PhysicalJoystick::eraseEvent(Event::Type event, EventMode mode)
{
  // Erase axis mappings
  for(int a = 0; a < numAxes; ++a)
    for (int d = 0; d < NUM_JOY_DIRS; ++d)
      if(axisTable[a][d][mode] == event)
        axisTable[a][d][mode] = Event::NoType;

  // Erase button mappings
  for(int b = 0; b < numButtons; ++b)
    if(btnTable[b][mode] == event)
      btnTable[b][mode] = Event::NoType;

  // Erase hat mappings
  for(int h = 0; h < numHats; ++h)
    for (int d = 0; d < NUM_JOY_HAT_DIRS; ++d)
      if(hatTable[h][d][mode] == event)
        hatTable[h][d][mode] = Event::NoType;
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
  buf << name;
  if(type == JT_REGULAR)
    buf << " with: " << numAxes << " axes, " << numButtons << " buttons, "
        << numHats << " hats";

  return buf.str();
}
