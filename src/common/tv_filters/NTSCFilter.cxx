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
// Copyright (c) 1995-2020 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include "TIASurface.hxx"
#include "Settings.hxx"

#include "NTSCFilter.hxx"

constexpr float scaleFrom100(float x) { return (x / 50.F) - 1.F;     }
constexpr uInt32 scaleTo100(float x)  { return uInt32(50.0001F * (x + 1.F)); }

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string NTSCFilter::setPreset(Preset preset)
{
  myPreset = preset;
  string msg = "disabled";
  switch(myPreset)
  {
    case Preset::COMPOSITE:
      mySetup = AtariNTSC::TV_Composite;
      msg = "COMPOSITE";
      break;
    case Preset::SVIDEO:
      mySetup = AtariNTSC::TV_SVideo;
      msg = "S-VIDEO";
      break;
    case Preset::RGB:
      mySetup = AtariNTSC::TV_RGB;
      msg = "RGB";
      break;
    case Preset::BAD:
      mySetup = AtariNTSC::TV_Bad;
      msg = "BAD ADJUST";
      break;
    case Preset::CUSTOM:
      mySetup = myCustomSetup;
      msg = "CUSTOM";
      break;
    default:
      return msg;
  }
  myNTSC.initialize(mySetup);
  return msg;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string NTSCFilter::getPreset() const
{
  switch(myPreset)
  {
    case Preset::RGB:       return "RGB";
    case Preset::SVIDEO:    return "S-VIDEO";
    case Preset::COMPOSITE: return "COMPOSITE";
    case Preset::BAD:       return "BAD ADJUST";
    case Preset::CUSTOM:    return "CUSTOM";
    default:                return "Disabled";
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NTSCFilter::selectAdjustable(int direction,
                                  string& text, string& valueText, Int32& value)
{
  if(direction == +1)
  {
  #ifdef BLARGG_PALETTE
    myCurrentAdjustable = (myCurrentAdjustable + 1) % 10;
  #else
    myCurrentAdjustable = (myCurrentAdjustable + 1) % 5;
  #endif
  }
  else if(direction == -1)
  {
  #ifdef BLARGG_PALETTE
    if(myCurrentAdjustable == 0) myCurrentAdjustable = 9;
  #else
    if(myCurrentAdjustable == 0) myCurrentAdjustable = 4;
  #endif
    else                         --myCurrentAdjustable;
  }

  ostringstream msg, val;

  value = scaleTo100(*ourCustomAdjustables[myCurrentAdjustable].value);
  msg << "Custom " << ourCustomAdjustables[myCurrentAdjustable].type;
  val << value << "%";

  text = msg.str();
  valueText = val.str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NTSCFilter::changeAdjustable(int adjustable, int direction,
                                  string& text, string& valueText, Int32& newValue)
{
  myCurrentAdjustable = adjustable;
  changeCurrentAdjustable(direction, text, valueText, newValue);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NTSCFilter::changeCurrentAdjustable(int direction,
                                         string& text, string& valueText, Int32& newValue)
{
  //if(myPreset != Preset::CUSTOM)
  //  return "'Custom' TV mode not selected";

  newValue = scaleTo100(*ourCustomAdjustables[myCurrentAdjustable].value);
  newValue = BSPF::clamp(newValue + direction * 2, 0, 100);

  *ourCustomAdjustables[myCurrentAdjustable].value = scaleFrom100(newValue);

  setPreset(myPreset);

  ostringstream msg, val;

  msg << "Custom " << ourCustomAdjustables[myCurrentAdjustable].type;
  val << newValue << "%";

  text = msg.str();
  valueText = val.str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NTSCFilter::loadConfig(const Settings& settings)
{

  // Load adjustables for custom mode
#ifdef BLARGG_PALETTE
  myCustomSetup.hue = BSPF::clamp(settings.getFloat("tv.hue"), -1.0F, 1.0F);
  myCustomSetup.saturation = BSPF::clamp(settings.getFloat("tv.saturation"), -1.0F, 1.0F);
  myCustomSetup.contrast = BSPF::clamp(settings.getFloat("tv.contrast"), -1.0F, 1.0F);
  myCustomSetup.brightness = BSPF::clamp(settings.getFloat("tv.brightness"), -1.0F, 1.0F);
  myCustomSetup.gamma = BSPF::clamp(settings.getFloat("tv.gamma"), -1.0F, 1.0F);
#endif
  myCustomSetup.sharpness = BSPF::clamp(settings.getFloat("tv.sharpness"), -1.0F, 1.0F);
  myCustomSetup.resolution = BSPF::clamp(settings.getFloat("tv.resolution"), -1.0F, 1.0F);
  myCustomSetup.artifacts = BSPF::clamp(settings.getFloat("tv.artifacts"), -1.0F, 1.0F);
  myCustomSetup.fringing = BSPF::clamp(settings.getFloat("tv.fringing"), -1.0F, 1.0F);
  myCustomSetup.bleed = BSPF::clamp(settings.getFloat("tv.bleed"), -1.0F, 1.0F);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NTSCFilter::saveConfig(Settings& settings) const
{
  // Save adjustables for custom mode
#ifdef BLARGG_PALETTE
  settings.setValue("tv.hue", myCustomSetup.hue);
  settings.setValue("tv.saturation", myCustomSetup.saturation);
  settings.setValue("tv.contrast", myCustomSetup.contrast);
  settings.setValue("tv.brightness", myCustomSetup.brightness);
  settings.setValue("tv.gamma", myCustomSetup.gamma);
#endif
  settings.setValue("tv.sharpness", myCustomSetup.sharpness);
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
    case Preset::RGB:
      convertToAdjustable(adjustable, AtariNTSC::TV_RGB);  break;
    case Preset::SVIDEO:
      convertToAdjustable(adjustable, AtariNTSC::TV_SVideo);  break;
    case Preset::COMPOSITE:
      convertToAdjustable(adjustable, AtariNTSC::TV_Composite);  break;
    case Preset::BAD:
      convertToAdjustable(adjustable, AtariNTSC::TV_Bad);  break;
    case Preset::CUSTOM:
      convertToAdjustable(adjustable, myCustomSetup);  break;
    default:
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NTSCFilter::setCustomAdjustables(Adjustable& adjustable)
{
#ifdef BLARGG_PALETTE
  //myCustomSetup.hue = scaleFrom100(adjustable.hue);
  //myCustomSetup.saturation = scaleFrom100(adjustable.saturation);
  //myCustomSetup.contrast = scaleFrom100(adjustable.contrast);
  //myCustomSetup.brightness = scaleFrom100(adjustable.brightness);
  //myCustomSetup.gamma = scaleFrom100(adjustable.gamma);
#endif
  myCustomSetup.sharpness = scaleFrom100(adjustable.sharpness);
  myCustomSetup.resolution = scaleFrom100(adjustable.resolution);
  myCustomSetup.artifacts = scaleFrom100(adjustable.artifacts);
  myCustomSetup.fringing = scaleFrom100(adjustable.fringing);
  myCustomSetup.bleed = scaleFrom100(adjustable.bleed);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NTSCFilter::convertToAdjustable(Adjustable& adjustable,
                                     const AtariNTSC::Setup& setup) const
{
#ifdef BLARGG_PALETTE
  //adjustable.hue         = scaleTo100(setup.hue);
  //adjustable.saturation  = scaleTo100(setup.saturation);
  //adjustable.contrast    = scaleTo100(setup.contrast);
  //adjustable.brightness  = scaleTo100(setup.brightness);
  //adjustable.gamma       = scaleTo100(setup.gamma);
#endif
  adjustable.sharpness   = scaleTo100(setup.sharpness);
  adjustable.resolution  = scaleTo100(setup.resolution);
  adjustable.artifacts   = scaleTo100(setup.artifacts);
  adjustable.fringing    = scaleTo100(setup.fringing);
  adjustable.bleed       = scaleTo100(setup.bleed);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AtariNTSC::Setup NTSCFilter::myCustomSetup = AtariNTSC::TV_Composite;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#ifdef BLARGG_PALETTE
const std::array<NTSCFilter::AdjustableTag, 10> NTSCFilter::ourCustomAdjustables = { {
  { "contrast", &myCustomSetup.contrast },
  { "brightness", &myCustomSetup.brightness },
  { "hue", &myCustomSetup.hue },
  { "saturation", &myCustomSetup.saturation },
  { "gamma", &myCustomSetup.gamma },
#else
const std::array<NTSCFilter::AdjustableTag, int(NTSCFilter::Adjustables::NUM_ADJUSTABLES)> NTSCFilter::ourCustomAdjustables = { {
#endif
  { "sharpness", &myCustomSetup.sharpness },
  { "resolution", &myCustomSetup.resolution },
  { "artifacts", &myCustomSetup.artifacts },
  { "fringing", &myCustomSetup.fringing },
  { "bleeding", &myCustomSetup.bleed }
} };
