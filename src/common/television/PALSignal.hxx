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

  SIGNAL MODEL
  ─────────────────────────────────────────────────────────────────────────
  The PAL TIA colour clock runs at 3.546894 MHz.  The PAL colour subcarrier
  is 4.433618 MHz.  Their ratio is exactly 5/4:

      fsc  =  TIA_clock × 5/4   →   4×fsc = TIA_clock × 5

  We therefore work at an internal sample rate of 4×fsc = 17.734476 MHz,
  which gives exactly 5 internal samples per TIA colour clock, and exactly
  4 samples per subcarrier cycle — the minimal integer grid on which the
  composite waveform can be represented without fractional-sample error.

  ENCODING
  ─────────────────────────────────────────────────────────────────────────
  Each TIA colour-index byte is mapped to a phase angle (hue) and an
  amplitude (saturation × luma).  The encoder generates the composite
  waveform:

      s(t) = Y(t) + U(t)·cos(2π·fsc·t) − V(t)·sin(2π·fsc·t)

  where U and V are the BT.601 colour-difference signals.  The V component
  is negated on alternate scanlines (the PAL phase-alternation law).

  DECODING
  ─────────────────────────────────────────────────────────────────────────
  A one-line comb filter separates Y, U, and V:

      U_out = (U_current + U_delayed) / 2       ← averages (reinforces)
      V_out = (V_current − V_delayed_inv) / 2   ← cancels noise

  The bandwidth of each channel is limited by separate low-pass filters
  tuned to PAL-B/G specifications:

      Luma bandwidth:   5.0 MHz  (PAL-B/G standard)
      Chroma bandwidth: 1.3 MHz  (each colour-difference axis)

  ARTIFACTS
  ─────────────────────────────────────────────────────────────────────────
  Because encoding and decoding are performed in the composite domain, the
  following artifacts emerge naturally without special-casing:

    • Chroma/luma crosstalk (dot crawl pattern, correctly offset from NTSC)
    • Colour fringing at sharp horizontal edges
    • Bandwidth softening on both luma and chroma
    • Correct PAL Hanover bar suppression via the comb filter

  OUTPUT
  ─────────────────────────────────────────────────────────────────────────
  The output pixel width is TIA_WIDTH pixels (one per TIA colour clock).
  Luma bandwidth limiting produces mild horizontal softening consistent with
  a real PAL composite display.  A "sharpness" control applies aperture
  correction to the luma channel, matching the peaking circuits found in
  real PAL television sets.
*/
class PALSignal
{
  public:
    // Normalized parameter pack for one PAL signal quality level.
    // User-exposed fields are in [-1..1] so that TVSignal's scaleTo100/
    // scaleFrom100 helpers apply without change.
    //
    // saturation, hue, and gamma are held at neutral values; PaletteHandler
    // owns those dimensions for the user and applies them to the palette
    // before it reaches the composite pipeline.
    //
    // Physical-value mappings:
    //   sharpness  : [-1..1]      luma aperture correction (0 = flat)
    //   saturation : neutral 0.0  → physical ×1.0 (sat + 1.0)
    //   hue        : neutral 0.0  → physical 0°   (hue × 180°)
    //   gamma      : neutral 0.0  → physical 1.75  (gamma × 0.75 + 1.75)
    //   blend      : [-1..1]      comb blend;  physical = blend × 0.5 + 0.5
    struct Setup {
      float sharpness  { 0.2F };
      float saturation { 0.0F };
      float hue        { 0.0F };
      float gamma      { 0.0F };
      float blend      { 0.0F };
    };

    // Indices into the adjustableTags() span; used by GlobalKeyHandler
    enum class Adjustables : uInt8 {
      SHARPNESS,
      BLEND,
      NUM_ADJUSTABLES
    };

    // Named-field struct for GUI dialogs that read/write PAL adjustables
    struct Adjustable {
      uInt32 sharpness{0}, blend{0};
    };

