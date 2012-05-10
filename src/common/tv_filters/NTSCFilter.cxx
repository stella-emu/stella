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

#include "NTSCFilter.hxx"

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

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
NTSCFilter::NTSCFilter()
  : mySetup(atari_ntsc_composite),
    myCustomSetup(atari_ntsc_composite)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
NTSCFilter::~NTSCFilter()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NTSCFilter::setTIAPalette(const uInt32* palette)
{
  // The TIA palette consists of 128 colours, but the palette array actually
  // contains 256 entries, where only every second value is a valid colour
  uInt8* ptr = myTIAPalette;
  for(int i = 0; i < 256; i+=2)
  {
    *ptr++ = (palette[i] >> 16) & 0xff;
    *ptr++ = (palette[i] >> 8) & 0xff;
    *ptr++ = palette[i] & 0xff;
  }
  updateFilter();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string NTSCFilter::setPreset(Preset preset)
{
  string msg = "disabled";
  switch(preset)
  {
    case PRESET_COMPOSITE:
      mySetup = atari_ntsc_composite;
      msg = "COMPOSITE";
      break;
    case PRESET_SVIDEO:
      mySetup = atari_ntsc_svideo;
      msg = "S-VIDEO";
      break;
    case PRESET_RGB:
      mySetup = atari_ntsc_rgb;
      msg = "RGB";
      break;
    case PRESET_BAD:
      mySetup = atari_ntsc_bad;
      msg = "BAD";
      break;
    case PRESET_CUSTOM:
      mySetup = myCustomSetup;
      msg = "CUSTOM";
      break;
    default:
      return msg;
  }
  updateFilter();
  return msg;
}

#if 0
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NTSCFilter::updateAdjustables(const atari_ntsc_setup_t& setup)
{
  myAdjustables.hue         = setup.hue;
  myAdjustables.saturation  = setup.saturation;
  myAdjustables.contrast    = setup.contrast;
  myAdjustables.brightness  = setup.brightness;
  myAdjustables.sharpness   = setup.sharpness;
  myAdjustables.gamma       = setup.gamma;
  myAdjustables.resolution  = setup.resolution;
  myAdjustables.artifacts   = setup.artifacts;
  myAdjustables.fringing    = setup.fringing;
  myAdjustables.bleed       = setup.bleed;
  myAdjustables.burst_phase = setup.burst_phase;
}
#endif
