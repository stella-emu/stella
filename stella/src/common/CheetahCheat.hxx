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
// Copyright (c) 1995-2005 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: CheetahCheat.hxx,v 1.3 2005-09-26 19:10:37 stephena Exp $
//============================================================================

#ifndef CHEETAH_CHEAT_HXX
#define CHEETAH_CHEAT_HXX

#include "OSystem.hxx"
#include "Cheat.hxx"

class CheetahCheat : public Cheat
{
  public:
    CheetahCheat(OSystem *os, string code);
    ~CheetahCheat();

    virtual bool enabled();
    virtual bool enable();
    virtual bool disable();

  private:
    OSystem* myOSystem;

    bool   _enabled;
    uInt8  savedRom[16];
    uInt16 address;
    uInt8  value;
    uInt8  count;
};

#endif
