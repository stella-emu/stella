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
#include "NTSCFilter.hxx"
#include "PaletteHandler.hxx"

/**
  Emulates the TV signal delay-line circuitry present in PAL and SECAM
  televisions.  On each rendered frame, blends adjacent scanlines in the
  appropriate chroma colour space — YUV for PAL, YDbDr for SECAM — to
  reproduce the colour mixing that a real delay-line TV would produce.

  PAL: The delay line averages U and V from the current and previous
  scanlines.  When the total scanline count is odd the V-axis is phase-
  inverted, causing V to cancel rather than reinforce for same-colour lines
  (the classic "colour-loss" greyscale effect).

  SECAM: The signal alternates between transmitting Db (even lines) and Dr
  (odd lines).  The delay line holds the missing component from the previous
  line, so the decoder always has both Db and Dr regardless of which line it
  is on.  Mixing two different colours across adjacent scanlines produces
  new blended colours beyond the 8-entry SECAM palette.
*/
class TVSignal
{
  public:
    // Universal signal quality presets, applicable to all TV standards.
    // Numeric values are stored in the "tv.filter" setting — do not reorder.
    enum class SignalQuality {
      Off       = 0,
      RGB       = 1,
      SVideo    = 2,
      Composite = 3,
      Bad       = 4,
      Custom    = 5
    };

    explicit TVSignal(const PaletteHandler& paletteHandler);
    ~TVSignal() = default;

    // Called whenever the console timing changes
    void setTiming(ConsoleTiming timing);

    // Set the display palette (for NTSC non-Blargg lookup) and RGB palette
    // (for the Blargg filter's internal YIQ calculation)
    void setPalette(const PaletteArray& tiaPalette, const PaletteArray& rgbPalette);

    // Set the signal quality preset; Off disables Blargg for NTSC and uses
    // plain palette lookup; any other value activates the Blargg renderer
    void setSignalQuality(SignalQuality quality);

    // Enable or disable multi-threaded Blargg rendering
    void enableThreading(bool enable) { myNTSCFilter.enableThreading(enable); }

    // Return the pixel width of one rendered scanline for the current
    // timing/preset combination.  568 when NTSC+Blargg is active, 160 otherwise.
    uInt32 outputWidth() const;

    // Load and save NTSC custom-adjustable settings
    static void loadConfig(const Settings& settings);
    static void saveConfig(Settings& settings);

    // Get the current NTSC preset as a display string
    string getPreset() const { return myNTSCFilter.getPreset(); }

    // Cycle through the NTSC adjustable list; changes which one is "current"
    void selectAdjustable(int direction,
                          string& text, string& valueText, Int32& value) {
      myNTSCFilter.selectAdjustable(direction, text, valueText, value);
    }

    // Change a specific NTSC adjustable by index
    void changeAdjustable(int adjustable, int direction,
                          string& text, string& valueText, Int32& newValue) {
      myNTSCFilter.changeAdjustable(adjustable, direction, text, valueText, newValue);
    }

    // Change the currently selected NTSC adjustable
    void changeCurrentAdjustable(int direction,
                                 string& text, string& valueText, Int32& newValue) {
      myNTSCFilter.changeCurrentAdjustable(direction, text, valueText, newValue);
    }

    // Process one complete TIA frame.  Reads raw TIA colour-index bytes from
    // tiaSrc and writes 32-bit 0x00RRGGBB pixels to rgbDst.  For PAL, applies
    // the delay-line model; for NTSC, applies Blargg or plain palette lookup.
    // scanlinesLastFrame is used to derive the PAL V-axis phase.
    void render(const uInt8* tiaSrc, uInt32 srcWidth, uInt32 srcHeight,
                uInt32* rgbDst, uInt32 dstPitch, uInt32 scanlinesLastFrame);

  private:
    void renderNTSC(const uInt8* tiaSrc, uInt32 srcWidth, uInt32 srcHeight,
                    uInt32* rgbDst, uInt32 dstPitch);
    void renderPAL(const uInt8* tiaSrc, uInt32 srcWidth, uInt32 srcHeight,
                   uInt32* rgbDst, uInt32 dstPitch, bool phaseInverted);
    void renderSECAM(const uInt8* tiaSrc, uInt32 srcWidth, uInt32 srcHeight,
                     uInt32* rgbDst, uInt32 dstPitch);

    // BT.601 YUV → packed 0x00RRGGBB (values in linear [0..1])
    static FORCE_INLINE uInt32 yuvToRGB(float y, float u, float v);

    // SECAM YDbDr → packed 0x00RRGGBB (values in linear [0..1])
    static FORCE_INLINE uInt32 yDbDrToRGB(float y, float db, float dr);

  private:
    const PaletteHandler& myPaletteHandler;
    ConsoleTiming myTiming{ConsoleTiming::ntsc};

    NTSCFilter myNTSCFilter;
    SignalQuality mySignalQuality{SignalQuality::Off};

    // Display palette used for NTSC non-Blargg pixel lookup
    PaletteArray myPalette{};

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
