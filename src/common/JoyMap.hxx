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

#ifndef JOY_MAP_HXX
#define JOY_MAP_HXX

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
      EventMode mode{};
      int button{JOY_CTRL_NONE};         // button number
      JoyAxis axis{JoyAxis::NONE};       // horizontal/vertical
      JoyDir adir{JoyDir::NONE};         // axis direction (neg/pos)
      int hat{JOY_CTRL_NONE};            // hat number
      JoyHatDir hdir{JoyHatDir::CENTER}; // hat direction (left/right/up/down)

      explicit JoyMapping(EventMode c_mode, int c_button,
                          JoyAxis c_axis, JoyDir c_adir,
                          int c_hat, JoyHatDir c_hdir)
        : mode{c_mode}, button{c_button}, axis{c_axis}, adir{c_adir},
          hat{c_hat}, hdir{c_hdir} { }
      explicit JoyMapping(EventMode c_mode, int c_button,
                          JoyAxis c_axis, JoyDir c_adir)
        : mode{c_mode}, button{c_button}, axis{c_axis}, adir{c_adir},
          hat{JOY_CTRL_NONE}, hdir{JoyHatDir::CENTER} { }
      explicit JoyMapping(EventMode c_mode, int c_button,
                          int c_hat, JoyHatDir c_hdir)
        : mode{c_mode}, button{c_button},
          axis{JoyAxis::NONE}, adir{JoyDir::NONE},
          hat{c_hat}, hdir{c_hdir} { }

      auto operator<=>(const JoyMapping&) const = default;
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

    static nlohmann::json convertLegacyMapping(string_view lst);

    /** Erase all mappings for given mode */
    void eraseMode(EventMode mode);
    /** Erase given event's mapping for given mode */
    void eraseEvent(Event::Type event, EventMode mode);
    /** clear all mappings for a modes */
    // void clear() { myMap.clear(); }
    size_t size() const { return myMap.size(); }

  private:
    static string getDesc(Event::Type event, const JoyMapping& mapping);

    using MapEntry = std::pair<JoyMapping, Event::Type>;
    // IMPORTANT: myMap must always be kept sorted by JoyMapping::operator<.
    // All access must go through add/erase/get which maintain sort order.
    std::vector<MapEntry> myMap;

    // Following constructors and assignment operators not supported
    JoyMap(const JoyMap&) = delete;
    JoyMap(JoyMap&&) = delete;
    JoyMap& operator=(const JoyMap&) = delete;
    JoyMap& operator=(JoyMap&&) = delete;
};

#endif  // JOY_MAP_HXX
