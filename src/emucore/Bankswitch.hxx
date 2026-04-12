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

#ifndef BANKSWITCH_HXX
#define BANKSWITCH_HXX

#include <algorithm>

#include "FSNode.hxx"
#include "bspf.hxx"

/**
  This class contains all information about the bankswitch schemes supported
  by Stella, as well as convenience functions to map from scheme type to
  readable string, and vice-versa.

  It also includes all logic that determines what a 'valid' rom filename is.
  That is, all extensions that represent valid schemes.

  @author  Stephen Anthony
*/
class Bankswitch
{
  public:
    // Currently supported bankswitch schemes
    // Those starting with a number need an underscore (bugprone-reserved-identifier)
    enum class Type: uInt8 {
      AUTO,   _03E0,  _0840,   _0FA0,  _2IN1, _4IN1, _8IN1, _16IN1,
      _32IN1, _64IN1, _128IN1, _2K,    _3E,   _3EX,  _3EP,  _3F,
      _4A50,  _4K,    _4KSC,   AR,     BF,    BFSC,  BUS,   CDF,
      CM,     CTY,    CV,      DF,     DFSC,  DPC,   DPCP,  E0,
      E7,     EF,     EFF,     EFSC,   ELF,   F0,    F4,    F4SC,
      F6,     F6SC,   F8,      F8SC,   FA,    FA2,   FC,    FE,
      GL,     JANE,   MDM,     MVC,    SB,    TVBOY, UA,    UASW,
      WD,     WDSW,   WF8,     X07,
    #ifdef CUSTOM_ARM
      CUSTOM
    #endif
    };

    // Number of schemes (excluding sentinel enum values)
    static constexpr size_t NumSchemes =
    #ifdef CUSTOM_ARM
      static_cast<size_t>(Type::CUSTOM) + 1;
    #else
      static_cast<size_t>(Type::X07) + 1;
    #endif

    // Number of multi-cart schemes
    static constexpr size_t NumMulti =
      static_cast<size_t>(Type::_128IN1) - static_cast<size_t>(Type::_2IN1) + 1;

    struct SizesType {
      size_t minSize{0};
      size_t maxSize{0};
    };
    static constexpr size_t any_KB{0};

    static constexpr std::array<SizesType, NumSchemes> Sizes = {{
      { Bankswitch::any_KB, Bankswitch::any_KB }, // AUTO
      {    8_KB,   8_KB },    // _03E0
      {    8_KB,   8_KB },    // _0840
      {    8_KB,   8_KB },    // _0FA0
      {    4_KB,  64_KB },    // _2IN1
      {    8_KB,  64_KB },    // _4IN1
      {   16_KB,  64_KB },    // _8IN1
      {   32_KB, 128_KB },    // _16IN1
      {   64_KB, 128_KB },    // _32IN1
      {  128_KB, 256_KB },    // _64IN1
      {  256_KB, 512_KB },    // _128IN1
      {    0_KB,   4_KB },    // _2K
      {    8_KB, 512_KB },    // _3E
      {    8_KB, 512_KB },    // _3EX
      {    8_KB,  64_KB },    // _3EP
      {    8_KB, 512_KB },    // _3F
      {   64_KB,  64_KB },    // _4A50
      {    4_KB,   4_KB },    // _4K
      {    4_KB,   4_KB },    // _4KSC
      {    6_KB,  33_KB },    // AR
      {  256_KB, 256_KB },    // BF
      {  256_KB, 256_KB },    // BFSC
      {   32_KB,  32_KB },    // BUS
      {   32_KB, 512_KB },    // CDF
      {   16_KB,  16_KB },    // CM
      {   32_KB,  32_KB },    // CTY
      {    0_KB,   4_KB },    // CV
      {  128_KB, 128_KB },    // DF
      {  128_KB, 128_KB },    // DFSC
      {   10_KB,  11_KB },    // DPC
      {   16_KB,  64_KB },    // DPCP
      {    8_KB,   8_KB },    // E0
      {    8_KB,  16_KB },    // E7
      {   64_KB,  64_KB },    // EF
      {   64_KB,  64_KB },    // EFF
      {   64_KB,  64_KB },    // EFSC
      { Bankswitch::any_KB,
        Bankswitch::any_KB }, // ELF
      {   64_KB,  64_KB },    // F0
      {   32_KB,  32_KB },    // F4
      {   32_KB,  32_KB },    // F4SC
      {   16_KB,  16_KB },    // F6
      {   16_KB,  16_KB },    // F6SC
      {    8_KB,   8_KB },    // F8
      {    8_KB,   8_KB },    // F8SC
      {   12_KB,  12_KB },    // FA
      {   24_KB,  32_KB },    // FA2
      {   32_KB,  32_KB },    // FC
      {    8_KB,   8_KB },    // FE
      {    4_KB,   6_KB },    // GL
      {   16_KB,  16_KB },    // JANE
      {    8_KB,
        Bankswitch::any_KB }, // MDM
      { 1024_KB,
        Bankswitch::any_KB }, // MVC
      {  128_KB, 256_KB },    // SB
      {  512_KB, 512_KB },    // TVBOY
      {    8_KB,   8_KB },    // UA
      {    8_KB,   8_KB },    // UASW
      {    8_KB,   8_KB },    // WD
      {    8_KB,   8_KB+5 },  // WDSW
      {    8_KB,   8_KB },    // WF8
      {   64_KB,  64_KB },    // X07
    #ifdef CUSTOM_ARM
      { Bankswitch::any_KB,
        Bankswitch::any_KB }  // CUSTOM
    #endif
    }};

