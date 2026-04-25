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
// Copyright (c) 1995-2026 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include "Settings.hxx"

#include "NTSCFilter.hxx"

namespace {
  template<typename T>
    requires std::is_arithmetic_v<T>
  constexpr float scaleFrom100(T x) {
    return (static_cast<float>(x) / 50.F) - 1.F;
  }

  template<typename T>
    requires std::is_arithmetic_v<T>
  constexpr uInt32 scaleTo100(T x) {
    return static_cast<uInt32>(50.0001F * (static_cast<float>(x) + 1.F));
  }
}  // namespace

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
  constexpr uInt32 numAdjustables = ourCustomAdjustables.size();

  if(direction == +1)
  {
    myCurrentAdjustable = (myCurrentAdjustable + 1) % numAdjustables;
  }
  else if(direction == -1)
  {
    if(myCurrentAdjustable == 0) myCurrentAdjustable = numAdjustables - 1;
    else                         --myCurrentAdjustable;
  }

  value = scaleTo100(*ourCustomAdjustables[myCurrentAdjustable].value);

  text = std::format("Custom {}",
                     ourCustomAdjustables[myCurrentAdjustable].type);
  valueText = std::format("{}%", value);
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
  newValue = scaleTo100(*ourCustomAdjustables[myCurrentAdjustable].value);
  newValue = BSPF::clamp(newValue + direction, 0, 100);

  *ourCustomAdjustables[myCurrentAdjustable].value = scaleFrom100(newValue);

  setPreset(myPreset);

  text = std::format("Custom {}",
                     ourCustomAdjustables[myCurrentAdjustable].type);
  valueText = std::format("{}%", newValue);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NTSCFilter::loadConfig(const Settings& settings)
{
  // Load adjustables for custom mode
  myCustomSetup.sharpness = BSPF::clamp(settings.getFloat("tv.sharpness"), -1.0F, 1.0F);
  myCustomSetup.resolution = BSPF::clamp(settings.getFloat("tv.resolution"), -1.0F, 1.0F);
  myCustomSetup.artifacts = BSPF::clamp(settings.getFloat("tv.artifacts"), -1.0F, 1.0F);
  myCustomSetup.fringing = BSPF::clamp(settings.getFloat("tv.fringing"), -1.0F, 1.0F);
  myCustomSetup.bleed = BSPF::clamp(settings.getFloat("tv.bleed"), -1.0F, 1.0F);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NTSCFilter::saveConfig(Settings& settings)
{
  // Save adjustables for custom mode
  settings.setValue("tv.sharpness", myCustomSetup.sharpness);
  settings.setValue("tv.resolution", myCustomSetup.resolution);
  settings.setValue("tv.artifacts", myCustomSetup.artifacts);
  settings.setValue("tv.fringing", myCustomSetup.fringing);
  settings.setValue("tv.bleed", myCustomSetup.bleed);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NTSCFilter::getAdjustables(Adjustable& adjustable, Preset preset)
{
  switch(preset)
  {
    case Preset::RGB:
      convertToAdjustable(adjustable, AtariNTSC::TV_RGB);
      break;
    case Preset::SVIDEO:
      convertToAdjustable(adjustable, AtariNTSC::TV_SVideo);
      break;
    case Preset::COMPOSITE:
      convertToAdjustable(adjustable, AtariNTSC::TV_Composite);
      break;
    case Preset::BAD:
      convertToAdjustable(adjustable, AtariNTSC::TV_Bad);
      break;
    case Preset::CUSTOM:
      convertToAdjustable(adjustable, myCustomSetup);
      break;
    default:
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NTSCFilter::setCustomAdjustables(const Adjustable& adjustable)
{
  myCustomSetup.sharpness  = scaleFrom100(adjustable.sharpness);
  myCustomSetup.resolution = scaleFrom100(adjustable.resolution);
  myCustomSetup.artifacts  = scaleFrom100(adjustable.artifacts);
  myCustomSetup.fringing   = scaleFrom100(adjustable.fringing);
  myCustomSetup.bleed      = scaleFrom100(adjustable.bleed);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NTSCFilter::convertToAdjustable(Adjustable& adjustable,
                                     const AtariNTSC::Setup& setup)
{
  adjustable.sharpness   = scaleTo100(setup.sharpness);
  adjustable.resolution  = scaleTo100(setup.resolution);
  adjustable.artifacts   = scaleTo100(setup.artifacts);
  adjustable.fringing    = scaleTo100(setup.fringing);
  adjustable.bleed       = scaleTo100(setup.bleed);
}
