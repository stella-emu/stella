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
// $Id: scaler.cxx,v 1.1 2006-10-22 18:58:46 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2001-2006 The ScummVM project
//============================================================================

#include "intern.hxx"
#include "scalebit.hxx"
#include "scaler.hxx"


int gBitFormat = 565;

// RGB-to-YUV lookup table
extern "C" {

#ifdef USE_NASM
// NOTE: if your compiler uses different mangled names, add another
//       condition here

#if !defined(_WIN32) && !defined(MACOSX)
	#define RGBtoYUV _RGBtoYUV
	#define LUT16to32 _LUT16to32
#endif

#endif

// FIXME/TODO: The following two tables suck up 512 KB. This is bad.
// In addition we never free them...
//
// Note: a memory lookup table is *not* necessarily faster than computing
// these things on the fly, because of its size. Both tables together, plus
// the code, plus the input/output GFX data, won't fit in the cache on many
// systems, so main memory has to be accessed, which is about the worst thing
// that can happen to code which tries to be fast...
//
// So we should think about ways to get these smaller / removed. The LUT16to32
// is only used by the HQX asm right now; maybe somebody can modify the code
// there to work w/o it (and do some benchmarking, too?). To do that, just
// do the conversion on the fly, or even do w/o it (as the C++ code manages to),
// by making different versions of the code based on gBitFormat (or by writing
// bit masks into registers which are computed based on gBitFormat).
//
// RGBtoYUV is also used by the C(++) version of the HQX code. Maybe we can
// use the same technique which is employed by our MPEG code to reduce the
// size of the lookup tables at the cost of some additional computations? That
// might actually result in a speedup, too, if done right (and the code code
// might actually be suitable for AltiVec/MMX/SSE speedup).
//
// Of course, the above is largely a conjecture, and the actual speed
// differences are likely to vary a lot between different architectures and
// CPUs.
uint32 *RGBtoYUV = 0;
uint32 *LUT16to32 = 0;
}

template<class T>
void InitLUT() {
	int r, g, b;
	int Y, u, v;
	
	assert(T::kBytesPerPixel == 2);

	// Allocate the YUV/LUT buffers on the fly if needed.	
	if (RGBtoYUV == 0)
		RGBtoYUV = (uint32 *)malloc(65536 * sizeof(uint32));
	if (LUT16to32 == 0)
		LUT16to32 = (uint32 *)malloc(65536 * sizeof(uint32));

	for (int color = 0; color < 65536; ++color) {
		r = ((color & T::kRedMask) >> T::kRedShift) << (8 - T::kRedBits);
		g = ((color & T::kGreenMask) >> T::kGreenShift) << (8 - T::kGreenBits);
		b = ((color & T::kBlueMask) >> T::kBlueShift) << (8 - T::kBlueBits);
		LUT16to32[color] = (r << 16) | (g << 8) | b;

		Y = (r + g + b) >> 2;
		u = 128 + ((r - b) >> 2);
		v = 128 + ((-r + 2 * g - b) >> 3);
		RGBtoYUV[color] = (Y << 16) | (u << 8) | v;
	}
}

/** Always use 16-bit, since OpenGL will always use it */
void InitScalers() {
	InitLUT<ColorMasks<565> >();
}

void FreeScalers() {
	if (RGBtoYUV != 0) {
		free(RGBtoYUV);
		RGBtoYUV = 0;
	}
	if (LUT16to32 != 0)
	{
		free(LUT16to32);
		LUT16to32 = 0;
	}
}

/**
 * The Scale2x filter, also known as AdvMame2x.
 * See also http://scale2x.sourceforge.net
 */
void AdvMame2x(const uint8 *srcPtr, uint32 srcPitch, uint8 *dstPtr, uint32 dstPitch,
							 int width, int height) {
	scale(2, dstPtr, dstPitch, srcPtr - srcPitch, srcPitch, 2, width, height);
}

/**
 * The Scale3x filter, also known as AdvMame3x.
 * See also http://scale2x.sourceforge.net
 */
void AdvMame3x(const uint8 *srcPtr, uint32 srcPitch, uint8 *dstPtr, uint32 dstPitch,
							 int width, int height) {
	scale(3, dstPtr, dstPitch, srcPtr - srcPitch, srcPitch, 2, width, height);
}
