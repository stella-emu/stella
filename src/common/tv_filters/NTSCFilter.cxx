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

#include "FrameBuffer.hxx"
#include "Settings.hxx"

#include "NTSCFilter.hxx"

#define SCALE_FROM_100(x) ((x/50.0)-1.0)
#define SCALE_TO_100(x) (uInt32)(50*(x+1.0))

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
NTSCFilter::NTSCFilter()
  : mySetup(atari_ntsc_composite),
    myPreset(PRESET_OFF),
    myCurrentAdjustable(0)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
NTSCFilter::~NTSCFilter()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NTSCFilter::setTIAPalette(const FrameBuffer& fb, const uInt32* palette)
{
  // Normal TIA palette contains 256 colours, where every odd indexed colour
  // is used for PAL colour-loss effect
  // This can't be emulated here, since the memory requirements would be too
  // great (a 4x increase)
  // Therefore, we need to skip every second index, since the array passed to
  // the Blargg code assumes 128 colours
  uInt8* ptr = myTIAPalette;

  // Set palette for phosphor effect
  for(int i = 0; i < 256; i+=2)
  {
    for(int j = 0; j < 256; j+=2)
    {
      uInt8 ri = (palette[i] >> 16) & 0xff;
      uInt8 gi = (palette[i] >> 8) & 0xff;
      uInt8 bi = palette[i] & 0xff;
      uInt8 rj = (palette[j] >> 16) & 0xff;
      uInt8 gj = (palette[j] >> 8) & 0xff;
      uInt8 bj = palette[j] & 0xff;

      *ptr++ = fb.getPhosphor(ri, rj);
      *ptr++ = fb.getPhosphor(gi, gj);
      *ptr++ = fb.getPhosphor(bi, bj);
    }
  }
  // Set palette for normal fill
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
  myPreset = preset;
  string msg = "disabled";
  switch(myPreset)
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
      msg = "BAD ADJUST";
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

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string NTSCFilter::setNextAdjustable()
{
  if(myPreset != PRESET_CUSTOM)
    return "'Custom' TV mode not selected";

  myCurrentAdjustable = (myCurrentAdjustable + 1) % 10;
  ostringstream buf;
  buf << "Custom adjustable '" << ourCustomAdjustables[myCurrentAdjustable].type
      << "' selected";

  return buf.str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string NTSCFilter::setPreviousAdjustable()
{
  if(myPreset != PRESET_CUSTOM)
    return "'Custom' TV mode not selected";

  if(myCurrentAdjustable == 0) myCurrentAdjustable = 9;
  else                         myCurrentAdjustable--;
  ostringstream buf;
  buf << "Custom adjustable '" << ourCustomAdjustables[myCurrentAdjustable].type
      << "' selected";

  return buf.str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string NTSCFilter::increaseAdjustable()
{
  if(myPreset != PRESET_CUSTOM)
    return "'Custom' TV mode not selected";

  uInt32 newval = SCALE_TO_100(*ourCustomAdjustables[myCurrentAdjustable].value);
  newval += 2;  if(newval > 100) newval = 100;
  *ourCustomAdjustables[myCurrentAdjustable].value = SCALE_FROM_100(newval);

  ostringstream buf;
  buf << "Custom '" << ourCustomAdjustables[myCurrentAdjustable].type
      << "' set to " << newval;

  setPreset(myPreset);
  return buf.str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string NTSCFilter::decreaseAdjustable()
{
  if(myPreset != PRESET_CUSTOM)
    return "'Custom' TV mode not selected";

  uInt32 newval = SCALE_TO_100(*ourCustomAdjustables[myCurrentAdjustable].value);
  if(newval < 2) newval = 0;
  else           newval -= 2;
  *ourCustomAdjustables[myCurrentAdjustable].value = SCALE_FROM_100(newval);

  ostringstream buf;
  buf << "Custom '" << ourCustomAdjustables[myCurrentAdjustable].type
      << "' set to " << newval;

  setPreset(myPreset);
  return buf.str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NTSCFilter::loadConfig(const Settings& settings)
{
  // Load adjustables for custom mode
  myCustomSetup.hue = BSPF_clamp(settings.getFloat("tv_hue"), -1.0f, 1.0f);
  myCustomSetup.saturation = BSPF_clamp(settings.getFloat("tv_saturation"), -1.0f, 1.0f);
  myCustomSetup.contrast = BSPF_clamp(settings.getFloat("tv_contrast"), -1.0f, 1.0f);
  myCustomSetup.brightness = BSPF_clamp(settings.getFloat("tv_brightness"), -1.0f, 1.0f);
  myCustomSetup.sharpness = BSPF_clamp(settings.getFloat("tv_sharpness"), -1.0f, 1.0f);
  myCustomSetup.gamma = BSPF_clamp(settings.getFloat("tv_gamma"), -1.0f, 1.0f);
  myCustomSetup.resolution = BSPF_clamp(settings.getFloat("tv_resolution"), -1.0f, 1.0f);
  myCustomSetup.artifacts = BSPF_clamp(settings.getFloat("tv_artifacts"), -1.0f, 1.0f);
  myCustomSetup.fringing = BSPF_clamp(settings.getFloat("tv_fringing"), -1.0f, 1.0f);
  myCustomSetup.bleed = BSPF_clamp(settings.getFloat("tv_bleed"), -1.0f, 1.0f);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NTSCFilter::saveConfig(Settings& settings) const
{
  // Save adjustables for custom mode
  settings.setFloat("tv_hue", myCustomSetup.hue);
  settings.setFloat("tv_saturation", myCustomSetup.saturation);
  settings.setFloat("tv_contrast", myCustomSetup.contrast);
  settings.setFloat("tv_brightness", myCustomSetup.brightness);
  settings.setFloat("tv_sharpness", myCustomSetup.sharpness);
  settings.setFloat("tv_gamma", myCustomSetup.gamma);
  settings.setFloat("tv_resolution", myCustomSetup.resolution);
  settings.setFloat("tv_artifacts", myCustomSetup.artifacts);
  settings.setFloat("tv_fringing", myCustomSetup.fringing);
  settings.setFloat("tv_bleed", myCustomSetup.bleed);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NTSCFilter::getAdjustables(Adjustable& adjustable, Preset preset)
{
  switch(preset)
  {
    case PRESET_COMPOSITE:
      convertToAdjustable(adjustable, atari_ntsc_composite);  break;
    case PRESET_SVIDEO:
      convertToAdjustable(adjustable, atari_ntsc_svideo);  break;
    case PRESET_RGB:
      convertToAdjustable(adjustable, atari_ntsc_rgb);  break;
    case PRESET_BAD:
      convertToAdjustable(adjustable, atari_ntsc_bad);  break;
    case PRESET_CUSTOM:
      convertToAdjustable(adjustable, myCustomSetup);  break;
    default:
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NTSCFilter::setCustomAdjustables(Adjustable& adjustable)
{
  myCustomSetup.hue = SCALE_FROM_100(adjustable.hue);
  myCustomSetup.saturation = SCALE_FROM_100(adjustable.saturation);
  myCustomSetup.contrast = SCALE_FROM_100(adjustable.contrast);
  myCustomSetup.brightness = SCALE_FROM_100(adjustable.brightness);
  myCustomSetup.sharpness = SCALE_FROM_100(adjustable.sharpness);
  myCustomSetup.gamma = SCALE_FROM_100(adjustable.gamma);
  myCustomSetup.resolution = SCALE_FROM_100(adjustable.resolution);
  myCustomSetup.artifacts = SCALE_FROM_100(adjustable.artifacts);
  myCustomSetup.fringing = SCALE_FROM_100(adjustable.fringing);
  myCustomSetup.bleed = SCALE_FROM_100(adjustable.bleed);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NTSCFilter::convertToAdjustable(Adjustable& adjustable,
                                     const atari_ntsc_setup_t& setup) const
{
  adjustable.hue         = SCALE_TO_100(setup.hue);
  adjustable.saturation  = SCALE_TO_100(setup.saturation);
  adjustable.contrast    = SCALE_TO_100(setup.contrast);
  adjustable.brightness  = SCALE_TO_100(setup.brightness);
  adjustable.sharpness   = SCALE_TO_100(setup.sharpness);
  adjustable.gamma       = SCALE_TO_100(setup.gamma);
  adjustable.resolution  = SCALE_TO_100(setup.resolution);
  adjustable.artifacts   = SCALE_TO_100(setup.artifacts);
  adjustable.fringing    = SCALE_TO_100(setup.fringing);
  adjustable.bleed       = SCALE_TO_100(setup.bleed);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
atari_ntsc_setup_t NTSCFilter::myCustomSetup = atari_ntsc_composite;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const NTSCFilter::AdjustableTag NTSCFilter::ourCustomAdjustables[10] = {
  { "contrast", &myCustomSetup.contrast },
  { "brightness", &myCustomSetup.brightness },
  { "hue", &myCustomSetup.hue },
  { "saturation", &myCustomSetup.saturation },
  { "gamma", &myCustomSetup.gamma },
  { "sharpness", &myCustomSetup.sharpness },
  { "resolution", &myCustomSetup.resolution },
  { "artifacts", &myCustomSetup.artifacts },
  { "fringing", &myCustomSetup.fringing },
  { "bleeding", &myCustomSetup.bleed }
};