    // Info about the various bankswitch schemes, useful for displaying
    // in GUI dropdown boxes, etc
    struct Description {
      string_view name;
      string_view desc;
    };
    static constexpr std::array<Description, NumSchemes> BSList = {{
      { "AUTO"    , "Auto-detect"                 },
      { "03E0"    , "03E0 (8K Braz. Parker Bros)" },
      { "0840"    , "0840 (8K EconoBanking)"      },
      { "0FA0"    , "0FA0 (8K Fotomania)"         },
      { "2IN1"    , "2in1 Multicart (4-64K)"      },
      { "4IN1"    , "4in1 Multicart (8-64K)"      },
      { "8IN1"    , "8in1 Multicart (16-64K)"     },
      { "16IN1"   , "16in1 Multicart (32-128K)"   },
      { "32IN1"   , "32in1 Multicart (64/128K)"   },
      { "64IN1"   , "64in1 Multicart (128/256K)"  },
      { "128IN1"  , "128in1 Multicart (256/512K)" },
      { "2K"      , "2K (32-2048 bytes Atari)"    },
      { "3E"      , "3E (Tigervision, 32K RAM)"   },
      { "3EX"     , "3EX (Tigervision, 256K RAM)" },
      { "3E+"     , "3E+ (TJ modified 3E)"        },
      { "3F"      , "3F (512K Tigervision)"       },
      { "4A50"    , "4A50 (64K 4A50 + RAM)"       },
      { "4K"      , "4K (4K Atari)"               },
      { "4KSC"    , "4KSC (CPUWIZ 4K + RAM)"      },
      { "AR"      , "AR (Supercharger)"           },
      { "BF"      , "BF (CPUWIZ 256K)"            },
      { "BFSC"    , "BFSC (CPUWIZ 256K + RAM)"    },
      { "BUS"     , "BUS (Experimental)"          },
      { "CDF"     , "CDF (Chris, Darrell, Fred)"  },
      { "CM"      , "CM (SpectraVideo CompuMate)" },
      { "CTY"     , "CTY (CDW - Chetiry)"         },
      { "CV"      , "CV (Commavid extra RAM)"     },
      { "DF"      , "DF (CPUWIZ 128K)"            },
      { "DFSC"    , "DFSC (CPUWIZ 128K + RAM)"    },
      { "DPC"     , "DPC (Pitfall II)"            },
      { "DPC+"    , "DPC+ (Enhanced DPC)"         },
      { "E0"      , "E0 (8K Parker Bros)"         },
      { "E7"      , "E7 (8-16K M Network)"        },
      { "EF"      , "EF (64K H. Runner)"          },
      { "EFF"     , "EFF (64K+2Kee/Grizzards)"    },
      { "EFSC"    , "EFSC (64K H. Runner + RAM)"  },
      { "ELF"     , "ELF (ARM bus stuffing)"      },
      { "F0"      , "F0 (Dynacom Megaboy)"        },
      { "F4"      , "F4 (32K Atari)"              },
      { "F4SC"    , "F4SC (32K Atari + RAM)"      },
      { "F6"      , "F6 (16K Atari)"              },
      { "F6SC"    , "F6SC (16K Atari + RAM)"      },
      { "F8"      , "F8 (8K Atari)"               },
      { "F8SC"    , "F8SC (8K Atari + RAM)"       },
      { "FA"      , "FA (CBS RAM Plus)"           },
      { "FA2"     , "FA2 (CBS RAM Plus 24-32K)"   },
      { "FC"      , "FC (32K Amiga)"              },
      { "FE"      , "FE (8K Activision)"          },
      { "GL"      , "GL (GameLine Master Module)" },
      { "JANE"    , "JANE (16K Tarzan prototype)" },
      { "MDM"     , "MDM (Menu Driven Megacart)"  },
      { "MVC"     , "MVC (Movie Cart)"            },
      { "SB"      , "SB (128-256K SUPERbank)"     },
      { "TVBOY"   , "TV Boy (512K)"               },
      { "UA"      , "UA (8K UA Ltd.)"             },
      { "UASW"    , "UASW (8K UA swapped banks)"  },
      { "WD"      , "WD (Pink Panther)"           },
      { "WDSW"    , "WDSW (Pink Panther, bad)"    },
      { "WF8"     , "WF8 (Coleco, white carts)"   },
      { "X07"     , "X07 (64K AtariAge)"          },
    #ifdef CUSTOM_ARM
      { "CUSTOM"  , "CUSTOM (ARM)"                }
    #endif
    }};

