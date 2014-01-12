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
// Copyright (c) 1995-2014 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

#ifndef FSNODE_FACTORY_HXX
#define FSNODE_FACTORY_HXX

class AbstractFSNode;
#if defined(BSPF_UNIX) || defined(BSPF_MAC_OSX)
  #include "FSNodePOSIX.hxx"
#elif defined(BSPF_WINDOWS)
  #include "FSNodeWINDOWS.hxx"
#else
  #error Unsupported platform in FSNodeFactory!
#endif
#include "FSNodeZIP.hxx"

/**
  This class deals with creating the different FSNode implementations.
  I think you can see why this mess was put into a factory class :)

  @author  Stephen Anthony
*/
class FilesystemNodeFactory
{
  public:
    enum Type { SYSTEM, ZIP };

  public:
    static AbstractFSNode* create(const string& path, Type type)
    {
      switch(type)
      {
        case SYSTEM:
      #if defined(BSPF_UNIX) || defined(BSPF_MAC_OSX)
          return new FilesystemNodePOSIX(path);
      #elif defined(BSPF_WINDOWS)
          return new FilesystemNodeWINDOWS(path);
      #endif
          break;
        case ZIP:
          return new FilesystemNodeZIP(path);
          break;
      }
      return 0;
    }
};

#endif
