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
// Copyright (c) 1995-2012 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

/* Based on nes_ntsc 0.2.2. http://www.slack.net/~ant/ */

#ifdef DISPLAY_TV

#include "atari_ntsc.h"

/* Copyright (C) 2006-2007 Shay Green. This module is free software; you
can redistribute it and/or modify it under the terms of the GNU Lesser
General Public License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version. This
module is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
details. You should have received a copy of the GNU Lesser General Public
License along with this module; if not, write to the Free Software Foundation,
Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA */

/* Atari change: removal and addition of structure fields.
   Values of resolution and sharpness adjusted to make NTSC artifacts look better. */
atari_ntsc_setup_t const atari_ntsc_monochrome = { 0, -1, 0, 0, -.3,  .3,  .2, -.2, -.2, -1, 0, 0, 0, 0. };
atari_ntsc_setup_t const atari_ntsc_composite  = { 0,  0, 0, 0, -.5,  .3, -.1,   0,   0,  0, 0, 0, 0, 0. };
atari_ntsc_setup_t const atari_ntsc_svideo     = { 0,  0, 0, 0, -.3,  .3,  .2,  -1,  -1,  0, 0, 0, 0, 0. };
atari_ntsc_setup_t const atari_ntsc_rgb        = { 0,  0, 0, 0, -.3,  .3,  .7,  -1,  -1, -1, 0, 0, 0, 0. };

#define alignment_count 4
#define burst_count     1
#define rescale_in      8
#define rescale_out     7

#define artifacts_mid   1.0f
#define fringing_mid    1.0f
/* Atari change: default palette is already at correct hue.
#define std_decoder_hue -15 */
#define std_decoder_hue 0

/* Atari change: only one palette - remove base_palete field. */
#define STD_HUE_CONDITION( setup ) !(setup->palette)

#include "atari_ntsc_impl.h"

/* Atari change: adapted to 4/7 pixel ratio. */
/* 4 input pixels -> 8 composite samples */
pixel_info_t const atari_ntsc_pixels [alignment_count] = {
  { PIXEL_OFFSET( -6, -6 ), { 0, 0, 1, 1 } },
  { PIXEL_OFFSET( -4, -4 ), { 0, 0, 1, 1 } },
  { PIXEL_OFFSET( -2, -2 ), { 0, 0, 1, 1 } },
  { PIXEL_OFFSET(  0,  0 ), { 0, 0, 1, 1 } },
};

/* Atari change: no alternating burst phases - removed merge_kernel_fields function. */

static void correct_errors( atari_ntsc_rgb_t color, atari_ntsc_rgb_t* out )
{
  int n;
  for ( n = burst_count; n; --n )
  {
    unsigned i;
    for ( i = 0; i < rgb_kernel_size / 2; i++ )
    {
      /* Atari change: adapted to 4/7 pixel ratio */
      atari_ntsc_rgb_t error = color -
          out [i    ] - out [(i+12)%14+14] - out [(i+10)%14+28] - out[(i+8)%14+42] -
          out [i + 7] - out [i + 5    +14] - out [i + 3    +28] - out [ i+1    +42];
      DISTRIBUTE_ERROR( i+1+42, i+3+28, i+5+14, i+7 );
    }
    out += alignment_count * rgb_kernel_size;
  }
}

void atari_ntsc_init( atari_ntsc_t* ntsc, atari_ntsc_setup_t const* setup )
{
  /* Atari change: no alternating burst phases - remove merge_fields variable. */
  int entry;
  init_t impl;
  /* Atari change: NES palette generation and reading removed.
     Atari palette generation is located in colours_ntsc.c, and colours are read
     from setup->yiq_palette. */

  if ( !setup )
    setup = &atari_ntsc_composite;

  init( &impl, setup );
  
  /* Atari change: no alternating burst phases - remove code for merge_fields. */

  for ( entry = 0; entry < atari_ntsc_palette_size; entry++ )
  {
    /* Atari change: Instead of palette generation, load colours
       from setup->yiq_palette. */
    double y;
    double i;
    double q;

    {
      double *yiq_ptr = setup->yiq_palette + 3 * entry;
      y = *yiq_ptr++;
      i = *yiq_ptr++;
      q = *yiq_ptr++;
    }

    i *= rgb_unit;
    q *= rgb_unit;
    y *= rgb_unit;
    y += rgb_offset;

    /* Generate kernel */
    {
      int r, g, b = YIQ_TO_RGB( y, i, q, impl.to_rgb, int, r, g );
      /* blue tends to overflow, so clamp it */
      atari_ntsc_rgb_t rgb = PACK_RGB( r, g, (b < 0x3E0 ? b: 0x3E0) );
      
      if ( setup->palette_out )
        RGB_PALETTE_OUT( rgb, &setup->palette_out [entry * 3] );
      
      if ( ntsc )
      {
        atari_ntsc_rgb_t* kernel = ntsc->table [entry];
        gen_kernel( &impl, y, i, q, kernel );
        /* Atari change: no alternating burst phases - remove code for merge_fields. */
        correct_errors( rgb, kernel );
      }
    }
  }
}

