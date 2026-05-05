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

#ifndef PROPERTIES_HXX
#define PROPERTIES_HXX

#include "repository/KeyValueRepository.hxx"
#include "bspf.hxx"

enum class PropType : uInt8 {
  Cart_MD5,
  Cart_Manufacturer,
  Cart_ModelNo,
  Cart_Name,
  Cart_Note,
  Cart_Rarity,
  Cart_Sound,
  Cart_StartBank,
  Cart_Type,
  Cart_Highscore,
  Cart_Url,
  Console_LeftDiff,
  Console_RightDiff,
  Console_TVType,
  Console_SwapPorts,
  Controller_Left,
  Controller_Left1,
  Controller_Left2,
  Controller_Right,
  Controller_Right1,
  Controller_Right2,
  Controller_SwapPaddles,
  Controller_PaddlesXCenter,
  Controller_PaddlesYCenter,
  Controller_MouseAxis,
  Display_Format,
  Display_VCenter,
  Display_Phosphor,
  Display_PPBlend,
  Bezel_Name,
  NumTypes
};

/**
  This class represents objects which maintain a collection of
  properties.  A property is a key and its corresponding value.

  A properties object can contain a reference to another properties
  object as its "defaults"; this second properties object is searched
  if the property key is not found in the original property list.

  @author  Bradford W. Mott and Stephen Anthony
*/
class Properties
{
  friend class PropertiesSet;

  public:
    static constexpr size_t NUM_PROPS = static_cast<size_t>(PropType::NumTypes);

    /**
      Creates an empty properties object with the specified defaults.
    */
    Properties();
    ~Properties() = default;

    Properties(const Properties&) = default;
    Properties& operator=(const Properties&) = default;

    // Enable move semantics for efficient storage in PropsList maps
    Properties(Properties&&) = default;
    Properties& operator=(Properties&&) = default;

  public:
    void load(KeyValueRepository& repo);

    bool save(KeyValueRepository& repo) const;

    /**
      Get the value assigned to the specified key.  If the key does
      not exist then the empty string is returned.

      @param key  The key of the property to lookup
      @return     The value of the property
    */
    const string& get(PropType key) const {
      const auto pos = static_cast<size_t>(key);
      return pos < NUM_PROPS ? myProperties[pos] : EmptyString();
    }

    /**
      Set the value associated with key to the given value.

      @param key    The key of the property to set
      @param value  The value to assign to the property
    */
    void set(PropType key, string_view value);

    /**
      Print the attributes of this properties object
    */
    void print() const;

    /**
      Resets all properties to their defaults
    */
    void setDefaults();

    /**
      Resets the property of the given key to its default

      @param key  The key of the property to reset
    */
    void reset(PropType key);

    /**
      Overloaded equality operators

      @param properties The properties object to compare to
      @return True if the properties are equal, else false
    */
    bool operator==(const Properties& properties) const {
      return myProperties == properties.myProperties;
    }
    bool operator!=(const Properties& properties) const {
      return !(*this == properties);
    }

  private:
    /**
      Get the property type associated with the named property

      @param name  The PropType key associated with the given string
    */
    static PropType getPropType(string_view name);

    /**
      When printing each collection of ROM properties, it is useful to
      see which columns correspond to the output fields; this method
      provides that output.
    */
    static void printHeader();

  private:
    // The array of properties for this instance (mutable runtime values)
    std::array<string, NUM_PROPS> myProperties;

    // Default property values — constexpr string_view, no heap allocation
    static constexpr std::array<string_view, NUM_PROPS> ourDefaultProperties = {{
      "",       // Cart.MD5
      "",       // Cart.Manufacturer
      "",       // Cart.ModelNo
      "",       // Cart.Name
      "",       // Cart.Note
      "",       // Cart.Rarity
      "MONO",   // Cart.Sound
      "AUTO",   // Cart.StartBank
      "AUTO",   // Cart.Type
      "",       // Cart.Highscore
      "",       // Cart.Url
      "B",      // Console.LeftDiff
      "B",      // Console.RightDiff
      "COLOR",  // Console.TVType
      "NO",     // Console.SwapPorts
      "AUTO",   // Controller.Left
      "AUTO",   // Controller.Left1
      "AUTO",   // Controller.Left2
      "AUTO",   // Controller.Right
      "AUTO",   // Controller.Right1
      "AUTO",   // Controller.Right2
      "NO",     // Controller.SwapPaddles
      "12",     // Controller.PaddlesXCenter
      "12",     // Controller.PaddlesYCenter
      "AUTO",   // Controller.MouseAxis
      "AUTO",   // Display.Format
      "0",      // Display.VCenter
      "NO",     // Display.Phosphor
      "0",      // Display.PPBlend
      ""        // Bezel.Name
    }};

