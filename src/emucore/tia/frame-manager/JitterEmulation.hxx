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

#ifndef JITTER_EMULATION_HXX
#define JITTER_EMULATION_HXX

//#define VSYNC_LINE_JITTER // makes e.g. Alien jitter, but not on my TV (TJ)

#include "bspf.hxx"
#include "Serializable.hxx"
#include "Random.hxx"

/**
  Emulates the vertical jitter visible on a real CRT when a ROM generates
  unstable vsync timing. Each completed frame is evaluated against configurable
  thresholds for scanline-count and vsync-cycle deviation; a run of unstable
  frames triggers a random vertical roll whose magnitude and recovery speed are
  both tunable.

  @author  Christian Speckner (DirtyHairy)
*/
class JitterEmulation : public Serializable
{
  public:
    // Jitter sensitivity range and defaults (player vs. developer mode)
    static constexpr Int32 MIN_SENSITIVITY = 1;
    static constexpr Int32 MAX_SENSITIVITY = 10;
    static constexpr Int32 PLR_SENSITIVITY = 3;
    static constexpr Int32 DEV_SENSITIVITY = 8;
    // Roll recovery speed range and defaults
    static constexpr Int32 MIN_RECOVERY = 1;
    static constexpr Int32 MAX_RECOVERY = 20;
    static constexpr Int32 PLR_RECOVERY = 10;
    static constexpr Int32 DEV_RECOVERY = 2;

  public:
    JitterEmulation();
    ~JitterEmulation() override = default;

    /**
      Reset to initial state.
     */
    void reset();

    /**
      Set the jitter sensitivity (1 = least sensitive, MAX_SENSITIVITY = most).
     */
    void setSensitivity(Int32 sensitivity);

    /**
      Set the recovery factor controlling how quickly the jitter roll decays.
     */
    void setRecovery(Int32 recoveryFactor) { myJitterRecovery = recoveryFactor; }

    /**
      Set the first visible scanline used for jitter offset clamping.
     */
    void setYStart(Int32 ystart) { myYStart = ystart; }

    /**
      Evaluate the completed frame for timing instability and update the
      jitter offset accordingly.
     */
    void frameComplete(Int32 scanlineCount, Int32 vsyncCycles, Int32 vblankCycles);

    /**
      The current vertical jitter offset in scanlines.
     */
    Int32 jitter() const { return myJitter; }

    /**
      Is the vsync timing within acceptable bounds?
     */
    bool vsyncCorrect() const { return myVsyncCorrect; }

    /**
      Save state.
     */
    bool save(Serializer& out) const override;

    /**
      Restore state.
     */
    bool load(Serializer& in) override;

  private:
    // Allowed scanline count variation before a frame is considered unstable
    // was: 3
    static constexpr Int32 MIN_SCANLINE_DELTA = 1;
    static constexpr Int32 MAX_SCANLINE_DELTA = 5;
    // Acceptable vsync duration range in color clocks
    static constexpr Int32 MIN_VSYNC_CYCLES = 76 * 3 / 4;
    static constexpr Int32 MAX_VSYNC_CYCLES = 76 * 3;
    // Allowed vsync start deviation in color clocks
    static constexpr Int32 MIN_VSYNC_DELTA_1 = 1;
    static constexpr Int32 MAX_VSYNC_DELTA_1 = 76 / 3;
#ifdef VSYNC_LINE_JITTER
    static constexpr Int32 MIN_VSYNC_DELTA_2 = 1;
    static constexpr Int32 MAX_VSYNC_DELTA_2 = 10;
#endif
    // Consecutive unstable frames required before jitter is triggered
    static constexpr Int32 MIN_UNSTABLE_FRAMES = 1;
    static constexpr Int32 MAX_UNSTABLE_FRAMES = 10;
    // Resulting jitter roll magnitude in scanlines
    static constexpr Int32 MIN_JITTER_LINES = 1;
    static constexpr Int32 MAX_JITTER_LINES = 200;
    // Vsync line jitter range
    static constexpr Int32 MIN_VSYNC_LINES = 1;
    static constexpr Int32 MAX_VSYNC_LINES = 5;

  private:
    // PRNG used to randomise jitter magnitude and direction
    Random myRandom;

    // Scanline count of the previous frame (for delta computation)
    Int32 myLastFrameScanlines{0};
    // Vsync cycle count of the previous frame (for delta computation)
    Int32 myLastFrameVsyncCycles{0};
    // Number of consecutive frames with unstable timing
    Int32 myUnstableCount{0};
    // Current vertical jitter offset in scanlines
    Int32 myJitter{0};
    // Recovery factor: how fast the jitter offset decays toward zero
    Int32 myJitterRecovery{0};
    // First visible scanline (used to clamp the jitter offset)
    Int32 myYStart{0};
    // Current sensitivity threshold
    Int32 mySensitivity{MIN_SENSITIVITY};
    // Maximum allowed scanline delta before instability is declared
    Int32 myScanlineDelta{MAX_SCANLINE_DELTA};
    // Maximum allowed vsync cycle count
    Int32 myVsyncCycles{MIN_VSYNC_CYCLES};
    // Maximum allowed vsync start deviation
    Int32 myVsyncDelta1{MAX_VSYNC_DELTA_1};
#ifdef VSYNC_LINE_JITTER
    Int32 myVsyncDelta2{MIN_VSYNC_DELTA_2};
#endif
    // Consecutive unstable frames needed to trigger a roll
    Int32 myUnstableFrames{MAX_UNSTABLE_FRAMES};
    // Roll magnitude in scanlines
    Int32 myJitterLines{MIN_JITTER_LINES};
    // Vsync line jitter magnitude
    Int32 myVsyncLines{MIN_VSYNC_LINES};
    // Whether the last frame's vsync timing was within spec
    bool myVsyncCorrect{true};

  private:
    // Following constructors and assignment operators not supported
    JitterEmulation(const JitterEmulation&) = delete;
    JitterEmulation(JitterEmulation&&) = delete;
    JitterEmulation& operator=(const JitterEmulation&) = delete;
    JitterEmulation& operator=(JitterEmulation&&) = delete;
};

#endif  // JITTER_EMULATION_HXX