#ifndef ATARI_NTSC_NO_BLITTERS

/* Atari change: no alternating burst phases - remove burst_phase parameter.
   Also removed the atari_ntsc_blit function and added specific blitters for various
   pixel formats. */

#include <limits.h>

#if USHRT_MAX == 0xFFFF
  typedef unsigned short atari_ntsc_out16_t;
#else
  #error "Need 16-bit int type"
#endif

#if UINT_MAX == 0xFFFFFFFF
  typedef unsigned int  atari_ntsc_out32_t;
#elif ULONG_MAX == 0xFFFFFFFF
  typedef unsigned long atari_ntsc_out32_t;
#else
  #error "Need 32-bit int type"
#endif

void atari_ntsc_blit_rgb16( atari_ntsc_t const* ntsc, ATARI_NTSC_IN_T const* input, long in_row_width,
    int in_width, int in_height, void* rgb_out, long out_pitch )
{
  int chunk_count = (in_width - 1) / atari_ntsc_in_chunk;
  for ( ; in_height; --in_height )
  {
    ATARI_NTSC_IN_T const* line_in = input;
    /* Atari change: no alternating burst phases - remove burst_phase parameter; adjust to 4/7 pixel ratio. */
    ATARI_NTSC_BEGIN_ROW( ntsc,
        atari_ntsc_black, atari_ntsc_black, atari_ntsc_black, ATARI_NTSC_ADJ_IN( *line_in ) );
    atari_ntsc_out16_t* restrict line_out = (atari_ntsc_out16_t*) rgb_out;
    int n;
    ++line_in;

    for ( n = chunk_count; n; --n )
    {
      /* order of input and output pixels must not be altered */
      ATARI_NTSC_COLOR_IN( 0, ATARI_NTSC_ADJ_IN( line_in [0] ) );
      ATARI_NTSC_RGB_OUT( 0, line_out [0], ATARI_NTSC_RGB_FORMAT_RGB16 );
      ATARI_NTSC_RGB_OUT( 1, line_out [1], ATARI_NTSC_RGB_FORMAT_RGB16 );

      ATARI_NTSC_COLOR_IN( 1, ATARI_NTSC_ADJ_IN( line_in [1] ) );
      ATARI_NTSC_RGB_OUT( 2, line_out [2], ATARI_NTSC_RGB_FORMAT_RGB16 );
      ATARI_NTSC_RGB_OUT( 3, line_out [3], ATARI_NTSC_RGB_FORMAT_RGB16 );

      ATARI_NTSC_COLOR_IN( 2, ATARI_NTSC_ADJ_IN( line_in [2] ) );
      ATARI_NTSC_RGB_OUT( 4, line_out [4], ATARI_NTSC_RGB_FORMAT_RGB16 );
      ATARI_NTSC_RGB_OUT( 5, line_out [5], ATARI_NTSC_RGB_FORMAT_RGB16 );

      ATARI_NTSC_COLOR_IN( 3, ATARI_NTSC_ADJ_IN( line_in [3] ) );
      ATARI_NTSC_RGB_OUT( 6, line_out [6], ATARI_NTSC_RGB_FORMAT_RGB16 );

      line_in  += 4;
      line_out += 7;
    }

    /* finish final pixels */
    ATARI_NTSC_COLOR_IN( 0, atari_ntsc_black );
    ATARI_NTSC_RGB_OUT( 0, line_out [0], ATARI_NTSC_RGB_FORMAT_RGB16 );
    ATARI_NTSC_RGB_OUT( 1, line_out [1], ATARI_NTSC_RGB_FORMAT_RGB16 );

    ATARI_NTSC_COLOR_IN( 1, atari_ntsc_black );
    ATARI_NTSC_RGB_OUT( 2, line_out [2], ATARI_NTSC_RGB_FORMAT_RGB16 );
    ATARI_NTSC_RGB_OUT( 3, line_out [3], ATARI_NTSC_RGB_FORMAT_RGB16 );

    ATARI_NTSC_COLOR_IN( 2, atari_ntsc_black );
    ATARI_NTSC_RGB_OUT( 4, line_out [4], ATARI_NTSC_RGB_FORMAT_RGB16 );
    ATARI_NTSC_RGB_OUT( 5, line_out [5], ATARI_NTSC_RGB_FORMAT_RGB16 );

    ATARI_NTSC_COLOR_IN( 3, atari_ntsc_black );
    ATARI_NTSC_RGB_OUT( 6, line_out [6], ATARI_NTSC_RGB_FORMAT_RGB16 );

    input += in_row_width;
    rgb_out = (char*) rgb_out + out_pitch;
  }
}

