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

#ifndef SECAM_SIGNAL_HXX
#define SECAM_SIGNAL_HXX

#include "bspf.hxx"
#include "FrameBufferConstants.hxx"
#include "TVMode.hxx"

/**
  Accurate SECAM chroma decoder for the Atari 2600 TIA.

  SECAM sends half its colour information per line and leaves the receiver to
  reassemble the rest from memory, so what is modelled here is not an effect
  layered onto a picture: it is the decoder, and without it a SECAM picture
  has no colour at all.  Item numbers below are Table 2 of ITU-R BT.470-6,
  cited in full beside the constants they justify.

  SIGNAL — one colour-difference signal per line
  ─────────────────────────────────────────────────────────────────────────
  The subcarrier is frequency modulated and carries D'B and D'R alternately
  from line to line (items 2.9, 2.10):

      E_M = E'Y + G·cos 2π( f_OB + Δf_OB/f_0 ∫ D'B* dt )   D'B lines
      E_M = E'Y + G·cos 2π( f_OR + Δf_OR/f_0 ∫ D'R* dt )   D'R lines

  COLOUR SPACE — YDbDr
  ─────────────────────────────────────────────────────────────────────────
  The two are BT.601 colour differences at SECAM's own scale factors, which
  bound the deviation (items 2.4, 2.5):

      E'Y =  0.299 E'R + 0.587 E'G + 0.114 E'B
      D'B =  1.505 (E'B − E'Y)
      D'R = −1.902 (E'R − E'Y)

  toRGB() applies the exact inverse.  Note what they are defined on: E'R, E'G
  and E'B are already gamma-pre-corrected, so the whole chain — band-limiting
  included — runs on palette bytes as transmitted, which is where a real
  encoder applies it.  Linearising here would depart from the signal rather
  than approach it.  (PALSignal linearises before its FIR and comb so that
  blending mixes light the way a CRT's beam current does; what is filtered
  here is the transmitted colour-difference signal itself, and gamma-corrected
  space is that signal's own.)

  DECODE — the delay line is the decoder
  ─────────────────────────────────────────────────────────────────────────
  One component per line is not enough to make a colour, so a set holds each
  line for 64 µs and pairs the component the current line carries with the
  one the previous line carried — the "M" (mémoire) of Séquentiel Couleur À
  Mémoire.

  Both lines of a pair therefore share one chroma pair and differ only in
  luma, halving vertical chroma resolution against NTSC/PAL.  The standard
  assumes adjacent lines are near-identical in colour, so that normally costs
  nothing.  Where a program breaks the assumption and alternates colours line
  by line, the recovered pair mixes one colour's D'B with the other's D'R and
  yields colours outside the console's 8-entry palette.  Reproducing that is
  why this class exists.

  PALETTE — measured entries, not idealised primaries
  ─────────────────────────────────────────────────────────────────────────
  setPalette() derives D'B/D'R from the display palette, deliberately: those
  entries describe what a set shows for each luminance value, so a solid area
  decodes back to exactly its entry and a blend interpolates between measured
  endpoints.  Substituting saturated RGB corners would oversaturate every
  blend and would exceed what the standard can carry — item 2.12's deviation
  limits confine a compliant encoder to D'R ∈ [−1.807, +1.25] and
  D'B ∈ [−1.52, +2.20], while ideal cyan alone needs D'R = +1.333.  A
  desaturated palette is the physical expectation, not an error to correct.

  CHROMA BANDWIDTH — exact at one sample per colour clock
  ─────────────────────────────────────────────────────────────────────────
  Item 2.6 puts the colour-difference signals −3 dB down by 1.3 MHz, so a set
  cannot resolve colour to a single TIA pixel.  initialize() picks a cutoff
  per connection type, and that filter is the only thing separating them.

  No oversampled grid is needed, and that is exactness rather than a
  compromise: the TIA holds each chroma value for exactly one colour clock
  and the result is sampled back onto the same grid, so staircase → analogue
  filter → sample at pixel centres collapses to a discrete convolution whose
  taps are the impulse response integrated over each pixel bin.  PALSignal
  has no such luck — its subcarrier genuinely needs the finer grid.

  The taps come from a Gaussian prototype at the −3 dB point, which truncates
  to three (the fourth is 2e−5) and invents no ringing.  A steeper prototype
  satisfies item 2.6 equally, at roughly double the side tap plus overshoot,
  so read the values as carrying that much uncertainty.  The item's other
  half — 30 dB down by 3.5 MHz — lies above this grid's Nyquist and cannot be
  represented at all.

  WHAT THIS MODEL DOES AND DOES NOT PRODUCE
  ─────────────────────────────────────────────────────────────────────────
  Emerges from the chain:
    • Colours beyond the 8-entry palette wherever adjacent lines differ
    • Vertical chroma resolution halved, shared across each line pair
    • Horizontal colour bleed, and with it the only difference between the
      three colour connection types

  Deliberately absent — do not "restore" these without hardware data:
    • Low-frequency pre-correction (item 2.7) and the bell shaping of
      subcarrier amplitude (item 2.13), which together give SECAM its
      characteristic "fire" on sharp edges
    • Deviation limiting (item 2.12): moot while the palette already sits
      within range, and applying it too would clip twice
    • Line identification and the colour killer (item 2.18).  Whether the
      console's daughterboard emits identification at all is undocumented;
      AbstractFrameManager::chromaLineParity() records what that costs.
*/
class SECAMSignal
{
  public:
    SECAMSignal() = default;
    ~SECAMSignal() = default;

