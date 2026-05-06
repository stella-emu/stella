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

#include "KeyMap.hxx"
#include "Logger.hxx"
#include "jsonDefinitions.hxx"

using json = nlohmann::json;

namespace {
  json serializeModkeyMask(int mask)
  {
    if(mask == StellaMod::KBDM_NONE) return {};

    json serializedMask = json::array();

    for(const StellaMod mod: {
      StellaMod::KBDM_CTRL,
      StellaMod::KBDM_SHIFT,
      StellaMod::KBDM_ALT,
      StellaMod::KBDM_GUI,
      StellaMod::KBDM_LSHIFT,
      StellaMod::KBDM_RSHIFT,
      StellaMod::KBDM_LCTRL,
      StellaMod::KBDM_RCTRL,
      StellaMod::KBDM_LALT,
      StellaMod::KBDM_RALT,
      StellaMod::KBDM_LGUI,
      StellaMod::KBDM_RGUI,
      StellaMod::KBDM_NUM,
      StellaMod::KBDM_CAPS,
      StellaMod::KBDM_MODE
    }) {
      if((mask & mod) != mod) continue;

      serializedMask.push_back(json(mod));
      mask &= ~mod;
    }

    return serializedMask.size() == 1 ? serializedMask.at(0) : serializedMask;
  }

