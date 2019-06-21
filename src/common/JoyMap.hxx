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

#ifndef CONTROLLERMAP_HXX
#define CONTROLLERMAP_HXX

#include <unordered_map>
#include "Event.hxx"
#include "EventHandlerConstants.hxx"

/**
  This class handles controller mappings in Stella.

  @author  Thomas Jentzsch
*/
class JoyMap
{
  public:

    struct JoyMapping
    {
      EventMode mode;
      int button;   // button number
      JoyAxis axis; // horizontal/vertical
      JoyDir adir;  // axis direction (neg/pos)

      JoyMapping()
        : mode(EventMode(0)), button(0),
          axis(JoyAxis(0)), adir(JoyDir(0)) { }
      JoyMapping(const JoyMapping& m)
        : mode(m.mode), button(m.button),
          axis(m.axis), adir(m.adir) { }
      explicit JoyMapping(EventMode c_mode, int c_button,
        JoyAxis c_axis, JoyDir c_adir)
        : mode(c_mode), button(c_button),
          axis(c_axis), adir(c_adir) { }

      bool operator==(const JoyMapping& other) const
      {
        return (mode == other.mode
          && button == other.button
          && axis == other.axis
          && adir == other.adir
        );
      }
    };
    using JoyMappingArray = std::vector<JoyMapping>;

    JoyMap();
    virtual ~JoyMap() = default;

    /** Add new mapping for given event */
    void add(const Event::Type event, const JoyMapping& mapping);
    void add(const Event::Type event, const EventMode mode, const int button,
             const JoyAxis axis, const JoyDir adir);

    /** Erase mapping */
    void erase(const JoyMapping& mapping);
    void erase(const EventMode mode, const int button,
               const JoyAxis axis, const JoyDir adir);

    /** Get event for mapping */
    Event::Type get(const JoyMapping& mapping) const;
    Event::Type get(const EventMode mode, const int button,
                    const JoyAxis axis = JoyAxis::NONE, const JoyDir adir = JoyDir::NONE) const;

    /** Check if a mapping exists */
    bool check(const JoyMapping& mapping) const;
    bool check(const EventMode mode, const int button,
               const JoyAxis axis, const JoyDir adir) const;

    /** Get mapping description */
    string getDesc(const Event::Type event, const JoyMapping& mapping) const;
    string getDesc(const Event::Type event, const EventMode mode, const int button,
                   const JoyAxis axis, const JoyDir adir) const;

    /** Get the mapping description(s) for given stick, event and mode */
    string getEventMappingDesc(int stick, const Event::Type event, const EventMode mode) const;

    JoyMappingArray getEventMapping(const Event::Type event, const EventMode mode) const;

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
    struct JoyHash {
      size_t operator()(const JoyMapping& m)const {
        return std::hash<uInt64>()((uInt64(m.mode)) // 3 bit
          ^ ((uInt64(m.button)) << 2)  // 2 bits
          ^ ((uInt64(m.axis)) << 4)    // 1 bit
          ^ ((uInt64(m.adir)) << 5)    // 1 bit
          );
      }
    };

    std::unordered_map<JoyMapping, Event::Type, JoyHash> myMap;
};

#endif
