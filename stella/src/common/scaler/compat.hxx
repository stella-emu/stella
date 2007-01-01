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
// $Id: compat.hxx,v 1.3 2007-01-01 18:04:40 stephena Exp $
//
//============================================================================

/**
  Compatibility layer to convert ScummVM datatypes to Stella format.
 */
#ifndef COMPAT_TYPE_HXX
#define COMPAT_TYPE_HXX

#include "bspf.hxx"

typedef uInt8 byte;
typedef uInt32 uint;
typedef uInt8 uint8;
typedef uInt16 uint16;
typedef uInt32 uint32;
typedef Int8 int8;
typedef Int16 int16;
typedef Int32 int32;

//
// GCC specific stuff
//
#if defined(__GNUC__)
	#define GCC_PACK __attribute__((packed))
	#define NORETURN __attribute__((__noreturn__)) 
	#define GCC_PRINTF(x,y) __attribute__((format(printf, x, y)))
#else
	#define GCC_PACK
	#define GCC_PRINTF(x,y)
#endif


#endif
