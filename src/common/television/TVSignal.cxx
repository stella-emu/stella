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

#include "TIAConstants.hxx"
#include "TVSignal.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TVSignal::setTiming(ConsoleTiming timing)
{
  if(timing == myTiming)
    return;

  myTiming = timing;

  // setTVMode/setPalette only feed the engine for the active standard, so a
  // timing change must replay the stored mode and palette into the engine
  // that has just become active
  if(myTiming == ConsoleTiming::ntsc)
  {
    if(myTVMode != TVMode::None)
      myNTSCSignal.initialize(myTVMode);
    myNTSCSignal.setPalette(myRGBPalette);
  }
  else if(myTiming == ConsoleTiming::pal)
  {
    if(myTVMode != TVMode::None)
      myPALSignal.initialize(myTVMode);
    myPALSignal.setPalette(myRGBPalette);
  }
  else if(myTiming == ConsoleTiming::secam)
  {
    if(myTVMode != TVMode::None)
      mySECAMSignal.initialize(myTVMode);
    mySECAMSignal.setPalette(myRGBPalette);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TVSignal::setPalette(const PaletteArray& tiaPalette,
                          const PaletteArray& rgbPalette)
{
  myPalette    = tiaPalette;
  myRGBPalette = rgbPalette;

  // Only the engine for the active standard consumes the RGB palette; a
  // later timing change replays it into the other engine (see setTiming)
  if(myTiming == ConsoleTiming::ntsc)
    myNTSCSignal.setPalette(myRGBPalette);
  else if(myTiming == ConsoleTiming::pal)
    myPALSignal.setPalette(myRGBPalette);
  else if(myTiming == ConsoleTiming::secam)
    mySECAMSignal.setPalette(myRGBPalette);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TVSignal::setTVMode(TVMode type)
{
  myTVMode = type;

  if(type == TVMode::None)
    return;

  // Only the engine for the active standard is (re)built; a later timing
  // change replays the mode into the other engine (see setTiming)
  if(myTiming == ConsoleTiming::ntsc)
    myNTSCSignal.initialize(type);
  else if(myTiming == ConsoleTiming::pal)
    myPALSignal.initialize(type);
  else if(myTiming == ConsoleTiming::secam)
    mySECAMSignal.initialize(type);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string_view TVSignal::getPreset() const
{
  switch(myTVMode)
  {
    case TVMode::RGB:       return "RGB";
    case TVMode::SVideo:    return "S-VIDEO";
    case TVMode::Composite: return "COMPOSITE";
    case TVMode::Custom:    return "CUSTOM";
    default:                return "Disabled";
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 TVSignal::outputWidth() const
{
  // For NTSC and PAL, every engine mode (RGB, S-Video, Composite, Custom)
  // renders to the wider, oversampled grid; only None passes through at the
  // native TIA width.  SECAM has no oversampled engine, so it is always native.
  if(myTiming == ConsoleTiming::ntsc && myTVMode != TVMode::None)
    return NTSCSignal::outWidth(TIAConstants::frameBufferWidth);
  if(myTiming == ConsoleTiming::pal && myTVMode != TVMode::None)
    return PALSignal::outWidth(TIAConstants::frameBufferWidth);
  return TIAConstants::frameBufferWidth;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SpanOf<AdjustableTag> TVSignal::currentAdjustableTags() const
{
  switch(myTiming)
  {
    case ConsoleTiming::ntsc:  return NTSCSignal::adjustableTags();
    case ConsoleTiming::pal:   return PALSignal::adjustableTags();
    default:                   return {};
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TVSignal::selectAdjustable(int direction,
                                string& text, string& valueText, Int32& value)
{
  const auto tags = currentAdjustableTags();
  const auto n    = static_cast<uInt32>(tags.size());
  if(n == 0) return;

  if(direction == +1)
    myCurrentAdjustable = (myCurrentAdjustable + 1) % n;
  else if(direction == -1)
    myCurrentAdjustable = (myCurrentAdjustable == 0) ? n - 1 : myCurrentAdjustable - 1;

  value     = static_cast<Int32>(scaleTo100(*tags[myCurrentAdjustable].value));
  text      = std::format("Custom {}", tags[myCurrentAdjustable].type);
  valueText = std::format("{}%", value);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TVSignal::changeAdjustable(int adjustable, int direction,
                                string& text, string& valueText, Int32& newValue)
{
  myCurrentAdjustable = static_cast<uInt32>(adjustable);
  changeCurrentAdjustable(direction, text, valueText, newValue);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TVSignal::changeCurrentAdjustable(int direction,
                                       string& text, string& valueText, Int32& newValue)
{
  const auto tags = currentAdjustableTags();
  if(tags.empty() || myCurrentAdjustable >= tags.size()) return;

  newValue = static_cast<Int32>(scaleTo100(*tags[myCurrentAdjustable].value));
  newValue = BSPF::clamp(newValue + direction, 0, 100);
  *tags[myCurrentAdjustable].value = scaleFrom100(newValue);

  // Re-apply the custom setup so the filter sees the updated parameter
  if(myTiming == ConsoleTiming::ntsc)
    myNTSCSignal.reinitializeCustom();
  else if(myTiming == ConsoleTiming::pal)
    myPALSignal.reinitializeCustom();

  text      = std::format("Custom {}", tags[myCurrentAdjustable].type);
  valueText = std::format("{}%", newValue);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TVSignal::render(const uInt8* tiaSrc, uInt32 srcWidth, uInt32 srcHeight,
                      uInt32* rgbDst, uInt32 dstPitch, bool phaseInverted,
                      bool lineParity)
{
  switch(myTiming)
  {
    case ConsoleTiming::ntsc:
      renderNTSC(tiaSrc, srcWidth, srcHeight, rgbDst, dstPitch);
      break;

    case ConsoleTiming::pal:
      renderPAL(tiaSrc, srcWidth, srcHeight, rgbDst, dstPitch, phaseInverted);
      break;

    case ConsoleTiming::secam:
      renderSECAM(tiaSrc, srcWidth, srcHeight, rgbDst, dstPitch, lineParity);
      break;

    default:
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TVSignal::renderPassthrough(const uInt8* tiaSrc, uInt32 srcWidth,
                                 uInt32 srcHeight, uInt32* rgbDst,
                                 uInt32 dstPitch) const
{
  for(uInt32 y = 0; y < srcHeight; ++y)
  {
    const uInt8* src = tiaSrc + static_cast<size_t>(y) * srcWidth;
    uInt32* dst      = rgbDst + static_cast<size_t>(y) * dstPitch;
    for(uInt32 x = 0; x < srcWidth; ++x)
      dst[x] = myPalette[src[x]];
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TVSignal::renderNTSC(const uInt8* tiaSrc, uInt32 srcWidth, uInt32 srcHeight,
                           uInt32* rgbDst, uInt32 dstPitch)
{
  if(myTVMode != TVMode::None)
  {
    // Blargg filter takes byte pitch; dstPitch here is pixel pitch
    myNTSCSignal.render(tiaSrc, srcWidth, srcHeight, rgbDst, dstPitch << 2);
  }
  else
    renderPassthrough(tiaSrc, srcWidth, srcHeight, rgbDst, dstPitch);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TVSignal::renderPAL(const uInt8* tiaSrc, uInt32 srcWidth, uInt32 srcHeight,
                          uInt32* rgbDst, uInt32 dstPitch, bool phaseInverted)
{
  // None: raw palette lookup with no signal processing (capture source).
  if(myTVMode == TVMode::None)
  {
    renderPassthrough(tiaSrc, srcWidth, srcHeight, rgbDst, dstPitch);
    return;
  }

  // RGB, S-Video, Composite, Custom all run the PAL engine; the active mode
  // selects how much of the chroma chain runs (RGB = full-bandwidth, no comb;
  // S-Video = band-limited, no comb; Composite/Custom = full comb).  Only the
  // composite modes can lose colour, so phaseInverted (an odd scanline count)
  // drives both the per-line V-phase alternation and the colour-killer there.
  myPALSignal.render(tiaSrc, srcWidth, srcHeight, rgbDst, dstPitch, phaseInverted);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TVSignal::renderSECAM(const uInt8* tiaSrc, uInt32 srcWidth, uInt32 srcHeight,
                            uInt32* rgbDst, uInt32 dstPitch, bool lineParity)
{
  // None: raw palette lookup with no signal processing (capture source).
  if(myTVMode == TVMode::None)
  {
    renderPassthrough(tiaSrc, srcWidth, srcHeight, rgbDst, dstPitch);
    return;
  }

  // Every colour mode (RGB, S-Video, Composite, Custom) runs the delay line,
  // because for SECAM it is the decoder rather than an effect layered on one.
  // Neither chroma bandwidth nor Y/C separation is modelled, so the three
  // colour connections render identically; they differ only from None.
  mySECAMSignal.render(tiaSrc, srcWidth, srcHeight, rgbDst, dstPitch, lineParity);
}
