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

#include <cmath>

#include "TIAConstants.hxx"

#include "SECAMSignal.hxx"

static_assert(SECAMSignal::TIA_WIDTH == TIAConstants::frameBufferWidth,
              "SECAM delay line must span a full TIA scanline");

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SECAMSignal::initialize(TVMode mode)
{
  float cutoff = 0.F;
  switch(mode)
  {
    // RGB is the idealised reference: chroma at full bandwidth
    case TVMode::RGB:
      cutoff = 0.F;
      break;

    case TVMode::SVideo:
      cutoff = CUTOFF_SVIDEO;
      break;

    // Composite, and Custom until it has adjustables of its own
    default:
      cutoff = CUTOFF_COMPOSITE;
      break;
  }

  // {0, 1, 0} passes the signal through untouched, so the bypass needs no
  // separate path at render time
  if(cutoff <= 0.F)
  {
    myChromaSide   = 0.F;
    myChromaCentre = 1.F;
    return;
  }

  // Gaussian prototype at the requested −3 dB point, integrated over one
  // colour clock either side of the sample instant.  Even at the widest
  // cutoff the fourth tap is 2e−5, so three carry the whole kernel;
  // renormalising folds that remainder back in and holds DC gain at unity,
  // which is what keeps flat colour untinted.
  const float sigma = std::sqrt(std::log(2.F)) / (2.F * BSPF::PI_f * cutoff);
  const float scale = 1.F / (sigma * std::sqrt(2.F) * TIA_FREQ);
  const auto bin = [scale](float edge) { return std::erf(edge * scale); };

  const float centre = bin(0.5F);
  const float side   = 0.5F * (bin(1.5F) - bin(0.5F));
  const float sum    = centre + 2.F * side;

  myChromaCentre = centre / sum;
  myChromaSide   = side / sum;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SECAMSignal::filterChroma(std::array<float, TIA_WIDTH>& chroma,
                               uInt32 width) const
{
  if(myChromaSide == 0.F)
    return;

  // Clamp at both ends: the picture edge is the blanking interval, which
  // carries no chroma to bleed in from
  float left = chroma[0];
  for(uInt32 x = 0; x < width; ++x)
  {
    const float right = (x + 1 < width) ? chroma[x + 1] : chroma[x];
    const float here  = chroma[x];

    chroma[x] = myChromaSide * (left + right) + myChromaCentre * here;
    left = here;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SECAMSignal::setPalette(IntSpan palette)
{
  constexpr float inv255 = 1.F / 255.F;

  for(size_t i = 0; i < myYDbDr.size(); ++i)
  {
    const float r = static_cast<float>((palette[i] >> 16) & 0xff) * inv255;
    const float g = static_cast<float>((palette[i] >>  8) & 0xff) * inv255;
    const float b = static_cast<float>((palette[i] >>  0) & 0xff) * inv255;

    // Luma and the two colour-difference signals (items 2.4, 2.5), multiplied
    // out.  The palette is already gamma-pre-corrected, which is the space
    // those definitions are written in.
    myYDbDr[i].y  =  0.299F * r + 0.587F * g + 0.114F * b;
    myYDbDr[i].db = -0.450F * r - 0.883F * g + 1.333F * b;
    myYDbDr[i].dr = -1.333F * r + 1.116F * g + 0.217F * b;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SECAMSignal::render(const uInt8* tiaSrc, uInt32 srcWidth, uInt32 srcHeight,
                         uInt32* rgbDst, uInt32 dstPitch, bool firstLineCarriesDr)
{
  // Chroma is blanked through the field-blanking interval (item 2.17), so the
  // delay line holds nothing when the picture starts and the first scanline
  // pairs against black
  myPrevChroma.fill(0.F);

  for(uInt32 y = 0; y < srcHeight; ++y)
  {
    const uInt8* src = tiaSrc + static_cast<size_t>(y) * srcWidth;
    uInt32* dst      = rgbDst + static_cast<size_t>(y) * dstPitch;

    // A line carrying D'B takes D'R from the delay line and vice versa, which
    // is what leaves both lines of a pair sharing one chroma pair and
    // differing only in luma
    const bool carriesDb = ((y & 1) == 0) != firstLineCarriesDr;

    // Only the component this line transmits exists on the wire, so only that
    // one is band-limited here; its partner was filtered on the line that
    // carried it and has been in the delay line since.
    if(carriesDb)
      for(uInt32 x = 0; x < srcWidth; ++x)
        myChroma[x] = myYDbDr[src[x]].db;
    else
      for(uInt32 x = 0; x < srcWidth; ++x)
        myChroma[x] = myYDbDr[src[x]].dr;

    filterChroma(myChroma, srcWidth);

    if(carriesDb)
      for(uInt32 x = 0; x < srcWidth; ++x)
        dst[x] = toRGB(myYDbDr[src[x]].y, myChroma[x], myPrevChroma[x]);
    else
      for(uInt32 x = 0; x < srcWidth; ++x)
        dst[x] = toRGB(myYDbDr[src[x]].y, myPrevChroma[x], myChroma[x]);

    myPrevChroma.swap(myChroma);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 SECAMSignal::toRGB(float y, float db, float dr)
{
  // Inverting item 2.5: R = Y − Dr/1.902 and B = Y + Db/1.505, with G falling
  // out of the item 2.4 luma equation once those are substituted back in
  const int r = BSPF::clamp(static_cast<int>((y              - 0.526F * dr) * 255.F), 0, 255);
  const int g = BSPF::clamp(static_cast<int>((y - 0.129F * db + 0.268F * dr) * 255.F), 0, 255);
  const int b = BSPF::clamp(static_cast<int>((y + 0.665F * db             ) * 255.F), 0, 255);
  return static_cast<uInt32>((r << 16) | (g << 8) | b);
}
