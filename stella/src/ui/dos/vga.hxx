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
// Copyright (c) 1995-2003 by Bradford W. Mott
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: vga.hxx,v 1.1 2003-02-17 05:17:42 bwmott Exp $
//============================================================================

#ifndef VGA_HXX
#define VGA_HXX

#include "bspf.hxx"

#define VGA_320_200_60HZ   1
#define VGA_320_200_70HZ   2
#define VGA_320_240_60HZ   3

/**
  Change the graphics mode to the specified mode.
*/
extern bool VgaSetMode(int mode);

#endif

