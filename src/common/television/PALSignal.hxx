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

#ifndef PAL_SIGNAL_HXX
#define PAL_SIGNAL_HXX

#include "bspf.hxx"
#include "TVAdjustable.hxx"
#include "TVMode.hxx"

class Settings;

/**
  Accurate PAL video encoder/decoder for the Atari 2600 TIA.

  Unlike a palette-based "fake TV" filter, this class models the real signal
  chain: it builds the analogue composite waveform a PAL TIA would emit, then
  decodes it the way a PAL receiver would.  Colour fringing, cross-colour on
  sharp luma edges, bandwidth softening and the vertical chroma averaging of
  the delay line all fall out of that chain rather than being hand-drawn.
  Receiver *behaviour* that is not a consequence of the chain — colour loss on
  malformed fields and the PAL-switch model behind it — is added explicitly
  and is labelled as such where it appears.  The math below is standard
  analogue-television theory; the specific numbers and their sources are
  called out so they can be checked.

  SIGNAL MODEL — the 5/4 sample grid
  ─────────────────────────────────────────────────────────────────────────
  The PAL colour subcarrier is fsc = 4.43361875 MHz (the value defined by the
  PAL standard, ITU-R BT.470).  The PAL-TIA colour clock is 3.546894 MHz.
  Their ratio is 5/4 to within ~3e-5 %:

      fsc  =  TIA_clock × 5/4   →   4·fsc = TIA_clock × 5

  We work at an internal sample rate of 4·fsc = 5·TIA_clock = 17.734472 MHz,
  which is what makes the whole model cheap and exact:
    • exactly 4 samples per subcarrier cycle  → the quadrature carrier is a
      4-entry table (no trig at sample time, no fractional-phase error);
    • exactly 5 samples per TIA colour clock  → every TIA pixel maps to a
      whole number of samples, so encode/decode never straddle a sample.

  The grid is fixed, but the subcarrier's PHASE on it is not — see
  SUBCARRIER DRIFT below.  This is where PAL differs fundamentally from
  NTSC, and getting it backwards was a long-standing error here.

  SUBCARRIER DRIFT — the PAL console has two crystals
  ─────────────────────────────────────────────────────────────────────────
  On an NTSC console a single 3.579545 MHz crystal *is* the subcarrier and
  also, divided, the pixel clock: the two are locked by construction, so the
  artifact pattern is rock-steady.  The PAL console is not built that way.
  Its motherboard (CO12283 rev. B) carries TWO independent oscillators:

      Y200  3.546894 MHz (C016112)  →  TIA OSC pin 11   — pixel clock
      Y201  4.433619 MHz            →  TIA PALS pin 12 / PALI pin 8
                                       (pins that exist only on the PAL TIA)

  There is no PLL and no divider between them.  So although the nominal
  frequencies are in exact 5:4 ratio, the actual phase of the subcarrier
  relative to the pixel grid *walks*, at whatever beat the two crystals
  happen to produce: 1.25 Hz for perfect parts, tens of Hz for ordinary
  crystal tolerance.  Hue is unaffected — the receiver demodulates against
  the burst, which comes from the same oscillator as the chroma — but
  everything measured against the PIXEL grid moves: cross-colour on fine
  luma detail rolls slowly through hues, and edge fringing shimmers.  That
  slow, restless quality is a signature of the real PAL machine, and it is
  also why NTSC-style stable artifact colouring was never usable on PAL.

  render() models this by walking the encode/decode subcarrier phase a
  little each frame; see PHASE_STEPS and setDriftRate().

  COLOUR SPACE — BT.601 Y′UV
  ─────────────────────────────────────────────────────────────────────────
  RGB is converted to luma + colour-difference using the BT.601 weightings,
  with the classic analogue U,V scale factors (U = 0.436·(B−Y)/0.886,
  V = 0.615·(R−Y)/0.701) that bound the composite amplitude:

      Y =  0.299 R + 0.587 G + 0.114 B
      U = −0.147 R − 0.289 G + 0.436 B
      V =  0.615 R − 0.515 G − 0.100 B

  toRGB() applies the exact inverse of this matrix.  All of it is done on
  the palette's gamma-encoded code values, NOT in linear light.  That is the
  physically correct domain: everything this class models — band-limiting,
  the chroma filters, the delay-line comb, peaking — happens electrically,
  on a VOLTAGE, upstream of the picture tube.  The CRT's nonlinearity comes
  after, and display code values are already the gamma-encoded quantity, so
  they stand in for those voltages directly.  Filtering linear light instead
  would model a receiver that linearises, filters and re-encodes, which no
  analogue set does; it renders every dithered texture, band-limited edge
  and comb-averaged colour too bright (a 50/50 black/white mix lands at 173
  instead of 128).  Every comparable implementation — PAL-CRT, PALcolour,
  and this codebase's own AtariNTSC — likewise filters code values.

  ENCODING
  ─────────────────────────────────────────────────────────────────────────
  Each TIA colour-clock sample becomes one point of the composite waveform:

      s(t) = Y + U·cos(2π·fsc·t) − V·sin(2π·fsc·t)

  On our grid 2π·fsc·t collapses to the 4-entry cos/sin tables above.  The V
  term is negated on alternate scanlines — the defining "Phase Alternating
  Line" law, tracked here by the vSign / isEvenLine logic.

  DECODING
  ─────────────────────────────────────────────────────────────────────────
  Chroma is recovered by synchronous demodulation: multiply the composite by
  cos (→ U) and −sin (→ V), which shifts the wanted sideband to baseband and
  the unwanted products to 2·fsc, then low-pass filter them away.  A one-line
  (PAL-D delay-line) comb then averages the current line against the previous
  one.  The V-sign is already undone by the matched kernels, so both axes
  simply add:

      U_out = (U_current + U_previous) / 2
      V_out = (V_current + V_previous) / 2

  In a real receiver this comb is what turns a line-to-line differential
  phase error into a mild desaturation instead of the alternating hue error
  known as Hanover bars.  This model has no such error to cancel — the grid
  is exact and the kernels are matched per V-sign — so here the comb
  contributes only its other, equally real effect: vertical chroma averaging.
  That is why Composite resolves colour more softly in the vertical direction
  than S-Video, which skips the comb entirely.

  Luma on the composite path is the composite waveform itself, so before it
  can be used as luminance the subcarrier has to be taken back out of it.
  We do that by SUBTRACTION rather than with a notch: the chroma the decoder
  just recovered is re-modulated onto the subcarrier and subtracted from the
  composite, leaving luma.

      luma = composite − ( U·cos(ωt) − vSign·V·sin(ωt) )

  This is the architecture of the PALcolour decoder (ld-chroma-decoder, from
  W.A. Steer's work with BBC pedigree), and it is chosen over a band-stop
  filter for a measured reason.  Whatever the chroma path extracts at fsc is
  exactly what gets subtracted, so the null at the subcarrier is essentially
  perfect (measured |H(fsc)| = 4e-4) — but unlike a trap wide enough to be
  built from a short FIR, it does not flatten the whole upper band to buy
  that null.  Luma reaches −3 dB at 2.16 MHz here, against 1.50 MHz for the
  5-tap trap cascade this replaced; at 1.77 MHz — the fundamental of a
  one-clock dither, the most common fine detail a 2600 draws — it passes
  0.80 where the trap passed 0.59.  That figure also lands on top of PAL-CRT,
  the closest comparable real-world implementation, whose luma equaliser is
  flat to 1.89 MHz and down 17 dB past 3.32 MHz.

  Without any such removal the subcarrier survives into the picture as a
  full-amplitude mesh over every coloured area — the classic cross-luminance
  artifact, and once a real bug here.  S-Video and RGB carry chroma on their
  own wires, so nothing is subtracted from their luma and they keep the full
  low-pass bandwidth; that is the physical reason they look sharper than
  composite, and it is modelled rather than asserted.

  Per-channel bandwidth is set by windowed-sinc low-pass filters:

      Luma:    5.0 MHz baseband (nominally PAL-B/G; PAL-I allows 5.5), which
               a 7-tap kernel realises at −3 dB ≈ 4.0 MHz.  On the composite
               path the chroma subtraction then dominates, as above.
      Chroma:  −3 dB ≈ 1.2 MHz per colour-difference axis; ITU-R BT.470
               gives the sidebands as +1.07/−1.30 MHz.

  Kernel lengths matter as much as the cutoffs here: a windowed-sinc FIR only
  realises its design cutoff if it is long enough, and a too-short kernel
  silently lands somewhere much wider.  The tap counts below are chosen from
  the measured response, not from the nominal cutoff.

  PERFORMANCE — precomputed per-colour kernels
  ─────────────────────────────────────────────────────────────────────────
  The whole chain (encode → demodulate → FIR → chroma subtraction → comb) is
  *linear* in each input clock's (Y,U,V).  By superposition, one clock's
  contribution to the output samples around it is therefore a fixed kernel
  that depends only on its colour, its column phase (x & 3), the PAL V-sign
  and the current subcarrier drift step.  We characterise the chain once with
  unit impulses (buildCoeff), bake those into per-colour kernels
  (expandKernels), and at render time merely scatter-add kernels — no
  per-pixel filtering.  This mirrors the approach used by AtariNTSC for the
  NTSC (Blargg) path.

  Luma and chroma reach different distances (the chroma FIR is long, and it
  feeds the luma subtraction, so luma reaches further still), so they carry
  separate kernel widths rather than padding both out to the larger one.

  WHAT THIS MODEL DOES AND DOES NOT PRODUCE
  ─────────────────────────────────────────────────────────────────────────
  Emerges from the signal chain:
    • Colour fringing at sharp horizontal edges (band-limited chroma)
    • Cross-colour — luma detail near fsc demodulating into chroma
    • Cross-luminance residue at chroma edges — what the subtraction cannot
      cancel, which is what a real receiver leaves behind too
    • Bandwidth softening of luma and chroma; more of it on composite,
      because chroma subtraction costs luma resolution
    • Vertical chroma averaging from the 1-line comb
    • Slow drift of all of the above, from the free-running subcarrier
      crystal (see SUBCARRIER DRIFT)

  Added explicitly, because the chain cannot produce it:
    • Colour loss on a sustained odd scanline count, via a hysteretic
      colour-killer that ignores single malformed frames (see render())

  Deliberately absent — do not "restore" these:
    • Hanover bars.  These require a differential phase error, which this
      model does not have — so there is also nothing for the comb to
      suppress, and no claim should be made that it does.
    • Full-amplitude cross-luminance (subcarrier as a mesh over coloured
      areas).  A real set removes it and so do we; a visible mesh would be
      a bug, and once was.

  OUTPUT
  ─────────────────────────────────────────────────────────────────────────
  render() emits the internal oversampled grid directly — SAMPLES_PER_CLOCK
  output pixels per TIA colour clock (see outWidth()) — rather than resampling
  back down.  The reason is *not* "to preserve subcarrier-rate detail": the
  chroma subtraction deliberately removes that, exactly as a real set does.
  It is:
    • The internal rate is pinned at 4 samples per subcarrier cycle, which on
      the 5/4 grid means 5 per colour clock.  Only 1× and 5× are integer
      decimations of that.  1× folds real luma content back into the picture
      (|H| ≈ 0.64 at its Nyquist); anything in between needs a resampler.
    • A display upscaler is not an ideal reconstruction filter, so a wider
      source grid still looks better than a narrow one carrying identical
      information.  This is also why AtariNTSC emits 568 rather than 160.

  A "sharpness" control adds aperture correction to the luma channel, matching
  the peaking circuits in real PAL television sets.  Its tap spacing is set so
  that the boost lands in the band the picture actually occupies, and so that
  it is transparent at the subcarrier — see APERTURE_SPACING.
*/
class PALSignal
{
  public:
    // Normalized parameter pack for one PAL signal quality level.
    // User-exposed fields are in [-1..1] so that TVSignal's scaleTo100/
    // scaleFrom100 helpers apply without change.
    //
    // saturation and hue are held at neutral values; PaletteHandler owns
    // those dimensions for the user and applies them to the palette before
    // it reaches the composite pipeline.
    //
    // Physical-value mappings:
    //   sharpness  : [-1..1]      luma aperture correction (0 = flat)
    //   saturation : neutral 0.0  → physical ×1.0 (sat + 1.0)
    //   hue        : neutral 0.0  → physical 0°   (hue × 180°)
    //   blend      : [-1..1]      comb blend;  physical = blend × 0.5 + 0.5
    struct Setup {
      float sharpness  { 0.2F };
      float saturation { 0.F  };
      float hue        { 0.F  };
      float blend      { 0.F  };
    };

