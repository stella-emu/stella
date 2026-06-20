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

#include "NTSCSignal.hxx"

namespace {
  // Map a TVMode to its fixed AtariNTSC preset.  Custom is not handled here
  // (callers substitute the live myCustomSetup); None and unknown fall back to
  // Composite.
  const AtariNTSC::Setup& setupFor(TVMode mode)
  {
    switch(mode)
    {
      case TVMode::RGB:       return AtariNTSC::TV_RGB;
      case TVMode::SVideo:    return AtariNTSC::TV_SVideo;
      case TVMode::Composite:
      default:                return AtariNTSC::TV_Composite;
    }
  }
}  // namespace

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NTSCSignal::initialize(TVMode mode)
{
  myNTSC.initialize((mode == TVMode::Custom) ? myCustomSetup : setupFor(mode));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NTSCSignal::reinitializeCustom()
{
  myNTSC.initialize(myCustomSetup);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NTSCSignal::loadConfig(const Settings& settings)
{
  myCustomSetup.sharpness  = BSPF::clamp(settings.getFloat("ntsc.sharpness"),  -1.0F, 1.0F);
  myCustomSetup.resolution = BSPF::clamp(settings.getFloat("ntsc.resolution"), -1.0F, 1.0F);
  myCustomSetup.artifacts  = BSPF::clamp(settings.getFloat("ntsc.artifacts"),  -1.0F, 1.0F);
  myCustomSetup.fringing   = BSPF::clamp(settings.getFloat("ntsc.fringing"),   -1.0F, 1.0F);
  myCustomSetup.bleed      = BSPF::clamp(settings.getFloat("ntsc.bleed"),      -1.0F, 1.0F);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NTSCSignal::saveConfig(Settings& settings)
{
  settings.setValue("ntsc.sharpness",  myCustomSetup.sharpness);
  settings.setValue("ntsc.resolution", myCustomSetup.resolution);
  settings.setValue("ntsc.artifacts",  myCustomSetup.artifacts);
  settings.setValue("ntsc.fringing",   myCustomSetup.fringing);
  settings.setValue("ntsc.bleed",      myCustomSetup.bleed);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NTSCSignal::getAdjustables(Adjustable& adjustable, TVMode mode)
{
  convertToAdjustable(adjustable, (mode == TVMode::Custom) ? myCustomSetup : setupFor(mode));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NTSCSignal::setCustomAdjustables(const Adjustable& adjustable)
{
  myCustomSetup.sharpness  = scaleFrom100(adjustable.sharpness);
  myCustomSetup.resolution = scaleFrom100(adjustable.resolution);
  myCustomSetup.artifacts  = scaleFrom100(adjustable.artifacts);
  myCustomSetup.fringing   = scaleFrom100(adjustable.fringing);
  myCustomSetup.bleed      = scaleFrom100(adjustable.bleed);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NTSCSignal::convertToAdjustable(Adjustable& adjustable,
                                     const AtariNTSC::Setup& setup)
{
  adjustable.sharpness   = scaleTo100(setup.sharpness);
  adjustable.resolution  = scaleTo100(setup.resolution);
  adjustable.artifacts   = scaleTo100(setup.artifacts);
  adjustable.fringing    = scaleTo100(setup.fringing);
  adjustable.bleed       = scaleTo100(setup.bleed);
}
