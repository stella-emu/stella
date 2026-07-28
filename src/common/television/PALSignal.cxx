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

#include "Settings.hxx"
#include "PALSignal.hxx"

namespace {
  // Map a TVMode to the corresponding PALSignal preset Setup.
  constexpr PALSignal::Setup setupFor(TVMode mode, const PALSignal::Setup& customSetup)
  {
    switch(mode)
    {
      case TVMode::RGB:       return PALSignal::TV_RGB;
      case TVMode::SVideo:    return PALSignal::TV_SVideo;
      case TVMode::Custom:    return customSetup;
      case TVMode::Composite:
      default:                return PALSignal::TV_Composite;
    }
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
  // Nominal PAL field rate, used only to turn the crystal beat frequency
  // into a per-frame phase advance.  The beat itself is uncertain by orders
  // of magnitude (crystal tolerance), so nothing is gained by deriving the
  // exact frame duration from the current scanline count.
  constexpr float PAL_FIELD_RATE = 50.F;

  void buildFIR(float cutoffNorm, uInt32 taps, float* kernel)
  {
    const int half = static_cast<int>(taps) / 2;
    float sum = 0.F;

    for(int i = 0; std::cmp_less(i, taps); ++i)
    {
      const int   n = i - half;
      const float hann = 0.5F + 0.5F * std::cos(BSPF::PI_f * n / (half + 1));
      // sinc(0) is the 2·fc limit; the else branch is sin(2π·fc·n)/(π·n)
      const float sinc = (n == 0)
        ? 2.F * cutoffNorm
        : std::sin(2.F * BSPF::PI_f * cutoffNorm * n) / (BSPF::PI_f * n);
      kernel[i] = sinc * hann;
      sum += kernel[i];
    }
    // Normalise to unit DC gain
    for(uInt32 i = 0; i < taps; ++i)
      kernel[i] /= sum;
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
  // The FIR kernels and carrier tables depend only on compile-time
  // constants, so they are built once here; applySetup() covers everything
  // setup-dependent
  buildLumaKernel();
  buildChromaKernel();
  buildCarrierTables();
  applySetup();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PALSignal::initialize(TVMode mode)
{
  myPath  = (mode == TVMode::RGB)    ? Path::RGB
          : (mode == TVMode::SVideo) ? Path::SVideo
          :                            Path::Composite;
  mySetup = setupFor(mode, myCustomSetup);

  // Reset the colour-killer to its powered-up (colour-active) state.  This is
  // the receiver-reset chokepoint: Television::initialize() routes here via
  // setTVMode on a TV-mode change and on a mid-game console reset / ROM
  // reload, and TVSignal::setTiming() replays the mode here when PAL becomes
  // the active standard, so stale killer state never survives into PAL
  // rendering.  (A soft Reset switch does not re-initialise the surface —
  // correctly, since a real TV's killer is unaware of it — and the hysteresis
  // self-heals within a frame or two regardless.)
  myColourKilled = false;
  myKillerRun    = 0;

  // Likewise the console's subcarrier phase: a power-up starts wherever it
  // starts, and there is no more meaningful choice than the un-drifted grid.
  myDriftPhase = 0.F;
  myDriftStep  = 0;

  applySetup();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PALSignal::reinitializeCustom()
{
  myPath  = Path::Composite;
  mySetup = myCustomSetup;
  applySetup();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PALSignal::applySetup()
{
  // Aperture (peaking) correction models the high-frequency boost circuit in
  // a real TV's luma path.  It is an unsharp-mask: a 3-tap [-k/2, 1+k, -k/2]
  // kernel taken at APERTURE_SPACING, which leaves DC untouched (taps sum to
  // 1) but lifts edges.  sharpness in [-1..1] maps to k = sharpness/2;
  // positive sharpens, negative softens.  Applied after the low-pass in
  // applyLumaFilter().
  myApertureK = mySetup.sharpness * 0.5F;

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
  myCustomSetup.sharpness = BSPF::clamp(settings.getFloat("pal.sharpness"), -1.F, 1.F);
  myCustomSetup.blend     = BSPF::clamp(settings.getFloat("pal.blend"),     -1.F, 1.F);
  setColourLoss(settings.getInt("pal.colorloss"));
  setDriftRate(settings.getFloat("pal.drift"));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PALSignal::saveConfig(Settings& settings)
{
  settings.setValue("pal.sharpness", myCustomSetup.sharpness);
  settings.setValue("pal.blend",     myCustomSetup.blend);
  settings.setValue("pal.colorloss", static_cast<int>(myColourLoss));
  settings.setValue("pal.drift",     myDriftRate);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PALSignal::setColourLoss(int model)
{
  myColourLoss = static_cast<ColourLoss>(
      BSPF::clamp(model, 0, static_cast<int>(ColourLoss::NUM_MODELS) - 1));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PALSignal::setDriftRate(float hz)
{
  // Past a few Hz the per-frame phase step is large enough that the motion
  // stops reading as drift and starts aliasing into jitter (at the 50 Hz field
  // rate it would fold completely at 25 Hz), so the range is capped where the
  // effect is still what it claims to be.  This matches the dialog's slider.
  myDriftRate = BSPF::clamp(hz, 0.F, 5.F);
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
  // (saturation/hue are held neutral here; PaletteHandler owns those
  // dimensions for the user, but the mappings are kept for completeness.)
  const float sat    = mySetup.saturation + 1.F;      // → [0..2]
  const float hueRad = mySetup.hue * BSPF::PI_f;      // → [-π..+π]
  const float cosHue = std::cos(hueRad);
  const float sinHue = std::sin(hueRad);

  // Convert each palette entry to BT.601 Y′UV, working on the gamma-encoded
  // code values themselves.  Those stand in for the composite VOLTAGE, which
  // is the domain the whole chain models — see COLOUR SPACE in PALSignal.hxx
  // for why linearising here would be wrong.
  const auto numColours =
    static_cast<uInt32>(std::min<size_t>(palette.size(), myClockTable.size()));
  for(uInt32 i = 0; i < numColours; ++i)
  {
    // Unpack 0x00RRGGBB
    const float r = ((palette[i] >> 16) & 0xFF) / 255.F;
    const float g = ((palette[i] >>  8) & 0xFF) / 255.F;
    const float b = ( palette[i]        & 0xFF) / 255.F;

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
  expandKernels();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PALSignal::buildLumaKernel()
{
  // The receiver's video bandwidth, nominally 5.0 MHz for PAL-B/G.  Every
  // path uses this; the composite path removes its subcarrier by subtracting
  // the recovered chroma instead (see buildCoeff), not by filtering it out,
  // so this stays a plain low-pass rather than a trap cascade.
  buildFIR(5.0e6F / SAMPLE_RATE, LUMA_TAPS, myLumaKernel.data());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PALSignal::buildCarrierTables()
{
  // One quadrature table per drift step.  cos(2π(i/4 + phase)) still has
  // period 4 in the sample index i for any phase offset, so each step needs
  // only 4 entries and sample-time code never calls trig.  Step 0 reproduces
  // the exact integer sequences {1,0,-1,0} / {0,1,0,-1}.
  for(uInt32 step = 0; step < PHASE_STEPS; ++step)
  {
    const float phase = 2.F * BSPF::PI_f * static_cast<float>(step)
                      / static_cast<float>(PHASE_STEPS);
    for(uInt32 i = 0; i < 4; ++i)
    {
      const float angle = BSPF::PI_f * 0.5F * static_cast<float>(i) + phase;
      myCarrierCos[step][i] = std::cos(angle);
      myCarrierSin[step][i] = std::sin(angle);
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PALSignal::buildChromaKernel()
{
  // PAL colour-difference bandwidth is ~1.1-1.3 MHz per axis (U and V share
  // the same limit in PAL, unlike NTSC's asymmetric I/Q); ITU-R BT.470 gives
  // the sidebands as +1.07/−1.30 MHz.  With CHROMA_TAPS the kernel is long
  // enough for the design cutoff to be the realised one (−3 dB ≈ 1.21 MHz).
  buildFIR(1.3e6F / SAMPLE_RATE, CHROMA_TAPS, myChromaKernel.data());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PALSignal::buildCoeff()
{
  // Generate the palette-independent linear decode coefficients by running
  // the full encode→demod→filter pipeline on isolated unit impulses.  Because
  // the pipeline is linear in (y, u, v), three unit inputs fully characterise
  // it.  This costs a couple of dozen small decodes and is only repeated when
  // the filters change.
  constexpr uInt32 HBLANK_SAMPLES = 68 * SAMPLES_PER_CLOCK;

  // The impulse is isolated, and every filter here has finite support, so the
  // response is identically zero more than LUMA_REACH samples away from it.
  // Running the filters over the whole scanline would be arithmetic on known
  // zeros, and there are 4 column phases × 2 V-signs × 3 impulses × PHASE_STEPS
  // of them to build.  This pad is several times the reach, so the windowed
  // result is not an approximation — it is the same answer.
  constexpr int WINDOW_PAD = 48;
  static_assert(WINDOW_PAD > 2 * LUMA_REACH, "window must clear the filters");
  constexpr uInt32 win = SAMPLES_PER_CLOCK + 2 * WINDOW_PAD;

  // Decode the isolated composite waveform in myCurrentLine into the kernel
  // window around column xref (current-line only; the comb blend is applied
  // separately at render time).  No downsample: the output grid is the
  // oversampled grid itself.  Luma and chroma are captured at their own
  // widths, since the chroma FIR spreads them differently.
  const auto decodeComposite = [&](uInt32 step, int xref, float vSign,
                                   float* oY, float* oU, float* oV)
  {
    const auto& cosTab = myCarrierCos[step];
    const auto& sinTab = myCarrierSin[step];

    // Work only in a window around the impulse; see WINDOW_PAD.  Buffer
    // indices stay absolute so that the capture offsets below are unchanged,
    // and the carrier phase keeps coming from the true position on the line.
    const auto base =
      static_cast<uInt32>(xref * static_cast<int>(SAMPLES_PER_CLOCK)
                          - WINDOW_PAD);

    for(uInt32 k = 0; k < win; ++k)
    {
      const uInt32 i       = base + k;
      const uInt32 lineIdx = HBLANK_SAMPLES + i;
      const uInt32 phase   = lineIdx & 3U;
      const float  curr    = myCurrentLine[lineIdx];

      // Synchronous demodulation: multiply by the quadrature carrier to bring
      // the wanted sideband to baseband.  The ×2 restores unity gain, since a
      // product of matched carriers averages to ½ (cos²θ = ½(1+cos2θ)); the
      // 2·fsc term it leaves behind is removed by the chroma low-pass below.
      myUBuf[i] = curr * cosTab[phase] * 2.F;
      myVBuf[i] = curr * (-vSign * sinTab[phase]) * 2.F;
      myYBuf[i] = curr;
    }
    applyChromaFilter(myUBuf.data() + base, win);
    applyChromaFilter(myVBuf.data() + base, win);

    // Composite luma: put the chroma just recovered back onto the subcarrier
    // and subtract it from the composite.  What the chroma path took is
    // exactly what leaves the luma path, so the subcarrier cancels to within
    // 4e-4 without flattening the rest of the band the way a trap short
    // enough to be a 5-tap FIR does.  See DECODING in PALSignal.hxx.
    for(uInt32 k = 0; k < win; ++k)
    {
      const uInt32 i     = base + k;
      const uInt32 phase = (HBLANK_SAMPLES + i) & 3U;
      myYBuf[i] -= myUBuf[i] * cosTab[phase]
                 - vSign * myVBuf[i] * sinTab[phase];
    }
    applyLumaFilter(myYBuf.data() + base, win);

    const int lumaBase =
      xref * static_cast<int>(SAMPLES_PER_CLOCK) - LUMA_KERNEL_LEFT;
    for(int t = 0; t < LUMA_KERNEL_WIDTH; ++t)
      oY[t] = myYBuf[static_cast<uInt32>(lumaBase + t)];

    const int chromaBase =
      xref * static_cast<int>(SAMPLES_PER_CLOCK) - CHROMA_KERNEL_LEFT;
    for(int t = 0; t < CHROMA_KERNEL_WIDTH; ++t)
    {
      const auto idx = static_cast<uInt32>(chromaBase + t);
      oU[t] = myUBuf[idx];
      oV[t] = myVBuf[idx];
    }
  };

  // Place a single unit clock (y, u, v) at column xref with V-sign vSign
  // into myCurrentLine, zeroing the rest.
  const auto fillComposite = [&](uInt32 step, int xref, float y, float u,
                                 float v, float vSign)
  {
    std::ranges::fill(myCurrentLine, 0.F);
    const auto& cosTab = myCarrierCos[step];
    const auto& sinTab = myCarrierSin[step];
    const uInt32 base = HBLANK_SAMPLES
                        + static_cast<uInt32>(xref) * SAMPLES_PER_CLOCK;
    for(uInt32 s = 0; s < SAMPLES_PER_CLOCK; ++s)
    {
      const uInt32 phase = (base + s) & 3U;
      myCurrentLine[base + s] = y
        + u * cosTab[phase]
        - vSign * v * sinTab[phase];
    }
  };

  std::array<float, LUMA_KERNEL_WIDTH>   oY{};
  std::array<float, CHROMA_KERNEL_WIDTH> oU{}, oV{};

  // Composite coefficients, per drift step, column phase and V-sign
  for(uInt32 step = 0; step < PHASE_STEPS; ++step)
    for(uInt32 p = 0; p < 4; ++p)
    {
      const int xref = 80 + static_cast<int>(p);  // (xref & 3) == p, centred
      for(uInt32 vi = 0; vi < 2; ++vi)
      {
        const float vSign = (vi == 0) ? 1.F : -1.F;
        auto& lumaCoeff   = myLumaCoeff[step][p][vi];
        auto& chromaCoeff = myChromaCoeff[step][p][vi];

        // Response to a unit luma input
        fillComposite(step, xref, 1.F, 0.F, 0.F, vSign);
        decodeComposite(step, xref, vSign, oY.data(), oU.data(), oV.data());
        for(int d = 0; d < LUMA_KERNEL_WIDTH; ++d)
          lumaCoeff[d].yy = oY[d];
        for(int d = 0; d < CHROMA_KERNEL_WIDTH; ++d)
        {
          chromaCoeff[d].uy = oU[d];
          chromaCoeff[d].vy = oV[d];
        }
        // Response to a unit U input
        fillComposite(step, xref, 0.F, 1.F, 0.F, vSign);
        decodeComposite(step, xref, vSign, oY.data(), oU.data(), oV.data());
        for(int d = 0; d < LUMA_KERNEL_WIDTH; ++d)
          lumaCoeff[d].yu = oY[d];
        for(int d = 0; d < CHROMA_KERNEL_WIDTH; ++d)
        {
          chromaCoeff[d].uu = oU[d];
          chromaCoeff[d].vu = oV[d];
        }
        // Response to a unit V input
        fillComposite(step, xref, 0.F, 0.F, 1.F, vSign);
        decodeComposite(step, xref, vSign, oY.data(), oU.data(), oV.data());
        for(int d = 0; d < LUMA_KERNEL_WIDTH; ++d)
          lumaCoeff[d].yv = oY[d];
        for(int d = 0; d < CHROMA_KERNEL_WIDTH; ++d)
        {
          chromaCoeff[d].uv = oU[d];
          chromaCoeff[d].vv = oV[d];
        }
      }
  }

  // Normalise the luma path to unity gain at DC.
  //
  // Demodulating a flat field puts its luma AT the subcarrier, where the
  // chroma low-pass still passes 0.7% of it; re-modulating brings that back
  // to DC, so the subtraction takes away slightly the wrong amount and a flat
  // field comes out ~1.4% bright.  A real receiver's luma path is unity-gain
  // at DC — its chroma take-off is a bandpass with no DC response at all — so
  // this is an artefact of realising that bandpass as demodulate/filter/
  // re-modulate, not a receiver behaviour worth keeping.  The correction is a
  // uniform scale, so it moves no part of the frequency response relative to
  // any other.
  //
  // Over one full column-phase cycle four clocks cover 4·SAMPLES_PER_CLOCK
  // output samples, so a flat unit field wants their luma taps to total that.
  for(uInt32 step = 0; step < PHASE_STEPS; ++step)
    for(uInt32 vi = 0; vi < 2; ++vi)
    {
      float sum = 0.F;
      for(uInt32 p = 0; p < 4; ++p)
        for(int d = 0; d < LUMA_KERNEL_WIDTH; ++d)
          sum += myLumaCoeff[step][p][vi][d].yy;

      const float gain = sum / (4.F * static_cast<float>(SAMPLES_PER_CLOCK));
      if(gain > 0.F)
        for(uInt32 p = 0; p < 4; ++p)
          for(int d = 0; d < LUMA_KERNEL_WIDTH; ++d)
            myLumaCoeff[step][p][vi][d].yy /= gain;
    }

  // Capture a luma/chroma impulse pair into a phase-independent coefficient
  // set.  Shared by the S-Video and RGB paths, which differ only in whether
  // the chroma impulse is band-limited.
  const auto buildDirectCoeff = [&](bool filterChroma,
                                    std::array<LumaCoeff, LUMA_KERNEL_WIDTH>& lumaOut,
                                    std::array<ChromaCoeff, CHROMA_KERNEL_WIDTH>& chromaOut)
  {
    constexpr int xref = 80;
    constexpr uInt32 base = static_cast<uInt32>(xref) * SAMPLES_PER_CLOCK;
    constexpr uInt32 winBase = base - WINDOW_PAD;

    // Unit luma impulse through the luma FIR + aperture.  Nothing is
    // subtracted: these wires never carried a subcarrier.
    std::ranges::fill(myYBuf, 0.F);
    for(uInt32 s = 0; s < SAMPLES_PER_CLOCK; ++s) myYBuf[base + s] = 1.F;
    applyLumaFilter(myYBuf.data() + winBase, win);

    // Unit chroma impulse.  S-Video band-limits it; RGB carries it at full
    // bandwidth, so there it stays a box — the crispest colour, and the
    // source of RGB's "least artifacts" character.
    std::ranges::fill(myUBuf, 0.F);
    for(uInt32 s = 0; s < SAMPLES_PER_CLOCK; ++s) myUBuf[base + s] = 1.F;
    if(filterChroma)
      applyChromaFilter(myUBuf.data() + winBase, win);

    constexpr int lumaBase =
      xref * static_cast<int>(SAMPLES_PER_CLOCK) - LUMA_KERNEL_LEFT;
    for(int t = 0; t < LUMA_KERNEL_WIDTH; ++t)
    {
      lumaOut[t] = LumaCoeff{};
      lumaOut[t].yy = myYBuf[static_cast<uInt32>(lumaBase + t)];
    }

    constexpr int chromaBase =
      xref * static_cast<int>(SAMPLES_PER_CLOCK) - CHROMA_KERNEL_LEFT;
    for(int t = 0; t < CHROMA_KERNEL_WIDTH; ++t)
    {
      const auto idx = static_cast<uInt32>(chromaBase + t);
      chromaOut[t] = ChromaCoeff{};
      // Luma and chroma are independent here, so only the diagonal uu / vv
      // terms are non-zero, and V reuses U's kernel.
      chromaOut[t].uu = myUBuf[idx];
      chromaOut[t].vv = myUBuf[idx];
    }
  };

  buildDirectCoeff(true,  mySVLumaCoeff,  mySVChromaCoeff);
  buildDirectCoeff(false, myRGBLumaCoeff, myRGBChromaCoeff);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PALSignal::expandKernels(bool compositeOnly)
{
  // Combine the palette's per-colour YUV with the palette-independent decode
  // coefficients to produce the per-colour scatter kernels used at render.
  // The composite set is taken at the current subcarrier drift step; the
  // S-Video and RGB sets have no subcarrier, so a drift change skips them.
  const auto& lumaCoeff   = myLumaCoeff[myDriftStep];
  const auto& chromaCoeff = myChromaCoeff[myDriftStep];

  for(uInt32 c = 0; c < 256; ++c)
  {
    const float Y = myClockTable[c].y;
    const float U = myClockTable[c].u;
    const float V = myClockTable[c].v;

    for(uInt32 p = 0; p < 4; ++p)
      for(uInt32 vi = 0; vi < 2; ++vi)
      {
        Kernel& k = myKernel[c][p][vi];
        for(int d = 0; d < LUMA_KERNEL_WIDTH; ++d)
        {
          const LumaCoeff& co = lumaCoeff[p][vi][d];
          k.y[d] = co.yy * Y + co.yu * U + co.yv * V;
        }
        for(int d = 0; d < CHROMA_KERNEL_WIDTH; ++d)
        {
          const ChromaCoeff& co = chromaCoeff[p][vi][d];
          k.u[d] = co.uy * Y + co.uu * U + co.uv * V;
          k.v[d] = co.vy * Y + co.vu * U + co.vv * V;
        }
      }

    if(compositeOnly)
      continue;

    Kernel& sv = mySVKernel[c];
    Kernel& rgb = myRGBKernel[c];
    for(int d = 0; d < LUMA_KERNEL_WIDTH; ++d)
    {
      sv.y[d]  = mySVLumaCoeff[d].yy * Y;
      rgb.y[d] = myRGBLumaCoeff[d].yy * Y;
    }
    for(int d = 0; d < CHROMA_KERNEL_WIDTH; ++d)
    {
      sv.u[d]  = mySVChromaCoeff[d].uu * U;
      sv.v[d]  = mySVChromaCoeff[d].vv * V;
      rgb.u[d] = myRGBChromaCoeff[d].uu * U;
      rgb.v[d] = myRGBChromaCoeff[d].vv * V;
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PALSignal::convolve(SpanOf<float> kernel, const float* in, float* out,
                         uInt32 n)
{
  const int half = static_cast<int>(kernel.size()) / 2;

  for(uInt32 i = 0; i < n; ++i)
  {
    float acc = 0.F;
    for(int k = 0; std::cmp_less(k, kernel.size()); ++k)
    {
      const int j = static_cast<int>(i) - k + half;
      if(j >= 0 && std::cmp_less(j, n))
        acc += in[j] * kernel[k];
    }
    out[i] = acc;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PALSignal::applyLumaFilter(float* buf, uInt32 n)
{
  convolve(myLumaKernel, buf, myFilterTmp.data(), n);

  // Aperture correction (sharpness): unsharp-mask with the 3-tap kernel
  // [-k/2, 1+k, -k/2], its outer taps APERTURE_SPACING samples away rather
  // than adjacent — see the APERTURE_SPACING comment for why the spacing is
  // what makes this control do anything at all.  Centre weight 1+k boosts the
  // sample, the outer weights subtract a blurred estimate; the three sum to 1
  // so flat areas (DC) are unchanged and only edges are emphasised.
  std::copy_n(myFilterTmp.data(), n, buf);

  // Samples nearer than APERTURE_SPACING to an end have no pair of
  // neighbours, so they pass through unpeaked.
  constexpr auto reach = static_cast<uInt32>(APERTURE_SPACING);
  for(uInt32 i = reach; i + reach < n; ++i)
    buf[i] = myFilterTmp[i] * (1.F + myApertureK)
           - myApertureK * 0.5F * (myFilterTmp[i-reach] + myFilterTmp[i+reach]);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PALSignal::applyChromaFilter(float* buf, uInt32 n)
{
  convolve(myChromaKernel, buf, myFilterTmp.data(), n);
  std::copy_n(myFilterTmp.data(), n, buf);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PALSignal::render(const uInt8* tiaSrc, uInt32 srcWidth, uInt32 srcHeight,
                       uInt32* rgbDst, uInt32 dstPitch, bool phaseInverted)
{
  // phaseInverted is true when the previous frame had an odd scanline count.
  // It drives two independent things: the per-line PAL V-phase alternation
  // (always, from the real parity — see the loop below), and PAL colour loss
  // via the hysteretic colour-killer updated here.  An odd count breaks the
  // PAL field/burst sequence, so a real set's colour-killer cuts chroma and
  // the frame renders luma-only (greyscale) — but only once the break has
  // persisted, never on a single malformed frame (see COLOUR_KILLER_FRAMES).
  if(phaseInverted != myColourKilled)
  {
    if(++myKillerRun >= COLOUR_KILLER_FRAMES)
    {
      myColourKilled = phaseInverted;
      myKillerRun    = 0;
    }
  }
  else
    myKillerRun = 0;
  const bool   colourKilled = myColourKilled;

  const bool   composite = (myPath == Path::Composite);
  // S-Video and RGB carry chroma off the luma wire, so there is no 1-line
  // comb (blend = 0); only Composite combs against the previous scanline.
  const float  blend  = composite ? (mySetup.blend * 0.5F + 0.5F) : 0.F;
  const uInt32 outW   = outWidth(srcWidth);   // oversampled output width

  // Walk the subcarrier against the pixel grid.  The PAL console's two
  // crystals are independent, so this phase is never stationary; only the
  // composite path carries a subcarrier for it to affect.  See SUBCARRIER
  // DRIFT in PALSignal.hxx.  Re-expanding the kernels is the whole cost, so
  // it happens only when the quantised step actually changes.
  if(composite)
  {
    uInt32 step = 0;
    if(myDriftRate > 0.F)
    {
      myDriftPhase += myDriftRate / PAL_FIELD_RATE;
      myDriftPhase -= std::floor(myDriftPhase);
      step = static_cast<uInt32>(myDriftPhase * PHASE_STEPS) % PHASE_STEPS;
    }
    else
      myDriftPhase = 0.F;

    if(step != myDriftStep)
    {
      myDriftStep = step;
      expandKernels(true);
    }
  }

  // PALSwitch colour-loss model: when the killer has tripped on a composite
  // mode, keep chroma but negate V (reflection about the U axis) on the lines
  // where the simulated PAL-switch bistable is mis-locked — the wrong V-switch
  // sign.  See the PALSWITCH_* block in PALSignal.hxx.
  const bool   palSwitch = colourKilled && composite &&
                           myColourLoss == ColourLoss::PALSwitch;

  // PALSwitch: the switch decodes V with the wrong sign down to a re-lock
  // boundary scanline, which lands PALSWITCH_FLICKER_LINES deeper every other
  // rendered frame (myPALSwitchField).  So the top PALSWITCH_SOLID_LINES are wrong
  // on both frames (steady), the next FLICKER_LINES are wrong on one and right on
  // the next (the magenta↔cyan flicker, ale-79's zone 2).  See PALSignal.hxx.
  uInt32 palSwitchBand = 0;
  if(palSwitch)
  {
    palSwitchBand = PALSWITCH_SOLID_LINES
                  + (myPALSwitchField ? PALSWITCH_FLICKER_LINES : 0);
    myPALSwitchField = !myPALSwitchField;
  }

  // Reset the 1-line comb delay at the start of each frame; the first
  // scanline blends against a virtual "black" previous line.  (S-Video and
  // RGB have no comb, so the delay line is never read.)
  if(composite)
  {
    std::fill(myPrevU.begin(), myPrevU.begin() + outW, 0.F);
    std::fill(myPrevV.begin(), myPrevV.begin() + outW, 0.F);
  }

  for(uInt32 y = 0; y < srcHeight; ++y)
  {
    const uInt8* src = tiaSrc + static_cast<size_t>(y) * srcWidth;
    uInt32*      dst = rgbDst + static_cast<size_t>(y) * dstPitch;

    // PAL V-phase alternation: even lines (relative to field start) use +V,
    // odd lines −V.  phaseInverted tracks absolute field parity so the phase
    // is consistent across frames regardless of variable scanline counts.
    const bool   isEvenLine = ((y & 1U) == 0U) ^ phaseInverted;
    const uInt32 vi = isEvenLine ? 0U : 1U;

    std::fill(myAccY.begin(), myAccY.begin() + outW, 0.F);
    std::fill(myAccU.begin(), myAccU.begin() + outW, 0.F);
    std::fill(myAccV.begin(), myAccV.begin() + outW, 0.F);

    // Scatter each clock's precomputed kernel into the line accumulators.
    // A clock at input column x lands on output samples starting at x·5.
    // Luma and chroma are scattered over their own widths (see the Kernel
    // struct); both clamp their tap range to the visible region, which
    // zero-pads the output at the line edges.
    for(uInt32 x = 0; x < srcWidth; ++x)
    {
      const Kernel& k = composite ? myKernel[src[x]][x & 3U][vi]
                      : (myPath == Path::RGB) ? myRGBKernel[src[x]]
                      :                         mySVKernel[src[x]];
      const int start = static_cast<int>(x * SAMPLES_PER_CLOCK);

      const int lumaBase = start - LUMA_KERNEL_LEFT;
      const int lumaLo   = lumaBase < 0 ? -lumaBase : 0;
      const int lumaLim  = static_cast<int>(outW) - 1 - lumaBase;
      const int lumaHi   = lumaLim < LUMA_KERNEL_WIDTH - 1
                         ? lumaLim : LUMA_KERNEL_WIDTH - 1;
      for(int t = lumaLo; t <= lumaHi; ++t)
        myAccY[static_cast<uInt32>(lumaBase + t)] += k.y[t];

      const int chromaBase = start - CHROMA_KERNEL_LEFT;
      const int chromaLo   = chromaBase < 0 ? -chromaBase : 0;
      const int chromaLim  = static_cast<int>(outW) - 1 - chromaBase;
      const int chromaHi   = chromaLim < CHROMA_KERNEL_WIDTH - 1
                           ? chromaLim : CHROMA_KERNEL_WIDTH - 1;
      for(int t = chromaLo; t <= chromaHi; ++t)
      {
        const auto oj = static_cast<uInt32>(chromaBase + t);
        myAccU[oj] += k.u[t];
        myAccV[oj] += k.v[t];
      }
    }

    // Convert the line to RGB.  S-Video carries Y and C on separate wires,
    // so there is no comb and the chroma passes straight through; Composite
    // applies the PAL 1-line comb blend with the previous line's chroma.
    // The matched kernels have already undone the V-sign, so both axes just
    // average — which softens colour vertically, the comb's visible effect
    // here (see the DECODING section in the header).
    if(colourKilled && composite && myColourLoss == ColourLoss::SaturationLoss)
    {
      // SaturationLoss model (composite modes only): the set's colour-killer
      // has cut chroma, so emit luma only.  S-Video and RGB are immune because
      // chroma is carried off the luma wire with no phase dependency.  The
      // greyscale comes out clean rather than patterned: the chroma trap sits
      // in the luma path whatever the killer is doing, on a real set as here.
      for(uInt32 j = 0; j < outW; ++j)
        dst[j] = toRGB(myAccY[j], 0.F, 0.F);
    }
    else if(!composite)
    {
      // S-Video / RGB: no comb (blend = 0, prev aliases acc), so only this
      // line is read; the per-colour kernel already set the chroma bandwidth.
      convertLine(myAccY.data(), myAccU.data(), myAccV.data(),
                  myAccU.data(), myAccV.data(), 0.F, outW, dst);
    }
    else
    {
      // PALSwitch model: negate this line's demodulated V (reflection about
      // the U axis, U left alone) above the re-lock boundary line palSwitchBand,
      // i.e. where the switch decodes V with the wrong sign (no-op unless the
      // killer has tripped — see `palSwitch`).  The negated V feeds the delay line
      // below, so the comb stays consistent and turns the line-to-line sign
      // inconsistency partly into desaturation, as real hardware does (see the
      // PALSWITCH_* citation in PALSignal.hxx).
      if(palSwitch)
      {
        if(y < palSwitchBand)
          for(uInt32 j = 0; j < outW; ++j)
            myAccV[j] = -myAccV[j];
      }

      convertLine(myAccY.data(), myAccU.data(), myAccV.data(),
                  myPrevU.data(), myPrevV.data(), blend, outW, dst);

      // This line's chroma becomes the delay line for the next scanline.
      std::swap(myAccU, myPrevU);
      std::swap(myAccV, myPrevV);
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PALSignal::convertLine(const float* yBuf, const float* uBuf,
                            const float* vBuf, const float* uPrev,
                            const float* vPrev, float blend,
                            uInt32 n, uInt32* dst)
{
  const float invBlend = 1.F - blend;

  // Comb blend, BT.601 inverse, clamp and pack.  This is pure float math
  // feeding an integer conversion, with no table lookups to serialise it, so
  // the compiler can vectorise the whole loop.  The arithmetic matches
  // toRGB(); see there for the rounding rationale.
  // NOLINTBEGIN(bugprone-incorrect-roundings)
  for(uInt32 j = 0; j < n; ++j)
  {
    const float u = uBuf[j] * invBlend + uPrev[j] * blend;
    const float v = vBuf[j] * invBlend + vPrev[j] * blend;
    const float y = yBuf[j];

    const float r = BSPF::clamp(y              + 1.140F * v, 0.F, 1.F);
    const float g = BSPF::clamp(y - 0.395F * u - 0.581F * v, 0.F, 1.F);
    const float b = BSPF::clamp(y + 2.032F * u,              0.F, 1.F);

    dst[j] = (static_cast<uInt32>(r * 255.F + 0.5F) << 16)
           | (static_cast<uInt32>(g * 255.F + 0.5F) <<  8)
           |  static_cast<uInt32>(b * 255.F + 0.5F);
  }
  // NOLINTEND(bugprone-incorrect-roundings)
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 PALSignal::toRGB(float y, float u, float v)
{
  // Exact inverse of the BT.601 Y′UV matrix used in setPalette():
  // R = Y + 1.140·V, G = Y − 0.395·U − 0.581·V, B = Y + 2.032·U.  Both
  // directions work on gamma-encoded code values, so this is the whole
  // output stage — there is no gamma to undo (see COLOUR SPACE in the
  // header).  Clamp to the legal gamut on the way out.
  const float r = BSPF::clamp(y              + 1.140F * v, 0.F, 1.F);
  const float g = BSPF::clamp(y - 0.395F * u - 0.581F * v, 0.F, 1.F);
  const float b = BSPF::clamp(y + 2.032F * u,              0.F, 1.F);

  // r/g/b are clamped non-negative above, so +0.5 truncation rounds
  // identically to std::lround, which GCC cannot inline here (no x86
  // instruction has its round-half-away-from-zero semantics) — that would
  // cost three libm calls per output pixel.
  // NOLINTBEGIN(bugprone-incorrect-roundings)
  return (static_cast<uInt32>(r * 255.F + 0.5F) << 16)
       | (static_cast<uInt32>(g * 255.F + 0.5F) <<  8)
       |  static_cast<uInt32>(b * 255.F + 0.5F);
  // NOLINTEND(bugprone-incorrect-roundings)
}
