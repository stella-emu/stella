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
// Copyright (c) 1995-2024 by Bradford W. Mott, Stephen Anthony
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
#include "json_lib.hxx"

/**
  This class handles controller mappings in Stella.

  @author  Thomas Jentzsch
*/
class JoyMap
{
  public:
    struct JoyMapping
    {
      EventMode mode{EventMode(0)};
      int button{0};                // button number
      JoyAxis axis{JoyAxis(0)};     // horizontal/vertical
      JoyDir adir{JoyDir(0)};       // axis direction (neg/pos)
      int hat{0};                   // hat number
      JoyHatDir hdir{JoyHatDir(0)}; // hat direction (left/right/up/down)

      explicit JoyMapping(EventMode c_mode, int c_button,
                          JoyAxis c_axis, JoyDir c_adir,
                          int c_hat, JoyHatDir c_hdir)
        : mode{c_mode}, button{c_button},
          axis{c_axis}, adir{c_adir},
          hat{c_hat}, hdir{c_hdir} { }
      explicit JoyMapping(EventMode c_mode, int c_button,
                          JoyAxis c_axis, JoyDir c_adir)
        : mode{c_mode}, button{c_button},
          axis{c_axis}, adir{c_adir},
          hat{JOY_CTRL_NONE}, hdir{JoyHatDir::CENTER} { }
      explicit JoyMapping(EventMode c_mode, int c_button,
                          int c_hat, JoyHatDir c_hdir)
        : mode{c_mode}, button{c_button},
          axis{JoyAxis::NONE}, adir{JoyDir::NONE},
          hat{c_hat}, hdir{c_hdir} { }

      ~JoyMapping() = default;
      JoyMapping(const JoyMapping&) = default;
      JoyMapping& operator=(const JoyMapping&) = default;
      JoyMapping(JoyMapping&&) = default;
      JoyMapping& operator=(JoyMapping&&) = default;

      bool operator==(const JoyMapping& other) const
      {
        return (mode == other.mode
          && button == other.button
          && axis == other.axis
          && adir == other.adir
          && hat == other.hat
          && hdir == other.hdir
        );
      }
    };
    using JoyMappingArray = std::vector<JoyMapping>;

    JoyMap() = default;
    ~JoyMap() = default;

    /** Add new mapping for given event */
    void add(Event::Type event, const JoyMapping& mapping);
    void add(Event::Type event, EventMode mode, int button,
             JoyAxis axis, JoyDir adir,
             int hat = JOY_CTRL_NONE, JoyHatDir hdir = JoyHatDir::CENTER);
    void add(Event::Type event, EventMode mode, int button,
             int hat, JoyHatDir hdir);

    /** Erase mapping */
    void erase(const JoyMapping& mapping);
    void erase(EventMode mode, int button, JoyAxis axis, JoyDir adir);
    void erase(EventMode mode, int button, int hat, JoyHatDir hdir);

    /** Get event for mapping */
    Event::Type get(const JoyMapping& mapping) const;
    Event::Type get(EventMode mode, int button, JoyAxis axis = JoyAxis::NONE,
                    JoyDir adir = JoyDir::NONE) const;
    Event::Type get(EventMode mode, int button, int hat, JoyHatDir hdir) const;

    /** Check if a mapping exists */
    bool check(const JoyMapping& mapping) const;
    bool check(EventMode mode, int button, JoyAxis axis, JoyDir adir,
               int hat = JOY_CTRL_NONE, JoyHatDir hdir = JoyHatDir::CENTER) const;

    /** Get mapping description */
    string getEventMappingDesc(int stick, Event::Type event, EventMode mode) const;

    JoyMappingArray getEventMapping(Event::Type event, EventMode mode) const;

    nlohmann::json saveMapping(EventMode mode) const;
    int loadMapping(const nlohmann::json& eventMappings, EventMode mode);

    static nlohmann::json convertLegacyMapping(string lst);

    /** Erase all mappings for given mode */
    void eraseMode(EventMode mode);
    /** Erase given event's mapping for given mode */
    void eraseEvent(Event::Type event, EventMode mode);
    /** clear all mappings for a modes */
    // void clear() { myMap.clear(); }
    size_t size() const { return myMap.size(); }

  private:
    static string getDesc(Event::Type event, const JoyMapping& mapping);

    struct JoyHash {
      size_t operator()(const JoyMapping& m)const {
        return std::hash<uInt64>()((static_cast<uInt64>(m.mode)) // 3 bits
          + ((static_cast<uInt64>(m.button)) * 7)  // 3 bits
          + (((static_cast<uInt64>(m.axis)) << 0)  // 2 bits
           | ((static_cast<uInt64>(m.adir)) << 2)  // 2 bits
           | ((static_cast<uInt64>(m.hat )) << 4)  // 1 bit
           | ((static_cast<uInt64>(m.hdir)) << 5)  // 2 bits
            ) * 61
        );
      }
    };

    std::unordered_map<JoyMapping, Event::Type, JoyHash> myMap;

    // Following constructors and assignment operators not supported
    JoyMap(const JoyMap&) = delete;
    JoyMap(JoyMap&&) = delete;
    JoyMap& operator=(const JoyMap&) = delete;
    JoyMap& operator=(JoyMap&&) = delete;
};

#endif
