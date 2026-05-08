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

#ifndef KEYMAP_HXX
#define KEYMAP_HXX

#include "Event.hxx"
#include "EventHandlerConstants.hxx"
#include "StellaKeys.hxx"
#include "json_lib.hxx"

/**
  This class handles keyboard mappings in Stella.

  @author  Thomas Jentzsch
*/
class KeyMap
{
  public:
    struct Mapping
    {
      EventMode mode{};
      StellaKey key{StellaKey::UNKNOWN};
      StellaMod mod{StellaMod::NONE};

      explicit Mapping(EventMode c_mode, StellaKey c_key, StellaMod c_mod)
        : mode{c_mode}, key{c_key}, mod{c_mod} { }
      explicit Mapping(EventMode c_mode, int c_key, int c_mod)
        : mode{c_mode}, key{static_cast<StellaKey>(c_key)}, mod{static_cast<StellaMod>(c_mod)} { }

      bool operator==(const Mapping& other) const {
        return (key == other.key
          && mode == other.mode
          && (((mod | other.mod) & StellaMod::SHIFT) != StellaMod::NONE
            ? (mod & other.mod & StellaMod::SHIFT) != StellaMod::NONE
            : true)
          && (((mod | other.mod) & StellaMod::CTRL ) != StellaMod::NONE
            ? (mod & other.mod & StellaMod::CTRL ) != StellaMod::NONE
            : true)
          && (((mod | other.mod) & StellaMod::ALT  ) != StellaMod::NONE
            ? (mod & other.mod & StellaMod::ALT  ) != StellaMod::NONE
            : true)
          && (((mod | other.mod) & StellaMod::GUI  ) != StellaMod::NONE
            ? (mod & other.mod & StellaMod::GUI  ) != StellaMod::NONE
            : true)
          );
      }
      bool operator<(const Mapping& other) const {
        if(mode != other.mode) return mode < other.mode;
        if(key  != other.key)  return key  < other.key;
        return mod < other.mod;
      }
    };
    using MappingArray = std::vector<Mapping>;

    KeyMap() = default;
    ~KeyMap() = default;

    /** Add new mapping for given event */
    void add(Event::Type event, const Mapping& mapping);
    void add(Event::Type event, EventMode mode, StellaKey key, StellaMod mod);

    /** Erase mapping */
    void erase(const Mapping& mapping);
    void erase(EventMode mode, StellaKey key, StellaMod mod);

    /** Get event for mapping */
    Event::Type get(const Mapping& mapping) const;
    Event::Type get(EventMode mode, StellaKey key, StellaMod mod) const;

    /** Check if a mapping exists */
    bool check(const Mapping& mapping) const;
    bool check(EventMode mode, StellaKey key, StellaMod mod) const;

    /** Get mapping description */
    static string getDesc(const Mapping& mapping);
    static string getDesc(EventMode mode, StellaKey key, StellaMod mod);

    /** Get the mapping description(s) for given event and mode */
    string getEventMappingDesc(Event::Type event, EventMode mode) const;

    MappingArray getEventMapping(Event::Type event, EventMode mode) const;

    nlohmann::json saveMapping(EventMode mode) const;
    int loadMapping(const nlohmann::json& mapping, EventMode mode);

    static nlohmann::json convertLegacyMapping(string_view lm);

    /** Erase all mappings for given mode */
    void eraseMode(EventMode mode);
    /** Erase given event's mapping for given mode */
    void eraseEvent(Event::Type event, EventMode mode);
    /** clear all mappings for a modes */
    // void clear() { myMap.clear(); }
    size_t size() const { return myMap.size(); }

    bool& enableMod() { return myModEnabled;  }

  private:
    //** Convert modifiers */
    static Mapping convertMod(const Mapping& mapping);

    // IMPORTANT: myMap must always be kept sorted by Mapping::operator<.
    // All access must go through convertMod() first to normalise modifiers,
    // ensuring operator== and operator< are consistent for binary search.
    using MapEntry = std::pair<Mapping, Event::Type>;
    std::vector<MapEntry> myMap;

    // Indicates whether the key-combos tied to a modifier key are
    // being used or not (e.g. Ctrl by default is the fire button,
    // pressing it with a movement key could inadvertantly activate
    // a Ctrl combo when it isn't wanted)
    bool myModEnabled{true};

  private:
    // Following constructors and assignment operators not supported
    KeyMap(const KeyMap&) = delete;
    KeyMap(KeyMap&&) = delete;
    KeyMap& operator=(const KeyMap&) = delete;
    KeyMap& operator=(KeyMap&&) = delete;
};

#endif  // KEYMAP_HXX
