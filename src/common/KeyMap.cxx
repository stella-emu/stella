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

#include "KeyMap.hxx"
#include "jsonDefinitions.hxx"
#include <map>

using json = nlohmann::json;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void KeyMap::add(const Event::Type event, const Mapping& mapping)
{
  myMap[convertMod(mapping)] = event;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void KeyMap::add(const Event::Type event, const EventMode mode, const int key, const int mod)
{
  add(event, Mapping(mode, key, mod));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void KeyMap::erase(const Mapping& mapping)
{
  myMap.erase(convertMod(mapping));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void KeyMap::erase(const EventMode mode, const int key, const int mod)
{
  erase(Mapping(mode, key, mod));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Event::Type KeyMap::get(const Mapping& mapping) const
{
  Mapping m = convertMod(mapping);

  if (myModEnabled)
  {
    auto find = myMap.find(m);
    if (find != myMap.end())
      return find->second;
  }

  // mapping not found, try without modifiers
  m.mod = StellaMod(0);

  auto find = myMap.find(m);
  if (find != myMap.end())
    return find->second;

  return Event::Type::NoType;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Event::Type KeyMap::get(const EventMode mode, const int key, const int mod) const
{
  return get(Mapping(mode, key, mod));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool KeyMap::check(const Mapping& mapping) const
{
  auto find = myMap.find(convertMod(mapping));

  return (find != myMap.end());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool KeyMap::check(const EventMode mode, const int key, const int mod) const
{
  return check(Mapping(mode, key, mod));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string KeyMap::getDesc(const Mapping& mapping) const
{
  ostringstream buf;
#if defined(BSPF_MACOS) || defined(MACOS_KEYS)
  string mod2 = "Option";
  int MOD2 = KBDM_ALT;
  int LMOD2 = KBDM_LALT;
  int RMOD2 = KBDM_RALT;
  string mod3 = "Cmd";
  int MOD3 = KBDM_GUI;
  int LMOD3 = KBDM_LGUI;
  int RMOD3 = KBDM_RGUI;
#else
  string mod2 = "Windows";
  int MOD2 = KBDM_GUI;
  int LMOD2 = KBDM_LGUI;
  int RMOD2 = KBDM_RGUI;
  string mod3 = "Alt";
  int MOD3 = KBDM_ALT;
  int LMOD3 = KBDM_LALT;
  int RMOD3 = KBDM_RALT;
#endif

  if ((mapping.mod & KBDM_CTRL) == KBDM_CTRL) buf << "Ctrl";
  else if (mapping.mod & KBDM_LCTRL) buf << "Left Ctrl";
  else if (mapping.mod & KBDM_RCTRL) buf << "Right Ctrl";

  if ((mapping.mod & (MOD2)) && buf.tellp()) buf << "+";
  if ((mapping.mod & MOD2) == MOD2) buf << mod2;
  else if (mapping.mod & LMOD2) buf << "Left " << mod2;
  else if (mapping.mod & RMOD2) buf << "Right " << mod2;

  if ((mapping.mod & (MOD3)) && buf.tellp()) buf << "+";
  if ((mapping.mod & MOD3) == MOD3) buf << mod3;
  else if (mapping.mod & LMOD3) buf << "Left " << mod3;
  else if (mapping.mod & RMOD3) buf << "Right " << mod3;

  if ((mapping.mod & (KBDM_SHIFT)) && buf.tellp()) buf << "+";
  if ((mapping.mod & KBDM_SHIFT) == KBDM_SHIFT) buf << "Shift";
  else if (mapping.mod & KBDM_LSHIFT) buf << "Left Shift";
  else if (mapping.mod & KBDM_RSHIFT) buf << "Right Shift";

  if (buf.tellp()) buf << "+";
  buf << StellaKeyName::forKey(mapping.key);

  return buf.str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string KeyMap::getDesc(const EventMode mode, const int key, const int mod) const
{
  return getDesc(Mapping(mode, key, mod));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string KeyMap::getEventMappingDesc(const Event::Type event, const EventMode mode) const
{
  ostringstream buf;

  for (auto item : myMap)
  {
    if (item.second == event && item.first.mode == mode)
    {
      if (buf.str() != "")
        buf << ", ";
      buf << getDesc(item.first);
    }
  }
  return buf.str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
KeyMap::MappingArray KeyMap::getEventMapping(const Event::Type event, const EventMode mode) const
{
  MappingArray map;

  for (auto item : myMap)
    if (item.second == event && item.first.mode == mode)
      map.push_back(item.first);

  return map;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
json KeyMap::saveMapping(const EventMode mode) const
{
  json mappings = json::array();

  for (auto item : myMap) {
    if (item.first.mode != mode) continue;

    json mapping = json::object();

    mapping["event"] = item.second;
    mapping["key"] = item.first.key;

    if (item.first.mod != StellaMod::KBDM_NONE)
      mapping["mod"] = item.first.mod;

    mappings.push_back(mapping);
  }

  return mappings;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int KeyMap::loadMapping(const json& mappings, const EventMode mode) {
  int i = 0;

  for (const json& mapping: mappings) {
    add(
      mapping.at("event").get<Event::Type>(),
      mode,
      mapping.at("key").get<StellaKey>(),
      mapping.contains("mod") ? mapping.at("mod").get<StellaMod>() : StellaMod::KBDM_NONE
    );

    i++;
  }

  return i;
}
/*
int KeyMap::loadMapping(string& list, const EventMode mode)
{
  // Since istringstream swallows whitespace, we have to make the
  // delimiters be spaces
  std::replace(list.begin(), list.end(), '|', ' ');
  std::replace(list.begin(), list.end(), ':', ' ');
  std::replace(list.begin(), list.end(), ',', ' ');
  istringstream buf(list);
  int event, key, mod, i = 0;

  while (buf >> event && buf >> key && buf >> mod && ++i)
    add(Event::Type(event), mode, key, mod);

  return i;
}
*/

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void KeyMap::eraseMode(const EventMode mode)
{
  for (auto item = myMap.begin(); item != myMap.end();)
    if (item->first.mode == mode) {
      auto _item = item++;
      erase(_item->first);
    }
    else item++;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void KeyMap::eraseEvent(const Event::Type event, const EventMode mode)
{
  for (auto item = myMap.begin(); item != myMap.end();)
    if (item->second == event && item->first.mode == mode) {
      auto _item = item++;
      erase(_item->first);
    }
    else item++;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
KeyMap::Mapping KeyMap::convertMod(const Mapping& mapping) const
{
  Mapping m = mapping;

  if (m.key >= KBDK_LCTRL && m.key <= KBDK_RGUI)
    // handle solo modifier keys differently
    m.mod = KBDM_NONE;
  else
  {
    // limit to modifiers we want to support
    m.mod = StellaMod(m.mod & (KBDM_SHIFT | KBDM_CTRL | KBDM_ALT | KBDM_GUI));
  }

  return m;
}
