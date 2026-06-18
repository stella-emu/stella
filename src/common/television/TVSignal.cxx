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

#include "PaletteHandler.hxx"
#include "Settings.hxx"
#include "TVSignal.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TVSignal::TVSignal(const PaletteHandler& paletteHandler)
  : myPaletteHandler{paletteHandler}
{
}

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
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string_view TVSignal::getPreset() const
{
  switch(myTVMode)
  {
    case TVMode::RGB:       return "RGB";
    case TVMode::SVideo:    return "S-VIDEO";
    case TVMode::Composite: return "COMPOSITE";
    case TVMode::Bad:       return "BAD ADJUST";
    case TVMode::Custom:    return "CUSTOM";
    default:                return "Disabled";
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 TVSignal::outputWidth() const
{
  // NTSC composite/S-Video and PAL composite/S-Video render to a wider,
  // oversampled grid; None and (for PAL) RGB pass through at native width.
  if(myTiming == ConsoleTiming::ntsc && myTVMode != TVMode::None)
    return NTSCSignal::outWidth(TIAConstants::frameBufferWidth);
  if(myTiming == ConsoleTiming::pal &&
     myTVMode != TVMode::None && myTVMode != TVMode::RGB)
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
                      uInt32* rgbDst, uInt32 dstPitch, bool phaseInverted)
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
      renderSECAM(tiaSrc, srcWidth, srcHeight, rgbDst, dstPitch);
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
  // None, RGB: raw palette lookup with no signal processing.
  if(myTVMode == TVMode::None || myTVMode == TVMode::RGB)
  {
    renderPassthrough(tiaSrc, srcWidth, srcHeight, rgbDst, dstPitch);
    return;
  }

  // Composite, Bad, Custom: full PALSignal composite pipeline.  phaseInverted
  // (an odd scanline count) drives both V-phase alternation and PAL colour loss.
  myPALSignal.render(tiaSrc, srcWidth, srcHeight, rgbDst, dstPitch, phaseInverted);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TVSignal::renderSECAM(const uInt8* tiaSrc, uInt32 srcWidth, uInt32 srcHeight,
                            uInt32* rgbDst, uInt32 dstPitch)
{
  if(myTVMode == TVMode::None)
  {
    renderPassthrough(tiaSrc, srcWidth, srcHeight, rgbDst, dstPitch);
    return;
  }

  // Reset the delay line at the start of each frame; the first scanline
  // always blends against a virtual "black" previous line.
  myPrevLine.fill(0);

  const auto& ydbdr = myPaletteHandler.secamYDbDrTable();

  for(uInt32 y = 0; y < srcHeight; ++y)
  {
    const uInt8* src = tiaSrc + static_cast<size_t>(y) * srcWidth;
    uInt32* dst      = rgbDst + static_cast<size_t>(y) * dstPitch;

    // Even scanlines carry Db; the delay line provides Dr from the previous
    // (odd) line.  Odd scanlines carry Dr; the delay line provides Db from
    // the previous (even) line.
    const bool evenLine = (y & 1) == 0;
    const uInt8* dbLine = evenLine ? src : myPrevLine.data();
    const uInt8* drLine = evenLine ? myPrevLine.data() : src;

    for(uInt32 x = 0; x < srcWidth; ++x)
      dst[x] = yDbDrToRGB(ydbdr[src[x]].y, ydbdr[dbLine[x]].db,
                          ydbdr[drLine[x]].dr);

    std::copy_n(src, srcWidth, myPrevLine.data());
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 TVSignal::yDbDrToRGB(float y, float db, float dr)
{
  // SECAM inverse: R = Y - 0.526Dr; G = Y - 0.129Db + 0.268Dr; B = Y + 0.665Db
  const int r = BSPF::clamp(static_cast<int>((y              - 0.526F * dr) * 255.F), 0, 255);
  const int g = BSPF::clamp(static_cast<int>((y - 0.129F * db + 0.268F * dr) * 255.F), 0, 255);
  const int b = BSPF::clamp(static_cast<int>((y + 0.665F * db             ) * 255.F), 0, 255);
  return static_cast<uInt32>((r << 16) | (g << 8) | b);
}