  int deserializeModkeyMask(const json& serializedMask)
  {
    if(serializedMask.is_null()) return StellaMod::KBDM_NONE;
    if(!serializedMask.is_array()) return serializedMask.get<StellaMod>();

    int mask = 0;
    for(const json& mod: serializedMask) mask |= mod.get<StellaMod>();

    return mask;
  }
} // namespace

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void KeyMap::add(Event::Type event, const Mapping& mapping)
{
  myMap[convertMod(mapping)] = event;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void KeyMap::add(Event::Type event, EventMode mode, int key, int mod)
{
  add(event, Mapping(mode, key, mod));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void KeyMap::erase(const Mapping& mapping)
{
  myMap.erase(convertMod(mapping));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void KeyMap::erase(EventMode mode, int key, int mod)
{
  erase(Mapping(mode, key, mod));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Event::Type KeyMap::get(const Mapping& mapping) const
{
  Mapping m = convertMod(mapping);

  if(myModEnabled)
  {
    const auto find = myMap.find(m);
    if(find != myMap.end())
      return find->second;
  }

  // mapping not found, try without modifiers
  m.mod = static_cast<StellaMod>(0);

  const auto find = myMap.find(m);
  if(find != myMap.end())
    return find->second;

  return Event::Type::NoType;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Event::Type KeyMap::get(EventMode mode, int key, int mod) const
{
  return get(Mapping(mode, key, mod));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool KeyMap::check(const Mapping& mapping) const
{
  return myMap.contains(convertMod(mapping));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool KeyMap::check(EventMode mode, int key, int mod) const
{
  return check(Mapping(mode, key, mod));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string KeyMap::getDesc(const Mapping& mapping)
{
#if defined(BSPF_MACOS) || defined(MACOS_KEYS)
  static constexpr string_view mod2 = "Option";
  static constexpr int MOD2  = KBDM_ALT;
  static constexpr int LMOD2 = KBDM_LALT;
  static constexpr int RMOD2 = KBDM_RALT;
  static constexpr string_view mod3 = "Cmd";
  static constexpr int MOD3  = KBDM_GUI;
  static constexpr int LMOD3 = KBDM_LGUI;
  static constexpr int RMOD3 = KBDM_RGUI;
#else
  static constexpr string_view mod2 = "Windows";
  static constexpr int MOD2  = KBDM_GUI;
  static constexpr int LMOD2 = KBDM_LGUI;
  static constexpr int RMOD2 = KBDM_RGUI;
  static constexpr string_view mod3 = "Alt";
  static constexpr int MOD3  = KBDM_ALT;
  static constexpr int LMOD3 = KBDM_LALT;
  static constexpr int RMOD3 = KBDM_RALT;
#endif

  string buf;
  buf.reserve(32);

  const auto append = [&](string_view part) {
    if(!buf.empty()) buf += '-';
    buf += part;
  };

  if((mapping.mod & KBDM_CTRL) == KBDM_CTRL) append("Ctrl");
  else if(mapping.mod & KBDM_LCTRL) append("Left Ctrl");
  else if(mapping.mod & KBDM_RCTRL) append("Right Ctrl");

  if((mapping.mod & MOD2) == MOD2) append(mod2);
  else if(mapping.mod & LMOD2) append(std::format("Left {}", mod2));
  else if(mapping.mod & RMOD2) append(std::format("Right {}", mod2));

  if((mapping.mod & MOD3) == MOD3) append(mod3);
  else if(mapping.mod & LMOD3) append(std::format("Left {}", mod3));
  else if(mapping.mod & RMOD3) append(std::format("Right {}", mod3));

  if((mapping.mod & KBDM_SHIFT) == KBDM_SHIFT) append("Shift");
  else if(mapping.mod & KBDM_LSHIFT) append("Left Shift");
  else if(mapping.mod & KBDM_RSHIFT) append("Right Shift");

  if(!buf.empty()) buf += '+';
  buf += StellaKeyName::forKey(mapping.key);

  return buf;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string KeyMap::getDesc(EventMode mode, int key, int mod)
{
  return getDesc(Mapping(mode, key, mod));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string KeyMap::getEventMappingDesc(Event::Type event, EventMode mode) const
{
  string buf;

  for(const auto& [_mapping, _event]: myMap)
  {
    if(_event == event && _mapping.mode == mode)
    {
      if(!buf.empty()) buf += ", ";
      buf += getDesc(_mapping);
    }
  }
  return buf;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
KeyMap::MappingArray KeyMap::getEventMapping(Event::Type event,
                                             EventMode mode) const
{
  MappingArray ma;

  for (const auto& [_mapping, _event]: myMap)
    if (_event == event && _mapping.mode == mode)
      ma.push_back(_mapping);

  return ma;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
json KeyMap::saveMapping(EventMode mode) const
{
  using MapType = std::pair<Mapping, Event::Type>;
  std::vector<MapType> sortedMap(myMap.begin(), myMap.end());

  std::ranges::sort(sortedMap, [](const MapType& a, const MapType& b)
  {
    // Event::Type first
    if(a.first.key != b.first.key)
      return a.first.key < b.first.key;

    if(a.first.mod != b.first.mod)
      return a.first.mod < b.first.mod;

    return a.second < b.second;
  }
  );

  json mappings = json::array();

  for (const auto& [_mapping, _event]: sortedMap) {
    if (_mapping.mode != mode || _event == Event::NoType) continue;

    json mapping = json::object();

    mapping["event"] = _event;
    mapping["key"] = _mapping.key;

    if (_mapping.mod != StellaMod::KBDM_NONE)
      mapping["mod"] = serializeModkeyMask(_mapping.mod);

    mappings.push_back(mapping);
  }

  return mappings;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int KeyMap::loadMapping(const json& mappings, EventMode mode)
{
  int i = 0;

  for(const json& mapping : mappings)
  {
    try {
      // avoid blocking mappings for NoType events
      if(mapping.at("event").get<Event::Type>() == Event::NoType)
        continue;

      add(
        mapping.at("event").get<Event::Type>(),
        mode,
        mapping.at("key").get<StellaKey>(),
        mapping.contains("mod") ? deserializeModkeyMask(mapping.at("mod")) : StellaMod::KBDM_NONE
      );

      i++;
    } catch (const json::exception&) {
      Logger::error("ignoring bad keyboard mapping");
    }
  }

  return i;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
json KeyMap::convertLegacyMapping(string_view lm)
{
  json convertedMapping = json::array();

  const char* p = lm.data();
  const char* end = p + lm.size();

  const auto nextInt = [&](int& val) -> bool {
    while(p < end && (*p == ' ' || *p == '|' || *p == ':' || *p == ',')) ++p;
    auto [next, ec] = std::from_chars(p, end, val);
    if(ec != std::errc{}) return false;
    p = next;
    return true;
  };

  int event = 0, key = 0, mod = 0;

  while(nextInt(event) && nextInt(key) && nextInt(mod))
  {
    json mapping = json::object();

    mapping["event"] = static_cast<Event::Type>(event);
    mapping["key"] = static_cast<StellaKey>(key);

    if(static_cast<StellaMod>(mod) != StellaMod::KBDM_NONE)
      mapping["mod"] = serializeModkeyMask(static_cast<StellaMod>(mod));

    convertedMapping.push_back(mapping);
  }

  return convertedMapping;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void KeyMap::eraseMode(EventMode mode)
{
  std::erase_if(myMap, [mode](const auto& item) {
    return item.first.mode == mode;
  });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void KeyMap::eraseEvent(Event::Type event, EventMode mode)
{
  std::erase_if(myMap, [event, mode](const auto& item) {
    return item.second == event && item.first.mode == mode;
  });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
KeyMap::Mapping KeyMap::convertMod(const Mapping& mapping)
{
  Mapping m = mapping;

  if(m.key >= KBDK_LCTRL && m.key <= KBDK_RGUI)
    // handle solo modifier keys differently
    m.mod = KBDM_NONE;
  else
  {
    // limit to modifiers we want to support
    m.mod = static_cast<StellaMod>(m.mod & (KBDM_SHIFT | KBDM_CTRL | KBDM_ALT | KBDM_GUI));
  }

  return m;
}
