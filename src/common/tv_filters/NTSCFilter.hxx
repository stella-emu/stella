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

#ifndef NTSC_FILTER_HXX
#define NTSC_FILTER_HXX

#ifdef DISPLAY_TV

#include "bspf.hxx"
#include "Array.hxx"

// Although the blitter supports 15 and 16 bit, we always use 16-bit
#ifndef ATARI_NTSC_RGB_BITS
  #define ATARI_NTSC_RGB_BITS 16
#endif


/**
  This class is based on the Blargg NTSC filter code from Atari800MacX.

  Original code based on implementation from http://www.slack.net/~ant
  Based on algorithm by NewRisingSun
  License note by Perry: Expat License. 
    http://www.gnu.org/licenses/license-list.html#GPLCompatibleLicenses
  "This is a simple, permissive non-copyleft free software license, compatible with the GNU GPL."

  Atari TIA NTSC composite video to RGB emulator/blitter.
*/
class NTSCFilter
{
  public:
    NTSCFilter();
    virtual ~NTSCFilter();

  public:
    /**
      Cycle through each available NTSC preset mode

      @return  A message explaining the current NTSC preset mode
    */
    const string& next();

    /* Blit one or more scanlines of Atari 8-bit palette values to 16-bit 5-6-5 RGB output.
       For every 7 output pixels, reads approximately 4 source pixels. Use constants below for
       definite input and output pixel counts. */
    void atari_ntsc_blit( unsigned char const* atari_in, long in_pitch,
                          int out_width, int out_height, unsigned short* rgb_out, long out_pitch );

    // Atari800 Initialise function by perrym
    // void ATARI_NTSC_DEFAULTS_Initialise(int *argc, char *argv[], atari_ntsc_setup_t *atari_ntsc_setup);

  private:
    /* Picture parameters, ranging from -1.0 to 1.0 where 0.0 is normal. To easily
       clear all fields, make it a static object then set whatever fields you want:
        static snes_ntsc_setup_t setup;
        setup.hue = ...
    */
    struct atari_ntsc_setup_t
    {
      float hue;
      float saturation;
      float contrast;
      float brightness;
      float sharpness;
      float burst_phase;     // not in radians; -1.0 = -180 degrees, 1.0 = +180 degrees
      float gaussian_factor;
      float gamma_adj;       // gamma adjustment
      float saturation_ramp; // lower saturation for higher luma values
    };

    enum {
      atari_ntsc_entry_size  = 56,
      atari_ntsc_color_count = 256,

      // Useful values to use for output width and number of input pixels read
      atari_ntsc_min_out_width  = 570, // minimum width that doesn't cut off active area
      atari_ntsc_min_in_width   = 320,
      atari_ntsc_full_out_width = 598, // room for 8-pixel left & right overscan borders
      atari_ntsc_full_in_width  = 336,

      // Originally present in .c file
      composite_border = 6,
      center_offset    = 1,
      alignment_count  = 4,  // different pixel alignments with respect to yiq quads */
      rgb_kernel_size  = atari_ntsc_entry_size / alignment_count,

      // Originally present in .c file
      composite_size = composite_border + 8 + composite_border,
      rgb_pad = (center_offset + composite_border + 7) / 8 * 8 - center_offset - composite_border,
      rgb_size = (rgb_pad + composite_size + 7) / 8 * 8,
      rescaled_size = rgb_size / 8 * 7,
      ntsc_kernel_size = composite_size * 2
    };
    typedef uInt32 ntsc_rgb_t;

    // Caller must allocate space for blitter data, which uses 56 KB of memory.
    struct atari_ntsc_t
    {
      ntsc_rgb_t table [atari_ntsc_color_count] [atari_ntsc_entry_size];
    };

    // Originall present in .c file
    struct ntsc_to_rgb_t
    {
      float composite [composite_size];
      float to_rgb [6];
      float brightness;
      float contrast;
      float sharpness;
      short rgb [rgb_size] [3];
      short rescaled [rescaled_size + 1] [3]; /* extra space for sharpen */
      float kernel [ntsc_kernel_size];
    };

    atari_ntsc_t* myNTSCEmu;

    int myCurrentModeNum;
    Common::Array<atari_ntsc_setup_t> myModeList;

  private:
    /* Initialize and adjust parameters. Can be called multiple times on the same
       atari_ntsc_t object. */
    void atari_ntsc_init( atari_ntsc_setup_t const* setup );

    static void rotate_matrix( float const* in, float s, float c, float* out );
    static void ntsc_to_rgb_init( ntsc_to_rgb_t* ntsc, atari_ntsc_setup_t const* setup, float hue );

    /* Convert NTSC composite signal to RGB, where composite signal contains
       only four non-zero samples beginning at offset */
    static void ntsc_to_rgb( ntsc_to_rgb_t const* ntsc, int offset, short* out );

    /* Rescale pixels to NTSC aspect ratio using linear interpolation,
       with 7 output pixels for every 8 input pixels, linear interpolation */
    static void rescale( short const* in, int count, short* out );

    /* Sharpen image using (level-1)/2, level, (level-1)/2 convolution kernel */
    static void sharpen( short const* in, float level, int count, short* out );

    /* Generate pixel and capture into table */
    static ntsc_rgb_t* gen_pixel( ntsc_to_rgb_t* ntsc, int ntsc_pos, int rescaled_pos, ntsc_rgb_t* out );
};

#endif // DISPLAY_TV

#endif
