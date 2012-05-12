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

#include "bspf.hxx"
#include "Array.hxx"
#include "Settings.hxx"
#include "atari_ntsc.h"

/**
  This class is based on the Blargg NTSC filter code from Atari800,
  and is derived from 'filter_ntsc.(h|c)'.  Original code based on
  implementation from http://www.slack.net/~ant.

  The class is basically a thin wrapper around atari_ntsc_xxx structs
  and methods, so that the rest of the codebase isn't affected by
  updated versions of Blargg code.
*/
class NTSCFilter
{
  public:
    NTSCFilter();
    virtual ~NTSCFilter();

  public:
    // Set one of the available preset adjustments (Composite, S-Video, RGB, etc)
    enum Preset {
      PRESET_OFF,
      PRESET_COMPOSITE,
      PRESET_SVIDEO,
      PRESET_RGB,
      PRESET_BAD,
      PRESET_CUSTOM
    };

    /* Normally used in conjunction with custom mode, contains all
       aspects currently adjustable in NTSC TV emulation. */
    struct Adjustable {
      uInt32 hue, saturation, contrast, brightness, gamma,
             sharpness, resolution, artifacts, fringing, bleed;
    };

  public:
    /* Informs the NTSC filter about the current TIA palette.  The filter
       uses this as a baseline for calculating its own internal palette
       in YIQ format.
    */
    void setTIAPalette(const uInt32* palette);

    // The following are meant to be used strictly for toggling from the GUI
    string setPreset(Preset preset);

    // Reinitialises the NTSC filter (automatically called after settings
    // have changed)
    void updateFilter()
    {
      atari_ntsc_init(&myFilter, &mySetup, myTIAPalette);
    }

    // Get adjustables for the given preset
    // Values will be scaled to 0 - 100 range, independent of how
    // they're actually stored internally
    void getAdjustables(Adjustable& adjustable, Preset preset);

    // Set custom adjustables to given values
    // Values will be scaled to 0 - 100 range, independent of how
    // they're actually stored internally
    void setCustomAdjustables(Adjustable& adjustable);

    // Load and save NTSC-related settings
    void loadConfig(const Settings& settings);
    void saveConfig(Settings& settings) const;

    // Perform Blargg filtering on input buffer, place results in
    // output buffer
    // In the current implementation, the source pitch is always the
    // same as the actual width
    void blit_5551(uInt8* src_buf, int src_width, int src_height,
                   uInt16* dest_buf, long dest_pitch)
    {
      atari_ntsc_blit_5551(&myFilter, src_buf, src_width,
                           src_width, src_height,
                           dest_buf, dest_pitch);
    }
    void blit_1555(uInt8* src_buf, int src_width, int src_height,
                   uInt16* dest_buf, long dest_pitch)
    {
      atari_ntsc_blit_1555(&myFilter, src_buf, src_width,
                           src_width, src_height,
                           dest_buf, dest_pitch);
    }

  private:
    // Convert from atari_ntsc_setup_t values to equivalent adjustables
    void convertToAdjustable(Adjustable& adjustable,
                             const atari_ntsc_setup_t& setup) const;

  private:
    // The NTSC filter structure
    atari_ntsc_t myFilter;

    // Contains controls used to adjust the palette in the NTSC filter
    // This is the main setup object used by the underlying ntsc code
    atari_ntsc_setup_t mySetup;

    // This setup is used only in custom mode (after it is modified,
    // it is copied to mySetup)
    atari_ntsc_setup_t myCustomSetup;

    // Current preset in use
    Preset myPreset;

    // 128 colours by 3 components per colour
    uInt8 myTIAPalette[128 * 3];
};

#endif
