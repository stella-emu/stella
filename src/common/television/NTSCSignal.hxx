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

#ifndef NTSC_SIGNAL_HXX
#define NTSC_SIGNAL_HXX

class Settings;

#include "bspf.hxx"
#include "TVAdjustable.hxx"
#include "AtariNTSC.hxx"
#include "FrameBufferConstants.hxx"
#include "TVMode.hxx"

/**
  This class is based on the Blargg NTSC filter code from Atari800,
  and is derived from 'filter_ntsc.(h|c)'.  Original code based on
  implementation from http://www.slack.net/~ant.

  The class is a thin wrapper around the AtariNTSC engine.  It owns the
  custom setup parameters and exposes them as a tag span so that TVSignal
  can drive the adjustable-cycling UI uniformly across signal standards.
*/
class NTSCSignal
{
  public:
    NTSCSignal() = default;
    ~NTSCSignal() = default;

  public:
    // Indices into the adjustableTags() span; used by GlobalKeyHandler
    enum class Adjustables: uInt8 {
      SHARPNESS,
      RESOLUTION,
      ARTIFACTS,
      FRINGING,
      BLEEDING,
      NUM_ADJUSTABLES
    };

    // Named-field struct for GUI dialogs that read/write all NTSC adjustables
    struct Adjustable {
      uInt32 sharpness{0}, resolution{0}, artifacts{0}, fringing{0}, bleed{0};
    };


  public:
    void setPalette(IntSpan palette) {
      myNTSC.setPalette(palette);
    }

    // Apply the AtariNTSC setup corresponding to the given TV mode
    void initialize(TVMode mode);

    // Re-apply the custom setup after a tag value has been edited in-place
    void reinitializeCustom();

    // Span of {name, float*} entries for the custom adjustables.
    // Pointers are into the static myCustomSetup, so they remain valid.
    SpanOf<AdjustableTag> adjustableTags() const { return ourCustomAdjustables; }

    // Get the named adjustables for a given TV mode (for GUI dialogs)
    static void getAdjustables(Adjustable& adjustable, TVMode mode);

    // Set the shared custom setup from named adjustable values
    static void setCustomAdjustables(const Adjustable& adjustable);

    // Load and save NTSC-related settings
    static void loadConfig(const Settings& settings);
    static void saveConfig(Settings& settings);

    // Perform Blargg filtering on input buffer, place results in output buffer
    void render(const uInt8* src_buf, uInt32 src_width, uInt32 src_height,
                uInt32* dest_buf, uInt32 dest_pitch)
    {
      myNTSC.render(src_buf, src_width, src_height, dest_buf, dest_pitch);
    }
    void render(const uInt8* src_buf, uInt32 src_width, uInt32 src_height,
                uInt32* dest_buf, uInt32 dest_pitch, uInt32* prev_buf)
    {
      myNTSC.render(src_buf, src_width, src_height, dest_buf, dest_pitch, prev_buf);
    }

    void enableThreading(bool enable) {
      myNTSC.enableThreading(enable);
    }

    // Width in pixels of one Blargg-filtered scanline for the given input width
    static constexpr uInt32 outWidth(uInt32 inWidth) {
      return AtariNTSC::outWidth(inWidth);
    }

  private:
    static void convertToAdjustable(Adjustable& adjustable,
                                    const AtariNTSC::Setup& setup);

  private:
    AtariNTSC myNTSC;

    AtariNTSC::Setup mySetup{AtariNTSC::TV_Composite};

    static inline AtariNTSC::Setup myCustomSetup = AtariNTSC::TV_Composite;

    static constexpr std::array<AdjustableTag, 5> ourCustomAdjustables = {{
      { "sharpness", &myCustomSetup.sharpness },
      { "resolution", &myCustomSetup.resolution },
      { "artifacts", &myCustomSetup.artifacts },
      { "fringing", &myCustomSetup.fringing },
      { "bleeding", &myCustomSetup.bleed }
    }};

  private:
    NTSCSignal(const NTSCSignal&) = delete;
    NTSCSignal(NTSCSignal&&) = delete;
    NTSCSignal& operator=(const NTSCSignal&) = delete;
    NTSCSignal& operator=(NTSCSignal&&) = delete;
};

#endif  // NTSC_SIGNAL_HXX
