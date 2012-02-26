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
#include "atari_ntsc.h"

// Limits for the adjustable values.
#define FILTER_NTSC_SHARPNESS_MIN -1.0
#define FILTER_NTSC_SHARPNESS_MAX 1.0
#define FILTER_NTSC_RESOLUTION_MIN -1.0
#define FILTER_NTSC_RESOLUTION_MAX 1.0
#define FILTER_NTSC_ARTIFACTS_MIN -1.0
#define FILTER_NTSC_ARTIFACTS_MAX 1.0
#define FILTER_NTSC_FRINGING_MIN -1.0
#define FILTER_NTSC_FRINGING_MAX 1.0
#define FILTER_NTSC_BLEED_MIN -1.0
#define FILTER_NTSC_BLEED_MAX 1.0
#define FILTER_NTSC_BURST_PHASE_MIN -1.0
#define FILTER_NTSC_BURST_PHASE_MAX 1.0

/**
  This class is based on the Blargg NTSC filter code from Atari800,
  and is derived from 'filter_ntsc.(h|c)'.

  Original code based on implementation from http://www.slack.net/~ant.

  Atari TIA NTSC composite video to RGB emulator/blitter.
*/
class NTSCFilter
{
  public:
    NTSCFilter();
    virtual ~NTSCFilter();

    /* Set/get one of the available preset adjustments: Composite, S-Video, RGB,
       Monochrome. */
    enum {
      PRESET_COMPOSITE,
      PRESET_SVIDEO,
      PRESET_RGB,
      PRESET_MONOCHROME,
      PRESET_CUSTOM,
      /* Number of "normal" (not including CUSTOM) values in enumerator */
      PRESET_SIZE = PRESET_CUSTOM
    };

  public:
    /* Informs the NTSC filter about the current TIA palette.  The filter
       uses this as a baseline for calculating its own internal palette
       in YIQ format.
    */
    void setTIAPalette(const uInt32* palette);

    /* Restores default values for NTSC-filter-specific colour controls.
       updateFilter should be called afterwards to apply changes. */
    void restoreDefaults();

    /* updateFilter should be called afterwards these functions to apply changes. */
    void setPreset(int preset);
    int getPreset();
    void nextPreset();

#if 0 // FIXME
    /* Read/write to configuration file. */
    int FILTER_NTSC_ReadConfig(char *option, char *ptr);
    void FILTER_NTSC_WriteConfig(FILE *fp);

    /* NTSC filter initialisation and processing of command-line arguments. */
    int FILTER_NTSC_Initialise(int *argc, char *argv[]);
#endif
  private:
    /* Reinitialises the an NTSC filter. Should be called after changing
       palette setup or loading/unloading an external palette. */
    void updateFilter();

    // The following function is originally from colours_ntsc.
    /* Creates YIQ_TABLE from external palette. START_ANGLE and START_SATURATIION
       are provided as parameters, because NTSC_FILTER needs to set these values
       according to its internal setup (burst_phase etc).
    */
    void updateYIQTable(double yiq_table[768], double start_angle);

  private:
    // Pointer to the NTSC filter structure
    atari_ntsc_t myFilter;

    // Contains controls used to adjust the palette in the NTSC filter
    atari_ntsc_setup_t mySetup;

    uInt8 myTIAPalette[384];  // 128 colours by 3 components per colour

    int myCurrentModeNum;
    Common::Array<atari_ntsc_setup_t> myModeList;

    static atari_ntsc_setup_t const * const presets[PRESET_SIZE];
    static char const * const preset_cfg_strings[PRESET_SIZE];
};

#endif
