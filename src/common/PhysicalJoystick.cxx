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

#include "bspf.hxx"
#include "PhysicalJoystick.hxx"
#include "jsonDefinitions.hxx"
#include "Logger.hxx"

using json = nlohmann::json;

namespace {
  string jsonName(EventMode eventMode) {
    return json(eventMode).get<string>();
  }

  EventMode eventModeFromJsonName(string_view name) {
    EventMode result{};

    from_json(json(name), result);

    return result;
  }
} // namespace

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PhysicalJoystick::initialize(int index, string_view desc,
            int axes, int buttons, int hats, int /*balls*/)
{
  ID = index;
  name = desc;

  numAxes    = axes;
  numButtons = buttons;
  numHats    = hats;
  axisLastValue.resize(numAxes, 0);

  // Erase the mappings
  for(const auto mode: {
    EventMode::kMenuMode, EventMode::kJoystickMode, EventMode::kPaddlesMode,
    EventMode::kKeyboardMode, EventMode::kDrivingMode, EventMode::kCommonMode
  })
    eraseMap(mode);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
json PhysicalJoystick::getMap() const
{
  json mapping = json::object();

  mapping["name"] = name;
  mapping["port"] = getName(port);

  for (const auto& mode: {
    EventMode::kMenuMode, EventMode::kJoystickMode, EventMode::kPaddlesMode, EventMode::kKeyboardMode, EventMode::kDrivingMode, EventMode::kCommonMode
  })
    mapping[jsonName(mode)] = joyMap.saveMapping(mode);

  return mapping;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool PhysicalJoystick::setMap(const json& map)
{
  int i = 0;

  for (const auto& entry: map.items()) {
    if (entry.key() == "name")
      continue;
    if(entry.key() == "port")
    {
      port = getPort(string{entry.value()});  // json doesn't support string_view
      continue;
    }

    try {
      joyMap.loadMapping(entry.value(), eventModeFromJsonName(entry.key()));
    } catch (const json::exception&) {
      Logger::error(std::format("ignoring invalid json mapping for {}", entry.key()));
    }
    i++;
  }

  if(i != 6)
  {
    Logger::error(std::format("invalid controller mappings found for {}",
      (map.contains("name") && map.at("name").is_string())
        ? std::format("stick {}", map["name"].get<string>())
        : "unknown stick"));

    return false;
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
json PhysicalJoystick::convertLegacyMapping(string_view mapping, string_view name)
{
  json convertedMapping = json::object();

  // Skip joystick name (first segment)
  auto pos = mapping.find(MODE_DELIM);
  if(pos == string_view::npos)
  {
    convertedMapping["name"] = name;
    return convertedMapping;
  }
  mapping = mapping.substr(pos + 1);

  while(!mapping.empty())
  {
    pos = mapping.find(MODE_DELIM);
    string_view segment = mapping.substr(0, pos);

    // Get event mode from leading integer before '|'
    int mode{0};
    const auto pipePos = segment.find('|');
    if(pipePos != string_view::npos)
    {
      const string_view modeStr = segment.substr(0, pipePos);
      std::from_chars(modeStr.data(), modeStr.data() + modeStr.size(), mode);
      // Remove leading "<mode>|" string
      segment = segment.substr(pipePos + 1);
    }

    const json lmappingForMode = JoyMap::convertLegacyMapping(segment);
    convertedMapping[jsonName(static_cast<EventMode>(mode))] = lmappingForMode;

    if(pos == string_view::npos) break;
    mapping = mapping.substr(pos + 1);
  }

  convertedMapping["name"] = name;
  return convertedMapping;
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
string PhysicalJoystick::about() const
{
  static constexpr std::array<string_view, 3> PORT_NAMES = {
    "auto", "left", "right"
  };
  return std::format("'{}' in {} port with: {} axes, {} buttons, {} hats",
    name,
    PORT_NAMES[static_cast<uInt8>(port)],
    numAxes, numButtons, numHats);
}
