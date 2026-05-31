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

#include <algorithm>
#include <cmath>
#include <numbers>
#include "Settings.hxx"
#include "PALSignal.hxx"

namespace {
  // Map a TVMode to the corresponding PALSignal preset Setup.
  // Custom returns myCustomSetup; None and unknown fall back to Composite.
  const PALSignal::Setup& setupFor(TVMode mode,
                                   const PALSignal::Setup& customSetup)
  {
    switch(mode)
    {
      case TVMode::SVideo:    return PALSignal::TV_SVideo;
      case TVMode::Bad:       return PALSignal::TV_Bad;
      case TVMode::Custom:    return customSetup;
      case TVMode::Composite:
      default:                return PALSignal::TV_Composite;
    }
  }
}  // namespace

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PALSignal::PALSignal()
  : myCurrentLine(SAMPLES_PER_LINE, 0.F),
    myPreviousLine(SAMPLES_PER_LINE, 0.F),
    mySVideoY(VISIBLE_SAMPLES, 0.F),
    mySVideoU(VISIBLE_SAMPLES, 0.F),
    mySVideoV(VISIBLE_SAMPLES, 0.F)
{
  buildLumaKernel();
  buildChromaKernel();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PALSignal::initialize(TVMode mode)
{
  mySVideo = (mode == TVMode::SVideo);
  mySetup  = setupFor(mode, myCustomSetup);
  buildLumaKernel();
  buildChromaKernel();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PALSignal::reinitializeCustom()
{
  mySVideo = false;
  mySetup  = myCustomSetup;
  buildLumaKernel();
  buildChromaKernel();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PALSignal::getAdjustables(Adjustable& adjustable, TVMode mode)
{
  convertToAdjustable(adjustable, setupFor(mode, myCustomSetup));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PALSignal::setCustomAdjustables(const Adjustable& adjustable)
{
  myCustomSetup.sharpness = scaleFrom100(adjustable.sharpness);
  myCustomSetup.bleed     = scaleFrom100(adjustable.blend);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PALSignal::loadConfig(const Settings& settings)
{
  myCustomSetup.sharpness = BSPF::clamp(settings.getFloat("pal.sharpness"), -1.0F, 1.0F);
  myCustomSetup.bleed     = BSPF::clamp(settings.getFloat("pal.blend"),     -1.0F, 1.0F);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PALSignal::saveConfig(Settings& settings)
{
  settings.setValue("pal.sharpness", myCustomSetup.sharpness);
  settings.setValue("pal.blend",     myCustomSetup.bleed);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PALSignal::convertToAdjustable(Adjustable& adjustable, const Setup& setup)
{
  adjustable.sharpness = scaleTo100(setup.sharpness);
  adjustable.blend     = scaleTo100(setup.bleed);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PALSignal::setPalette(IntSpan palette)
{
  // Denormalize [-1..1] setup fields to physical units before encoding
  const float gamma  = mySetup.gamma * 0.75F + 1.75F;          // → [1.0..2.5]
  const float sat    = mySetup.saturation + 1.0F;               // → [0..2]
  const float hueRad = mySetup.hue * std::numbers::pi_v<float>; // → [-π..+π]
  const float cosHue = std::cos(hueRad);
  const float sinHue = std::sin(hueRad);

  // Convert each palette entry to linear-light YUV.
  // The palette is already in display gamma; we linearise before encoding
  // and re-apply gamma on decode, giving physically correct blending.
  for(uInt32 i = 0; i < std::min(static_cast<uInt32>(palette.size()), 256U); ++i)
  {
    // Unpack 0x00RRGGBB and linearise from display gamma
    const float r = std::pow(((palette[i] >> 16) & 0xFF) / 255.F, gamma);
    const float g = std::pow(((palette[i] >>  8) & 0xFF) / 255.F, gamma);
    const float b = std::pow(( palette[i]        & 0xFF) / 255.F, gamma);

    // BT.601 luma
    const float y =  0.299F * r + 0.587F * g + 0.114F * b;

    // BT.601 colour differences, scaled by saturation
    const float u = sat * (-0.147F * r - 0.289F * g + 0.436F * b);
    const float v = sat * ( 0.615F * r - 0.515F * g - 0.100F * b);

    // Apply global hue rotation in the UV plane
    myClockTable[i] = {
      .y = y,
      .u = u * cosHue - v * sinHue,
      .v = u * sinHue + v * cosHue
    };
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Sinc-windowed (Hann) low-pass FIR kernel.
// cutoffNorm: normalised cutoff = cutoff_Hz / SAMPLE_RATE
// taps: total kernel length (should be odd)
// kernel: output array of length taps, normalised to unit DC gain
static void buildFIR(float cutoffNorm, uInt32 taps, float* kernel)
{
  const int half = static_cast<int>(taps) / 2;
  float sum = 0.F;

  for(int i = 0; i < static_cast<int>(taps); ++i)
  {
    const int   n = i - half;
    const float hann = 0.5F + 0.5F * std::cos(std::numbers::pi_v<float> * n / (half + 1));
    const float sinc = (n == 0)
      ? 2.F * cutoffNorm
      : std::sin(2.F * std::numbers::pi_v<float> * cutoffNorm * n) /
        (std::numbers::pi_v<float> * n);
    kernel[i] = sinc * hann;
    sum += kernel[i];
  }
  for(uInt32 i = 0; i < taps; ++i)
    kernel[i] /= sum;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PALSignal::buildLumaKernel()
{
  // PAL-B/G luma bandwidth: 5.0 MHz
  buildFIR(5.0e6F / SAMPLE_RATE, LUMA_TAPS, myLumaKernel.data());

  // Aperture correction: 3-tap kernel [-k/2, 1+k, -k/2]
  // sharpness is in [-1..1]; positive = boost highs
  myApertureK = mySetup.sharpness * 0.5F;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PALSignal::buildChromaKernel()
{
  // PAL chroma bandwidth: ±1.3 MHz per axis
  buildFIR(1.3e6F / SAMPLE_RATE, CHROMA_TAPS, myChromaKernel.data());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PALSignal::encodeLine(const uInt8* src, uInt32 width, float vSign)
{
  // Zero the full line buffer (including the non-visible blanking region)
  std::fill(myCurrentLine.begin(), myCurrentLine.end(), 0.F);

  // Visible region starts after 68 TIA clocks of blanking
  // (matching the 2600's HBLANK: 68 colour clocks)
  constexpr uInt32 HBLANK_CLOCKS  = 68;
  constexpr uInt32 HBLANK_SAMPLES = HBLANK_CLOCKS * SAMPLES_PER_CLOCK;

  uInt32 sampleIdx = HBLANK_SAMPLES;

  for(uInt32 x = 0; x < width; ++x)
  {
    const ClockEntry& ce = myClockTable[src[x]];

    // Expand one TIA colour clock into SAMPLES_PER_CLOCK (= 5) composite
    // samples.  Each sample uses the subcarrier quadrature values at the
    // appropriate phase offset.  The subcarrier phase accumulates across
    // the entire line including blanking, so we track it relative to the
    // global sample index.
    for(uInt32 s = 0; s < SAMPLES_PER_CLOCK; ++s, ++sampleIdx)
    {
      // Phase index within the 4-sample subcarrier cycle.
      // sampleIdx counts from the start of the line including blanking.
      const uInt32 phase = sampleIdx & 3u;   // mod 4 (power of two)

      const float cosSc = SUBCARRIER_COS[phase];
      const float sinSc = SUBCARRIER_SIN[phase];

      // PAL composite: Y + U·cos − vSign·V·sin
      // vSign flips the V component on alternate lines (PAL law)
      myCurrentLine[sampleIdx] = ce.y
                                + ce.u * cosSc
                                - vSign * ce.v * sinSc;
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PALSignal::applyLumaFilter(float* buf, uInt32 n) const
{
  // FIR convolution (direct form, in-place via temporary)
  std::vector<float> tmp(n, 0.F);
  const int half = static_cast<int>(LUMA_TAPS) / 2;

  for(uInt32 i = 0; i < n; ++i)
  {
    float acc = 0.F;
    for(int k = 0; k < static_cast<int>(LUMA_TAPS); ++k)
    {
      const int j = static_cast<int>(i) - k + half;
      if(j >= 0 && j < static_cast<int>(n))
        acc += buf[j] * myLumaKernel[k];
    }
    tmp[i] = acc;
  }

  // Aperture correction (sharpness): 3-tap [-k/2, 1+k, -k/2]
  for(uInt32 i = 1; i < n - 1; ++i)
    buf[i] = tmp[i] * (1.F + myApertureK)
           - myApertureK * 0.5F * (tmp[i-1] + tmp[i+1]);

  // Edge pixels: no correction
  buf[0]   = tmp[0];
  buf[n-1] = tmp[n-1];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PALSignal::applyChromaFilter(float* buf, uInt32 n) const
{
  std::vector<float> tmp(n, 0.F);
  const int half = static_cast<int>(CHROMA_TAPS) / 2;

  for(uInt32 i = 0; i < n; ++i)
  {
    float acc = 0.F;
    for(int k = 0; k < static_cast<int>(CHROMA_TAPS); ++k)
    {
      const int j = static_cast<int>(i) - k + half;
      if(j >= 0 && j < static_cast<int>(n))
        acc += buf[j] * myChromaKernel[k];
    }
    tmp[i] = acc;
  }
  std::copy(tmp.begin(), tmp.end(), buf);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PALSignal::decodeLine(uInt32* dst, uInt32 width, float vSign)
{
  // We decode the visible portion of myCurrentLine and myPreviousLine.
  // Blanking offset in samples:
  constexpr uInt32 HBLANK_SAMPLES = 68 * SAMPLES_PER_CLOCK;

  const uInt32 visibleSamples = width * SAMPLES_PER_CLOCK;

  // ── Step 1: Quadrature demodulation ────────────────────────────────────
  // For each visible sample, multiply by cos and sin subcarrier to
  // extract U and V from the composite signal.  We do this for both the
  // current line and the delayed (previous) line.

  std::vector<float> yBuf (visibleSamples);
  std::vector<float> uBuf (visibleSamples);
  std::vector<float> vBuf (visibleSamples);
  std::vector<float> yuBuf(visibleSamples);
  std::vector<float> yvBuf(visibleSamples);

  for(uInt32 i = 0; i < visibleSamples; ++i)
  {
    const uInt32 lineIdx = HBLANK_SAMPLES + i;
    const uInt32 phase   = lineIdx & 3u;

    const float cosSc = SUBCARRIER_COS[phase];
    const float sinSc = SUBCARRIER_SIN[phase];

    // Current line demodulation:
    //   composite = Y + U·cos − vSign·V·sin
    //   × cos  →  U/2 + high-freq cross-terms (removed by LPF)
    //   × −vSign·sin  →  V/2 + high-freq cross-terms
    const float curr = myCurrentLine[lineIdx];
    uBuf[i]  = curr * cosSc * 2.F;
    vBuf[i]  = curr * (-vSign * sinSc) * 2.F;
    yBuf[i]  = curr;

    // Previous line (which had opposite vSign, so its V was encoded as +vSign·V·sin):
    //   × cos  →  +U (same sign — averages correctly with current U)
    //   × −vSign·sin  →  −V (opposite sign — subtracted in comb to reinforce V)
    const float prev = myPreviousLine[lineIdx];
    yuBuf[i] = prev * cosSc * 2.F;
    yvBuf[i] = prev * (-vSign * sinSc) * 2.F;
  }

  // ── Step 2: Low-pass filter U and V (removes demodulation artifacts) ───
  applyChromaFilter(uBuf.data(),  visibleSamples);
  applyChromaFilter(vBuf.data(),  visibleSamples);
  applyChromaFilter(yuBuf.data(), visibleSamples);
  applyChromaFilter(yvBuf.data(), visibleSamples);

  // ── Step 3: PAL 1-line comb filter ────────────────────────────────────
  // U: both lines encode U with the same sign → add for reinforcement.
  // V: yvBuf contains −V_prev (opposite sign due to PAL alternation) →
  //    subtract to get (V + V)/2 = V; using + would cancel V to zero.
  const float blend    = mySetup.bleed * 0.5F + 0.5F;   // [-1..1] → [0..1]
  const float invBlend = 1.F - blend;

  for(uInt32 i = 0; i < visibleSamples; ++i)
  {
    uBuf[i] = uBuf[i] * invBlend + yuBuf[i] * blend;
    vBuf[i] = vBuf[i] * invBlend - yvBuf[i] * blend;
  }

  // ── Step 4: Luma = low-pass of composite (removes subcarrier) ──────────
  applyLumaFilter(yBuf.data(), visibleSamples);

  // ── Step 5: Downsample 5:1 and convert to RGB ─────────────────────────
  // Average the 5 samples per TIA colour clock down to 1 pixel.
  for(uInt32 x = 0; x < width; ++x)
  {
    const uInt32 base = x * SAMPLES_PER_CLOCK;
    float y = 0.F, u = 0.F, v = 0.F;
    for(uInt32 s = 0; s < SAMPLES_PER_CLOCK; ++s)
    {
      y += yBuf[base + s];
      u += uBuf[base + s];
      v += vBuf[base + s];
    }
    dst[x] = toRGB(y / SAMPLES_PER_CLOCK,
                   u / SAMPLES_PER_CLOCK,
                   v / SAMPLES_PER_CLOCK);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PALSignal::render(const uInt8* tiaSrc, uInt32 srcWidth, uInt32 srcHeight,
                          uInt32* rgbDst, uInt32 dstPitch, bool evenField)
{
  if(mySVideo)
  {
    for(uInt32 y = 0; y < srcHeight; ++y)
      renderLineSVideo(tiaSrc + y * srcWidth, rgbDst + y * dstPitch, srcWidth);
    return;
  }

  // Reset delay-line state at the start of each frame
  std::fill(myPreviousLine.begin(), myPreviousLine.end(), 0.F);

  for(uInt32 y = 0; y < srcHeight; ++y)
  {
    const uInt8* src = tiaSrc + y * srcWidth;
    uInt32*      dst = rgbDst + y * dstPitch;

    // PAL V-phase alternation:
    // Even lines (relative to field start) use +V; odd lines use −V.
    // evenField tracks the absolute field parity so that phase is
    // consistent across frames regardless of variable scanline counts.
    const bool isEvenLine = ((y & 1u) == 0u) ^ !evenField;
    const float vSign = isEvenLine ? 1.F : -1.F;

    encodeLine(src, srcWidth, vSign);
    decodeLine(dst, srcWidth, vSign);

    std::swap(myCurrentLine, myPreviousLine);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PALSignal::renderLineSVideo(const uInt8* src, uInt32* dst, uInt32 width)
{
  // S-Video: Y and C are carried on separate wires, so there is no
  // composite encode/decode and no crosstalk between luma and chroma.
  // We still apply bandwidth filters to simulate finite channel bandwidth,
  // and aperture correction for the sharpness control.

  const uInt32 n = width * SAMPLES_PER_CLOCK;

  // Expand each TIA colour clock into SAMPLES_PER_CLOCK constant YUV samples
  for(uInt32 x = 0; x < width; ++x)
  {
    const ClockEntry& ce = myClockTable[src[x]];
    const uInt32 base = x * SAMPLES_PER_CLOCK;
    for(uInt32 s = 0; s < SAMPLES_PER_CLOCK; ++s)
    {
      mySVideoY[base + s] = ce.y;
      mySVideoU[base + s] = ce.u;
      mySVideoV[base + s] = ce.v;
    }
  }

  // Luma: bandwidth limit + aperture correction (sharpness)
  applyLumaFilter(mySVideoY.data(), n);

  // Chroma: bandwidth limit on each colour-difference axis independently
  applyChromaFilter(mySVideoU.data(), n);
  applyChromaFilter(mySVideoV.data(), n);

  // Downsample 5:1 and convert to RGB
  for(uInt32 x = 0; x < width; ++x)
  {
    const uInt32 base = x * SAMPLES_PER_CLOCK;
    float y = 0.F, u = 0.F, v = 0.F;
    for(uInt32 s = 0; s < SAMPLES_PER_CLOCK; ++s)
    {
      y += mySVideoY[base + s];
      u += mySVideoU[base + s];
      v += mySVideoV[base + s];
    }
    dst[x] = toRGB(y / SAMPLES_PER_CLOCK,
                   u / SAMPLES_PER_CLOCK,
                   v / SAMPLES_PER_CLOCK);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 PALSignal::toRGB(float y, float u, float v) const
{
  // BT.601 inverse matrix (linear light)
  float r = y              + 1.140F * v;
  float g = y - 0.395F * u - 0.581F * v;
  float b = y + 2.032F * u;

  // Apply inverse gamma to go from linear back to display gamma
  const float invGamma = 1.F / (mySetup.gamma * 0.75F + 1.75F);
  r = std::pow(BSPF::clamp(r, 0.F, 1.F), invGamma);
  g = std::pow(BSPF::clamp(g, 0.F, 1.F), invGamma);
  b = std::pow(BSPF::clamp(b, 0.F, 1.F), invGamma);

  const uInt32 ri = static_cast<uInt32>(r * 255.F + 0.5F);
  const uInt32 gi = static_cast<uInt32>(g * 255.F + 0.5F);
  const uInt32 bi = static_cast<uInt32>(b * 255.F + 0.5F);
  return (ri << 16) | (gi << 8) | bi;
}
