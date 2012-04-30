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

#include "atari_ntsc.h"

/* Copyright (C) 2006-2009 Shay Green. This module is free software; you
   can redistribute it and/or modify it under the terms of the GNU Lesser
   General Public License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version. This
   module is distributed in the hope that it will be useful, but WITHOUT ANY
   WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
   FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
   details. You should have received a copy of the GNU Lesser General Public
   License along with this module; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
*/

/* Atari change: removal and addition of structure fields.
   Values of resolution and sharpness adjusted to make NTSC artifacts look better. */
atari_ntsc_setup_t const atari_ntsc_composite  =
  {  0.0,  0.0, 0.0, 0.0 , -0.5, .3, -0.1 ,  0.0,  0.0,  0.0, 0, 0, 0, 0. };
atari_ntsc_setup_t const atari_ntsc_svideo     =
  {  0.0,  0.0, 0.0, 0.0 , -0.3, .3,  0.2 , -1.0, -1.0,  0.0, 0, 0, 0, 0. };
atari_ntsc_setup_t const atari_ntsc_rgb        =
  {  0.0,  0.0, 0.0, 0.0 , -0.3, .3,  0.7 , -1.0, -1.0, -1.0, 0, 0, 0, 0. };
atari_ntsc_setup_t const atari_ntsc_monochrome =
  {  0.0, -1.0, 0.0, 0.0 , -0.3, .3,  0.2 , -0.2, -0.2, -1.0, 0, 0, 0, 0. };
atari_ntsc_setup_t const atari_ntsc_bad        =
  {  0.1, -0.3, 0.3, 0.25,  0.2,  0,  0.1 ,  0.5,  0.5,  0.5, 0, 0, 0, 0. };
atari_ntsc_setup_t const atari_ntsc_horrible   =
  { -0.1, -0.5, 0.6, 0.43,  0.4,  0,  0.05,  0.7, -0.8, -0.7, 0, 0, 0, 0. };

#define alignment_count 2
#define burst_count     1
#define rescale_in      8
#define rescale_out     7

#define artifacts_mid   1.0f
#define fringing_mid    1.0f
#define std_decoder_hue 0

#include "atari_ntsc_impl.h"

/* 2 input pixels -> 8 composite samples */
pixel_info_t const atari_ntsc_pixels [alignment_count] = {
  { PIXEL_OFFSET( -4, -9 ), { 1, 1, 1, 1            } },
  { PIXEL_OFFSET(  0, -5 ), {            1, 1, 1, 1 } },
};

static void correct_errors( atari_ntsc_rgb_t color, atari_ntsc_rgb_t* out )
{
  unsigned i;
  for ( i = 0; i < rgb_kernel_size / 2; i++ )
  {
    atari_ntsc_rgb_t error = color -
        out [i    ] - out [(i+10)%14+14] -
        out [i + 7] - out [i + 3    +14];
    CORRECT_ERROR( i + 3 + 14 );
  }
}

