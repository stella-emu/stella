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
#include "NTSCSignal.hxx"
#include "PALSignal.hxx"
#include "PaletteHandler.hxx"

/**
  Converts raw TIA colour-index data into RGB pixels, applying the
  appropriate TV standard emulation for NTSC, PAL, or SECAM.  The active
  standard is set by setTiming(); the TVMode controls which signal path
  within that standard is emulated (composite, S-Video, RGB, etc.).

  NTSC: Delegates to NTSCSignal, which wraps the Blargg NTSC engine.
  TVMode::None bypasses the engine and performs a plain palette lookup.

  PAL: Emulates the chroma delay-line present in PAL televisions.  The
  delay line averages U and V from the current and previous scanlines.
  When the total scanline count is odd the V-axis is phase-inverted,
  causing V to cancel rather than reinforce for same-colour lines (the
  classic "colour-loss" greyscale effect).  TVMode::RGB and TVMode::SVideo
  bypass the chroma decoder entirely, so the delay line is skipped for
  those modes.

  SECAM: Emulates the chroma delay-line present in SECAM televisions.  The
  signal alternates between transmitting Db (even lines) and Dr (odd lines);
  the delay line holds the missing component from the previous line so the
  decoder always has both.  Mixing two colours across adjacent scanlines
  produces blended colours beyond the 8-entry SECAM palette.  The delay
  line is always active except when TVMode::None is selected.
*/
class TVSignal
{
  public:
    explicit TVSignal(const PaletteHandler& paletteHandler);
    ~TVSignal() = default;

    // Called whenever the console timing changes
    void setTiming(ConsoleTiming timing);

    // Set the display palette (for NTSC non-Blargg lookup) and RGB palette
    // (for the Blargg filter's internal YIQ calculation)
    void setPalette(const PaletteArray& tiaPalette, const PaletteArray& rgbPalette);

    // Set the signal type
    void setTVMode(TVMode type);

    // Return the pixel width of one rendered scanline for the current
    // timing/type combination.  568 when NTSC+Blargg is active, 160 otherwise.
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
    // tiaSrc and writes 32-bit 0x00RRGGBB pixels to rgbDst.  For PAL, applies
    // the delay-line model; for NTSC, applies Blargg or plain palette lookup.
    // scanlinesLastFrame is used to derive the PAL V-axis phase.
    void render(const uInt8* tiaSrc, uInt32 srcWidth, uInt32 srcHeight,
                uInt32* rgbDst, uInt32 dstPitch, uInt32 scanlinesLastFrame);

  private:
    // Plain palette lookup with no signal processing (the TVMode::None
    // paths, plus TVMode::RGB for PAL)
    void renderPassthrough(const uInt8* tiaSrc, uInt32 srcWidth,
                           uInt32 srcHeight, uInt32* rgbDst,
                           uInt32 dstPitch) const;

    void renderNTSC(const uInt8* tiaSrc, uInt32 srcWidth, uInt32 srcHeight,
                    uInt32* rgbDst, uInt32 dstPitch);
    void renderPAL(const uInt8* tiaSrc, uInt32 srcWidth, uInt32 srcHeight,
                   uInt32* rgbDst, uInt32 dstPitch, bool phaseInverted);
    void renderSECAM(const uInt8* tiaSrc, uInt32 srcWidth, uInt32 srcHeight,
                     uInt32* rgbDst, uInt32 dstPitch);

    // Returns the adjustable tag span for the currently active filter
    SpanOf<AdjustableTag> currentAdjustableTags() const;

    // SECAM YDbDr → packed 0x00RRGGBB (values in linear [0..1])
    static FORCE_INLINE uInt32 yDbDrToRGB(float y, float db, float dr);

  private:
    const PaletteHandler& myPaletteHandler;
    ConsoleTiming myTiming{ConsoleTiming::ntsc};

    NTSCSignal myNTSCSignal;
    PALSignal  myPALSignal;
    TVMode myTVMode{TVMode::None};

    // Index of the currently selected custom adjustable
    uInt32 myCurrentAdjustable{0};

    // Display palette used for NTSC non-Blargg pixel lookup
    PaletteArray myPalette{};

    // RGB palette for the engines' internal YIQ/YUV encoding; kept so
    // setTiming() can replay it into a newly active engine
    PaletteArray myRGBPalette{};

    // Delay-line buffer: TIA colour-index bytes for the previous scanline
    static constexpr uInt32 MAX_LINE_WIDTH = 160;
    std::array<uInt8, MAX_LINE_WIDTH> myPrevLine{};

  private:
    TVSignal() = delete;
    TVSignal(const TVSignal&) = delete;
    TVSignal(TVSignal&&) = delete;
    TVSignal& operator=(const TVSignal&) = delete;
    TVSignal& operator=(TVSignal&&) = delete;
};

#endif  // TV_SIGNAL_HXX
