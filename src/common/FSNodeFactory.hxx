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

#ifndef FS_NODE_FACTORY_HXX
#define FS_NODE_FACTORY_HXX

class AbstractFSNode;

#ifdef ZIP_SUPPORT
  #include "FSNodeZIP.hxx"
#endif
#if defined(BSPF_UNIX) || defined(BSPF_MACOS)
  #include "FSNodePOSIX.hxx"
#elifdef BSPF_WINDOWS
  #include "FSNodeWINDOWS.hxx"
#elifdef __LIB_RETRO__
  #include "FSNodeLIBRETRO.hxx"
#else
  #error Unsupported platform in FSNodeFactory!
#endif

/**
  This class deals with creating the different FSNode implementations.

  @author  Stephen Anthony
*/
class FSNodeFactory
{
  public:
    FSNodeFactory() = delete;

    enum class Type: uInt8 { SYSTEM, ZIP };

  public:
    static AbstractFSNodePtr create(string_view path, Type type)
    {
      switch(type)
      {
        case Type::SYSTEM:
        #if defined(BSPF_UNIX) || defined(BSPF_MACOS)
          return std::make_shared<FSNodePOSIX>(path);
        #elifdef BSPF_WINDOWS
          return std::make_shared<FSNodeWINDOWS>(path);
        #elifdef __LIB_RETRO__
          return std::make_shared<FSNodeLIBRETRO>(path);
        #endif
        case Type::ZIP:
        #ifdef ZIP_SUPPORT
          return std::make_shared<FSNodeZIP>(path);
        #else
          throw std::runtime_error("ZIP support not compiled in");
        #endif
        default:
          std::unreachable();  // all Type values handled above
      }
    }
};

#endif  // FS_NODE_FACTORY_HXX
