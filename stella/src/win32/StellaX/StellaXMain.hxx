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
// Copyright (c) 1995-2000 by Jeff Miller
// Copyright (c) 2004 by Stephen Anthony
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: StellaXMain.hxx,v 1.3 2004-07-10 22:25:58 stephena Exp $
//============================================================================ 

#ifndef STELLAX_H
#define STELLAX_H

#include "bspf.hxx"

class CGlobalData;

class CStellaXMain
{
  public:
    CStellaXMain();
    ~CStellaXMain();

    void PlayROM( string& romfile, CGlobalData& globaldata );

  private:
    CStellaXMain( const CStellaXMain& );    // no implementation
    void operator=( const CStellaXMain& );  // no implementation
};

#endif