  public:
    // Convert BSType enum to string_view
    static constexpr string_view typeToName(Bankswitch::Type type) {
      return BSList[static_cast<size_t>(type)].name;
    }

    // Convert BSType enum to description string_view
    static constexpr string_view typeToDesc(Bankswitch::Type type) {
      return BSList[static_cast<size_t>(type)].desc;
    }

    // Convert string to BSType enum via binary search on sorted constexpr table
    static Bankswitch::Type nameToType(string_view name) {
      const auto it = std::ranges::lower_bound( // NOLINT(readability-qualified-auto)
        ourNameToTypes, name,
        [](string_view a, string_view b) {
          return BSPF::compareIgnoreCase(a, b) < 0;
        },
        &TypeEntry::first);  // projection: extract the key from each element
      if(it != ourNameToTypes.end() && BSPF::compareIgnoreCase(it->first, name) == 0)
        return it->second;

      return Bankswitch::Type::AUTO;
    }

    // Determine bankswitch type by filename extension
    // Use 'AUTO' if unknown
    static Bankswitch::Type typeFromExtension(const FSNode& file) {
      const string_view path = file.getPath();
      const auto idx = path.find_last_of('.');
      if(idx != string_view::npos)
      {
        const auto it = std::ranges::lower_bound( // NOLINT(readability-qualified-auto)
          ourExtensions, path.substr(idx + 1),
          [](string_view a, string_view b) {
            return BSPF::compareIgnoreCase(a, b) < 0;
          },
          &TypeEntry::first);
        if(it != ourExtensions.end() &&
           BSPF::compareIgnoreCase(it->first, path.substr(idx + 1)) == 0)
          return it->second;
      }

      return Bankswitch::Type::AUTO;
    }

