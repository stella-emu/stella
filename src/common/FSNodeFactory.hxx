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

#ifndef FSNODE_FACTORY_HXX
#define FSNODE_FACTORY_HXX

class AbstractFSNode;

#include <cassert>

#ifdef ZIP_SUPPORT
  #include "FSNodeZIP.hxx"
#endif
#if defined(BSPF_UNIX) || defined(BSPF_MACOS)
  #include "FSNodePOSIX.hxx"
#elif defined(BSPF_WINDOWS)
  #include "FSNodeWINDOWS.hxx"
#elif defined(__LIB_RETRO__)
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
    static unique_ptr<AbstractFSNode> create(string_view path, Type type)
    {
      switch(type)
      {
        case Type::SYSTEM:
        #if defined(BSPF_UNIX) || defined(BSPF_MACOS)
          return std::make_unique<FSNodePOSIX>(path);
        #elif defined(BSPF_WINDOWS)
          return std::make_unique<FSNodeWINDOWS>(path);
        #elif defined(__LIB_RETRO__)
          return std::make_unique<FSNodeLIBRETRO>(path);
        #endif
        case Type::ZIP:
        #ifdef ZIP_SUPPORT
          return std::make_unique<FSNodeZIP>(path);
        #else
          throw std::runtime_error("ZIP support not compiled in");
        #endif
        default:
          assert(false);  // all Type values handled above
      }
      return nullptr;  // satisfy compiler
    }
};

#endif
