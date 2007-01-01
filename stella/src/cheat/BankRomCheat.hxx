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
// Copyright (c) 1995-2007 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: BankRomCheat.hxx,v 1.3 2007-01-01 18:04:39 stephena Exp $
//============================================================================

#ifndef BANK_ROM_CHEAT_HXX
#define BANK_ROM_CHEAT_HXX

#include "Cheat.hxx"

class BankRomCheat : public Cheat
{
  public:
    BankRomCheat(OSystem* os, const string& name, const string& code);
    ~BankRomCheat();

    virtual bool enable();
    virtual bool disable();

    virtual void evaluate();

  private:
    uInt8  savedRom[16];
    uInt16 address;
    uInt8  value;
    uInt8  count;
    int    bank;
};

#endif