void atari_ntsc_init( atari_ntsc_t* ntsc, atari_ntsc_setup_t const* setup )
{
  init_t impl;
  if ( !setup )
    setup = &atari_ntsc_composite;
  init( &impl, setup );
  
  /* Atari change: no alternating burst phases - remove code for merge_fields. */

  for (int entry = 0; entry < atari_ntsc_palette_size; entry++ )
  {
    double* yiq_ptr = setup->yiq_palette + 3 * entry;
    double y = *yiq_ptr++;
    double i = *yiq_ptr++;
    double q = *yiq_ptr++;

    i *= rgb_unit;
    q *= rgb_unit;
    y *= rgb_unit;
    y += rgb_offset;

    /* Generate kernel */
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

void atari_ntsc_blit_rgb16( atari_ntsc_t const* ntsc, ATARI_NTSC_IN_T const* atari_in,
    long in_row_width, int in_width, int in_height,
    void* rgb_out, long out_pitch )
{
  int const chunk_count = (in_width - 1) / atari_ntsc_in_chunk;
  while ( in_height-- )
  {
    ATARI_NTSC_IN_T const* line_in = atari_in;
    ATARI_NTSC_BEGIN_ROW( ntsc, atari_ntsc_black, ATARI_NTSC_ADJ_IN( *(line_in[0]) ) );
    atari_ntsc_out_t* restrict line_out = (atari_ntsc_out_t*) rgb_out;
    int n;
    ++line_in;
    
    for ( n = chunk_count; n; --n )
    {
      /* order of input and output pixels must not be altered */
      ATARI_NTSC_COLOR_IN( 0, ntsc, ATARI_NTSC_ADJ_IN( *(line_in [0]) ) ); //CHANGED TO DEREFERENCE POINTER
      ATARI_NTSC_RGB_OUT_RGB16( 0, line_out [0] );
      ATARI_NTSC_RGB_OUT_RGB16( 1, line_out [1] );
      ATARI_NTSC_RGB_OUT_RGB16( 2, line_out [2] );
      ATARI_NTSC_RGB_OUT_RGB16( 3, line_out [3] );
      
      ATARI_NTSC_COLOR_IN( 1, ntsc, ATARI_NTSC_ADJ_IN( *(line_in [1]) ) ); //CHANGED TO DEREFERENCE POINTER
      ATARI_NTSC_RGB_OUT_RGB16( 4, line_out [4] );
      ATARI_NTSC_RGB_OUT_RGB16( 5, line_out [5] );
      ATARI_NTSC_RGB_OUT_RGB16( 6, line_out [6] );
      
      line_in  += 2;
      line_out += 7;
    }
    
    /* finish final pixels */
    ATARI_NTSC_COLOR_IN( 0, ntsc, atari_ntsc_black );
    ATARI_NTSC_RGB_OUT_RGB16( 0, line_out [0] );
    ATARI_NTSC_RGB_OUT_RGB16( 1, line_out [1] );
    ATARI_NTSC_RGB_OUT_RGB16( 2, line_out [2] );
    ATARI_NTSC_RGB_OUT_RGB16( 3, line_out [3] );
    
    ATARI_NTSC_COLOR_IN( 1, ntsc, atari_ntsc_black );
    ATARI_NTSC_RGB_OUT_RGB16( 4, line_out [4] );
    ATARI_NTSC_RGB_OUT_RGB16( 5, line_out [5] );
    ATARI_NTSC_RGB_OUT_RGB16( 6, line_out [6] );
    
    atari_in += in_row_width;
    rgb_out = (char*) rgb_out + out_pitch;
  }
}

void atari_ntsc_blit_bgr16( atari_ntsc_t const* ntsc, ATARI_NTSC_IN_T const* atari_in,
    long in_row_width, int in_width, int in_height,
    void* rgb_out, long out_pitch )
{
  int const chunk_count = (in_width - 1) / atari_ntsc_in_chunk;
  while ( in_height-- )
  {
    ATARI_NTSC_IN_T const* line_in = atari_in;
    ATARI_NTSC_BEGIN_ROW( ntsc, atari_ntsc_black, ATARI_NTSC_ADJ_IN( *(line_in[0]) ) );
    atari_ntsc_out_t* restrict line_out = (atari_ntsc_out_t*) rgb_out;
    int n;
    ++line_in;
    
    for ( n = chunk_count; n; --n )
    {
      /* order of input and output pixels must not be altered */
      ATARI_NTSC_COLOR_IN( 0, ntsc, ATARI_NTSC_ADJ_IN( *(line_in [0]) ) ); //CHANGED TO DEREFERENCE POINTER
      ATARI_NTSC_RGB_OUT_BGR16( 0, line_out [0] );
      ATARI_NTSC_RGB_OUT_BGR16( 1, line_out [1] );
      ATARI_NTSC_RGB_OUT_BGR16( 2, line_out [2] );
      ATARI_NTSC_RGB_OUT_BGR16( 3, line_out [3] );
      
      ATARI_NTSC_COLOR_IN( 1, ntsc, ATARI_NTSC_ADJ_IN( *(line_in [1]) ) ); //CHANGED TO DEREFERENCE POINTER
      ATARI_NTSC_RGB_OUT_BGR16( 4, line_out [4] );
      ATARI_NTSC_RGB_OUT_BGR16( 5, line_out [5] );
      ATARI_NTSC_RGB_OUT_BGR16( 6, line_out [6] );
      
      line_in  += 2;
      line_out += 7;
    }
    
    /* finish final pixels */
    ATARI_NTSC_COLOR_IN( 0, ntsc, atari_ntsc_black );
    ATARI_NTSC_RGB_OUT_BGR16( 0, line_out [0] );
    ATARI_NTSC_RGB_OUT_BGR16( 1, line_out [1] );
    ATARI_NTSC_RGB_OUT_BGR16( 2, line_out [2] );
    ATARI_NTSC_RGB_OUT_BGR16( 3, line_out [3] );
    
    ATARI_NTSC_COLOR_IN( 1, ntsc, atari_ntsc_black );
    ATARI_NTSC_RGB_OUT_BGR16( 4, line_out [4] );
    ATARI_NTSC_RGB_OUT_BGR16( 5, line_out [5] );
    ATARI_NTSC_RGB_OUT_BGR16( 6, line_out [6] );
    
    atari_in += in_row_width;
    rgb_out = (char*) rgb_out + out_pitch;
  }
}
