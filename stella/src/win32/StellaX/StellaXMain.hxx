//============================================================================
//
//   SSSS    tt          lll  lll          XX     XX
//  SS  SS   tt           ll   ll           XX   XX
//  SS     tttttt  eeee   ll   ll   aaaa     XX XX
//   SSSS    tt   ee  ee  ll   ll      aa     XXX
//      SS   tt   eeeeee  ll   ll   aaaaa    XX XX
//  SS  SS   tt   ee      ll   ll  aa  aa   XX   XX
//   SSSS     ttt  eeeee llll llll  aaaaa  XX     XX
//
// Copyright (c) 1995-2000 by Jeff Miller
// Copyright (c) 2004 by Stephen Anthony
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: StellaXMain.hxx,v 1.4 2004-07-15 03:03:27 stephena Exp $
//============================================================================

#ifndef STELLAX_MAIN_HXX
#define STELLAX_MAIN_HXX

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
