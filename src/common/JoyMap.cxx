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
  const auto it = std::ranges::lower_bound(myMap, mapping,
                                           std::less{}, &MapEntry::first);
  if(it != myMap.end() && it->first == mapping)
    it->second = event;
  else
    myMap.insert(it, {mapping, event});
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
  const auto it = std::ranges::lower_bound(myMap, mapping,
                                           std::less{}, &MapEntry::first);
  if(it != myMap.end() && it->first == mapping)
    myMap.erase(it);
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
  const auto it = std::ranges::lower_bound(myMap, mapping,
                                           std::less{}, &MapEntry::first);
  if(it != myMap.end() && it->first == mapping)
    return it->second;

  // try without button as modifier
  JoyMapping m = mapping;
  m.button = JOY_CTRL_NONE;
  const auto it2 = std::ranges::lower_bound(myMap, m,
                                            std::less{}, &MapEntry::first);
  if(it2 != myMap.end() && it2->first == m)
    return it2->second;

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
  const auto it = std::ranges::lower_bound(myMap, mapping,
                                           std::less{}, &MapEntry::first);
  return it != myMap.end() && it->first == mapping;
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

  for(const auto& [_mapping, _event]: myMap)
    if(_event == event && _mapping.mode == mode)
      map.push_back(_mapping);

  return map;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
json JoyMap::saveMapping(EventMode mode) const
{
  json eventMappings = json::array();

  for(const auto& [_mapping, _event]: myMap) {
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
json JoyMap::convertLegacyMapping(string_view lst)
{
  json eventMappings = json::array();

  const char* p = lst.data();
  const char* end = p + lst.size();

  const auto nextInt = [&](int& val) -> bool {
    while(p < end && (*p == ' ' || *p == '|' || *p == ':' || *p == ',')) ++p;
    auto [next, ec] = std::from_chars(p, end, val);
    if(ec != std::errc{}) return false;
    p = next;
    return true;
  };

  int event = 0, button = 0, axis = 0, adir = 0, hat = 0, hdir = 0;

  while(nextInt(event) && nextInt(button)
        && nextInt(axis) && nextInt(adir)
        && nextInt(hat)  && nextInt(hdir))
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