void atari_ntsc_blit_bgr16( atari_ntsc_t const* ntsc, ATARI_NTSC_IN_T const* input, long in_row_width,
    int in_width, int in_height, void* rgb_out, long out_pitch )
{
  int chunk_count = (in_width - 1) / atari_ntsc_in_chunk;
  for ( ; in_height; --in_height )
  {
    ATARI_NTSC_IN_T const* line_in = input;
    /* Atari change: no alternating burst phases - remove burst_phase parameter; adjust to 4/7 pixel ratio. */
    ATARI_NTSC_BEGIN_ROW( ntsc,
        atari_ntsc_black, atari_ntsc_black, atari_ntsc_black, ATARI_NTSC_ADJ_IN( *line_in ) );
    atari_ntsc_out16_t* restrict line_out = (atari_ntsc_out16_t*) rgb_out;
    int n;
    ++line_in;

    for ( n = chunk_count; n; --n )
    {
      /* order of input and output pixels must not be altered */
      ATARI_NTSC_COLOR_IN( 0, ATARI_NTSC_ADJ_IN( line_in [0] ) );
      ATARI_NTSC_RGB_OUT( 0, line_out [0], ATARI_NTSC_RGB_FORMAT_BGR16 );
      ATARI_NTSC_RGB_OUT( 1, line_out [1], ATARI_NTSC_RGB_FORMAT_BGR16 );

      ATARI_NTSC_COLOR_IN( 1, ATARI_NTSC_ADJ_IN( line_in [1] ) );
      ATARI_NTSC_RGB_OUT( 2, line_out [2], ATARI_NTSC_RGB_FORMAT_BGR16 );
      ATARI_NTSC_RGB_OUT( 3, line_out [3], ATARI_NTSC_RGB_FORMAT_BGR16 );

      ATARI_NTSC_COLOR_IN( 2, ATARI_NTSC_ADJ_IN( line_in [2] ) );
      ATARI_NTSC_RGB_OUT( 4, line_out [4], ATARI_NTSC_RGB_FORMAT_BGR16 );
      ATARI_NTSC_RGB_OUT( 5, line_out [5], ATARI_NTSC_RGB_FORMAT_BGR16 );

      ATARI_NTSC_COLOR_IN( 3, ATARI_NTSC_ADJ_IN( line_in [3] ) );
      ATARI_NTSC_RGB_OUT( 6, line_out [6], ATARI_NTSC_RGB_FORMAT_BGR16 );

      line_in  += 4;
      line_out += 7;
    }

    /* finish final pixels */
    ATARI_NTSC_COLOR_IN( 0, atari_ntsc_black );
    ATARI_NTSC_RGB_OUT( 0, line_out [0], ATARI_NTSC_RGB_FORMAT_BGR16 );
    ATARI_NTSC_RGB_OUT( 1, line_out [1], ATARI_NTSC_RGB_FORMAT_BGR16 );

    ATARI_NTSC_COLOR_IN( 1, atari_ntsc_black );
    ATARI_NTSC_RGB_OUT( 2, line_out [2], ATARI_NTSC_RGB_FORMAT_BGR16 );
    ATARI_NTSC_RGB_OUT( 3, line_out [3], ATARI_NTSC_RGB_FORMAT_BGR16 );

    ATARI_NTSC_COLOR_IN( 2, atari_ntsc_black );
    ATARI_NTSC_RGB_OUT( 4, line_out [4], ATARI_NTSC_RGB_FORMAT_BGR16 );
    ATARI_NTSC_RGB_OUT( 5, line_out [5], ATARI_NTSC_RGB_FORMAT_BGR16 );

    ATARI_NTSC_COLOR_IN( 3, atari_ntsc_black );
    ATARI_NTSC_RGB_OUT( 6, line_out [6], ATARI_NTSC_RGB_FORMAT_BGR16 );

    input += in_row_width;
    rgb_out = (char*) rgb_out + out_pitch;
  }
}

