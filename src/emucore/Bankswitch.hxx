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

#include <map>

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
      CUSTOM,
    #endif
      NumSchemes,
      NumMulti = _128IN1 - _2IN1 + 1,
    };

    struct SizesType {
      size_t minSize{0};
      size_t maxSize{0};
    };
    static constexpr size_t any_KB{0};

    static constexpr std::array<SizesType, static_cast<uInt8>(Type::NumSchemes)>
      Sizes= {{
        { Bankswitch::any_KB, Bankswitch::any_KB }, // AUTO
        {    8_KB,   8_KB }, // _03E0
        {    8_KB,   8_KB }, // _0840
        {    8_KB,   8_KB }, // _0FA0
        {    4_KB,  64_KB }, // _2IN1
        {    8_KB,  64_KB }, // _4IN1
        {   16_KB,  64_KB }, // _8IN1
        {   32_KB, 128_KB }, // _16IN1
        {   64_KB, 128_KB }, // _32IN1
        {  128_KB, 256_KB }, // _64IN1
        {  256_KB, 512_KB }, // _128IN1
        {    0_KB,   4_KB }, // _2K
        {    8_KB, 512_KB }, // _3E
        {    8_KB, 512_KB }, // _3EX
        {    8_KB,  64_KB }, // _3EP
        {    8_KB, 512_KB }, // _3F
        {   64_KB,  64_KB }, // _4A50
        {    4_KB,   4_KB }, // _4K
        {    4_KB,   4_KB }, // _4KSC
        {    6_KB,  33_KB }, // _AR
        {  256_KB, 256_KB }, // _BF
        {  256_KB, 256_KB }, // _BFSC
        {   32_KB,  32_KB }, // _BUS
        {   32_KB, 512_KB }, // _CDF
        {   16_KB,  16_KB }, // _CM
        {   32_KB,  32_KB }, // _CTY
        {    0_KB,   4_KB }, // _CV
        {  128_KB, 128_KB }, // _DF
        {  128_KB, 128_KB }, // _DFSC
        {   10_KB,  11_KB }, // _DPC
        {   16_KB,  64_KB }, // _DPCP
        {    8_KB,   8_KB }, // _E0
        {    8_KB,  16_KB }, // _E7
        {   64_KB,  64_KB }, // _EF
        {   64_KB,  64_KB }, // _EFF
        {   64_KB,  64_KB }, // _EFSC
        {   Bankswitch::any_KB,  Bankswitch::any_KB }, // _ELF
        {   64_KB,  64_KB }, // _F0
        {   32_KB,  32_KB }, // _F4
        {   32_KB,  32_KB }, // _F4SC
        {   16_KB,  16_KB }, // _F6
        {   16_KB,  16_KB }, // _F6SC
        {    8_KB,   8_KB }, // _F8
        {    8_KB,   8_KB }, // _F8SC
        {   12_KB,  12_KB }, // _FA
        {   24_KB,  32_KB }, // _FA2
        {   32_KB,  32_KB }, // _FC
        {    8_KB,   8_KB }, // _FE
        {    4_KB,   6_KB }, // _GL
        {   16_KB,  16_KB }, // _JANE
        {    8_KB, Bankswitch::any_KB }, // _MDM
        { 1024_KB, Bankswitch::any_KB }, // _MVC
        {  128_KB, 256_KB }, // _SB
        {  512_KB, 512_KB }, // _TVBOY
        {    8_KB,   8_KB }, // _UA
        {    8_KB,   8_KB }, // _UASW
        {    8_KB,   8_KB }, // _WD
        {    8_KB,   8_KB+5 }, // _WDSW
        {    8_KB,   8_KB }, // _WF8
        {   64_KB,  64_KB }, // _X07
      #ifdef CUSTOM_ARM
        { Bankswitch::any_KB, Bankswitch::any_KB }
      #endif
      }};

    // Info about the various bankswitch schemes, useful for displaying
    // in GUI dropdown boxes, etc
    struct Description {
      string_view name;
      string_view desc;
    };
    static constexpr std::array<Description, static_cast<uInt8>(Type::NumSchemes)>
      BSList = {{
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
    // Convert BSType enum to string
    static string typeToName(Bankswitch::Type type) {
      return string{BSList[static_cast<int>(type)].name};
    }

    // Convert string to BSType enum
    static Bankswitch::Type nameToType(string_view name) {
      const auto it = ourNameToTypes.find(name);
      if(it != ourNameToTypes.end())
        return it->second;

      return Bankswitch::Type::AUTO;
    }

    // Convert BSType enum to description string
    static string typeToDesc(Bankswitch::Type type) {
      return string{BSList[static_cast<int>(type)].desc};
    }

    // Determine bankswitch type by filename extension
    // Use 'AUTO' if unknown
    static Bankswitch::Type typeFromExtension(const FSNode& file) {
      const string_view name = file.getPath();
      const auto idx = name.find_last_of('.');
      if(idx != string_view::npos)
      {
        const auto it = ourExtensions.find(name.substr(idx + 1));
        if(it != ourExtensions.end())
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
        const auto it = ourExtensions.find(e);
        if(it != ourExtensions.end())
        {
          ext = e;
          return true;
        }
      }
      return false;
    }

    /**
      Convenience functions for different parameter types.
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
    friend ostream& operator<<(ostream& os, const Bankswitch::Type& t) {
      return os << typeToName(t);
    }

  private:
    struct TypeComparator {
      bool operator() (string_view a, string_view b) const {
        return BSPF::compareIgnoreCase(a, b) < 0;
      }
    };
    using ExtensionMap = const std::map<string_view, Bankswitch::Type,
                                        TypeComparator>;
    inline static const ExtensionMap ourExtensions = {  // NOLINT
      // Normal file extensions that don't actually tell us anything
      // about the bankswitch type to use
      { "a26"   , Bankswitch::Type::AUTO    },
      { "bin"   , Bankswitch::Type::AUTO    },
      { "rom"   , Bankswitch::Type::AUTO    },
    #ifdef ZIP_SUPPORT
      { "zip"   , Bankswitch::Type::AUTO    },
    #endif
      { "cu"    , Bankswitch::Type::AUTO    },

      // All bankswitch types (those that UnoCart and HarmonyCart support have the same name)
      { "03E"   , Bankswitch::Type::_03E0   },
      { "03E0"  , Bankswitch::Type::_03E0   },
      { "084"   , Bankswitch::Type::_0840   },
      { "0840"  , Bankswitch::Type::_0840   },
      { "0FA"   , Bankswitch::Type::_0FA0   },
      { "0FA0"  , Bankswitch::Type::_0FA0   },
      { "2N1"   , Bankswitch::Type::_2IN1   },
      { "4N1"   , Bankswitch::Type::_4IN1   },
      { "8N1"   , Bankswitch::Type::_8IN1   },
      { "16N"   , Bankswitch::Type::_16IN1  },
      { "16N1"  , Bankswitch::Type::_16IN1  },
      { "32N"   , Bankswitch::Type::_32IN1  },
      { "32N1"  , Bankswitch::Type::_32IN1  },
      { "64N"   , Bankswitch::Type::_64IN1  },
      { "64N1"  , Bankswitch::Type::_64IN1  },
      { "128"   , Bankswitch::Type::_128IN1 },
      { "128N1" , Bankswitch::Type::_128IN1 },
      { "2K"    , Bankswitch::Type::_2K     },
      { "3E"    , Bankswitch::Type::_3E     },
      { "3EX"   , Bankswitch::Type::_3EX    },
      { "3EP"   , Bankswitch::Type::_3EP    },
      { "3E+"   , Bankswitch::Type::_3EP    },
      { "3F"    , Bankswitch::Type::_3F     },
      { "4A5"   , Bankswitch::Type::_4A50   },
      { "4A50"  , Bankswitch::Type::_4A50   },
      { "4K"    , Bankswitch::Type::_4K     },
      { "4KS"   , Bankswitch::Type::_4KSC   },
      { "4KSC"  , Bankswitch::Type::_4KSC   },
      { "AR"    , Bankswitch::Type::AR      },
      { "BF"    , Bankswitch::Type::BF      },
      { "BFS"   , Bankswitch::Type::BFSC    },
      { "BFSC"  , Bankswitch::Type::BFSC    },
      { "BUS"   , Bankswitch::Type::BUS     },
      { "CDF"   , Bankswitch::Type::CDF     },
      { "CM"    , Bankswitch::Type::CM      },
      { "CTY"   , Bankswitch::Type::CTY     },
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
      { "SB"    , Bankswitch::Type::SB      },
      { "TVB"   , Bankswitch::Type::TVBOY   },
      { "TVBOY" , Bankswitch::Type::TVBOY   },
      { "UA"    , Bankswitch::Type::UA      },
      { "UASW"  , Bankswitch::Type::UASW    },
      { "WD"    , Bankswitch::Type::WD      },
      { "WDSW"  , Bankswitch::Type::WDSW    },
      { "WF8"   , Bankswitch::Type::WF8     },
      { "X07"   , Bankswitch::Type::X07     }
    };

    using NameToTypeMap = const std::map<string_view, Bankswitch::Type,
                                         TypeComparator>;
    inline static const NameToTypeMap ourNameToTypes = {  // NOLINT
      { "AUTO"    , Bankswitch::Type::AUTO    },
      { "03E0"    , Bankswitch::Type::_03E0   },
      { "0840"    , Bankswitch::Type::_0840   },
      { "0FA0"    , Bankswitch::Type::_0FA0   },
      { "2IN1"    , Bankswitch::Type::_2IN1   },
      { "4IN1"    , Bankswitch::Type::_4IN1   },
      { "8IN1"    , Bankswitch::Type::_8IN1   },
      { "16IN1"   , Bankswitch::Type::_16IN1  },
      { "32IN1"   , Bankswitch::Type::_32IN1  },
      { "64IN1"   , Bankswitch::Type::_64IN1  },
      { "128IN1"  , Bankswitch::Type::_128IN1 },
      { "2K"      , Bankswitch::Type::_2K     },
      { "3E"      , Bankswitch::Type::_3E     },
      { "3E+"     , Bankswitch::Type::_3EP    },
      { "3EX"     , Bankswitch::Type::_3EX    },
      { "3F"      , Bankswitch::Type::_3F     },
      { "4A50"    , Bankswitch::Type::_4A50   },
      { "4K"      , Bankswitch::Type::_4K     },
      { "4KSC"    , Bankswitch::Type::_4KSC   },
      { "AR"      , Bankswitch::Type::AR      },
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
      { "X07"     , Bankswitch::Type::X07     }
    };

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