    // Preset Setup values for each TVMode
    static constexpr Setup TV_Composite{  0.2F, 0.0F, 0.0F, 0.0F, 0.0F };
    static constexpr Setup TV_SVideo   {  0.3F, 0.0F, 0.0F, 0.0F, 0.0F };
    static constexpr Setup TV_Bad      { -0.3F, 0.0F, 0.0F, 0.0F, 0.6F };

  public:
    PALSignal();
    ~PALSignal() = default;

    // Apply the Setup corresponding to the given TVMode; rebuilds kernels.
    void initialize(TVMode mode);

    // Re-apply myCustomSetup after a tag value has been edited in-place.
    void reinitializeCustom();

    // Span of {name, float*} entries for the custom adjustables.
    // Pointers are into the static myCustomSetup, so they remain valid.
    SpanOf<AdjustableTag> adjustableTags() const { return ourCustomAdjustables; }

    // Get the named adjustables for a given TVMode (for GUI dialogs)
    static void getAdjustables(Adjustable& adjustable, TVMode mode);

    // Set the shared custom setup from named adjustable values
    static void setCustomAdjustables(const Adjustable& adjustable);

    // Load and save PAL-related settings
    static void loadConfig(const Settings& settings);
    static void saveConfig(Settings& settings);

    // Rebuild internal lookup tables from the given RGB palette.
    void setPalette(IntSpan palette);

    // Render one complete frame.
    //   tiaSrc   : raw TIA colour-index bytes, srcWidth × srcHeight
    //   rgbDst   : destination 0x00RRGGBB pixels, dstPitch pixels wide
    //   phaseInverted: true when the previous frame had an odd scanline count.
    //              Drives PAL V-phase alternation, and also PAL colour loss:
    //              an odd count gives an inconsistent PAL field/burst sequence,
    //              so a real set's colour-killer cuts chroma and the frame is
    //              rendered as luma-only greyscale.
    void render(const uInt8* tiaSrc, uInt32 srcWidth, uInt32 srcHeight,
                uInt32* rgbDst, uInt32 dstPitch, bool phaseInverted);

    // Width of one TIA colour-clock scanline in pixels (input to render)
    static constexpr uInt32 TIA_WIDTH = 160;

    // Output width for a given input width.  The output is the internal
    // oversampled grid itself (SAMPLES_PER_CLOCK output pixels per TIA colour
    // clock), with no downsample, so the composite artifacts (dot crawl,
    // subcarrier beat, fringing) survive to the display instead of being
    // averaged away.
    static constexpr uInt32 outWidth(uInt32 inWidth) {
      return inWidth * SAMPLES_PER_CLOCK;
    }

  private:
    // ── Frequency constants ───────────────────────────────────────────────

    // PAL TIA colour clock (crystal frequency)
    static constexpr float TIA_FREQ    = 3'546'894.0F;   // Hz

    // PAL colour subcarrier = TIA_FREQ × 5/4
    static constexpr float FSC         = 4'433'618.0F;   // Hz  (= TIA_FREQ × 1.25)

    // Internal sample rate = 4 × fsc = 5 × TIA_FREQ
    // → 4 samples/subcarrier cycle, 5 samples/TIA colour clock
    static constexpr float SAMPLE_RATE = 4.0F * FSC;     // 17,734,472 Hz

    // Samples per TIA colour clock at SAMPLE_RATE (exact integer = 5)
    static constexpr uInt32 SAMPLES_PER_CLOCK = 5;

    // Samples per scanline at SAMPLE_RATE
    // 228 TIA clocks/line × 5 samples/clock = 1140
    static constexpr uInt32 SAMPLES_PER_LINE = 228 * SAMPLES_PER_CLOCK;

    // Visible samples per scanline (160 TIA clocks × 5)
    static constexpr uInt32 VISIBLE_SAMPLES  = TIA_WIDTH * SAMPLES_PER_CLOCK;

