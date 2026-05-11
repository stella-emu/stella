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

#ifndef FRAME_LAYOUT_DETECTOR_HXX
#define FRAME_LAYOUT_DETECTOR_HXX

class M6532;
class EventHandler;

#include "bspf.hxx"
#include "FrameLayout.hxx"
#include "AbstractFrameManager.hxx"
#include "TIAConstants.hxx"

/**
  Frame manager that auto-detects NTSC vs PAL layout by counting scanlines per
  frame and optionally by analysing per-hue pixel distributions. Used during
  ROM startup to select the correct frame timing before handing off to
  FrameManager.

  @author  Christian Speckner (DirtyHairy)
*/
class FrameLayoutDetector: public AbstractFrameManager
{
  public:
    FrameLayoutDetector();
    ~FrameLayoutDetector() override = default;

    /**
      Return the detected frame layout.
     */
    FrameLayout detectedLayout(bool detectPal60 = false,
                               bool detectNtsc50 = false,
                               string_view name = {}) const;

    /**
      Called when a pixel is rendered.
     */
    void pixelColor(uInt8 color) override;

    /**
      Simulate some input to pass a potential title screen.
     */
    static void simulateInput(M6532& riot, EventHandler& eventHandler,
                              bool pressed);

  protected:
    /**
      Hook into vsync changes.
     */
    void onSetVsync(uInt64) override;

    /**
      Hook into reset.
     */
    void onReset() override;

    /**
      Hook into line changes.
     */
    void onNextLine() override;

  private:
    // This frame manager only tracks frame boundaries
    enum class State: uInt8 {
      // Wait for VSYNC to be enabled.
      waitForVsyncStart,
      // Wait for VSYNC to be disabled.
      waitForVsyncEnd
    };

    enum Metrics: uInt16 {
      // ideal frame heights
      frameLinesNTSC        = 262,
      frameLinesPAL         = 312,

      // number of scanlines to wait for vsync to start and stop
      // (exceeding ideal frame height)
      waitForVsync          = 100,

      // these frames will not be considered for detection
      initialGarbageFrames  = TIAConstants::initialGarbageFrames
    };

  private:
    /**
      Change state and update internal bookkeeping accordingly.
     */
    void setState(State state);

    /**
      Finalize the current frame and infer layout from the scanline count.
     */
    void finalizeFrame();

  private:
    // Current state
    State myState{State::waitForVsyncStart};

    // Accumulated NTSC frame likelihood score
    double myNtscFrameSum{0.0};
    // Accumulated PAL frame likelihood score
    double myPalFrameSum{0.0};

    // Scanlines spent waiting for vsync to toggle; forces the transition
    // when the threshold is exceeded
    uInt32 myLinesWaitingForVsyncToStart{0};

    // Dimensions of the color distribution histogram
    static constexpr int NUM_HUES = 16;
    static constexpr int NUM_LUMS = 8;
    // Per-bin pixel counts; evaluated against statistical distributions to
    // override scanline-based layout detection when the result is decisive
    std::array<uInt64, static_cast<size_t>(NUM_HUES * NUM_LUMS)> myColorCount{0};

  private:
    // Following constructors and assignment operators not supported
    FrameLayoutDetector(const FrameLayoutDetector&) = delete;
    FrameLayoutDetector(FrameLayoutDetector&&) = delete;
    FrameLayoutDetector& operator=(const FrameLayoutDetector&) = delete;
    FrameLayoutDetector& operator=(FrameLayoutDetector&&) = delete;
};

#endif  // FRAME_LAYOUT_DETECTOR_HXX
