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
#include "TVSignal.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TVSignal::TVSignal(const PaletteHandler& paletteHandler)
  : myPaletteHandler{paletteHandler}
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TVSignal::setTiming(ConsoleTiming timing)
{
  myTiming = timing;
  myPrevLine.fill(0);
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
    case ConsoleTiming::pal:
      renderPAL(tiaSrc, srcWidth, srcHeight, rgbDst, dstPitch,
                (scanlinesLastFrame & 1) != 0);
      break;

    case ConsoleTiming::secam:
      renderSECAM(tiaSrc, srcWidth, srcHeight, rgbDst, dstPitch);
      break;

    default:
      // TVSignal is not called for NTSC; TIASurface handles that path directly
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TVSignal::renderPAL(const uInt8* tiaSrc, uInt32 srcWidth, uInt32 srcHeight,
                          uInt32* rgbDst, uInt32 dstPitch, bool phaseInverted)
{
  const auto& yuv = myPaletteHandler.palYUVTable();
  const float vSign = phaseInverted ? -1.F : 1.F;

  for(uInt32 y = 0; y < srcHeight; ++y)
  {
    const uInt8* src = tiaSrc + y * srcWidth;
    uInt32* dst      = rgbDst + y * dstPitch;

    for(uInt32 x = 0; x < srcWidth; ++x)
    {
      const auto& curr = yuv[src[x]];
      const auto& prev = yuv[myPrevLine[x]];

      // Luma passes through; only chroma goes through the delay line.
      // U is always averaged.  V is averaged (normal) or differenced
      // (phase-inverted, producing the colour-loss greyscale effect).
      const float yo = curr.y;
      const float uo = (curr.u + prev.u) * 0.5F;
      const float vo = (curr.v + vSign * prev.v) * 0.5F;

      dst[x] = yuvToRGB(yo, uo, vo);
    }

    std::copy_n(src, srcWidth, myPrevLine.data());
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TVSignal::renderSECAM(const uInt8* tiaSrc, uInt32 srcWidth, uInt32 srcHeight,
                            uInt32* rgbDst, uInt32 dstPitch)
{
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