    // ── Filter kernel lengths ─────────────────────────────────────────────

    // Luma low-pass kernel half-length (samples at SAMPLE_RATE)
    // 5 MHz cutoff → kernel length ≈ SAMPLE_RATE / (2 × 5e6) ≈ 1.77 → use 7-tap
    static constexpr uInt32 LUMA_TAPS   = 7;

    // Chroma low-pass kernel half-length
    // 1.3 MHz cutoff → kernel length ≈ SAMPLE_RATE / (2 × 1.3e6) ≈ 6.8 → use 7-tap
    static constexpr uInt32 CHROMA_TAPS = 7;

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

    // Pre-computed subcarrier quadrature samples for 4 samples/cycle.
    // Index i (0..3): cosine[i] = cos(2π·i/4), sine[i] = sin(2π·i/4)
    // Values: cos = {1, 0, -1, 0}, sin = {0, 1, 0, -1}
    static constexpr std::array<float, 4> SUBCARRIER_COS{ 1.F,  0.F, -1.F,  0.F};
    static constexpr std::array<float, 4> SUBCARRIER_SIN{ 0.F,  1.F,  0.F, -1.F};

    // ── Filter kernels ────────────────────────────────────────────────────

    std::array<float, LUMA_TAPS>   myLumaKernel{};
    std::array<float, CHROMA_TAPS> myChromaKernel{};

    // Aperture correction kernel strength (3-tap: [−k/2, 1+k, −k/2])
    float myApertureK{};

    // ── Per-output-pixel kernels (AtariNTSC-style precomputation) ─────────
    //
    // Every stage from composite encode through comb/low-pass decode and
    // 5:1 downsampling is *linear* in each input clock's (Y, U, V).  So the
    // contribution of one input clock to the YUV of the output pixels around
    // it is a fixed kernel that depends only on the clock's colour, its
    // column phase (x & 3) and the PAL V-sign.  At render time we simply
    // scatter-add each clock's kernel into the line accumulators; no per-
    // pixel filtering is performed.  See buildCoeff() and expandKernels().

    // Sample-domain reach of one clock's energy: the luma FIR half-width plus
    // one for aperture correction, or the chroma FIR half-width, whichever is
    // larger.  The FIRs have finite support, so beyond this there is exactly
    // zero overlap — the kernel is not merely small but identically zero.
    static constexpr int SAMPLE_REACH =
      (LUMA_TAPS / 2 + 1) > (CHROMA_TAPS / 2)
        ? (LUMA_TAPS / 2 + 1) : (CHROMA_TAPS / 2);

    // Since the output is the oversampled grid itself, one clock contributes
    // to its own SAMPLES_PER_CLOCK output samples plus SAMPLE_REACH on each
    // side.  KERNEL_LEFT is the offset of the first tap relative to the
    // clock's first output sample.
    static constexpr int KERNEL_LEFT  = SAMPLE_REACH;
    static constexpr int KERNEL_WIDTH =
      static_cast<int>(SAMPLES_PER_CLOCK) + 2 * SAMPLE_REACH;

    // One clock's YUV contribution across KERNEL_WIDTH output samples.
    struct Kernel {
      std::array<float, KERNEL_WIDTH> y{};
      std::array<float, KERNEL_WIDTH> u{};
      std::array<float, KERNEL_WIDTH> v{};
    };

    // Palette-independent linear decode coefficients: each output component
    // (Y, U, V) at a given offset as a linear combination of the clock's
    // input (y, u, v).  Depends only on the filters (sharpness/bandwidth),
    // so it is rebuilt only when those change, not on every palette change.
    struct DecodeCoeff {
      float yy{}, yu{}, yv{};   // Yout = yy·y + yu·u + yv·v
      float uy{}, uu{}, uv{};   // Uout = uy·y + uu·u + uv·v
      float vy{}, vu{}, vv{};   // Vout = vy·y + vu·u + vv·v
    };