    // Indices into the adjustableTags() span; used by GlobalKeyHandler
    enum class Adjustables: uInt8 {
      SHARPNESS,
      BLEND,
      NUM_ADJUSTABLES
    };

    // Named-field struct for GUI dialogs that read/write PAL adjustables
    struct Adjustable {
      uInt32 sharpness{0}, blend{0};
    };

    // Preset Setup values for each TVMode, cleanest first.  RGB and S-Video
    // carry chroma off the luma wire so they never comb (blend is unused);
    // RGB additionally carries chroma at full bandwidth, so it is the crispest
    // and takes the strongest aperture peaking.
    static constexpr Setup TV_RGB      {  0.4F, 0.F, 0.F, 0.F  };
    static constexpr Setup TV_SVideo   {  0.3F, 0.F, 0.F, 0.F  };
    static constexpr Setup TV_Composite{  0.2F, 0.F, 0.F, 0.F  };

    // PAL colour-loss model: how a receiver reacts once a sustained malformed
    // (odd-clock) field trips the colour-killer in render().  Real PAL CRTs are
    // documented doing either, so it is a user choice; SaturationLoss is the
    // default and preserves the long-standing behaviour.
    enum class ColourLoss: uInt8 {
      SaturationLoss,   // chroma cut → luma-only greyscale frame (classic loss)
      PALSwitch,        // chroma kept; the PAL-switch bistable decodes V with the
                        // wrong sign while re-locking → wrong-hue band at the top
                        // + mid-band flicker, settling to correct (the mechanism-
                        // faithful model; constants at PALSWITCH_* below)
      NUM_MODELS
    };

