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

#ifndef TV_SIGNAL_HXX
#define TV_SIGNAL_HXX

#include "bspf.hxx"
#include "ConsoleTiming.hxx"
#include "FrameBufferConstants.hxx"
#include "NTSCSignal.hxx"
#include "PALSignal.hxx"
#include "SECAMSignal.hxx"

class Settings;

/**
  Converts raw TIA colour-index data into RGB pixels, applying the
  appropriate TV standard emulation for NTSC, PAL, or SECAM.  The active
  standard is set by setTiming(); the TVMode controls which signal path
  within that standard is emulated (composite, S-Video, RGB, etc.).

  Across all three standards TVMode::None is the raw TIA palette: no signal
  model runs, so it is a faithful capture source for external processing.
  The remaining modes are ordered by how many channels the signal carries
  and therefore how few artifacts it shows (RGB → S-Video → Composite).

  NTSC: Delegates to NTSCSignal, which wraps the Blargg NTSC engine.  The
  RGB / S-Video / Composite modes map to Blargg setups of decreasing
  cleanliness; Custom is a user-tweaked setup.

  PAL: Delegates to PALSignal, an analogue encode/decode model.  RGB carries
  chroma at full bandwidth with no 1-line comb (the crispest result);
  S-Video band-limits the chroma but still skips the comb; Composite/Custom
  run the full comb, and are also the only modes whose luma passes a chroma
  trap — which is what makes composite the softest as well as the most
  artifact-prone.  There, an odd total scanline count phase-inverts the
  V-axis and trips the colour-killer (the classic "colour-loss" effect).

  SECAM: Delegates to SECAMSignal, which runs the chroma delay line present
  in SECAM televisions.  Because the delay line is the decoder rather than an
  effect layered on one, every colour mode (RGB, S-Video, Composite) runs it;
  SECAM models neither chroma bandwidth nor Y/C separation, so the three
  render identically and differ only from None.
*/
class TVSignal
{
  public:
    TVSignal() = default;
    ~TVSignal() = default;

    // Called whenever the console timing changes
    void setTiming(ConsoleTiming timing);

    // Set the display palette (for the None/passthrough lookup) and the RGB
    // palette (linearised by each engine for its internal YIQ/YUV encoding)
    void setPalette(const PaletteArray& tiaPalette, const PaletteArray& rgbPalette);

    // Set the signal type
    void setTVMode(TVMode type);

    // Return the pixel width of one rendered scanline for the current
    // timing/type combination: 568 for NTSC and 800 for PAL when an engine mode
    // is active (the oversampled grids), 160 otherwise (None, and all of SECAM).
    uInt32 outputWidth() const;

    // Load and save custom-adjustable settings
    static void loadConfig(const Settings& settings) {
      NTSCSignal::loadConfig(settings);
      PALSignal::loadConfig(settings);
    }
    static void saveConfig(Settings& settings) {
      NTSCSignal::saveConfig(settings);
      PALSignal::saveConfig(settings);
    }

    // Get the current signal type as a display string
    string_view getPreset() const;

    // Cycle through the adjustable list for the active signal standard;
    // changes which one is "current"
    void selectAdjustable(int direction,
                          string& text, string& valueText, Int32& value);

    // Change a specific adjustable by index
    void changeAdjustable(int adjustable, int direction,
                          string& text, string& valueText, Int32& newValue);

    // Change the currently selected adjustable
    void changeCurrentAdjustable(int direction,
                                 string& text, string& valueText, Int32& newValue);

    // Process one complete TIA frame.  Reads raw TIA colour-index bytes from
    // tiaSrc and writes 32-bit 0x00RRGGBB pixels to rgbDst, dispatched by the
    // active standard (NTSC/PAL/SECAM) and TVMode.  Both field-phase flags come
    // from the frame manager: phaseInverted is the PAL chroma V-axis field
    // phase (see AbstractFrameManager::chromaPhaseInverted()), and lineParity
    // says whether the first scanline carries SECAM D'R rather than D'B (see
    // AbstractFrameManager::chromaLineParity()).
    void render(const uInt8* tiaSrc, uInt32 srcWidth, uInt32 srcHeight,
                uInt32* rgbDst, uInt32 dstPitch, bool phaseInverted,
                bool lineParity);

  private:
    // Plain palette lookup with no signal processing (the TVMode::None path
    // for every standard — the raw capture source)
    void renderPassthrough(const uInt8* tiaSrc, uInt32 srcWidth,
                           uInt32 srcHeight, uInt32* rgbDst,
                           uInt32 dstPitch) const;

    void renderNTSC(const uInt8* tiaSrc, uInt32 srcWidth, uInt32 srcHeight,
                    uInt32* rgbDst, uInt32 dstPitch);
    void renderPAL(const uInt8* tiaSrc, uInt32 srcWidth, uInt32 srcHeight,
                   uInt32* rgbDst, uInt32 dstPitch, bool phaseInverted);
    void renderSECAM(const uInt8* tiaSrc, uInt32 srcWidth, uInt32 srcHeight,
                     uInt32* rgbDst, uInt32 dstPitch, bool lineParity);

    // Returns the adjustable tag span for the currently active filter
    SpanOf<AdjustableTag> currentAdjustableTags() const;

  private:
    ConsoleTiming myTiming{ConsoleTiming::ntsc};

    NTSCSignal  myNTSCSignal;
    PALSignal   myPALSignal;
    SECAMSignal mySECAMSignal;
    TVMode myTVMode{TVMode::None};

    // Index of the currently selected custom adjustable
    uInt32 myCurrentAdjustable{0};

    // Display palette used for NTSC non-Blargg pixel lookup
    PaletteArray myPalette{};

    // RGB palette for the engines' internal YIQ/YUV/YDbDr encoding; kept so
    // setTiming() can replay it into a newly active engine
    PaletteArray myRGBPalette{};

  private:
    TVSignal(const TVSignal&) = delete;
    TVSignal(TVSignal&&) = delete;
    TVSignal& operator=(const TVSignal&) = delete;
    TVSignal& operator=(TVSignal&&) = delete;
};

#endif  // TV_SIGNAL_HXX
