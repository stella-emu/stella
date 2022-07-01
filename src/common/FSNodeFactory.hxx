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
// Copyright (c) 1995-2022 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#ifndef FSNODE_FACTORY_HXX
#define FSNODE_FACTORY_HXX

class AbstractFSNode;

#if defined(ZIP_SUPPORT)
  #include "FSNodeZIP.hxx"
#endif
#if defined(__LIB_RETRO__)
  #include "FSNodeLIBRETRO.hxx"
#else
  #include "FSNodeREGULAR.hxx"
#endif

/**
  This class deals with creating the different FSNode implementations.

  @author  Stephen Anthony
*/
class FSNodeFactory
{
  public:
    enum class Type { REGULAR, ZIP };

  public:
    static unique_ptr<AbstractFSNode> create(const string& path, Type type)
    {
      switch(type)
      {
        case Type::REGULAR:
        #if defined(__LIB_RETRO__)
          return make_unique<FSNodeLIBRETRO>(path);
        #else
          return make_unique<FSNodeREGULAR>(path);
        #endif
          break;
        case Type::ZIP:
        #if defined(ZIP_SUPPORT)
          return make_unique<FSNodeZIP>(path);
        #endif
          break;
      }
      return nullptr;
    }

  private:
    // Following constructors and assignment operators not supported
    FSNodeFactory() = delete;
    FSNodeFactory(const FSNodeFactory&) = delete;
    FSNodeFactory(FSNodeFactory&&) = delete;
    FSNodeFactory& operator=(const FSNodeFactory&) = delete;
    FSNodeFactory& operator=(FSNodeFactory&&) = delete;
};

#endif