  public:
    PALSignal();
    ~PALSignal() = default;

    // Apply the Setup corresponding to the given TVMode; rebuilds kernels.
    void initialize(TVMode mode);

    // Re-apply myCustomSetup after a tag value has been edited in-place.
    void reinitializeCustom();

    // Span of {name, float*} entries for the custom adjustables.
    // Pointers are into the static myCustomSetup, so they remain valid.
    static SpanOf<AdjustableTag> adjustableTags() { return ourCustomAdjustables; }

    // Get the named adjustables for a given TVMode (for GUI dialogs)
    static void getAdjustables(Adjustable& adjustable, TVMode mode);

    // Set the shared custom setup from named adjustable values
    static void setCustomAdjustables(const Adjustable& adjustable);

    // Load and save PAL-related settings
    static void loadConfig(const Settings& settings);
    static void saveConfig(Settings& settings);

    // Set the PAL colour-loss model (see ColourLoss).  Global receiver choice,
    // applied by render() and persisted by saveConfig.
    static void setColourLoss(int model);

    // Set how fast the subcarrier phase walks against the pixel grid, in Hz
    // (0 disables it and pins the phase, as an NTSC console's would be).
    // This is a property of the console, not of the TV, so like the colour-loss
    // model it is global rather than part of a TVMode preset.  See the
    // SUBCARRIER DRIFT section above for why a PAL console drifts at all, and
    // DEF_DRIFT_HZ for where the default comes from.
    static void setDriftRate(float hz);