    /**
      Is this a valid ROM filename (does it have a valid extension?).

      @param name  Filename of potential ROM file
      @param ext   The extension extracted from the given file
     */
    static bool isValidRomName(string_view name, string& ext) {
      const auto idx = name.find_last_of('.');
      if(idx != string_view::npos)
      {
        const auto e = name.substr(idx + 1);
        const auto it = std::ranges::lower_bound( // NOLINT(readability-qualified-auto)
          ourExtensions, e,
          [](string_view a, string_view b) {
            return BSPF::compareIgnoreCase(a, b) < 0;
          },
          &TypeEntry::first);
        if(it != ourExtensions.end() && BSPF::compareIgnoreCase(it->first, e) == 0)
        {
          ext = e;
          return true;
        }
      }
      return false;
    }

    /**
      Convenience overloads for different parameter types.
     */
    static bool isValidRomName(const FSNode& name, string& ext) {
      return isValidRomName(name.getPath(), ext);
    }
    static bool isValidRomName(const FSNode& name) {
      string ext;  // extension not used
      return isValidRomName(name.getPath(), ext);
    }
    static bool isValidRomName(string_view name) {
      string ext;  // extension not used
      return isValidRomName(name, ext);
    }

    // Output operator
    friend std::ostream& operator<<(std::ostream& os, const Bankswitch::Type& t) {
      return os << typeToName(t);
    }

  private:
    // Key-value pair used in both sorted lookup tables
    using TypeEntry = std::pair<string_view, Bankswitch::Type>;

