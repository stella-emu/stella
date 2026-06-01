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
void TVSignal::loadConfig(const Settings& settings)
{
  NTSCSignal::loadConfig(settings);
  PALSignal::loadConfig(settings);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TVSignal::saveConfig(Settings& settings)
{
  NTSCSignal::saveConfig(settings);
  PALSignal::saveConfig(settings);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TVSignal::setTiming(ConsoleTiming timing)
{
  myTiming = timing;
  myPrevLine.fill(0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TVSignal::setPalette(const PaletteArray& tiaPalette,
                          const PaletteArray& rgbPalette)
{
  myPalette = tiaPalette;
  myNTSCSignal.setPalette(rgbPalette);
  myPALSignal.setPalette(rgbPalette);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TVSignal::setTVMode(TVMode type)
{
  myTVMode = type;

  if(type == TVMode::None)
    return;

  myNTSCSignal.initialize(type);
  myPALSignal.initialize(type);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string TVSignal::getPreset() const
{
  switch(myTVMode)
  {
    case TVMode::RGB:       return "RGB";
    case TVMode::SVideo:    return "S-VIDEO";
    case TVMode::Composite: return "COMPOSITE";
    case TVMode::Bad:       return "BAD ADJUST";
    case TVMode::Custom:    return "CUSTOM";
    default:                    return "Disabled";
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
    case ConsoleTiming::ntsc:  return myNTSCSignal.adjustableTags();
    case ConsoleTiming::pal:   return myPALSignal.adjustableTags();
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
                      uInt32* rgbDst, uInt32 dstPitch, uInt32 scanlinesLastFrame)
{
  // Reset delay-line at the start of each frame; the first scanline always
  // blends against a virtual "black" previous line.
  myPrevLine.fill(0);

  switch(myTiming)
  {
    case ConsoleTiming::ntsc:
      renderNTSC(tiaSrc, srcWidth, srcHeight, rgbDst, dstPitch);
      break;

    case ConsoleTiming::pal:
      renderPAL(tiaSrc, srcWidth, srcHeight, rgbDst, dstPitch,
                (scanlinesLastFrame & 1) != 0);
      break;

    case ConsoleTiming::secam:
      renderSECAM(tiaSrc, srcWidth, srcHeight, rgbDst, dstPitch);
      break;

    default:
      break;
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
  {
    uInt32 bufofs = 0, screenofsY = 0;
    for(uInt32 y = 0; y < srcHeight; ++y)
    {
      uInt32 pos = screenofsY;
      for(uInt32 x = srcWidth / 2; x; --x)
      {
        rgbDst[pos++] = myPalette[tiaSrc[bufofs++]];
        rgbDst[pos++] = myPalette[tiaSrc[bufofs++]];
      }
      screenofsY += dstPitch;
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TVSignal::renderPAL(const uInt8* tiaSrc, uInt32 srcWidth, uInt32 srcHeight,
                          uInt32* rgbDst, uInt32 dstPitch, bool phaseInverted)
{
  // None, RGB: raw palette lookup with no signal processing.
  if(myTVMode == TVMode::None || myTVMode == TVMode::RGB)
  {
    for(uInt32 y = 0; y < srcHeight; ++y)
    {
      const uInt8* src = tiaSrc + y * srcWidth;
      uInt32* dst      = rgbDst + y * dstPitch;
      for(uInt32 x = 0; x < srcWidth; ++x)
        dst[x] = myPalette[src[x]];
    }
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
    for(uInt32 y = 0; y < srcHeight; ++y)
    {
      const uInt8* src = tiaSrc + y * srcWidth;
      uInt32* dst      = rgbDst + y * dstPitch;
      for(uInt32 x = 0; x < srcWidth; ++x)
        dst[x] = myPalette[src[x]];
    }
    return;
  }

  const auto& ydbdr = myPaletteHandler.secamYDbDrTable();

  for(uInt32 y = 0; y < srcHeight; ++y)
  {
    const uInt8* src = tiaSrc + y * srcWidth;
    uInt32* dst      = rgbDst + y * dstPitch;

    // Even scanlines carry Db; the delay line provides Dr from the previous
    // (odd) line.  Odd scanlines carry Dr; the delay line provides Db from
    // the previous (even) line.
    const bool evenLine = (y & 1) == 0;

    for(uInt32 x = 0; x < srcWidth; ++x)
    {
      const auto& curr = ydbdr[src[x]];
      const auto& prev = ydbdr[myPrevLine[x]];

      const float yo  = curr.y;
      const float dbo = evenLine ? curr.db : prev.db;
      const float dro = evenLine ? prev.dr : curr.dr;

      dst[x] = yDbDrToRGB(yo, dbo, dro);
    }

    std::copy_n(src, srcWidth, myPrevLine.data());
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 TVSignal::yuvToRGB(float y, float u, float v)
{
  // BT.601 inverse: R = Y + 1.140V; G = Y - 0.395U - 0.581V; B = Y + 2.032U
  const int r = BSPF::clamp(static_cast<int>((y              + 1.140F * v) * 255.F), 0, 255);
  const int g = BSPF::clamp(static_cast<int>((y - 0.395F * u - 0.581F * v) * 255.F), 0, 255);
  const int b = BSPF::clamp(static_cast<int>((y + 2.032F * u             ) * 255.F), 0, 255);
  return static_cast<uInt32>((r << 16) | (g << 8) | b);
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