    // Nominal beat between the two PAL-console crystals: the 4.433618.75 MHz
    // subcarrier against 5/4 of the 3.546894 MHz pixel clock (= 4.4336175 MHz)
    // is 1.25 Hz, so the pattern works through a full subcarrier cycle in about
    // 0.8 s.  This is the *design* figure, for parts exactly on frequency; real
    // crystal tolerance dominates it (±10 ppm alone allows ~44 Hz) and varies
    // from console to console, so no single value can be right for every unit.
    // The nominal one is used because it is the only non-arbitrary choice, and
    // because it errs slow: it adds life without becoming a distraction.
    static constexpr float DEF_DRIFT_HZ = 1.25F;

    // Rebuild the per-colour YUV table (and the dependent kernels) from the
    // given RGB palette.  The palette is display-gamma encoded, so each entry
    // is linearised, converted to BT.601 Y′UV, and scaled by saturation/hue
    // before encoding; see PALSignal.cxx for the matrices.  Cheap relative to
    // buildCoeff(), so this is the path the palette/sat/hue sliders take.
    void setPalette(IntSpan palette);

    // Render one complete frame.
    //   tiaSrc   : raw TIA colour-index bytes, srcWidth × srcHeight
    //   rgbDst   : destination 0x00RRGGBB pixels, dstPitch pixels wide
    //   phaseInverted: true when the previous frame had an odd scanline count.
    //              Drives PAL V-phase alternation on all modes (per-line, from
    //              the real parity).  Also feeds the hysteretic colour-killer
    //              that triggers PAL colour loss on composite modes (Composite,
    //              Custom): a sustained odd count gives an inconsistent
    //              PAL field/burst sequence, so a real set's colour-killer cuts
    //              chroma and the frame is rendered as luma-only greyscale.  A
    //              single malformed frame is ignored (see COLOUR_KILLER_FRAMES).
    //              S-Video and RGB are immune (chroma is carried off the luma
    //              wire, so there is no colour-killer mechanism).
    void render(const uInt8* tiaSrc, uInt32 srcWidth, uInt32 srcHeight,
                uInt32* rgbDst, uInt32 dstPitch, bool phaseInverted);

    // Width of one TIA colour-clock scanline in pixels (input to render)
    static constexpr uInt32 TIA_WIDTH = 160;

    // Output width for a given input width.  The output is the internal
    // oversampled grid itself (SAMPLES_PER_CLOCK output pixels per TIA colour
    // clock), with no downsample.  See the OUTPUT section of the class
    // comment for why the grid is not narrowed.
    static constexpr uInt32 outWidth(uInt32 inWidth) {
      return inWidth * SAMPLES_PER_CLOCK;
    }

  private:
    // ── Frequency constants ───────────────────────────────────────────────

    // PAL TIA colour clock (crystal frequency).  Documentation only — the
    // model works entirely in ratios, so nothing reads this.
    static constexpr float TIA_FREQ    = 3'546'894.F;   // Hz

    // PAL colour subcarrier, the published 4'433'618.75 Hz to Hz precision.
    // Note this is NOT bit-exactly TIA_FREQ × 5/4 (= 4'433'617.5): the three
    // values — published fsc, 5/4·clock, and the constant below — sit within
    // ~1e-5 % of each other.  That never matters, because every use is a
    // ratio against SAMPLE_RATE, in which the scale cancels.
    static constexpr float FSC         = 4'433'618.F;   // Hz