    // Composite coefficients: [columnPhase 0..3][vSign 0..1][offset]
    std::array<std::array<std::array<DecodeCoeff, KERNEL_WIDTH>, 2>, 4> myCoeff{};
    // S-Video coefficients (no subcarrier → phase/vSign independent)
    std::array<DecodeCoeff, KERNEL_WIDTH> mySVCoeff{};

    // Per-colour expanded kernels, rebuilt on any palette or coeff change.
    // Composite: [colour][columnPhase 0..3][vSign 0..1]
    std::array<std::array<std::array<Kernel, 2>, 4>, 256> myKernel{};
    // S-Video: [colour]
    std::array<Kernel, 256> mySVKernel{};

    // ── Scratch buffers (used only while building coefficients) ───────────

    // Isolated-clock composite waveform (SAMPLES_PER_LINE)
    std::vector<float> myCurrentLine;

    // FIR work buffers (VISIBLE_SAMPLES)
    std::vector<float> myYBuf, myUBuf, myVBuf, myFilterTmp;

    // ── Per-line render accumulators (output-width = VISIBLE_SAMPLES) ─────
    std::vector<float> myAccY, myAccU, myAccV;
    // Previous line's filtered chroma, for the 1-line PAL comb blend
    std::vector<float> myPrevU, myPrevV;

    // ── Pre-baked lookup tables ───────────────────────────────────────────

    // Gamma LUT: quantized linear light [0..1023] → gamma-encoded display [0..255].
    // Rebuilt whenever mySetup.gamma changes.
    static constexpr uInt32 GAMMA_LUT_SIZE = 1024;
    std::array<uInt8, GAMMA_LUT_SIZE> myGammaLUT{};

    // True when the active mode bypasses composite encoding (S-Video)
    bool mySVideo{false};

    // ── Active and custom setups ──────────────────────────────────────────

    // Active setup (set by initialize or reinitializeCustom)
    Setup mySetup{TV_Composite};

    // Persistent custom setup written by setCustomAdjustables/loadConfig
    static inline Setup myCustomSetup{TV_Composite};

    // AdjustableTags pointing into myCustomSetup for the cycling UI
    static constexpr std::array<AdjustableTag, 2> ourCustomAdjustables = {{
      { "sharpness", &myCustomSetup.sharpness },
      { "blend",     &myCustomSetup.blend     }
    }};

    // ── Private methods ───────────────────────────────────────────────────

    // Build sinc-windowed low-pass FIR kernels
    void buildLumaKernel();
    void buildChromaKernel();

    // Build gamma LUT from mySetup.gamma; call after any setup change
    void buildGammaLUT();

    // Build the palette-independent decode coefficients (myCoeff/mySVCoeff)
    // from the current filters; call after buildLumaKernel/buildChromaKernel.
    void buildCoeff();

    // Expand myClockTable + coefficients into the per-colour kernels
    // (myKernel/mySVKernel); call after setPalette() or buildCoeff().
    void expandKernels();

    // Apply the luma FIR and aperture correction to a sample buffer in-place
    void applyLumaFilter(float* buf, uInt32 n);

    // Apply the chroma FIR to a sample buffer in-place
    void applyChromaFilter(float* buf, uInt32 n);

    // Gamma-correct and pack to 0x00RRGGBB
    FORCE_INLINE uInt32 toRGB(float y, float u, float v) const;

    static void convertToAdjustable(Adjustable& adjustable, const Setup& setup);

  private:
    // Following constructors and assignment operators not supported
    PALSignal(const PALSignal&) = delete;
    PALSignal(PALSignal&&) = delete;
    PALSignal& operator=(const PALSignal&) = delete;
    PALSignal& operator=(PALSignal&&) = delete;
};

#endif  // PAL_SIGNAL_HXX
