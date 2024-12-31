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
// Copyright (c) 1995-2025 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#ifndef KEYMAP_HXX
#define KEYMAP_HXX

#include <unordered_map>
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
      EventMode mode{EventMode(0)};
      StellaKey key{StellaKey(0)};
      StellaMod mod{StellaMod(0)};

      explicit Mapping(EventMode c_mode, StellaKey c_key, StellaMod c_mod)
        : mode{c_mode}, key{c_key}, mod{c_mod} { }
      explicit Mapping(EventMode c_mode, int c_key, int c_mod)
        : mode{c_mode}, key{static_cast<StellaKey>(c_key)}, mod{static_cast<StellaMod>(c_mod)} { }
      ~Mapping() = default;
      Mapping(const Mapping&) = default;
      Mapping& operator=(const Mapping&) = default;
      Mapping(Mapping&&) = default;
      Mapping& operator=(Mapping&&) = default;

      bool operator==(const Mapping& other) const
      {
        return (key == other.key
          && mode == other.mode
          && (((mod | other.mod) & KBDM_SHIFT) ? (mod & other.mod & KBDM_SHIFT) : true)
          && (((mod | other.mod) & KBDM_CTRL ) ? (mod & other.mod & KBDM_CTRL ) : true)
          && (((mod | other.mod) & KBDM_ALT  ) ? (mod & other.mod & KBDM_ALT  ) : true)
          && (((mod | other.mod) & KBDM_GUI  ) ? (mod & other.mod & KBDM_GUI  ) : true)
          );
      }
    };
    using MappingArray = std::vector<Mapping>;

    KeyMap() = default;
    ~KeyMap() = default;

    /** Add new mapping for given event */
    void add(Event::Type event, const Mapping& mapping);
    void add(Event::Type event, EventMode mode, int key, int mod);

    /** Erase mapping */
    void erase(const Mapping& mapping);
    void erase(EventMode mode, int key, int mod);

    /** Get event for mapping */
    Event::Type get(const Mapping& mapping) const;
    Event::Type get(EventMode mode, int key, int mod) const;

    /** Check if a mapping exists */
    bool check(const Mapping& mapping) const;
    bool check(EventMode mode, int key, int mod) const;

    /** Get mapping description */
    static string getDesc(const Mapping& mapping);
    static string getDesc(EventMode mode, int key, int mod);

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
    size_t size() { return myMap.size(); }

    bool& enableMod() { return myModEnabled;  }

  private:
    //** Convert modifiers */
    static Mapping convertMod(const Mapping& mapping);

    struct KeyHash {
      size_t operator()(const Mapping& m) const {
        return std::hash<uInt64>()((static_cast<uInt64>(m.mode))     // 3 bits
          + ((static_cast<uInt64>(m.key)) * 7)                       // 8 bits
          + (((static_cast<uInt64>((m.mod & KBDM_SHIFT) != 0) << 0)) // 1 bit
           | ((static_cast<uInt64>((m.mod & KBDM_ALT  ) != 0) << 1)) // 1 bit
           | ((static_cast<uInt64>((m.mod & KBDM_GUI  ) != 0) << 2)) // 1 bit
           | ((static_cast<uInt64>((m.mod & KBDM_CTRL ) != 0) << 3)) // 1 bit
            ) * 2047
        );
      }
    };

    std::unordered_map<Mapping, Event::Type, KeyHash> myMap;

    // Indicates whether the key-combos tied to a modifier key are
    // being used or not (e.g. Ctrl by default is the fire button,
    // pressing it with a movement key could inadvertantly activate
    // a Ctrl combo when it isn't wanted)
    bool myModEnabled{true};

    // Following constructors and assignment operators not supported
    KeyMap(const KeyMap&) = delete;
    KeyMap(KeyMap&&) = delete;
    KeyMap& operator=(const KeyMap&) = delete;
    KeyMap& operator=(KeyMap&&) = delete;
};

#endif