    // Internal sample rate: 4 samples per subcarrier cycle, which on the
    // idealised 5/4 grid is 5 samples per TIA colour clock.
    static constexpr float SAMPLE_RATE = 4.F * FSC;     // 17,734,472 Hz

    // Samples per TIA colour clock at SAMPLE_RATE (exact integer = 5)
    static constexpr uInt32 SAMPLES_PER_CLOCK = 5;

    // Samples per scanline at SAMPLE_RATE
    // 228 TIA clocks/line × 5 samples/clock = 1140
    static constexpr uInt32 SAMPLES_PER_LINE = 228 * SAMPLES_PER_CLOCK;

    // Visible samples per scanline (160 TIA clocks × 5)
    static constexpr uInt32 VISIBLE_SAMPLES  = TIA_WIDTH * SAMPLES_PER_CLOCK;

    // ── Filter kernel lengths ─────────────────────────────────────────────

    // Luma low-pass kernel length (samples at SAMPLE_RATE).  The receiver's
    // video bandwidth, nominally 5.0 MHz for PAL-B/G; 7 taps realise −3 dB at
    // 4.0 MHz.  Used by every path — the composite path applies it after the
    // chroma subtraction rather than instead of it.
    static constexpr uInt32 LUMA_TAPS   = 7;

    // Chroma low-pass kernel length.  Target is ~1.1–1.2 MHz per axis; 11
    // taps measures −3 dB ≈ 1.21 MHz.  (7 taps lands at 1.7 MHz — the design
    // cutoff alone does not set the response.)  This filter sets the reach of
    // BOTH the chroma kernels and, through the subtraction, the luma one.
    static constexpr uInt32 CHROMA_TAPS = 11;

    // ── Aperture correction (the "sharpness" control) ─────────────────────
    //
    // The unsharp kernel [−k/2, 1+k, −k/2] is applied at this tap SPACING,
    // not at adjacent samples, and the spacing is what decides which
    // frequencies it lifts: the boost is 1 + k·(1 − cos(2π·f·spacing/fs)),
    // peaking at fs/(2·spacing).  At 1 sample that peak is 8.87 MHz — a band
    // the picture does not reach, which made the control very nearly a no-op
    // (+1.4% at 1.5 MHz even at the slider's default).  4 samples puts it at
    // 2.22 MHz, in the band the chroma subtraction is rolling off, which is
    // also where real receivers peak.
    //
    // 4 samples is exactly one subcarrier cycle, so the kernel is transparent
    // at fsc (boost = 1 there, cos of a full turn): peaking cannot lift any
    // residual subcarrier back out of the luma it was just removed from.
    static constexpr int APERTURE_SPACING = 4;

    // ── Per-clock pre-computed tables ─────────────────────────────────────

    // One entry per TIA colour-clock = {luma, u_amplitude, v_amplitude}
    // u/v amplitudes are signed; multiply by the appropriate subcarrier
    // sample (cos or sin) when encoding.
    struct ClockEntry {
      float y{};   // luma [0..1]
      float u{};   // BT.601 U colour-difference (pre-scaled by saturation)
      float v{};   // BT.601 V colour-difference (pre-scaled by saturation)
    };
    std::array<ClockEntry, 256> myClockTable{};

    // ── Subcarrier drift ──────────────────────────────────────────────────
    //
    // The number of discrete subcarrier phases the drift is quantised to.
    // The phase advances continuously (myDriftPhase) but the kernels can only
    // be built for a finite set of phases, so render() snaps to the nearest.
    // 32 steps is 11.25° apart, which at the nominal beat moves the picture
    // by only a couple of code values at a time — the drift reads as movement
    // rather than as switching.  The cost is per-step coefficients (a few
    // tens of KB, built in well under a millisecond each), not per-frame work:
    // render() re-expands the kernels only when the step actually changes.
    static constexpr uInt32 PHASE_STEPS = 32;

    // Quadrature subcarrier samples, per drift step.  The carrier still has
    // period 4 in the sample index whatever its phase offset, so each step is
    // just a 4-entry table and there is still no trig at sample time.  Step 0
    // is the un-drifted grid: cos = {1, 0, -1, 0}, sin = {0, 1, 0, -1}.
    std::array<std::array<float, 4>, PHASE_STEPS> myCarrierCos{};
    std::array<std::array<float, 4>, PHASE_STEPS> myCarrierSin{};

    // ── Filter kernels ────────────────────────────────────────────────────

    // Receiver video bandwidth; used by every path (see LUMA_TAPS)
    std::array<float, LUMA_TAPS>   myLumaKernel{};

    std::array<float, CHROMA_TAPS> myChromaKernel{};

    // Aperture correction kernel strength (3-tap: [−k/2, 1+k, −k/2])
    float myApertureK{};