void atari_ntsc_blit_argb32( atari_ntsc_t const* ntsc, ATARI_NTSC_IN_T const* input, long in_row_width,
    int in_width, int in_height, void* rgb_out, long out_pitch )
{
  int chunk_count = (in_width - 1) / atari_ntsc_in_chunk;
  for ( ; in_height; --in_height )
  {
    ATARI_NTSC_IN_T const* line_in = input;
    /* Atari change: no alternating burst phases - remove burst_phase parameter; adjust to 4/7 pixel ratio. */
    ATARI_NTSC_BEGIN_ROW( ntsc,
        atari_ntsc_black, atari_ntsc_black, atari_ntsc_black, ATARI_NTSC_ADJ_IN( *line_in ) );
    atari_ntsc_out32_t* restrict line_out = (atari_ntsc_out32_t*) rgb_out;
    int n;
    ++line_in;

    for ( n = chunk_count; n; --n )
    {
      /* order of input and output pixels must not be altered */
      ATARI_NTSC_COLOR_IN( 0, ATARI_NTSC_ADJ_IN( line_in [0] ) );
      ATARI_NTSC_RGB_OUT( 0, line_out [0], ATARI_NTSC_RGB_FORMAT_ARGB32 );
      ATARI_NTSC_RGB_OUT( 1, line_out [1], ATARI_NTSC_RGB_FORMAT_ARGB32 );

      ATARI_NTSC_COLOR_IN( 1, ATARI_NTSC_ADJ_IN( line_in [1] ) );
      ATARI_NTSC_RGB_OUT( 2, line_out [2], ATARI_NTSC_RGB_FORMAT_ARGB32 );
      ATARI_NTSC_RGB_OUT( 3, line_out [3], ATARI_NTSC_RGB_FORMAT_ARGB32 );

      ATARI_NTSC_COLOR_IN( 2, ATARI_NTSC_ADJ_IN( line_in [2] ) );
      ATARI_NTSC_RGB_OUT( 4, line_out [4], ATARI_NTSC_RGB_FORMAT_ARGB32 );
      ATARI_NTSC_RGB_OUT( 5, line_out [5], ATARI_NTSC_RGB_FORMAT_ARGB32 );

      ATARI_NTSC_COLOR_IN( 3, ATARI_NTSC_ADJ_IN( line_in [3] ) );
      ATARI_NTSC_RGB_OUT( 6, line_out [6], ATARI_NTSC_RGB_FORMAT_ARGB32 );

      line_in  += 4;
      line_out += 7;
    }

    /* finish final pixels */
    ATARI_NTSC_COLOR_IN( 0, atari_ntsc_black );
    ATARI_NTSC_RGB_OUT( 0, line_out [0], ATARI_NTSC_RGB_FORMAT_ARGB32 );
    ATARI_NTSC_RGB_OUT( 1, line_out [1], ATARI_NTSC_RGB_FORMAT_ARGB32 );

    ATARI_NTSC_COLOR_IN( 1, atari_ntsc_black );
    ATARI_NTSC_RGB_OUT( 2, line_out [2], ATARI_NTSC_RGB_FORMAT_ARGB32 );
    ATARI_NTSC_RGB_OUT( 3, line_out [3], ATARI_NTSC_RGB_FORMAT_ARGB32 );

    ATARI_NTSC_COLOR_IN( 2, atari_ntsc_black );
    ATARI_NTSC_RGB_OUT( 4, line_out [4], ATARI_NTSC_RGB_FORMAT_ARGB32 );
    ATARI_NTSC_RGB_OUT( 5, line_out [5], ATARI_NTSC_RGB_FORMAT_ARGB32 );

    ATARI_NTSC_COLOR_IN( 3, atari_ntsc_black );
    ATARI_NTSC_RGB_OUT( 6, line_out [6], ATARI_NTSC_RGB_FORMAT_ARGB32 );

    input += in_row_width;
    rgb_out = (char*) rgb_out + out_pitch;
  }
}

