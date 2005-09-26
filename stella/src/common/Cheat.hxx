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
// $Id: Cheat.hxx,v 1.3 2005-09-26 19:10:37 stephena Exp $
//============================================================================

#ifndef CHEAT_HXX
#define CHEAT_HXX

#include "bspf.hxx"
#include "OSystem.hxx"

class Cheat
{
  public:
    Cheat() { }
    virtual ~Cheat() { }

    static Cheat *parse(OSystem *osystem, string code);
    static uInt16 unhex(string hex);

    virtual bool enabled() = 0;
    virtual bool enable() = 0;
    virtual bool disable() = 0;

  protected:
    //Cheat(string code);
};

#endif
