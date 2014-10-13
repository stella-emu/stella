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
// Copyright (c) 1995-2014 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

#include <sstream>
#include <map>

#include "OSystem.hxx"
#include "Settings.hxx"
#include "bspf.hxx"

#include "EventHandler.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EventHandler::StellaJoystick::StellaJoystick()
  : type(JT_NONE),
    ID(-1),
    name("None"),
    numAxes(0),
    numButtons(0),
    numHats(0),
    axisTable(NULL),
    btnTable(NULL),
    hatTable(NULL),
    axisLastValue(NULL)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EventHandler::StellaJoystick::~StellaJoystick()
{
  delete[] axisTable;      axisTable = NULL;
  delete[] btnTable;       btnTable = NULL;
  delete[] hatTable;       hatTable = NULL;
  delete[] axisLastValue;  axisLastValue = NULL;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::StellaJoystick::initialize(int index, const string& desc,
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
    axisTable = new Event::Type[numAxes][2][kNumModes];
  if(numButtons)
    btnTable = new Event::Type[numButtons][kNumModes];
  if(numHats)
    hatTable = new Event::Type[numHats][4][kNumModes];
  axisLastValue = new int[numAxes];

  // Erase the joystick axis mapping array and last axis value
  for(int a = 0; a < numAxes; ++a)
  {
    axisLastValue[a] = 0;
    for(int m = 0; m < kNumModes; ++m)
      axisTable[a][0][m] = axisTable[a][1][m] = Event::NoType;
  }

  // Erase the joystick button mapping array
  for(int b = 0; b < numButtons; ++b)
    for(int m = 0; m < kNumModes; ++m)
      btnTable[b][m] = Event::NoType;

  // Erase the joystick hat mapping array
  for(int h = 0; h < numHats; ++h)
    for(int m = 0; m < kNumModes; ++m)
      hatTable[h][0][m] = hatTable[h][1][m] =
      hatTable[h][2][m] = hatTable[h][3][m] = Event::NoType;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string EventHandler::StellaJoystick::getMap() const
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
        for(int k = 0; k < 2; ++k)
          joybuf << " " << axisTable[a][k][m];
    joybuf << "|" << numButtons;
    for(int m = 0; m < kNumModes; ++m)
      for(int b = 0; b < numButtons; ++b)
        joybuf << " " << btnTable[b][m];
    joybuf << "|" << numHats;
    for(int m = 0; m < kNumModes; ++m)
      for(int h = 0; h < numHats; ++h)
        for(int k = 0; k < 4; ++k)
          joybuf << " " << hatTable[h][k][m];

    return joybuf.str();
  }
  return EmptyString;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool EventHandler::StellaJoystick::setMap(const string& m)
{
  istringstream buf(m);
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
  if((int)map.size() == numAxes * 2 * kNumModes)
  {
    // Fill the axes table with events
    IntArray::const_iterator event = map.begin();
    for(int m = 0; m < kNumModes; ++m)
      for(int a = 0; a < numAxes; ++a)
        for(int k = 0; k < 2; ++k)
          axisTable[a][k][m] = (Event::Type) *event++;
  }
  getValues(items[2], map);
  if((int)map.size() == numButtons * kNumModes)
  {
    IntArray::const_iterator event = map.begin();
    for(int m = 0; m < kNumModes; ++m)
      for(int b = 0; b < numButtons; ++b)
        btnTable[b][m] = (Event::Type) *event++;
  }
  getValues(items[3], map);
  if((int)map.size() == numHats * 4 * kNumModes)
  {
    IntArray::const_iterator event = map.begin();
    for(int m = 0; m < kNumModes; ++m)
      for(int h = 0; h < numHats; ++h)
        for(int k = 0; k < 4; ++k)
          hatTable[h][k][m] = (Event::Type) *event++;
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::StellaJoystick::eraseMap(EventMode mode)
{
  // Erase axis mappings
  for(int a = 0; a < numAxes; ++a)
    axisTable[a][0][mode] = axisTable[a][1][mode] = Event::NoType;

  // Erase button mappings
  for(int b = 0; b < numButtons; ++b)
    btnTable[b][mode] = Event::NoType;

  // Erase hat mappings
  for(int h = 0; h < numHats; ++h)
    hatTable[h][0][mode] = hatTable[h][1][mode] =
    hatTable[h][2][mode] = hatTable[h][3][mode] = Event::NoType;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::StellaJoystick::eraseEvent(Event::Type event, EventMode mode)
{
  // Erase axis mappings
  for(int a = 0; a < numAxes; ++a)
  {
    if(axisTable[a][0][mode] == event)  axisTable[a][0][mode] = Event::NoType;
    if(axisTable[a][1][mode] == event)  axisTable[a][1][mode] = Event::NoType;
  }

  // Erase button mappings
  for(int b = 0; b < numButtons; ++b)
    if(btnTable[b][mode] == event)  btnTable[b][mode] = Event::NoType;

  // Erase hat mappings
  for(int h = 0; h < numHats; ++h)
  {
    if(hatTable[h][0][mode] == event)  hatTable[h][0][mode] = Event::NoType;
    if(hatTable[h][1][mode] == event)  hatTable[h][1][mode] = Event::NoType;
    if(hatTable[h][2][mode] == event)  hatTable[h][2][mode] = Event::NoType;
    if(hatTable[h][3][mode] == event)  hatTable[h][3][mode] = Event::NoType;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::StellaJoystick::getValues(const string& list, IntArray& map)
{
  map.clear();
  istringstream buf(list);

  int value;
  buf >> value;  // we don't need to know the # of items at this point
  while(buf >> value)
    map.push_back(value);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string EventHandler::StellaJoystick::about() const
{
  ostringstream buf;
  buf << name;
  if(type == JT_REGULAR)
    buf << " with: " << numAxes << " axes, " << numButtons << " buttons, "
        << numHats << " hats";

  return buf.str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EventHandler::JoystickHandler::JoystickHandler(OSystem& system)
  : myOSystem(system)
{
  // Load previously saved joystick mapping (if any) from settings
  istringstream buf(myOSystem.settings().getString("joymap"));
  string joymap, joyname;

  // First check the event type, and disregard the entire mapping if it's invalid
  getline(buf, joymap, '^');
  if(atoi(joymap.c_str()) == Event::LastType)
  {
    // Otherwise, put each joystick mapping entry into the database
    while(getline(buf, joymap, '^'))
    {
      istringstream namebuf(joymap);
      getline(namebuf, joyname, '|');
      if(joyname.length() != 0)
      {
        StickInfo info(joymap);
        myDatabase.insert(make_pair(joyname, info));
      }
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EventHandler::JoystickHandler::~JoystickHandler()
{
  map<string,StickInfo>::const_iterator it;
  for(it = myDatabase.begin(); it != myDatabase.end(); ++it)
    delete it->second.joy;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::JoystickHandler::printDatabase()
{
  cerr << "---------------------------------------------------------" << endl
       << "joy database:"  << endl;
  map<string,StickInfo>::const_iterator it;
  for(it = myDatabase.begin(); it != myDatabase.end(); ++it)
    cerr << it->first << endl << it->second << endl << endl;

  cerr << "---------------------------------------------------------" << endl
       << "joy active:"  << endl;
  for(uInt32 i = 0; i < mySticks.size(); ++i)
    cerr << *mySticks[i] << endl;
  cerr << endl;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int EventHandler::JoystickHandler::add(StellaJoystick* stick)
{
  // Skip if we couldn't open it for any reason
  if(stick->ID < 0)
    return stick->ID;

  // Figure out what type of joystick this is
  bool specialAdaptor = false;

  if(stick->name.find("2600-daptor", 0) != string::npos)
  {
    // 2600-daptorII devices have 3 axes and 12 buttons, and the value of the z-axis
    // determines how those 12 buttons are used (not all buttons are used in all modes)
    if(stick->numAxes == 3)
    {
      // TODO - stubbed out for now, until we find a way to reliably get info
      //        from the Z axis
      stick->name = "2600-daptor II";
    }
    else
      stick->name = "2600-daptor";

    specialAdaptor = true;
  }
  else if(stick->name.find("Stelladaptor", 0) != string::npos)
  {
    stick->name = "Stelladaptor";
    specialAdaptor = true;
  }
  else
  {
    // We need unique names for mappable devices
    int count = 0;
    map<string,StickInfo>::const_iterator c_it;
    for(c_it = myDatabase.begin(); c_it != myDatabase.end(); ++c_it)
      if(BSPF_startsWithIgnoreCase(c_it->first, stick->name))
        ++count;

    if(count > 1)
    {
      ostringstream name;
      name << stick->name << " " << count;
      stick->name = name.str();
    }
    stick->type = StellaJoystick::JT_REGULAR;
  }
  mySticks.insert_at(stick->ID, stick);

  // Map the stelladaptors we've found according to the specified ports
  if(specialAdaptor)
    mapStelladaptors(myOSystem.settings().getString("saport"));

  // Add stick to database
  map<string,StickInfo>::iterator it = myDatabase.find(stick->name);
  if(it != myDatabase.end())    // already present
  {
    it->second.joy = stick;
    stick->setMap(it->second.mapping);
  }
  else    // adding for the first time
  {
    StickInfo info("", stick);
    myDatabase.insert(make_pair(stick->name, info));
    setStickDefaultMapping(stick->ID, Event::NoType, kEmulationMode);
    setStickDefaultMapping(stick->ID, Event::NoType, kMenuMode);
  }

  return stick->ID;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int EventHandler::JoystickHandler::remove(int index)
{
  // When a joystick is removed, we delete the actual joystick object but
  // remember its mapping, since it will eventually be saved to settings

  // Sticks that are removed must have initially been added
  // So we use the 'active' joystick list to access them
  if(index >= 0 && index < mySticks.size() && mySticks[index] != NULL)
  {
    StellaJoystick* stick = mySticks[index];

    map<string,StickInfo>::iterator it = myDatabase.find(stick->name);
    if(it != myDatabase.end() && it->second.joy == stick)
    {
      ostringstream buf;
      buf << "Removed joystick " << mySticks[index]->ID << ":" << endl
          << "  " << mySticks[index]->about() << endl;
      myOSystem.logMessage(buf.str(), 1);

      // Remove joystick, but remember mapping
      it->second.mapping = stick->getMap();
      delete it->second.joy;  it->second.joy = NULL;
      mySticks[index] = NULL;

      return index;
    }
  }
  return -1;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::JoystickHandler::mapStelladaptors(const string& saport)
{
  // saport will have two values:
  //   'lr' means treat first valid adaptor as left port, second as right port
  //   'rl' means treat first valid adaptor as right port, second as left port
  // We know there will be only two such devices (at most), since the logic
  // in setupJoysticks take care of that
  int saCount = 0;
  int saOrder[2] = { 1, 2 };
  if(BSPF_equalsIgnoreCase(saport, "rl"))
  {
    saOrder[0] = 2; saOrder[1] = 1;
  }

  for(uInt32 i = 0; i < mySticks.size(); ++i)
  {
    if(BSPF_startsWithIgnoreCase(mySticks[i]->name, "Stelladaptor"))
    {
      if(saOrder[saCount] == 1)
      {
        mySticks[i]->name += " (emulates left joystick port)";
        mySticks[i]->type = StellaJoystick::JT_STELLADAPTOR_LEFT;
      }
      else if(saOrder[saCount] == 2)
      {
        mySticks[i]->name += " (emulates right joystick port)";
        mySticks[i]->type = StellaJoystick::JT_STELLADAPTOR_RIGHT;
      }
      saCount++;
    }
    else if(BSPF_startsWithIgnoreCase(mySticks[i]->name, "2600-daptor"))
    {
      if(saOrder[saCount] == 1)
      {
        mySticks[i]->name += " (emulates left joystick port)";
        mySticks[i]->type = StellaJoystick::JT_2600DAPTOR_LEFT;
      }
      else if(saOrder[saCount] == 2)
      {
        mySticks[i]->name += " (emulates right joystick port)";
        mySticks[i]->type = StellaJoystick::JT_2600DAPTOR_RIGHT;
      }
      saCount++;
    }
  }
  myOSystem.settings().setValue("saport", saport);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::JoystickHandler::setDefaultMapping(Event::Type event, EventMode mode)
{
  eraseMapping(event, mode);
  setStickDefaultMapping(0, event, mode);
  setStickDefaultMapping(1, event, mode);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::JoystickHandler::setStickDefaultMapping(int stick,
        Event::Type event, EventMode mode)
{
#define SET_DEFAULT_AXIS(sda_event, sda_mode, sda_stick, sda_axis, sda_val, sda_cmp_event) \
  if(eraseAll || sda_cmp_event == sda_event) \
    handler.addJoyAxisMapping(sda_event, sda_mode, sda_stick, sda_axis, sda_val, false);

#define SET_DEFAULT_BTN(sdb_event, sdb_mode, sdb_stick, sdb_button, sdb_cmp_event) \
  if(eraseAll || sdb_cmp_event == sdb_event) \
    handler.addJoyButtonMapping(sdb_event, sdb_mode, sdb_stick, sdb_button, false);

#define SET_DEFAULT_HAT(sdh_event, sdh_mode, sdh_stick, sdh_hat, sdh_dir, sdh_cmp_event) \
  if(eraseAll || sdh_cmp_event == sdh_event) \
    handler.addJoyHatMapping(sdh_event, sdh_mode, sdh_stick, sdh_hat, sdh_dir, false);

  EventHandler& handler = myOSystem.eventHandler();
  bool eraseAll = (event == Event::NoType);

  switch(mode)
  {
    case kEmulationMode:  // Default emulation events
      if(stick == 0)
      {
        // Left joystick left/right directions (assume joystick zero)
        SET_DEFAULT_AXIS(Event::JoystickZeroLeft, mode, 0, 0, 0, event);
        SET_DEFAULT_AXIS(Event::JoystickZeroRight, mode, 0, 0, 1, event);
        // Left joystick up/down directions (assume joystick zero)
        SET_DEFAULT_AXIS(Event::JoystickZeroUp, mode, 0, 1, 0, event);
        SET_DEFAULT_AXIS(Event::JoystickZeroDown, mode, 0, 1, 1, event);
        // Left joystick (assume joystick zero, button zero)
        SET_DEFAULT_BTN(Event::JoystickZeroFire, mode, 0, 0, event);
        // Left joystick left/right directions (assume joystick zero and hat 0)
        SET_DEFAULT_HAT(Event::JoystickZeroLeft, mode, 0, 0, EVENT_HATLEFT, event);
        SET_DEFAULT_HAT(Event::JoystickZeroRight, mode, 0, 0, EVENT_HATRIGHT, event);
        // Left joystick up/down directions (assume joystick zero and hat 0)
        SET_DEFAULT_HAT(Event::JoystickZeroUp, mode, 0, 0, EVENT_HATUP, event);
        SET_DEFAULT_HAT(Event::JoystickZeroDown, mode, 0, 0, EVENT_HATDOWN, event);
      }
      else if(stick == 1)
      {
        // Right joystick left/right directions (assume joystick one)
        SET_DEFAULT_AXIS(Event::JoystickOneLeft, mode, 1, 0, 0, event);
        SET_DEFAULT_AXIS(Event::JoystickOneRight, mode, 1, 0, 1, event);
        // Right joystick left/right directions (assume joystick one)
        SET_DEFAULT_AXIS(Event::JoystickOneUp, mode, 1, 1, 0, event);
        SET_DEFAULT_AXIS(Event::JoystickOneDown, mode, 1, 1, 1, event);
        // Right joystick (assume joystick one, button zero)
        SET_DEFAULT_BTN(Event::JoystickOneFire, mode, 1, 0, event);
        // Right joystick left/right directions (assume joystick one and hat 0)
        SET_DEFAULT_HAT(Event::JoystickOneLeft, mode, 1, 0, EVENT_HATLEFT, event);
        SET_DEFAULT_HAT(Event::JoystickOneRight, mode, 1, 0, EVENT_HATRIGHT, event);
        // Right joystick up/down directions (assume joystick one and hat 0)
        SET_DEFAULT_HAT(Event::JoystickOneUp, mode, 1, 0, EVENT_HATUP, event);
        SET_DEFAULT_HAT(Event::JoystickOneDown, mode, 1, 0, EVENT_HATDOWN, event);
      }
      break;

    case kMenuMode:  // Default menu/UI events
      if(stick == 0)
      {
        SET_DEFAULT_AXIS(Event::UILeft, mode, 0, 0, 0, event);
        SET_DEFAULT_AXIS(Event::UIRight, mode, 0, 0, 1, event);
        SET_DEFAULT_AXIS(Event::UIUp, mode, 0, 1, 0, event);
        SET_DEFAULT_AXIS(Event::UIDown, mode, 0, 1, 1, event);

        // Left joystick (assume joystick zero, button zero)
        SET_DEFAULT_BTN(Event::UISelect, mode, 0, 0, event);
        // Right joystick (assume joystick one, button zero)
        SET_DEFAULT_BTN(Event::UISelect, mode, 1, 0, event);

        SET_DEFAULT_HAT(Event::UILeft, mode, 0, 0, EVENT_HATLEFT, event);
        SET_DEFAULT_HAT(Event::UIRight, mode, 0, 0, EVENT_HATRIGHT, event);
        SET_DEFAULT_HAT(Event::UIUp, mode, 0, 0, EVENT_HATUP, event);
        SET_DEFAULT_HAT(Event::UIDown, mode, 0, 0, EVENT_HATDOWN, event);
      }
      break;

    default:
      break;
  }

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::JoystickHandler::eraseMapping(Event::Type event, EventMode mode)
{
  // If event is 'NoType', erase and reset all mappings
  // Otherwise, only reset the given event
  if(event == Event::NoType)
  {
    for(uInt32 i = 0; i < mySticks.size(); ++i)
      mySticks[i]->eraseMap(mode);          // erase all events
  }
  else
  {
    for(uInt32 i = 0; i < mySticks.size(); ++i)
      mySticks[i]->eraseEvent(event, mode); // only reset the specific event
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EventHandler::JoystickHandler::saveMapping()
{
  // Save the joystick mapping hash table, making sure to update it with
  // any changes that have been made during the program run
  ostringstream joybuf;
  joybuf << Event::LastType;

  map<string,StickInfo>::const_iterator it;
  for(it = myDatabase.begin(); it != myDatabase.end(); ++it)
  {
    const string& map = it->second.joy ?
        it->second.joy->getMap() : it->second.mapping;

    if(map != "")
      joybuf << "^" << map;
  }
  myOSystem.settings().setValue("joymap", joybuf.str());
}
