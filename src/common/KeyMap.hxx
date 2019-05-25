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

#ifndef KEYMAP_HXX
#define KEYMAP_HXX

#include "Event.hxx"
#include "EventHandlerConstants.hxx"

/**
  This class handles keyboard mappings in Stella.

  @author  Thomas Jentzsch
*/
class KeyMap
{
  public:

    struct Mapping
    {
      EventMode mode;
      StellaKey key;
      StellaMod mod;

      Mapping() : mode(EventMode(0)), key(StellaKey(0)), mod(StellaMod(0)) { }
      Mapping(const Mapping& k) : mode(k.mode), key(k.key), mod(k.mod) { }
      explicit Mapping(EventMode mode, StellaKey key, StellaMod mod = StellaMod::KBDM_NONE)
        : mode(mode), key(key), mod(mod) { }
      explicit Mapping(int mode, int key, int mod = StellaMod::KBDM_NONE)
        : mode(EventMode(mode)), key(StellaKey(key)), mod(StellaMod(mod)) { }

      bool operator==(const Mapping& other) const
      {
        return (mode == other.mode
          && key == other.key
          && mod == other.mod);
      }
    };

    KeyMap();
    virtual ~KeyMap() = default;

    /** Add new mapping for given event */
    void add(const Event::Type event, const Mapping& input);
    void add(const Event::Type event, const int mode, const int key, const int mod = StellaMod::KBDM_NONE);

    /** Erase mapping */
    void erase(const Mapping& input);
    void erase(const int mode, const int key, const int mod = StellaMod::KBDM_NONE);

    /** Get event for mapping */
    Event::Type get(const Mapping& input) const;
    Event::Type get(const int mode, const int key, const int mod = StellaMod::KBDM_NONE) const;

    /** Get the mapping(s) description for given event and mode */
    string getEventMappingDesc(const Event::Type event, const int mode) const;

    /** Get mapping description */
    string getDesc(const Mapping& input) const;
    string getDesc(const int mode, const int key, const int mod = StellaMod::KBDM_NONE) const;

    string saveMapping(const int mode) const;
    int loadMapping(string& list, const int mode);

    /** Erase all mappings for given mode */
    void eraseMode(const int mode);
    /** Erase given event's mapping for given mode */
    void eraseEvent(const Event::Type event, const int mode);
    /** clear all mappings for a modes */
    // void clear() { myMap.clear(); }
    size_t size() { return myMap.size(); }

  private:
    //** Convert modifiers */
    Mapping convertMod(const Mapping& input) const;

    struct KeyHash {
      size_t operator()(const Mapping& k)const {
        return std::hash<long long>()(((long long)k.mode)
          ^ (((long long)k.key) << 16)
          ^ (((long long)k.mod) << 32));
      }
    };

    std::unordered_map<Mapping, Event::Type, KeyHash> myMap;
};

#endif
