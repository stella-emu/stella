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

#include "JoyMap.hxx"
#include "Logger.hxx"
#include "jsonDefinitions.hxx"

using json = nlohmann::json;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void JoyMap::add(Event::Type event, const JoyMapping& mapping)
{
  myMap[mapping] = event;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void JoyMap::add(Event::Type event, EventMode mode, int button,
                 JoyAxis axis, JoyDir adir, int hat, JoyHatDir hdir)
{
  add(event, JoyMapping(mode, button, axis, adir, hat, hdir));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void JoyMap::add(Event::Type event, EventMode mode, int button,
                 int hat, JoyHatDir hdir)
{
  add(event, JoyMapping(mode, button, hat, hdir));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void JoyMap::erase(const JoyMapping& mapping)
{
  myMap.erase(mapping);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void JoyMap::erase(EventMode mode, int button, JoyAxis axis, JoyDir adir)
{
  erase(JoyMapping(mode, button, axis, adir));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void JoyMap::erase(EventMode mode, int button, int hat, JoyHatDir hdir)
{
  erase(JoyMapping(mode, button, hat, hdir));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Event::Type JoyMap::get(const JoyMapping& mapping) const
{
  auto find = myMap.find(mapping);
  if(find != myMap.end())
    return find->second;

  // try without button as modifier
  JoyMapping m = mapping;

  m.button = JOY_CTRL_NONE;

  find = myMap.find(m);
  if(find != myMap.end())
    return find->second;

  return Event::Type::NoType;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Event::Type JoyMap::get(EventMode mode, int button,
                        JoyAxis axis, JoyDir adir) const
{
  return get(JoyMapping(mode, button, axis, adir));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Event::Type JoyMap::get(EventMode mode, int button,
                        int hat, JoyHatDir hdir) const
{
  return get(JoyMapping(mode, button, hat, hdir));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool JoyMap::check(const JoyMapping& mapping) const
{
  return myMap.contains(mapping);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool JoyMap::check(EventMode mode, int button, JoyAxis axis, JoyDir adir,
                   int hat, JoyHatDir hdir) const
{
  return check(JoyMapping(mode, button, axis, adir, hat, hdir));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string JoyMap::getDesc(Event::Type event, const JoyMapping& mapping)
{
  string desc;

  // button description
  if(mapping.button != JOY_CTRL_NONE)
    desc += std::format("/B{}", mapping.button);

  // axis description
  if(mapping.axis != JoyAxis::NONE)
  {
    const string_view axisName = [&]() -> string_view {
      switch(mapping.axis)
      {
        case JoyAxis::X: return "X";
        case JoyAxis::Y: return "Y";
        case JoyAxis::Z: return "Z";
        default:         return "";
      }
    }();

    const string_view axisDir = Event::isAnalog(event) ? "+|-"
      : mapping.adir == JoyDir::NEG ? "-" : "+";

    if(axisName.empty())
      desc += std::format("/A{}{}", static_cast<int>(mapping.axis), axisDir);
    else
      desc += std::format("/A{}{}", axisName, axisDir);
  }

  // hat description
  if(mapping.hat != JOY_CTRL_NONE)
  {
    const string_view hatDir = [&]() -> string_view {
      switch(mapping.hdir)
      {
        case JoyHatDir::UP:    return "Y+";
        case JoyHatDir::DOWN:  return "Y-";
        case JoyHatDir::LEFT:  return "X-";
        case JoyHatDir::RIGHT: return "X+";
        default:               return "";
      }
    }();
    desc += std::format("/H{}{}", mapping.hat, hatDir);
  }

  return desc;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string JoyMap::getEventMappingDesc(int stick, Event::Type event,
                                   EventMode mode) const
{
  string desc;

  for(const auto& [_mapping, _event]: myMap)
  {
    if(_event == event && _mapping.mode == mode)
    {
      if(!desc.empty())
        desc += ", ";
      desc += std::format("C{}{}", stick, getDesc(event, _mapping));
    }
  }
  return desc;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
JoyMap::JoyMappingArray JoyMap::getEventMapping(Event::Type event,
                                                EventMode mode) const
{
  JoyMappingArray map;
  map.reserve(myMap.size());  // upper bound, avoids reallocations

  for(const auto& [_mapping, _event]: myMap)
    if(_event == event && _mapping.mode == mode)
      map.push_back(_mapping);

  return map;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
json JoyMap::saveMapping(EventMode mode) const
{
  using MapType = std::pair<JoyMapping, Event::Type>;
  std::vector<MapType> sortedMap(myMap.begin(), myMap.end());

  std::ranges::sort(sortedMap, [](const MapType& a, const MapType& b)
    {
      // Event::Type first
      if(a.first.button != b.first.button)
        return a.first.button < b.first.button;

      if(a.first.axis != b.first.axis)
        return a.first.axis < b.first.axis;

      if(a.first.adir != b.first.adir)
        return a.first.adir < b.first.adir;

      if(a.first.hat != b.first.hat)
        return a.first.hat < b.first.hat;

      if(a.first.hdir != b.first.hdir)
        return a.first.hdir < b.first.hdir;

      return a.second < b.second;
    }
  );

  json eventMappings = json::array();

  for (const auto& [_mapping, _event]: sortedMap) {
    if(_mapping.mode != mode || _event == Event::NoType) continue;

    json eventMapping = json::object();

    eventMapping["event"] = _event;

    if (_mapping.button != JOY_CTRL_NONE) eventMapping["button"] = _mapping.button;

    if (_mapping.axis != JoyAxis::NONE) {
      eventMapping["axis"] = _mapping.axis;
      eventMapping["axisDirection"] = _mapping.adir;
    }

    if (_mapping.hat != -1) {
      eventMapping["hat"] = _mapping.hat;
      eventMapping["hatDirection"] = _mapping.hdir;
    }

    eventMappings.push_back(eventMapping);
  }

  return eventMappings;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int JoyMap::loadMapping(const json& eventMappings, EventMode mode)
{
  int i = 0;

  for(const json& eventMapping : eventMappings) {
    const int button = eventMapping.contains("button")
      ? eventMapping.at("button").get<int>()
      : JOY_CTRL_NONE;
    const JoyAxis axis = eventMapping.contains("axis")
      ? eventMapping.at("axis").get<JoyAxis>()
      : JoyAxis::NONE;
    const JoyDir axisDirection = eventMapping.contains("axis")
      ? eventMapping.at("axisDirection").get<JoyDir>()
      : JoyDir::NONE;
    const int hat = eventMapping.contains("hat")
      ? eventMapping.at("hat").get<int>()
      : -1;
    const JoyHatDir hatDirection = eventMapping.contains("hat")
      ? eventMapping.at("hatDirection").get<JoyHatDir>()
      : JoyHatDir::CENTER;

    try {
      // avoid blocking mappings for NoType events
      if(eventMapping.at("event").get<Event::Type>() == Event::NoType)
        continue;

      add(
        eventMapping.at("event").get<Event::Type>(),
        mode,
        button,
        axis,
        axisDirection,
        hat,
        hatDirection
      );

      i++;
    } catch (const json::exception&) {
      Logger::error("ignoring invalid joystick event");
    }
  }

  return i;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
json JoyMap::convertLegacyMapping(string lst)
{
  json eventMappings = json::array();

  // Since istringstream swallows whitespace, we have to make the
  // delimiters be spaces
  std::ranges::replace(lst, '|', ' ');
  std::ranges::replace(lst, ':', ' ');
  std::ranges::replace(lst, ',', ' ');

  std::istringstream buf(lst);
  int event = 0, button = 0, axis = 0, adir = 0, hat = 0, hdir = 0;

  while(buf >> event && buf >> button
        && buf >> axis && buf >> adir
        && buf >> hat && buf >> hdir)
  {
    json eventMapping = json::object();

    eventMapping["event"] = static_cast<Event::Type>(event);

    if(button != JOY_CTRL_NONE) eventMapping["button"] = button;

    if(static_cast<JoyAxis>(axis) != JoyAxis::NONE) {
      eventMapping["axis"] = static_cast<JoyAxis>(axis);
      eventMapping["axisDirection"] = static_cast<JoyDir>(adir);
    }

    if(hat != -1) {
      eventMapping["hat"] = hat;
      eventMapping["hatDirection"] = static_cast<JoyHatDir>(hdir);
    }

    eventMappings.push_back(eventMapping);
  }

  return eventMappings;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void JoyMap::eraseMode(EventMode mode)
{
  std::erase_if(myMap, [mode](const auto& item) {
    return item.first.mode == mode;
  });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void JoyMap::eraseEvent(Event::Type event, EventMode mode)
{
  std::erase_if(myMap, [event, mode](const auto& item) {
    return item.second == event && item.first.mode == mode;
  });
}
