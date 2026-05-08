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
  json serializeModkeyMask(StellaMod mask)
  {
    if(mask == StellaMod::NONE) return {};

    json serializedMask = json::array();

    for(const StellaMod mod: {
      StellaMod::CTRL,
      StellaMod::SHIFT,
      StellaMod::ALT,
      StellaMod::GUI,
      StellaMod::LSHIFT,
      StellaMod::RSHIFT,
      StellaMod::LCTRL,
      StellaMod::RCTRL,
      StellaMod::LALT,
      StellaMod::RALT,
      StellaMod::LGUI,
      StellaMod::RGUI,
      StellaMod::NUM,
      StellaMod::CAPS,
      StellaMod::MODE
    }) {
      if((mask & mod) != mod) continue;

      serializedMask.push_back(json(mod));
      mask &= ~mod;
    }

    return serializedMask.size() == 1 ? serializedMask.at(0) : serializedMask;
  }

  StellaMod deserializeModkeyMask(const json& serializedMask)
  {
    if(serializedMask.is_null()) return StellaMod::NONE;
    if(!serializedMask.is_array()) return serializedMask.get<StellaMod>();

    StellaMod mask{StellaMod::NONE};
    for(const json& mod: serializedMask)
      mask = mask | mod.get<StellaMod>();

    return mask;
  }
}  // namespace

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void KeyMap::add(Event::Type event, const Mapping& mapping)
{
  const Mapping m = convertMod(mapping);
  const auto it = std::ranges::lower_bound(myMap, m, std::less{}, &MapEntry::first);
  if(it != myMap.end() && it->first == m)
    it->second = event;
  else
    myMap.insert(it, {m, event});
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void KeyMap::add(Event::Type event, EventMode mode, StellaKey key, StellaMod mod)
{
  add(event, Mapping(mode, key, mod));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void KeyMap::erase(const Mapping& mapping)
{
  const Mapping m = convertMod(mapping);
  const auto it = std::ranges::lower_bound(myMap, m, std::less{}, &MapEntry::first);
  if(it != myMap.end() && it->first == m)
    myMap.erase(it);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void KeyMap::erase(EventMode mode, StellaKey key, StellaMod mod)
{
  erase(Mapping(mode, key, mod));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Event::Type KeyMap::get(const Mapping& mapping) const
{
  Mapping m = convertMod(mapping);

  if(myModEnabled)
  {
    const auto it = std::ranges::lower_bound(myMap, m, std::less{}, &MapEntry::first);
    if(it != myMap.end() && it->first == m)
      return it->second;
  }

  // mapping not found, try without modifiers
  m.mod = StellaMod::NONE;
  const auto it = std::ranges::lower_bound(myMap, m, std::less{}, &MapEntry::first);
  if(it != myMap.end() && it->first == m)
    return it->second;

  return Event::Type::NoType;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Event::Type KeyMap::get(EventMode mode, StellaKey key, StellaMod mod) const
{
  return get(Mapping(mode, key, mod));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool KeyMap::check(const Mapping& mapping) const
{
  const Mapping m = convertMod(mapping);
  const auto it = std::ranges::lower_bound(myMap, m, std::less{}, &MapEntry::first);
  return it != myMap.end() && it->first == m;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool KeyMap::check(EventMode mode, StellaKey key, StellaMod mod) const
{
  return check(Mapping(mode, key, mod));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string KeyMap::getDesc(const Mapping& mapping)
{
#if defined(BSPF_MACOS) || defined(MACOS_KEYS)
  static constexpr string_view mod2 = "Option";
  static constexpr StellaMod MOD2   = StellaMod::ALT;
  static constexpr StellaMod LMOD2  = StellaMod::LALT;
  static constexpr StellaMod RMOD2  = StellaMod::RALT;
  static constexpr string_view mod3 = "Cmd";
  static constexpr StellaMod MOD3   = StellaMod::GUI;
  static constexpr StellaMod LMOD3  = StellaMod::LGUI;
  static constexpr StellaMod RMOD3  = StellaMod::RGUI;
#else
  static constexpr string_view mod2 = "Windows";
  static constexpr StellaMod MOD2   = StellaMod::GUI;
  static constexpr StellaMod LMOD2  = StellaMod::LGUI;
  static constexpr StellaMod RMOD2  = StellaMod::RGUI;
  static constexpr string_view mod3 = "Alt";
  static constexpr StellaMod MOD3   = StellaMod::ALT;
  static constexpr StellaMod LMOD3  = StellaMod::LALT;
  static constexpr StellaMod RMOD3  = StellaMod::RALT;
#endif

  string buf;
  buf.reserve(32);

  const auto append = [&](string_view part) {
    if(!buf.empty()) buf += '-';
    buf += part;
  };

  if((mapping.mod & StellaMod::CTRL) == StellaMod::CTRL) append("Ctrl");
  else if((mapping.mod & StellaMod::LCTRL) != StellaMod::NONE) append("Left Ctrl");
  else if((mapping.mod & StellaMod::RCTRL) != StellaMod::NONE) append("Right Ctrl");

  if((mapping.mod & MOD2) == MOD2) append(mod2);
  else if((mapping.mod & LMOD2) != StellaMod::NONE) append(std::format("Left {}", mod2));
  else if((mapping.mod & RMOD2) != StellaMod::NONE) append(std::format("Right {}", mod2));

  if((mapping.mod & MOD3) == MOD3) append(mod3);
  else if((mapping.mod & LMOD3) != StellaMod::NONE) append(std::format("Left {}", mod3));
  else if((mapping.mod & RMOD3) != StellaMod::NONE) append(std::format("Right {}", mod3));

  if((mapping.mod & StellaMod::SHIFT) == StellaMod::SHIFT) append("Shift");
  else if((mapping.mod & StellaMod::LSHIFT) != StellaMod::NONE) append("Left Shift");
  else if((mapping.mod & StellaMod::RSHIFT) != StellaMod::NONE) append("Right Shift");

  if(!buf.empty()) buf += '+';
  buf += StellaKeyName::forKey(mapping.key);

  return buf;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string KeyMap::getDesc(EventMode mode, StellaKey key, StellaMod mod)
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
  json mappings = json::array();

  for(const auto& [_mapping, _event]: myMap)
  {
    if(_mapping.mode != mode || _event == Event::NoType) continue;

    json mapping = json::object();
    mapping["event"] = _event;
    mapping["key"]   = _mapping.key;
    if(_mapping.mod != StellaMod::NONE)
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
        mapping.contains("mod") ? deserializeModkeyMask(mapping.at("mod")) : StellaMod::NONE
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

    if(static_cast<StellaMod>(mod) != StellaMod::NONE)
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

  if(StellaKeyTest::isModifierKey(m.key))  // handle solo modifier keys differently
    m.mod = StellaMod::NONE;
  else  // limit to modifiers we want to support
    m.mod = m.mod & (StellaMod::SHIFT | StellaMod::CTRL | StellaMod::ALT | StellaMod::GUI);

  return m;
}