    // ── Per-output-pixel kernels (AtariNTSC-style precomputation) ─────────
    //
    // Every stage from composite encode through comb/low-pass decode is
    // *linear* in each input clock's (Y, U, V).  So the contribution of one
    // input clock to the YUV of the output pixels around it is a fixed kernel
    // that depends only on the clock's colour, its column phase (x & 3) and the
    // PAL V-sign.  At render time we simply scatter-add each clock's kernel into
    // the line accumulators; no per-pixel filtering is performed.  See
    // buildCoeff() and expandKernels().

    // Sample-domain reach of one clock's energy.  The FIRs have finite
    // support, so beyond this there is exactly zero overlap — the kernel is
    // not merely small but identically zero.
    //
    // Chroma reaches one chroma-FIR half-width.  Luma reaches further,
    // because on the composite path the luma is the composite MINUS the
    // filtered, re-modulated chroma: it inherits the chroma FIR's spread,
    // then the luma low-pass on top of it, then the aperture kernel.  Deriving
    // both from the filters that produce them means changing a tap count
    // cannot silently leave the grid under-reaching.
    static constexpr int CHROMA_REACH = CHROMA_TAPS / 2;
    static constexpr int LUMA_REACH   =
      CHROMA_TAPS / 2 + LUMA_TAPS / 2 + APERTURE_SPACING;

    // Since the output is the oversampled grid itself, one clock contributes
    // to its own SAMPLES_PER_CLOCK output samples plus its reach on each side.
    // The LEFT values are the offset of the first tap relative to the clock's
    // first output sample.
    static constexpr int LUMA_KERNEL_LEFT    = LUMA_REACH;
    static constexpr int LUMA_KERNEL_WIDTH   =
      static_cast<int>(SAMPLES_PER_CLOCK) + 2 * LUMA_REACH;
    static constexpr int CHROMA_KERNEL_LEFT  = CHROMA_REACH;
    static constexpr int CHROMA_KERNEL_WIDTH =
      static_cast<int>(SAMPLES_PER_CLOCK) + 2 * CHROMA_REACH;

    // One clock's contribution to the output samples around it.  Luma and
    // chroma are held at their own widths: padding the chroma arrays out to
    // the luma width would put a third of the scatter loop's work into taps
    // that are known to be zero.
    struct Kernel {
      std::array<float, LUMA_KERNEL_WIDTH>   y{};
      std::array<float, CHROMA_KERNEL_WIDTH> u{};
      std::array<float, CHROMA_KERNEL_WIDTH> v{};
    };

    // Palette-independent linear decode coefficients: each output component
    // at a given offset as a linear combination of the clock's input (y,u,v).
    // Depends only on the filters (sharpness/bandwidth) and the subcarrier
    // phase, so they are rebuilt only when those change, not per palette.
    struct LumaCoeff {
      float yy{}, yu{}, yv{};   // Yout = yy·y + yu·u + yv·v
    };
    struct ChromaCoeff {
      float uy{}, uu{}, uv{};   // Uout = uy·y + uu·u + uv·v
      float vy{}, vu{}, vv{};   // Vout = vy·y + vu·u + vv·v
    };

    // Composite coefficients: [driftStep][columnPhase 0..3][vSign 0..1][offset]
    //
    // Every one of these terms moves with the subcarrier phase, so the whole
    // set is held per drift step rather than derived from step 0.  It is
    // tempting to rotate the cross-colour terms instead — (uy,vy) really does
    // rotate exactly with the phase — but the chroma gain of a SHORT run of
    // clocks moves too (an isolated clock's five samples land on different
    // points of the carrier), and that is a real effect on narrow objects.
    // The table is small; the per-colour kernels below are what cost memory.
    template<typename T, size_t W> using CoeffSet =
      std::array<std::array<std::array<std::array<T, W>, 2>, 4>, PHASE_STEPS>;
    CoeffSet<LumaCoeff, LUMA_KERNEL_WIDTH>     myLumaCoeff{};
    CoeffSet<ChromaCoeff, CHROMA_KERNEL_WIDTH> myChromaCoeff{};

    // S-Video coefficients (no subcarrier → phase/vSign/drift independent)
    std::array<LumaCoeff, LUMA_KERNEL_WIDTH>     mySVLumaCoeff{};
    std::array<ChromaCoeff, CHROMA_KERNEL_WIDTH> mySVChromaCoeff{};
    // RGB coefficients: S-Video luma (FIR + aperture) but full-bandwidth
    // chroma (separate wires, no chroma FIR); phase/vSign independent.
    std::array<LumaCoeff, LUMA_KERNEL_WIDTH>     myRGBLumaCoeff{};
    std::array<ChromaCoeff, CHROMA_KERNEL_WIDTH> myRGBChromaCoeff{};

    // Per-colour expanded kernels, rebuilt on any palette, coeff or drift
    // change.  Composite: [colour][columnPhase 0..3][vSign 0..1]
    std::array<std::array<std::array<Kernel, 2>, 4>, 256> myKernel{};
    // S-Video: [colour]
    std::array<Kernel, 256> mySVKernel{};
    // RGB: [colour]
    std::array<Kernel, 256> myRGBKernel{};