    // Property name strings — constexpr string_view, no heap allocation
    static constexpr std::array<string_view, NUM_PROPS> ourPropertyNames = {{
      "Cart.MD5",
      "Cart.Manufacturer",
      "Cart.ModelNo",
      "Cart.Name",
      "Cart.Note",
      "Cart.Rarity",
      "Cart.Sound",
      "Cart.StartBank",
      "Cart.Type",
      "Cart.Highscore",
      "Cart.Url",
      "Console.LeftDiff",
      "Console.RightDiff",
      "Console.TVType",
      "Console.SwapPorts",
      "Controller.Left",
      "Controller.Left1",
      "Controller.Left2",
      "Controller.Right",
      "Controller.Right1",
      "Controller.Right2",
      "Controller.SwapPaddles",
      "Controller.PaddlesXCenter",
      "Controller.PaddlesYCenter",
      "Controller.MouseAxis",
      "Display.Format",
      "Display.VCenter",
      "Display.Phosphor",
      "Display.PPBlend",
      "Bezel.Name"
    }};

    // Sorted lookup table for getPropType() binary search.
    // Each entry maps a property name to its PropType.
    // MUST remain in case-insensitive sorted order.
    using PropTypeEntry = std::pair<string_view, PropType>;
    static constexpr std::array<PropTypeEntry, NUM_PROPS> ourNameToPropType = {{
      { "Bezel.Name"                , PropType::Bezel_Name                },
      { "Cart.Highscore"            , PropType::Cart_Highscore            },
      { "Cart.Manufacturer"         , PropType::Cart_Manufacturer         },
      { "Cart.MD5"                  , PropType::Cart_MD5                  },
      { "Cart.ModelNo"              , PropType::Cart_ModelNo              },
      { "Cart.Name"                 , PropType::Cart_Name                 },
      { "Cart.Note"                 , PropType::Cart_Note                 },
      { "Cart.Rarity"               , PropType::Cart_Rarity               },
      { "Cart.Sound"                , PropType::Cart_Sound                },
      { "Cart.StartBank"            , PropType::Cart_StartBank            },
      { "Cart.Type"                 , PropType::Cart_Type                 },
      { "Cart.Url"                  , PropType::Cart_Url                  },
      { "Console.LeftDiff"          , PropType::Console_LeftDiff          },
      { "Console.RightDiff"         , PropType::Console_RightDiff         },
      { "Console.SwapPorts"         , PropType::Console_SwapPorts         },
      { "Console.TVType"            , PropType::Console_TVType            },
      { "Controller.Left"           , PropType::Controller_Left           },
      { "Controller.Left1"          , PropType::Controller_Left1          },
      { "Controller.Left2"          , PropType::Controller_Left2          },
      { "Controller.MouseAxis"      , PropType::Controller_MouseAxis      },
      { "Controller.PaddlesXCenter" , PropType::Controller_PaddlesXCenter },
      { "Controller.PaddlesYCenter" , PropType::Controller_PaddlesYCenter },
      { "Controller.Right"          , PropType::Controller_Right          },
      { "Controller.Right1"         , PropType::Controller_Right1         },
      { "Controller.Right2"         , PropType::Controller_Right2         },
      { "Controller.SwapPaddles"    , PropType::Controller_SwapPaddles    },
      { "Display.Format"            , PropType::Display_Format            },
      { "Display.Phosphor"          , PropType::Display_Phosphor          },
      { "Display.PPBlend"           , PropType::Display_PPBlend           },
      { "Display.VCenter"           , PropType::Display_VCenter           },
    }};
};

#endif  // PROPERTIES_HXX
