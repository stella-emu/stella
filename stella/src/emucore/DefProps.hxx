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
// Copyright (c) 1995-1998 by Bradford W. Mott
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: DefProps.hxx,v 1.1.1.1 2001-12-27 19:54:21 bwmott Exp $
//============================================================================

#ifndef DEFAULTPROPERTIES_HXX
#define DEFAULTPROPERTIES_HXX

/**
  Get the default properties file as an array of pointers to null-terminated 
  character arrays.  The last entry in the array is the null pointer.

  @return The default properties file
*/
const char** defaultPropertiesFile();

#endif