void atari_ntsc_blit_bgra32( atari_ntsc_t const* ntsc, ATARI_NTSC_IN_T const* input, long in_row_width,
    int in_width, int in_height, void* rgb_out, long out_pitch )
{
  int chunk_count = (in_width - 1) / atari_ntsc_in_chunk;
  for ( ; in_height; --in_height )
  {
    ATARI_NTSC_IN_T const* line_in = input;
    /* Atari change: no alternating burst phases - remove burst_phase parameter; adjust to 4/7 pixel ratio. */
    ATARI_NTSC_BEGIN_ROW( ntsc,
        atari_ntsc_black, atari_ntsc_black, atari_ntsc_black, ATARI_NTSC_ADJ_IN( *line_in ) );
    atari_ntsc_out32_t* restrict line_out = (atari_ntsc_out32_t*) rgb_out;
    int n;
    ++line_in;

    for ( n = chunk_count; n; --n )
    {
      /* order of input and output pixels must not be altered */
      ATARI_NTSC_COLOR_IN( 0, ATARI_NTSC_ADJ_IN( line_in [0] ) );
      ATARI_NTSC_RGB_OUT( 0, line_out [0], ATARI_NTSC_RGB_FORMAT_BGRA32 );
      ATARI_NTSC_RGB_OUT( 1, line_out [1], ATARI_NTSC_RGB_FORMAT_BGRA32 );

      ATARI_NTSC_COLOR_IN( 1, ATARI_NTSC_ADJ_IN( line_in [1] ) );
      ATARI_NTSC_RGB_OUT( 2, line_out [2], ATARI_NTSC_RGB_FORMAT_BGRA32 );
      ATARI_NTSC_RGB_OUT( 3, line_out [3], ATARI_NTSC_RGB_FORMAT_BGRA32 );

      ATARI_NTSC_COLOR_IN( 2, ATARI_NTSC_ADJ_IN( line_in [2] ) );
      ATARI_NTSC_RGB_OUT( 4, line_out [4], ATARI_NTSC_RGB_FORMAT_BGRA32 );
      ATARI_NTSC_RGB_OUT( 5, line_out [5], ATARI_NTSC_RGB_FORMAT_BGRA32 );

      ATARI_NTSC_COLOR_IN( 3, ATARI_NTSC_ADJ_IN( line_in [3] ) );
      ATARI_NTSC_RGB_OUT( 6, line_out [6], ATARI_NTSC_RGB_FORMAT_BGRA32 );

      line_in  += 4;
      line_out += 7;
    }

    /* finish final pixels */
    ATARI_NTSC_COLOR_IN( 0, atari_ntsc_black );
    ATARI_NTSC_RGB_OUT( 0, line_out [0], ATARI_NTSC_RGB_FORMAT_BGRA32 );
    ATARI_NTSC_RGB_OUT( 1, line_out [1], ATARI_NTSC_RGB_FORMAT_BGRA32 );

    ATARI_NTSC_COLOR_IN( 1, atari_ntsc_black );
    ATARI_NTSC_RGB_OUT( 2, line_out [2], ATARI_NTSC_RGB_FORMAT_BGRA32 );
    ATARI_NTSC_RGB_OUT( 3, line_out [3], ATARI_NTSC_RGB_FORMAT_BGRA32 );

    ATARI_NTSC_COLOR_IN( 2, atari_ntsc_black );
    ATARI_NTSC_RGB_OUT( 4, line_out [4], ATARI_NTSC_RGB_FORMAT_BGRA32 );
    ATARI_NTSC_RGB_OUT( 5, line_out [5], ATARI_NTSC_RGB_FORMAT_BGRA32 );

    ATARI_NTSC_COLOR_IN( 3, atari_ntsc_black );
    ATARI_NTSC_RGB_OUT( 6, line_out [6], ATARI_NTSC_RGB_FORMAT_BGRA32 );

    input += in_row_width;
    rgb_out = (char*) rgb_out + out_pitch;
  }
}

#endif

#endif // DISPLAY_TV
