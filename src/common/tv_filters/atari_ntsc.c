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

atari_ntsc_setup_t const atari_ntsc_composite = {  0.0,  0.0, 0.0, 0.0,  0.0, 0.0, 0.15,  0.0,  0.0,  0.0, 0 };
atari_ntsc_setup_t const atari_ntsc_svideo    = {  0.0,  0.0, 0.0, 0.0,  0.0, 0.0, 0.45, -1.0, -1.0,  0.0, 0 };
atari_ntsc_setup_t const atari_ntsc_rgb       = {  0.0,  0.0, 0.0, 0.0,  0.2, 0.0, 0.70, -1.0, -1.0, -1.0, 0 };
atari_ntsc_setup_t const atari_ntsc_bad       = {  0.1, -0.3, 0.3, 0.25, 0.2, 0.0, 0.1,   0.5,  0.5,  0.5, 0 };

#define alignment_count 2
#define burst_count     1
#define rescale_in      8
#define rescale_out     7

#define artifacts_mid   1.5f
#define artifacts_max   2.5f
#define fringing_mid    1.0f
#define std_decoder_hue 0

#define gamma_size 256

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

void atari_ntsc_init( atari_ntsc_t* ntsc, atari_ntsc_setup_t const* setup,
                      atari_ntsc_in_t const* palette )
{
  int entry;
  init_t impl;
  if ( !setup )
    setup = &atari_ntsc_composite;
  init( &impl, setup );

  // Palette stores R/G/B data for 'atari_ntsc_palette_size' entries
  for ( entry = 0; entry < atari_ntsc_palette_size; entry++ )
  {
    float r = impl.to_float [*palette++];
    float g = impl.to_float [*palette++];
    float b = impl.to_float [*palette++];

    float y, i, q = RGB_TO_YIQ( r, g, b, y, i );
      
    // Generate kernel
    int ir, ig, ib = YIQ_TO_RGB( y, i, q, impl.to_rgb, int, ir, ig );
    atari_ntsc_rgb_t rgb = PACK_RGB( ir, ig, ib );

    if ( ntsc )
    {
      atari_ntsc_rgb_t* kernel = ntsc->table [entry];
      gen_kernel( &impl, y, i, q, kernel );
      correct_errors( rgb, kernel );
    }
  }
}

void atari_ntsc_blit_1555( atari_ntsc_t const* ntsc, atari_ntsc_in_t const* atari_in,
    long in_row_width, int in_width, int in_height,
    void* rgb_out, long out_pitch )
{
  typedef unsigned short atari_ntsc_out_t;
  int const chunk_count = (in_width - 1) / atari_ntsc_in_chunk;
  while ( in_height-- )
  {
    atari_ntsc_in_t const* line_in = atari_in;
    ATARI_NTSC_BEGIN_ROW( ntsc, atari_ntsc_black, line_in[0] );
    atari_ntsc_out_t* restrict line_out = (atari_ntsc_out_t*) rgb_out;
    int n;
    ++line_in;
    
    for ( n = chunk_count; n; --n )
    {
      /* order of input and output pixels must not be altered */
      ATARI_NTSC_COLOR_IN( 0, ntsc, line_in[0] );
      ATARI_NTSC_RGB_OUT_1555( 0, line_out[0] );
      ATARI_NTSC_RGB_OUT_1555( 1, line_out[1] );
      ATARI_NTSC_RGB_OUT_1555( 2, line_out[2] );
      ATARI_NTSC_RGB_OUT_1555( 3, line_out[3] );
      
      ATARI_NTSC_COLOR_IN( 1, ntsc, line_in[1] );
      ATARI_NTSC_RGB_OUT_1555( 4, line_out[4] );
      ATARI_NTSC_RGB_OUT_1555( 5, line_out[5] );
      ATARI_NTSC_RGB_OUT_1555( 6, line_out[6] );
      
      line_in  += 2;
      line_out += 7;
    }
    
    /* finish final pixels */
    ATARI_NTSC_COLOR_IN( 0, ntsc, atari_ntsc_black );
    ATARI_NTSC_RGB_OUT_1555( 0, line_out[0] );
    ATARI_NTSC_RGB_OUT_1555( 1, line_out[1] );
    ATARI_NTSC_RGB_OUT_1555( 2, line_out[2] );
    ATARI_NTSC_RGB_OUT_1555( 3, line_out[3] );
    
    ATARI_NTSC_COLOR_IN( 1, ntsc, atari_ntsc_black );
    ATARI_NTSC_RGB_OUT_1555( 4, line_out[4] );
    ATARI_NTSC_RGB_OUT_1555( 5, line_out[5] );
    ATARI_NTSC_RGB_OUT_1555( 6, line_out[6] );
    
    atari_in += in_row_width;
    rgb_out = (char*) rgb_out + out_pitch;
  }
}

void atari_ntsc_blit_8888( atari_ntsc_t const* ntsc, atari_ntsc_in_t const* atari_in,
    long in_row_width, int in_width, int in_height,
    void* rgb_out, long out_pitch )
{
  typedef unsigned int atari_ntsc_out_t;
  int const chunk_count = (in_width - 1) / atari_ntsc_in_chunk;
  while ( in_height-- )
  {
    atari_ntsc_in_t const* line_in = atari_in;
    ATARI_NTSC_BEGIN_ROW( ntsc, atari_ntsc_black, line_in[0] );
    atari_ntsc_out_t* restrict line_out = (atari_ntsc_out_t*) rgb_out;
    int n;
    ++line_in;
    
    for ( n = chunk_count; n; --n )
    {
      /* order of input and output pixels must not be altered */
      ATARI_NTSC_COLOR_IN( 0, ntsc, line_in[0] );
      ATARI_NTSC_RGB_OUT_8888( 0, line_out[0] );
      ATARI_NTSC_RGB_OUT_8888( 1, line_out[1] );
      ATARI_NTSC_RGB_OUT_8888( 2, line_out[2] );
      ATARI_NTSC_RGB_OUT_8888( 3, line_out[3] );
      
      ATARI_NTSC_COLOR_IN( 1, ntsc, line_in[1] );
      ATARI_NTSC_RGB_OUT_8888( 4, line_out[4] );
      ATARI_NTSC_RGB_OUT_8888( 5, line_out[5] );
      ATARI_NTSC_RGB_OUT_8888( 6, line_out[6] );
      
      line_in  += 2;
      line_out += 7;
    }
    
    /* finish final pixels */
    ATARI_NTSC_COLOR_IN( 0, ntsc, atari_ntsc_black );
    ATARI_NTSC_RGB_OUT_8888( 0, line_out[0] );
    ATARI_NTSC_RGB_OUT_8888( 1, line_out[1] );
    ATARI_NTSC_RGB_OUT_8888( 2, line_out[2] );
    ATARI_NTSC_RGB_OUT_8888( 3, line_out[3] );
    
    ATARI_NTSC_COLOR_IN( 1, ntsc, atari_ntsc_black );
    ATARI_NTSC_RGB_OUT_8888( 4, line_out[4] );
    ATARI_NTSC_RGB_OUT_8888( 5, line_out[5] );
    ATARI_NTSC_RGB_OUT_8888( 6, line_out[6] );
    
    atari_in += in_row_width;
    rgb_out = (char*) rgb_out + out_pitch;
  }
}