    // Comparator used with std::lower_bound on sorted TypeEntry arrays
    static constexpr auto entryCmp = [](const TypeEntry& a, const TypeEntry& b) {
      return BSPF::compareIgnoreCase(a.first, b.first) < 0;
    };

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Extension table — sorted case-insensitively for binary search.
    // Precondition: entries below MUST remain in case-insensitive sorted order.
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    static constexpr std::array<TypeEntry, 112> ourExtensions = {{
      { "03E"   , Bankswitch::Type::_03E0   },
      { "03E0"  , Bankswitch::Type::_03E0   },
      { "084"   , Bankswitch::Type::_0840   },
      { "0840"  , Bankswitch::Type::_0840   },
      { "0FA"   , Bankswitch::Type::_0FA0   },
      { "0FA0"  , Bankswitch::Type::_0FA0   },
      { "128"   , Bankswitch::Type::_128IN1 },
      { "128N1" , Bankswitch::Type::_128IN1 },
      { "16N"   , Bankswitch::Type::_16IN1  },
      { "16N1"  , Bankswitch::Type::_16IN1  },
      { "2K"    , Bankswitch::Type::_2K     },
      { "2N1"   , Bankswitch::Type::_2IN1   },
      { "32N"   , Bankswitch::Type::_32IN1  },
      { "32N1"  , Bankswitch::Type::_32IN1  },
      { "3E"    , Bankswitch::Type::_3E     },
      { "3E+"   , Bankswitch::Type::_3EP    },
      { "3EP"   , Bankswitch::Type::_3EP    },
      { "3EX"   , Bankswitch::Type::_3EX    },
      { "3F"    , Bankswitch::Type::_3F     },
      { "4A5"   , Bankswitch::Type::_4A50   },
      { "4A50"  , Bankswitch::Type::_4A50   },
      { "4K"    , Bankswitch::Type::_4K     },
      { "4KS"   , Bankswitch::Type::_4KSC   },
      { "4KSC"  , Bankswitch::Type::_4KSC   },
      { "4N1"   , Bankswitch::Type::_4IN1   },
      { "64N"   , Bankswitch::Type::_64IN1  },
      { "64N1"  , Bankswitch::Type::_64IN1  },
      { "8N1"   , Bankswitch::Type::_8IN1   },
      { "a26"   , Bankswitch::Type::AUTO    },
      { "AR"    , Bankswitch::Type::AR      },
      { "BF"    , Bankswitch::Type::BF      },
      { "BFS"   , Bankswitch::Type::BFSC    },
      { "BFSC"  , Bankswitch::Type::BFSC    },
      { "bin"   , Bankswitch::Type::AUTO    },
      { "BUS"   , Bankswitch::Type::BUS     },
      { "CDF"   , Bankswitch::Type::CDF     },
      { "CM"    , Bankswitch::Type::CM      },
      { "CTY"   , Bankswitch::Type::CTY     },
      { "cu"    , Bankswitch::Type::AUTO    },
      { "CV"    , Bankswitch::Type::CV      },
      { "DF"    , Bankswitch::Type::DF      },
      { "DFS"   , Bankswitch::Type::DFSC    },
      { "DFSC"  , Bankswitch::Type::DFSC    },
      { "DPC"   , Bankswitch::Type::DPC     },
      { "DPP"   , Bankswitch::Type::DPCP    },
      { "DPCP"  , Bankswitch::Type::DPCP    },
      { "E0"    , Bankswitch::Type::E0      },
      { "E7"    , Bankswitch::Type::E7      },
      { "E78"   , Bankswitch::Type::E7      },
      { "E78K"  , Bankswitch::Type::E7      },
      { "EF"    , Bankswitch::Type::EF      },
      { "EFF"   , Bankswitch::Type::EFF     },
      { "EFS"   , Bankswitch::Type::EFSC    },
      { "EFSC"  , Bankswitch::Type::EFSC    },
      { "ELF"   , Bankswitch::Type::ELF     },
      { "F0"    , Bankswitch::Type::F0      },
      { "F4"    , Bankswitch::Type::F4      },
      { "F4S"   , Bankswitch::Type::F4SC    },
      { "F4SC"  , Bankswitch::Type::F4SC    },
      { "F6"    , Bankswitch::Type::F6      },
      { "F6S"   , Bankswitch::Type::F6SC    },
      { "F6SC"  , Bankswitch::Type::F6SC    },
      { "F8"    , Bankswitch::Type::F8      },
      { "F8S"   , Bankswitch::Type::F8SC    },
      { "F8SC"  , Bankswitch::Type::F8SC    },
      { "FA"    , Bankswitch::Type::FA      },
      { "FA2"   , Bankswitch::Type::FA2     },
      { "FC"    , Bankswitch::Type::FC      },
      { "FE"    , Bankswitch::Type::FE      },
      { "GL"    , Bankswitch::Type::GL      },
      { "JAN"   , Bankswitch::Type::JANE    },
      { "JANE"  , Bankswitch::Type::JANE    },
      { "MDM"   , Bankswitch::Type::MDM     },
      { "MVC"   , Bankswitch::Type::MVC     },
      { "rom"   , Bankswitch::Type::AUTO    },
      { "SB"    , Bankswitch::Type::SB      },
      { "TVB"   , Bankswitch::Type::TVBOY   },
      { "TVBOY" , Bankswitch::Type::TVBOY   },
      { "UA"    , Bankswitch::Type::UA      },
      { "UASW"  , Bankswitch::Type::UASW    },
      { "WD"    , Bankswitch::Type::WD      },
      { "WDSW"  , Bankswitch::Type::WDSW    },
      { "WF8"   , Bankswitch::Type::WF8     },
      { "X07"   , Bankswitch::Type::X07     },
    #ifdef ZIP_SUPPORT
      { "zip"   , Bankswitch::Type::AUTO    }
    #endif
    }};

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Name-to-type table — sorted case-insensitively for binary search.
    // Precondition: entries below MUST remain in case-insensitive sorted order.
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    static constexpr std::array<TypeEntry, 61> ourNameToTypes = {{
      { "03E0"    , Bankswitch::Type::_03E0   },
      { "0840"    , Bankswitch::Type::_0840   },
      { "0FA0"    , Bankswitch::Type::_0FA0   },
      { "128IN1"  , Bankswitch::Type::_128IN1 },
      { "16IN1"   , Bankswitch::Type::_16IN1  },
      { "2IN1"    , Bankswitch::Type::_2IN1   },
      { "2K"      , Bankswitch::Type::_2K     },
      { "32IN1"   , Bankswitch::Type::_32IN1  },
      { "3E"      , Bankswitch::Type::_3E     },
      { "3E+"     , Bankswitch::Type::_3EP    },
      { "3EX"     , Bankswitch::Type::_3EX    },
      { "3F"      , Bankswitch::Type::_3F     },
      { "4A50"    , Bankswitch::Type::_4A50   },
      { "4IN1"    , Bankswitch::Type::_4IN1   },
      { "4K"      , Bankswitch::Type::_4K     },
      { "4KSC"    , Bankswitch::Type::_4KSC   },
      { "64IN1"   , Bankswitch::Type::_64IN1  },
      { "8IN1"    , Bankswitch::Type::_8IN1   },
      { "AR"      , Bankswitch::Type::AR      },
      { "AUTO"    , Bankswitch::Type::AUTO    },
      { "BF"      , Bankswitch::Type::BF      },
      { "BFSC"    , Bankswitch::Type::BFSC    },
      { "BUS"     , Bankswitch::Type::BUS     },
      { "CDF"     , Bankswitch::Type::CDF     },
      { "CM"      , Bankswitch::Type::CM      },
      { "CTY"     , Bankswitch::Type::CTY     },
      { "CV"      , Bankswitch::Type::CV      },
      { "DF"      , Bankswitch::Type::DF      },
      { "DFSC"    , Bankswitch::Type::DFSC    },
      { "DPC"     , Bankswitch::Type::DPC     },
      { "DPC+"    , Bankswitch::Type::DPCP    },
      { "E0"      , Bankswitch::Type::E0      },
      { "E7"      , Bankswitch::Type::E7      },
      { "EF"      , Bankswitch::Type::EF      },
      { "EFF"     , Bankswitch::Type::EFF     },
      { "EFSC"    , Bankswitch::Type::EFSC    },
      { "ELF"     , Bankswitch::Type::ELF     },
      { "F0"      , Bankswitch::Type::F0      },
      { "F4"      , Bankswitch::Type::F4      },
      { "F4SC"    , Bankswitch::Type::F4SC    },
      { "F6"      , Bankswitch::Type::F6      },
      { "F6SC"    , Bankswitch::Type::F6SC    },
      { "F8"      , Bankswitch::Type::F8      },
      { "F8SC"    , Bankswitch::Type::F8SC    },
      { "FA"      , Bankswitch::Type::FA      },
      { "FA2"     , Bankswitch::Type::FA2     },
      { "FC"      , Bankswitch::Type::FC      },
      { "FE"      , Bankswitch::Type::FE      },
      { "GL"      , Bankswitch::Type::GL      },
      { "JANE"    , Bankswitch::Type::JANE    },
      { "MDM"     , Bankswitch::Type::MDM     },
      { "MVC"     , Bankswitch::Type::MVC     },
      { "SB"      , Bankswitch::Type::SB      },
      { "TVBOY"   , Bankswitch::Type::TVBOY   },
      { "UA"      , Bankswitch::Type::UA      },
      { "UASW"    , Bankswitch::Type::UASW    },
      { "WD"      , Bankswitch::Type::WD      },
      { "WDSW"    , Bankswitch::Type::WDSW    },
      { "WF8"     , Bankswitch::Type::WF8     },
      { "X07"     , Bankswitch::Type::X07     },
    #ifdef CUSTOM_ARM
      { "CUSTOM"  , Bankswitch::Type::CUSTOM  }
    #endif
    }};

  private:
    // Following constructors and assignment operators not supported
    Bankswitch() = delete;
    ~Bankswitch() = delete;
    Bankswitch(const Bankswitch&) = delete;
    Bankswitch(Bankswitch&&) = delete;
    Bankswitch& operator=(const Bankswitch&) = delete;
    Bankswitch& operator=(Bankswitch&&) = delete;
};

#endif
