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

#ifndef JOYHATMAP_HXX
#define JOYHATMAP_HXX

#include <unordered_map>
#include "Event.hxx"
#include "EventHandlerConstants.hxx"

/**
  This class handles controller mappings in Stella.

  @author  Thomas Jentzsch
*/
class JoyHatMap
{
public:

  struct JoyHatMapping
  {
    EventMode mode;
    int button;   // button number
    int hat;      // hat number
    JoyHat hdir;  // hat direction (left/right/up/down)

    JoyHatMapping()
      : mode(EventMode(0)), button(0), hat(0), hdir(JoyHat(0)) { }
    JoyHatMapping(const JoyHatMapping& m)
      : mode(m.mode), button(m.button), hat(m.hat), hdir(m.hdir) { }
    explicit JoyHatMapping(EventMode c_mode, int c_button, int c_hat, JoyHat c_hdir)
      : mode(c_mode), button(c_button), hat(c_hat), hdir(c_hdir) { }

    bool operator==(const JoyHatMapping& other) const
    {
      return (mode == other.mode
        && button == other.button
        && hat == other.hat
        && hdir == other.hdir
        );
    }
  };
  using JoyHatMappingArray = std::vector<JoyHatMapping>;

  JoyHatMap();
  virtual ~JoyHatMap() = default;

  /** Add new mapping for given event */
  void add(const Event::Type event, const JoyHatMapping& mapping);
  void add(const Event::Type event, const EventMode mode,
           const int button, const int hat, const JoyHat hdir);

  /** Erase mapping */
  void erase(const JoyHatMapping& mapping);
  void erase(const EventMode mode,
             const int button, const int hat, const JoyHat hdir);

  /** Get event for mapping */
  Event::Type get(const JoyHatMapping& mapping) const;
  Event::Type get(const EventMode mode,
                  const int button, const int hat, const JoyHat hdir) const;

  /** Check if a mapping exists */
  bool check(const JoyHatMapping& mapping) const;
  bool check(const EventMode mode,
             const int button, const int hat, const JoyHat hdir) const;

  /** Get mapping description */
  string getDesc(const Event::Type event, const JoyHatMapping& mapping) const;
  string getDesc(const Event::Type event, const EventMode mode,
                 const int button, const int hat, const JoyHat hdir) const;

  /** Get the mapping description(s) for given stick, event and mode */
  string getEventMappingDesc(const int stick, const Event::Type event, const EventMode mode) const;

  JoyHatMappingArray getEventMapping(const Event::Type event, const EventMode mode) const;

  string saveMapping(const EventMode mode) const;
  int loadMapping(string& list, const EventMode mode);

  /** Erase all mappings for given mode */
  void eraseMode(const EventMode mode);
  /** Erase given event's mapping for given mode */
  void eraseEvent(const Event::Type event, const EventMode mode);
  /** clear all mappings for a modes */
  // void clear() { myMap.clear(); }
  size_t size() { return myMap.size(); }

private:
  struct JoyHatHash {
    size_t operator()(const JoyHatMapping& m)const {
      return std::hash<uInt64>()((uInt64(m.mode)) // 3 bit
        ^ ((uInt64(m.button)) << 3)  // 2 bits
        ^ ((uInt64(m.hat)) << 5)     // 1 bit
        ^ ((uInt64(m.hdir)) << 6)    // 2 bits
        );
    }
  };

  std::unordered_map<JoyHatMapping, Event::Type, JoyHatHash> myMap;
};

#endif