    // ── Scratch buffers (used only while building coefficients) ───────────

    // Isolated-clock composite waveform (SAMPLES_PER_LINE)
    std::vector<float> myCurrentLine;

    // FIR work buffers (VISIBLE_SAMPLES)
    std::vector<float> myYBuf, myUBuf, myVBuf, myFilterTmp;

    // ── Per-line render accumulators (output-width = VISIBLE_SAMPLES) ─────
    std::vector<float> myAccY, myAccU, myAccV;
    // Previous line's filtered chroma, for the 1-line PAL comb blend
    std::vector<float> myPrevU, myPrevV;

    // Which signal path the active mode renders.  Composite runs the full
    // subcarrier encode/decode + 1-line comb.  S-Video and RGB skip the comb
    // (chroma is carried off the luma wire), differing only in chroma
    // bandwidth: S-Video band-limits it through the chroma FIR; RGB carries it
    // on its own wires at full bandwidth.
    enum class Path: uInt8 { Composite, SVideo, RGB };
    Path myPath{Path::Composite};

    // ── Colour-killer hysteresis ──────────────────────────────────────────
    //
    // A real PAL receiver's colour-killer is an analog integrator with a time
    // constant of a few fields and built-in on/off hysteresis, so it ignores a
    // single malformed (odd-scanline) frame and does not chatter when the
    // scanline count alternates odd/even.  We model that minimally: the
    // killed/active state flips only after COLOUR_KILLER_FRAMES consecutive
    // frames demand the opposite state.  This is decoupled from the per-line
    // V-sign parity, which always tracks the real frame parity (see render()).
    //
    // COLOUR_KILLER_FRAMES is the transient-rejection floor, NOT a measured
    // hardware figure: 2 is the minimum that rejects a one-frame glitch and
    // prevents field-rate flicker on an alternating count.  Real killer time
    // constants are receiver-specific, so a larger value has no basis.
    static constexpr uInt32 COLOUR_KILLER_FRAMES = 2;
    bool   myColourKilled{false};   // current killer state (true = chroma cut)
    uInt32 myKillerRun{0};          // consecutive frames demanding the flip
    bool   myPALSwitchField{false}; // PALSwitch per-frame field-parity toggle

    // ── Subcarrier drift state ────────────────────────────────────────────
    //
    // myDriftPhase is the subcarrier's phase against the pixel grid, in
    // cycles, advanced by render() once per frame; myDriftStep is the
    // quantised PHASE_STEPS index the kernels are currently expanded for.
    float  myDriftPhase{0.F};
    uInt32 myDriftStep{0};

    // Beat frequency between the console's two crystals, in Hz (see
    // setDriftRate).  Static + loaded from settings, mirroring myColourLoss.
    static inline float myDriftRate{DEF_DRIFT_HZ};

    // ── PAL-switch model constants (ColourLoss::PALSwitch) ─────────────────
    //
    // The mechanism-faithful model of the "wrong-hue-that-settles" class of PAL
    // receivers (the alternative to SaturationLoss's whole-frame greyscale).  It
    // models the receiver's PAL-switch bistable: thrown into the wrong phase at
    // the top of a malformed field, it decodes V with the wrong SIGN — a
    // reflection about the U axis (V → −V, U unchanged) — until it re-locks.
    // render() negates myAccV on the mis-locked lines, before the comb
    // (consecutive wrong lines, so the PAL 1-line comb reinforces a full hue
    // reflection; an isolated wrong line would instead be averaged toward grey).
    //
    // The re-lock boundary is a scanline: the switch is wrong above it, locked
    // below.  Because the receiver's field alternation throws it the opposite way
    // each field, that boundary lands a few lines DEEPER every other rendered
    // frame (myPALSwitchField).  Two boundaries → three zones:
    //   • top SOLID_LINES    : wrong on both frames → steady wrong hue
    //   • next FLICKER_LINES : wrong on one frame, right on the next → flickers
    //                          magenta↔cyan (ale-79's zone 2, hard to see in a
    //                          still, exactly as he reports)
    //   • below              : locked → correct colour
    //
    // GROUNDED: a PAL-switch bistable mis-locked at field start and re-locking
    // over several lines, alternating with field parity, is the source below; a
    // wrong V-switch sign → V reflection is that report's PAL-switch mechanism.
    // (A subcarrier reference-phase error, by contrast, does NOT shift hue at all
    // — the PAL delay-line comb turns it into a cos θ desaturation.)
    // PHENOMENOLOGICAL: the hard per-frame boundary (vs a soft settling transient)
    // and the line counts — tuned to reference CRT captures, not derived (may
    // become user adjustables later).  Band height varies per game on real sets
    // (it depends on where the field-start disturbance falls relative to the
    // visible window); these fixed counts are a compromise across games.
    //
    // Source: BBC RD 1986/2, C.K.P. Clarke, "Colour encoding and decoding
    // techniques for line-locked sampled PAL and NTSC television signals" (BBC
    // Research Dept., Mar 1986), downloads.bbc.co.uk/rd/pubs/reports/1986-02.pdf
    //   - reference-phase error → cos θ desaturation: §4.3, p.21 (Fig 27)
    //   - PAL-switch bistable, reset when wrong for several lines: §4.3.2,
    //     Fig 32(a), p.24
    //   - ~10-line underdamped re-lock (the loop's control filter, a P+I loop):
    //     §4.3.2, Fig 32(b), p.24
    //
    // Both in scanlines: SOLID_LINES = the steady wrong band at the top;
    // FLICKER_LINES = the flickering band just below it.
    static constexpr uInt32 PALSWITCH_SOLID_LINES   = 55;  // solid band (scanlines)
    static constexpr uInt32 PALSWITCH_FLICKER_LINES = 12;  // flicker band (scanlines)

