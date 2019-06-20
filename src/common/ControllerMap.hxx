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
class ControllerMap
{
  public:

    struct ControllerMapping
    {
      EventMode mode;
      int stick;  // stick number
      int button; // button number
      JoyAxis axis;   // horizontal/vertical
      JoyDir adir;   // axis direction (neg/pos)
      int hat;    // hat number
      JoyHat hdir;   // hat direction (left/right/up/down)

      ControllerMapping()
        : mode(EventMode(0)), stick(0), button(0),
          axis(JoyAxis(0)), adir(JoyDir(0)), hat(0), hdir(JoyHat(0)) { }
      ControllerMapping(const ControllerMapping& m)
        : mode(m.mode), stick(m.stick), button(m.button),
          axis(m.axis), adir(m.adir), hat(m.hat), hdir(m.hdir) { }
      explicit ControllerMapping(EventMode c_mode, int c_stick, int c_button,
        JoyAxis c_axis, JoyDir c_adir, int c_hat, JoyHat c_hdir)
        : mode(c_mode), stick(c_stick), button(c_button),
          axis(c_axis), adir(c_adir), hat(c_hat), hdir(c_hdir) { }

      bool operator==(const ControllerMapping& other) const
      {
        return (mode == other.mode
          && stick == other.stick
          && button == other.button
          && axis == other.axis
          && adir == other.adir
          && hat == other.hat
          && hdir == other.hdir
        );
      }
    };
    using ControllerMappingArray = std::vector<ControllerMapping>;

    ControllerMap();
    virtual ~ControllerMap() = default;

    /** Add new mapping for given event */
    void add(const Event::Type event, const ControllerMapping& mapping);
    void add(const Event::Type event, const EventMode mode, const int stick, const int button,
      const JoyAxis axis, const JoyDir adir, const int hat, const JoyHat hdir);

    /** Erase mapping */
    void erase(const ControllerMapping& mapping);
    void erase(const EventMode mode, const int stick, const int button,
      const JoyAxis axis, const JoyDir adir, const int hat, const JoyHat hdir);

    /** Get event for mapping */
    Event::Type get(const ControllerMapping& mapping) const;
    Event::Type get(const EventMode mode, const int stick, const int button,
      const JoyAxis axis, const JoyDir adir, const int hat, const JoyHat hdir) const;

    /** Check if a mapping exists */
    bool check(const ControllerMapping& mapping) const;
    bool check(const EventMode mode, const int stick, const int button,
      const JoyAxis axis, const JoyDir adir, const int hat, const JoyHat hdir) const;

    /** Get mapping description */
    string getDesc(const Event::Type event, const ControllerMapping& mapping) const;
    string getDesc(const Event::Type event, const EventMode mode, const int stick, const int button,
      const JoyAxis axis, const JoyDir adir, const int hat, const JoyHat hdir) const;

    /** Get the mapping description(s) for given event and mode */
    string getEventMappingDesc(const Event::Type event, const EventMode mode) const;

    ControllerMappingArray getEventMapping(const Event::Type event, const EventMode mode) const;

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
    struct ControllerHash {
      size_t operator()(const ControllerMapping& m)const {
        return std::hash<uInt64>()((uInt64(m.mode)) // 3 bit
          ^ ((uInt64(m.stick)) << 3)  // 2 bits
          ^ ((uInt64(m.button)) << 5) // 2 bits
          ^ ((uInt64(m.axis)) << 7)   // 1 bit
          ^ ((uInt64(m.adir)) << 8)   // 1 bit
          ^ ((uInt64(m.hat)) << 9)    // 1 bit
          ^ ((uInt64(m.hdir)) << 10));  // 2 bits
      }
    };

    std::unordered_map<ControllerMapping, Event::Type, ControllerHash> myMap;
};

#endif