    // Set the chroma bandwidth for the given connection type
    void initialize(TVMode mode);

    // Rebuild the per-colour YDbDr table, indexed by TIA colour byte, from
    // the given RGB palette
    void setPalette(IntSpan palette);

    // Render one complete frame.
    //   tiaSrc : raw TIA colour-index bytes, srcWidth × srcHeight
    //   rgbDst : destination 0x00RRGGBB pixels, dstPitch pixels wide
    //   firstLineCarriesDr: which component the first rendered scanline
    //            carries.  Comes from the console's line counter (see
    //            AbstractFrameManager::chromaLineParity()) and pointedly not
    //            from the buffer row, which would tie the decoded colours to
    //            the ROM's VBLANK height.
    void render(const uInt8* tiaSrc, uInt32 srcWidth, uInt32 srcHeight,
                uInt32* rgbDst, uInt32 dstPitch, bool firstLineCarriesDr);

    // Width of one TIA colour-clock scanline in pixels (input to render)
    static constexpr uInt32 TIA_WIDTH = 160;

    // Output width for a given input width.  The chroma kernel is exact on
    // the native grid (see CHROMA BANDWIDTH), so there is no oversampled grid
    // to emit and the input width passes straight through.
    static constexpr uInt32 outWidth(uInt32 inWidth) {
      return inWidth;
    }

  private:
    // Packed 0x00RRGGBB from one YDbDr triple, inverting the item 2.5
    // definitions
    static FORCE_INLINE uInt32 toRGB(float y, float db, float dr);

    // Band-limit one scanline's colour-difference signal in place, edges
    // clamped.  Whichever of D'B / D'R the line carries takes the same path.
    void filterChroma(std::array<float, TIA_WIDTH>& chroma, uInt32 width) const;

  private:
    // Recommendation ITU-R BT.470-6, "Conventional television systems" (ITU,
    // Nov 1998), itu.int/dms_pubrec/itu-r/rec/bt/r-rec-bt.470-6-199811-s!!pdf-e.pdf
    //   - E'Y weightings: Table 2 item 2.4
    //   - D'R / D'B definitions: item 2.5
    //   - colour-difference bandwidth, −3 dB at 1.3 MHz: item 2.6
    //   - FM, alternating line to line: items 2.9, 2.10
    //   - rest frequencies and deviation limits: items 2.11, 2.12
    //   - line identification: item 2.18
    //
    // −3 dB points, in Hz.  Only the composite figure is the standard's; the
    // S-Video value is an interpolation, kept so the connection types stay
    // ordered by cleanliness as they are on the other two standards.
    static constexpr float CUTOFF_COMPOSITE = 1'300'000.F;
    static constexpr float CUTOFF_SVIDEO    = 2'000'000.F;

    // TIA colour clock of a PAL/SECAM console, in Hz; the SECAM machine drives
    // its encoder from the PAL TIA's luminance output
    static constexpr float TIA_FREQ = 3'546'894.F;

  private:
    // Per-colour decomposition, indexed by TIA colour byte
    struct YDbDrEntry {
      float y{0}, db{0}, dr{0};
    };
    std::array<YDbDrEntry, 256> myYDbDr{};

    // Symmetric 3-tap chroma kernel; {0, 1, 0} passes the signal untouched
    float myChromaSide{0.F};
    float myChromaCentre{1.F};

    // The delay line, holding the component the previous scanline carried
    std::array<float, TIA_WIDTH> myPrevChroma{};

    // This scanline's own component, band-limited
    std::array<float, TIA_WIDTH> myChroma{};

  private:
    SECAMSignal(const SECAMSignal&) = delete;
    SECAMSignal(SECAMSignal&&) = delete;
    SECAMSignal& operator=(const SECAMSignal&) = delete;
    SECAMSignal& operator=(SECAMSignal&&) = delete;
};

#endif  // SECAM_SIGNAL_HXX