    // ── Active and custom setups ──────────────────────────────────────────

    // Active setup (set by initialize or reinitializeCustom)
    Setup mySetup{TV_Composite};

    // Persistent custom setup written by setCustomAdjustables/loadConfig
    static inline Setup myCustomSetup{TV_Composite};

    // PAL colour-loss model (global receiver choice, not part of a TVMode
    // preset).  Static + loaded from settings, mirroring myCustomSetup.
    static inline ColourLoss myColourLoss{ColourLoss::SaturationLoss};

    // AdjustableTags pointing into myCustomSetup for the cycling UI
    static constexpr std::array<AdjustableTag, 2> ourCustomAdjustables = {{
      { "sharpness", &myCustomSetup.sharpness },
      { "blend",     &myCustomSetup.blend     }
    }};

    // ── Private methods ───────────────────────────────────────────────────

    // Build the windowed-sinc low-pass FIR kernels (luma 5.0 MHz, chroma
    // 1.3 MHz at SAMPLE_RATE).  The cutoffs are fixed by the PAL spec, so
    // the kernels are setup-independent and built once at construction.
    void buildLumaKernel();
    void buildChromaKernel();

    // Build the per-drift-step quadrature carrier tables; setup-independent,
    // so likewise built once at construction.
    void buildCarrierTables();

    // Apply everything that depends on mySetup: the aperture-correction
    // strength, the decode coefficients and the per-colour kernels.
    void applySetup();

    // Characterise the linear encode→demod→FIR→subtract chain with unit
    // (y,u,v) impulses to get the palette-independent decode coefficients,
    // for every drift step.  Depends only on the filters, so call after
    // buildLumaKernel/buildChromaKernel and re-run only when those change.
    void buildCoeff();

    // Fold the per-colour palette YUV (myClockTable) through the decode
    // coefficients for the current drift step to produce the ready-to-scatter
    // per-colour kernels.  Cheap; call after setPalette(), buildCoeff(), or a
    // change of drift step.  Composite kernels only when `compositeOnly`, the
    // drift case — S-Video and RGB have no subcarrier to drift.
    void expandKernels(bool compositeOnly = false);

    // Convolve in (length n) with a symmetric FIR kernel into out, treating
    // samples outside [0..n) as zero.  Used only while building coefficients.
    static void convolve(SpanOf<float> kernel, const float* in, float* out,
                         uInt32 n);

    // Apply the luma FIR then the aperture-correction (unsharp) pass to a
    // sample buffer in-place.  Used only while building coefficients.
    void applyLumaFilter(float* buf, uInt32 n);

    // Apply the chroma FIR to a sample buffer in-place (build-time only).
    void applyChromaFilter(float* buf, uInt32 n);

    // Convert one line of YUV accumulator samples to packed 0x00RRGGBB,
    // applying the comb blend u = uBuf·(1−blend) + uPrev·blend (same for v).
    // The arithmetic is identical to toRGB().  S-Video passes blend = 0 with
    // uPrev/vPrev aliased to uBuf/vBuf.
    void convertLine(const float* yBuf, const float* uBuf, const float* vBuf,
                     const float* uPrev, const float* vPrev, float blend,
                     uInt32 n, uInt32* dst);

    // Inverse BT.601, packed to 0x00RRGGBB.  Scalar reference for
    // convertLine(); used by the colour-killed path.
    static FORCE_INLINE uInt32 toRGB(float y, float u, float v);

    static void convertToAdjustable(Adjustable& adjustable, const Setup& setup);

  private:
    // Following constructors and assignment operators not supported
    PALSignal(const PALSignal&) = delete;
    PALSignal(PALSignal&&) = delete;
    PALSignal& operator=(const PALSignal&) = delete;
    PALSignal& operator=(PALSignal&&) = delete;
};

#endif  // PAL_SIGNAL_HXX
