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
// $Id: GlobalData.hxx,v 1.3 2004-07-15 03:03:27 stephena Exp $
//============================================================================

#ifndef GLOBAL_DATA_HXX
#define GLOBAL_DATA_HXX

#include "pch.hxx"
#include "bspf.hxx"

class CConfigPage;
class Settings;

class CGlobalData
{
  friend CConfigPage;

  public:
    CGlobalData( HINSTANCE hInstance );
    ~CGlobalData( void );

    HINSTANCE ModuleInstance( void ) const
    {
      return myInstance;
    }

    Settings& settings( void )
    {
      return *mySettings;
    }

  private:
    Settings* mySettings;

    HINSTANCE myInstance;

    CGlobalData( const CGlobalData& );     // no implementation
    void operator=( const CGlobalData& );  // no implementation
};

#endif
