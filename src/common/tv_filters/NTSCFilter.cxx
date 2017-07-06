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
// Copyright (c) 1995-2017 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include "TIASurface.hxx"
#include "Settings.hxx"

#include "NTSCFilter.hxx"

constexpr double scaleFrom100(double x) { return (x/50.0) - 1.0;     }
constexpr uInt32 scaleTo100(double x)   { return uInt32(50*(x+1.0)); }

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
NTSCFilter::NTSCFilter()
  : mySetup(AtariNTSC::TV_Composite),
    myPreset(PRESET_OFF),
    myCurrentAdjustable(0)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string NTSCFilter::setPreset(Preset preset)
{
  myPreset = preset;
  string msg = "disabled";
  switch(myPreset)
  {
    case PRESET_COMPOSITE:
      mySetup = AtariNTSC::TV_Composite;
      msg = "COMPOSITE";
      break;
    case PRESET_SVIDEO:
      mySetup = AtariNTSC::TV_SVideo;
      msg = "S-VIDEO";
      break;
    case PRESET_RGB:
      mySetup = AtariNTSC::TV_RGB;
      msg = "RGB";
      break;
    case PRESET_BAD:
      mySetup = AtariNTSC::TV_Bad;
      msg = "BAD ADJUST";
      break;
    case PRESET_CUSTOM:
      mySetup = myCustomSetup;
      msg = "CUSTOM";
      break;
    default:
      return msg;
  }
  myNTSC.initialize(mySetup, myTIAPalette);
  return msg;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string NTSCFilter::getPreset() const
{
  switch(myPreset)
  {
    case PRESET_COMPOSITE:  return "COMPOSITE";
    case PRESET_SVIDEO:     return "S-VIDEO";
    case PRESET_RGB:        return "RGB";
    case PRESET_BAD:        return "BAD ADJUST";
    case PRESET_CUSTOM:     return "CUSTOM";
    default:                return "Disabled";
  }
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

  uInt32 newval = scaleTo100(*ourCustomAdjustables[myCurrentAdjustable].value);
  newval += 2;  if(newval > 100) newval = 100;
  *ourCustomAdjustables[myCurrentAdjustable].value = scaleFrom100(newval);

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

  uInt32 newval = scaleTo100(*ourCustomAdjustables[myCurrentAdjustable].value);
  if(newval < 2) newval = 0;
  else           newval -= 2;
  *ourCustomAdjustables[myCurrentAdjustable].value = scaleFrom100(newval);

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
  myCustomSetup.hue = BSPF::clamp(settings.getFloat("tv.hue"), -1.0f, 1.0f);
  myCustomSetup.saturation = BSPF::clamp(settings.getFloat("tv.saturation"), -1.0f, 1.0f);
  myCustomSetup.contrast = BSPF::clamp(settings.getFloat("tv.contrast"), -1.0f, 1.0f);
  myCustomSetup.brightness = BSPF::clamp(settings.getFloat("tv.brightness"), -1.0f, 1.0f);
  myCustomSetup.sharpness = BSPF::clamp(settings.getFloat("tv.sharpness"), -1.0f, 1.0f);
  myCustomSetup.gamma = BSPF::clamp(settings.getFloat("tv.gamma"), -1.0f, 1.0f);
  myCustomSetup.resolution = BSPF::clamp(settings.getFloat("tv.resolution"), -1.0f, 1.0f);
  myCustomSetup.artifacts = BSPF::clamp(settings.getFloat("tv.artifacts"), -1.0f, 1.0f);
  myCustomSetup.fringing = BSPF::clamp(settings.getFloat("tv.fringing"), -1.0f, 1.0f);
  myCustomSetup.bleed = BSPF::clamp(settings.getFloat("tv.bleed"), -1.0f, 1.0f);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NTSCFilter::saveConfig(Settings& settings) const
{
  // Save adjustables for custom mode
  settings.setValue("tv.hue", myCustomSetup.hue);
  settings.setValue("tv.saturation", myCustomSetup.saturation);
  settings.setValue("tv.contrast", myCustomSetup.contrast);
  settings.setValue("tv.brightness", myCustomSetup.brightness);
  settings.setValue("tv.sharpness", myCustomSetup.sharpness);
  settings.setValue("tv.gamma", myCustomSetup.gamma);
  settings.setValue("tv.resolution", myCustomSetup.resolution);
  settings.setValue("tv.artifacts", myCustomSetup.artifacts);
  settings.setValue("tv.fringing", myCustomSetup.fringing);
  settings.setValue("tv.bleed", myCustomSetup.bleed);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NTSCFilter::getAdjustables(Adjustable& adjustable, Preset preset) const
{
  switch(preset)
  {
    case PRESET_COMPOSITE:
      convertToAdjustable(adjustable, AtariNTSC::TV_Composite);  break;
    case PRESET_SVIDEO:
      convertToAdjustable(adjustable, AtariNTSC::TV_SVideo);  break;
    case PRESET_RGB:
      convertToAdjustable(adjustable, AtariNTSC::TV_RGB);  break;
    case PRESET_BAD:
      convertToAdjustable(adjustable, AtariNTSC::TV_Bad);  break;
    case PRESET_CUSTOM:
      convertToAdjustable(adjustable, myCustomSetup);  break;
    default:
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NTSCFilter::setCustomAdjustables(Adjustable& adjustable)
{
  myCustomSetup.hue = scaleFrom100(adjustable.hue);
  myCustomSetup.saturation = scaleFrom100(adjustable.saturation);
  myCustomSetup.contrast = scaleFrom100(adjustable.contrast);
  myCustomSetup.brightness = scaleFrom100(adjustable.brightness);
  myCustomSetup.sharpness = scaleFrom100(adjustable.sharpness);
  myCustomSetup.gamma = scaleFrom100(adjustable.gamma);
  myCustomSetup.resolution = scaleFrom100(adjustable.resolution);
  myCustomSetup.artifacts = scaleFrom100(adjustable.artifacts);
  myCustomSetup.fringing = scaleFrom100(adjustable.fringing);
  myCustomSetup.bleed = scaleFrom100(adjustable.bleed);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NTSCFilter::convertToAdjustable(Adjustable& adjustable,
                                     const AtariNTSC::Setup& setup) const
{
  adjustable.hue         = scaleTo100(setup.hue);
  adjustable.saturation  = scaleTo100(setup.saturation);
  adjustable.contrast    = scaleTo100(setup.contrast);
  adjustable.brightness  = scaleTo100(setup.brightness);
  adjustable.sharpness   = scaleTo100(setup.sharpness);
  adjustable.gamma       = scaleTo100(setup.gamma);
  adjustable.resolution  = scaleTo100(setup.resolution);
  adjustable.artifacts   = scaleTo100(setup.artifacts);
  adjustable.fringing    = scaleTo100(setup.fringing);
  adjustable.bleed       = scaleTo100(setup.bleed);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AtariNTSC::Setup NTSCFilter::myCustomSetup = AtariNTSC::TV_Composite;

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
