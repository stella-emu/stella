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
    myYBuf(VISIBLE_SAMPLES, 0.F),
    myUBuf(VISIBLE_SAMPLES, 0.F),
    myVBuf(VISIBLE_SAMPLES, 0.F),
    myFilterTmp(VISIBLE_SAMPLES, 0.F),
    myAccY(VISIBLE_SAMPLES, 0.F),
    myAccU(VISIBLE_SAMPLES, 0.F),
    myAccV(VISIBLE_SAMPLES, 0.F),
    myPrevU(VISIBLE_SAMPLES, 0.F),
    myPrevV(VISIBLE_SAMPLES, 0.F)
{
  buildLumaKernel();
  buildChromaKernel();
  buildGammaLUT();
  buildCoeff();
  expandKernels();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PALSignal::initialize(TVMode mode)
{
  mySVideo = (mode == TVMode::SVideo);
  mySetup  = setupFor(mode, myCustomSetup);
  buildLumaKernel();
  buildChromaKernel();
  buildGammaLUT();
  buildCoeff();
  expandKernels();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PALSignal::reinitializeCustom()
{
  mySVideo = false;
  mySetup  = myCustomSetup;
  buildLumaKernel();
  buildChromaKernel();
  buildGammaLUT();
  buildCoeff();
  expandKernels();
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
  myCustomSetup.blend     = scaleFrom100(adjustable.blend);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PALSignal::loadConfig(const Settings& settings)
{
  myCustomSetup.sharpness = BSPF::clamp(settings.getFloat("pal.sharpness"), -1.0F, 1.0F);
  myCustomSetup.blend     = BSPF::clamp(settings.getFloat("pal.blend"),     -1.0F, 1.0F);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PALSignal::saveConfig(Settings& settings)
{
  settings.setValue("pal.sharpness", myCustomSetup.sharpness);
  settings.setValue("pal.blend",     myCustomSetup.blend);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PALSignal::convertToAdjustable(Adjustable& adjustable, const Setup& setup)
{
  adjustable.sharpness = scaleTo100(setup.sharpness);
  adjustable.blend     = scaleTo100(setup.blend);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PALSignal::setPalette(IntSpan palette)
{
  // Denormalize [-1..1] setup fields to physical units before encoding.
  // (saturation/hue/gamma are held neutral here; PaletteHandler owns those
  // dimensions for the user, but the mappings are kept for completeness.)
  const float gamma  = mySetup.gamma * 0.75F + 1.75F;          // → [1.0..2.5]
  const float sat    = mySetup.saturation + 1.0F;               // → [0..2]
  const float hueRad = mySetup.hue * std::numbers::pi_v<float>; // → [-π..+π]
  const float cosHue = std::cos(hueRad);
  const float sinHue = std::sin(hueRad);

  // Convert each palette entry to linear-light BT.601 Y′UV.
  // The palette is already display-gamma encoded, so we linearise with
  // pow(value, gamma) before encoding and re-apply 1/gamma on decode
  // (toRGB/myGammaLUT).  Filtering then averages light, not code values,
  // which is what a real CRT does.
  for(uInt32 i = 0; i < std::min(static_cast<uInt32>(palette.size()), 256U); ++i)
  {
    // Unpack 0x00RRGGBB and linearise from display gamma
    const float r = std::pow(((palette[i] >> 16) & 0xFF) / 255.F, gamma);
    const float g = std::pow(((palette[i] >>  8) & 0xFF) / 255.F, gamma);
    const float b = std::pow(( palette[i]        & 0xFF) / 255.F, gamma);

    // BT.601 luma weights (0.299 / 0.587 / 0.114)
    const float y =  0.299F * r + 0.587F * g + 0.114F * b;

    // BT.601 colour differences with the analogue U,V scale factors
    // (Umax = 0.436, Vmax = 0.615), then scaled by saturation
    const float u = sat * (-0.147F * r - 0.289F * g + 0.436F * b);
    const float v = sat * ( 0.615F * r - 0.515F * g - 0.100F * b);

    // Apply global hue rotation as a 2-D rotation in the (U,V) plane
    myClockTable[i] = {
      .y = y,
      .u = u * cosHue - v * sinHue,
      .v = u * sinHue + v * cosHue
    };
  }
  buildGammaLUT();
  expandKernels();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Windowed-sinc low-pass FIR design (the textbook method).
//
// The ideal brick-wall low-pass with normalised cutoff fc (= cutoff_Hz /
// SAMPLE_RATE) has the impulse response  h[n] = 2·fc · sinc(2·fc·n), an
// infinitely long, non-causal sequence.  We truncate it to `taps` samples
// and multiply by a Hann window to tame the truncation (Gibbs) ripple,
// trading a little transition-band sharpness for much lower stopband
// ripple.  The Hann window here is  0.5 + 0.5·cos(π·n/(half+1)),  centred
// at n = 0 and reaching ~0 just past the kernel edge.  Finally we normalise
// to unit DC gain (Σ = 1) so the filter neither brightens nor darkens.
//
// cutoffNorm: normalised cutoff = cutoff_Hz / SAMPLE_RATE
// taps: total kernel length (should be odd so it is symmetric about n = 0)
// kernel: output array of length taps, normalised to unit DC gain
static void buildFIR(float cutoffNorm, uInt32 taps, float* kernel)
{
  const int half = static_cast<int>(taps) / 2;
  float sum = 0.F;

  for(int i = 0; i < static_cast<int>(taps); ++i)
  {
    const int   n = i - half;
    const float hann = 0.5F + 0.5F * std::cos(std::numbers::pi_v<float> * n / (half + 1));
    // sinc(0) is the 2·fc limit; the else branch is sin(2π·fc·n)/(π·n)
    const float sinc = (n == 0)
      ? 2.F * cutoffNorm
      : std::sin(2.F * std::numbers::pi_v<float> * cutoffNorm * n) /
        (std::numbers::pi_v<float> * n);
    kernel[i] = sinc * hann;
    sum += kernel[i];
  }
  // Normalise to unit DC gain
  for(uInt32 i = 0; i < taps; ++i)
    kernel[i] /= sum;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PALSignal::buildLumaKernel()
{
  // PAL-B/G luma video baseband is nominally 5.0 MHz; band-limit luma there.
  buildFIR(5.0e6F / SAMPLE_RATE, LUMA_TAPS, myLumaKernel.data());

  // Aperture (peaking) correction models the high-frequency boost circuit in
  // a real TV's luma path.  It is an unsharp-mask: a 3-tap [-k/2, 1+k, -k/2]
  // kernel, which leaves DC untouched (taps sum to 1) but lifts edges.
  // sharpness in [-1..1] maps to k = sharpness/2; positive sharpens, negative
  // softens.  Applied after the low-pass in applyLumaFilter().
  myApertureK = mySetup.sharpness * 0.5F;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PALSignal::buildChromaKernel()
{
  // PAL colour-difference bandwidth is ~1.3 MHz per axis (U and V share the
  // same limit in PAL, unlike NTSC's asymmetric I/Q); band-limit chroma there.
  buildFIR(1.3e6F / SAMPLE_RATE, CHROMA_TAPS, myChromaKernel.data());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PALSignal::buildGammaLUT()
{
  // Output stage: re-encode the linear-light result to display gamma, the
  // inverse of the pow(value, gamma) linearisation done in setPalette().
  // Precomputed over GAMMA_LUT_SIZE quantised linear levels so toRGB() is a
  // table lookup instead of a per-pixel pow().
  const float invGamma = 1.F / (mySetup.gamma * 0.75F + 1.75F);
  for(uInt32 i = 0; i < GAMMA_LUT_SIZE; ++i)
  {
    const float linear = static_cast<float>(i) / static_cast<float>(GAMMA_LUT_SIZE - 1);
    myGammaLUT[i] = static_cast<uInt8>(std::pow(linear, invGamma) * 255.F + 0.5F);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PALSignal::buildCoeff()
{
  // Generate the palette-independent linear decode coefficients by running
  // the full encode→demod→filter→downsample pipeline on isolated unit
  // impulses.  Because the pipeline is linear in (y, u, v), three unit
  // inputs fully characterise it.  This costs a couple of dozen small
  // decodes and is only repeated when the filters change.
  constexpr uInt32 HBLANK_SAMPLES = 68 * SAMPLES_PER_CLOCK;
  constexpr uInt32 vis = VISIBLE_SAMPLES;

  // Decode the isolated composite waveform in myCurrentLine into KERNEL_WIDTH
  // output samples around column xref (current-line only; the comb blend is
  // applied separately at render time).  No downsample: the output grid is
  // the oversampled grid itself.
  const auto decodeComposite = [&](int xref, float vSign,
                                   float* oY, float* oU, float* oV)
  {
    for(uInt32 i = 0; i < vis; ++i)
    {
      const uInt32 lineIdx = HBLANK_SAMPLES + i;
      const uInt32 phase   = lineIdx & 3u;
      const float  curr    = myCurrentLine[lineIdx];
      // Synchronous demodulation: multiply by the quadrature carrier to bring
      // the wanted sideband to baseband.  The ×2 restores unity gain, since a
      // product of matched carriers averages to ½ (cos²θ = ½(1+cos2θ)); the
      // 2·fsc term it leaves behind is removed by the chroma low-pass below.
      // Luma is just the composite itself (its low-pass runs separately).
      myUBuf[i] = curr * SUBCARRIER_COS[phase] * 2.F;
      myVBuf[i] = curr * (-vSign * SUBCARRIER_SIN[phase]) * 2.F;
      myYBuf[i] = curr;
    }
    applyChromaFilter(myUBuf.data(), vis);
    applyChromaFilter(myVBuf.data(), vis);
    applyLumaFilter(myYBuf.data(),  vis);

    const int base = xref * static_cast<int>(SAMPLES_PER_CLOCK) - KERNEL_LEFT;
    for(int t = 0; t < KERNEL_WIDTH; ++t)
    {
      const uInt32 idx = static_cast<uInt32>(base + t);
      oY[t] = myYBuf[idx];
      oU[t] = myUBuf[idx];
      oV[t] = myVBuf[idx];
    }
  };

  // Place a single unit clock (y, u, v) at column xref with V-sign vSign
  // into myCurrentLine, zeroing the rest.
  const auto fillComposite = [&](int xref, float y, float u, float v,
                                 float vSign)
  {
    std::fill(myCurrentLine.begin(), myCurrentLine.end(), 0.F);
    const uInt32 base = HBLANK_SAMPLES
                        + static_cast<uInt32>(xref) * SAMPLES_PER_CLOCK;
    for(uInt32 s = 0; s < SAMPLES_PER_CLOCK; ++s)
    {
      const uInt32 phase = (base + s) & 3u;
      myCurrentLine[base + s] = y
        + u * SUBCARRIER_COS[phase]
        - vSign * v * SUBCARRIER_SIN[phase];
    }
  };

  std::array<float, KERNEL_WIDTH> oY{}, oU{}, oV{};

  // ── Composite coefficients, per column phase and V-sign ───────────────
  for(uInt32 p = 0; p < 4; ++p)
  {
    const int xref = 80 + static_cast<int>(p);   // (xref & 3) == p, well-centred
    for(uInt32 vi = 0; vi < 2; ++vi)
    {
      const float vSign = (vi == 0) ? 1.F : -1.F;

      // Response to a unit luma input
      fillComposite(xref, 1.F, 0.F, 0.F, vSign);
      decodeComposite(xref, vSign, oY.data(), oU.data(), oV.data());
      for(int d = 0; d < KERNEL_WIDTH; ++d)
      {
        myCoeff[p][vi][d].yy = oY[d];
        myCoeff[p][vi][d].uy = oU[d];
        myCoeff[p][vi][d].vy = oV[d];
      }
      // Response to a unit U input
      fillComposite(xref, 0.F, 1.F, 0.F, vSign);
      decodeComposite(xref, vSign, oY.data(), oU.data(), oV.data());
      for(int d = 0; d < KERNEL_WIDTH; ++d)
      {
        myCoeff[p][vi][d].yu = oY[d];
        myCoeff[p][vi][d].uu = oU[d];
        myCoeff[p][vi][d].vu = oV[d];
      }
      // Response to a unit V input
      fillComposite(xref, 0.F, 0.F, 1.F, vSign);
      decodeComposite(xref, vSign, oY.data(), oU.data(), oV.data());
      for(int d = 0; d < KERNEL_WIDTH; ++d)
      {
        myCoeff[p][vi][d].yv = oY[d];
        myCoeff[p][vi][d].uv = oU[d];
        myCoeff[p][vi][d].vv = oV[d];
      }
    }
  }

  // ── S-Video coefficients (Y and C carried separately, no subcarrier) ──
  // Luma and chroma are independent here, so the only non-zero terms are
  // the diagonal yy / uu / vv (V reuses the chroma kernel).
  {
    constexpr int xref = 80;
    constexpr uInt32 base = static_cast<uInt32>(xref) * SAMPLES_PER_CLOCK;

    // Unit luma impulse through the luma FIR + aperture
    std::fill(myYBuf.begin(), myYBuf.end(), 0.F);
    for(uInt32 s = 0; s < SAMPLES_PER_CLOCK; ++s) myYBuf[base + s] = 1.F;
    applyLumaFilter(myYBuf.data(), vis);

    // Unit chroma impulse through the chroma FIR
    std::fill(myUBuf.begin(), myUBuf.end(), 0.F);
    for(uInt32 s = 0; s < SAMPLES_PER_CLOCK; ++s) myUBuf[base + s] = 1.F;
    applyChromaFilter(myUBuf.data(), vis);

    const int capBase = xref * static_cast<int>(SAMPLES_PER_CLOCK) - KERNEL_LEFT;
    for(int t = 0; t < KERNEL_WIDTH; ++t)
    {
      const uInt32 idx = static_cast<uInt32>(capBase + t);
      mySVCoeff[t] = DecodeCoeff{};
      mySVCoeff[t].yy = myYBuf[idx];   // luma channel
      mySVCoeff[t].uu = myUBuf[idx];   // U reuses the chroma kernel
      mySVCoeff[t].vv = myUBuf[idx];   // V reuses the chroma kernel
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PALSignal::expandKernels()
{
  // Combine the palette's per-colour YUV with the palette-independent decode
  // coefficients to produce the per-colour scatter kernels used at render.
  for(uInt32 c = 0; c < 256; ++c)
  {
    const float Y = myClockTable[c].y;
    const float U = myClockTable[c].u;
    const float V = myClockTable[c].v;

    for(uInt32 p = 0; p < 4; ++p)
      for(uInt32 vi = 0; vi < 2; ++vi)
      {
        Kernel& k = myKernel[c][p][vi];
        for(int d = 0; d < KERNEL_WIDTH; ++d)
        {
          const DecodeCoeff& co = myCoeff[p][vi][d];
          k.y[d] = co.yy * Y + co.yu * U + co.yv * V;
          k.u[d] = co.uy * Y + co.uu * U + co.uv * V;
          k.v[d] = co.vy * Y + co.vu * U + co.vv * V;
        }
      }

    Kernel& sv = mySVKernel[c];
    for(int d = 0; d < KERNEL_WIDTH; ++d)
    {
      sv.y[d] = mySVCoeff[d].yy * Y;
      sv.u[d] = mySVCoeff[d].uu * U;
      sv.v[d] = mySVCoeff[d].vv * V;
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PALSignal::applyLumaFilter(float* buf, uInt32 n)
{
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
    myFilterTmp[i] = acc;
  }

  // Aperture correction (sharpness): unsharp-mask with the 3-tap kernel
  // [-k/2, 1+k, -k/2].  Centre weight 1+k boosts the sample, the neighbour
  // weights subtract a blurred estimate; the three sum to 1 so flat areas
  // (DC) are unchanged and only edges are emphasised.  Endpoints have no
  // pair of neighbours, so they pass through unmodified.
  for(uInt32 i = 1; i < n - 1; ++i)
    buf[i] = myFilterTmp[i] * (1.F + myApertureK)
           - myApertureK * 0.5F * (myFilterTmp[i-1] + myFilterTmp[i+1]);

  buf[0]   = myFilterTmp[0];
  buf[n-1] = myFilterTmp[n-1];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PALSignal::applyChromaFilter(float* buf, uInt32 n)
{
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
    myFilterTmp[i] = acc;
  }
  std::copy(myFilterTmp.begin(), myFilterTmp.begin() + n, buf);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PALSignal::render(const uInt8* tiaSrc, uInt32 srcWidth, uInt32 srcHeight,
                          uInt32* rgbDst, uInt32 dstPitch, bool phaseInverted)
{
  // phaseInverted is true when the previous frame had an odd scanline count.
  // It drives both the PAL V-phase alternation below and PAL colour loss: an
  // odd count breaks the PAL field/burst sequence, so a real set's colour-
  // killer cuts chroma and the frame is rendered luma-only (greyscale).
  const bool   svideo    = mySVideo;
  // S-Video has separate Y/C wires, so there is no 1-line comb (blend = 0).
  const float  blend    = svideo ? 0.F : (mySetup.blend * 0.5F + 0.5F);
  const float  invBlend = 1.F - blend;
  const uInt32 outW     = outWidth(srcWidth);   // oversampled output width

  // Reset the 1-line comb delay at the start of each frame; the first
  // scanline blends against a virtual "black" previous line.  (S-Video has
  // no comb, so the delay line is never read.)
  if(!svideo)
  {
    std::fill(myPrevU.begin(), myPrevU.begin() + outW, 0.F);
    std::fill(myPrevV.begin(), myPrevV.begin() + outW, 0.F);
  }

  for(uInt32 y = 0; y < srcHeight; ++y)
  {
    const uInt8* src = tiaSrc + y * srcWidth;
    uInt32*      dst = rgbDst + y * dstPitch;

    // PAL V-phase alternation: even lines (relative to field start) use +V,
    // odd lines −V.  phaseInverted tracks absolute field parity so the phase
    // is consistent across frames regardless of variable scanline counts.
    const bool   isEvenLine = ((y & 1u) == 0u) ^ phaseInverted;
    const uInt32 vi = isEvenLine ? 0u : 1u;

    std::fill(myAccY.begin(), myAccY.begin() + outW, 0.F);
    std::fill(myAccU.begin(), myAccU.begin() + outW, 0.F);
    std::fill(myAccV.begin(), myAccV.begin() + outW, 0.F);

    // Scatter each clock's precomputed kernel into the line accumulators.
    // A clock at input column x lands on output samples starting at x·5.
    for(uInt32 x = 0; x < srcWidth; ++x)
    {
      const Kernel& k = svideo ? mySVKernel[src[x]]
                               : myKernel[src[x]][x & 3u][vi];

      // First output sample this kernel touches, then clamp the tap range
      // to the visible region (zero-padded output at the line edges).
      const int base = static_cast<int>(x * SAMPLES_PER_CLOCK) - KERNEL_LEFT;
      const int lo   = base < 0 ? -base : 0;
      const int hiLim = static_cast<int>(outW) - 1 - base;
      const int hi   = hiLim < KERNEL_WIDTH - 1 ? hiLim : KERNEL_WIDTH - 1;
      for(int t = lo; t <= hi; ++t)
      {
        const uInt32 oj = static_cast<uInt32>(base + t);
        myAccY[oj] += k.y[t];
        myAccU[oj] += k.u[t];
        myAccV[oj] += k.v[t];
      }
    }

    // Convert the line to RGB.  S-Video carries Y and C on separate wires,
    // so there is no comb and the chroma passes straight through; Composite
    // applies the PAL 1-line comb blend with the previous line's chroma.
    // (The previous line was encoded with the opposite V-sign, so adding its
    // own-sign filtered chroma reinforces U and cancels chroma noise on V.)
    if(phaseInverted)
    {
      // PAL colour loss: the set's colour-killer has cut chroma for this
      // frame, so emit luma only.  The subcarrier residual still present in
      // the luma channel shows as faint dot-crawl in the greyscale image,
      // as on real hardware.
      for(uInt32 j = 0; j < outW; ++j)
        dst[j] = toRGB(myAccY[j], 0.F, 0.F);
    }
    else if(svideo)
    {
      for(uInt32 j = 0; j < outW; ++j)
        dst[j] = toRGB(myAccY[j], myAccU[j], myAccV[j]);
    }
    else
    {
      for(uInt32 j = 0; j < outW; ++j)
      {
        const float u = myAccU[j] * invBlend + myPrevU[j] * blend;
        const float v = myAccV[j] * invBlend + myPrevV[j] * blend;
        dst[j] = toRGB(myAccY[j], u, v);
      }

      // This line's chroma becomes the delay line for the next scanline.
      std::swap(myAccU, myPrevU);
      std::swap(myAccV, myPrevV);
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 PALSignal::toRGB(float y, float u, float v) const
{
  // Exact inverse of the BT.601 Y′UV matrix used in setPalette() (still in
  // linear light): R = Y + 1.140·V, G = Y − 0.395·U − 0.581·V, B = Y + 2.032·U.
  // Clamp to the legal gamut before the gamma re-encode.
  const float r = BSPF::clamp(y              + 1.140F * v, 0.F, 1.F);
  const float g = BSPF::clamp(y - 0.395F * u - 0.581F * v, 0.F, 1.F);
  const float b = BSPF::clamp(y + 2.032F * u,              0.F, 1.F);

  // LUT lookup: linear [0..1] → quantized index → gamma-encoded [0..255]
  const float scale = static_cast<float>(GAMMA_LUT_SIZE - 1);
  const uInt32 ri = myGammaLUT[static_cast<uInt32>(r * scale + 0.5F)];
  const uInt32 gi = myGammaLUT[static_cast<uInt32>(g * scale + 0.5F)];
  const uInt32 bi = myGammaLUT[static_cast<uInt32>(b * scale + 0.5F)];
  return (ri << 16) | (gi << 8) | bi;
}
