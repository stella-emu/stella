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

#ifndef FRAME_MANAGER_HXX
#define FRAME_MANAGER_HXX

#include "AbstractFrameManager.hxx"
#include "TIAConstants.hxx"
#include "bspf.hxx"
#include "JitterEmulation.hxx"

/**
  Full TIA frame manager. Implements the complete NTSC/PAL frame timing state
  machine (vsync detection, vblank windowing, visible frame boundaries, vcenter
  and vsize adjustment, and optional jitter emulation). Used for all normal
  emulation after the initial layout auto-detection phase.

  @author  Christian Speckner (DirtyHairy)
*/
class FrameManager: public AbstractFrameManager
{
  public:
    enum Metrics: uInt16 {
      vblankNTSC = 37,
      vblankPAL = 45,
      vsync = 3,
      frameSizeNTSC = 262,
      frameSizePAL = 312,
      // visible range: 217..239
      baseHeightNTSC = 228,
      // visible range: 260..288
      baseHeightPAL = 274,
      // NOLINTNEXTLINE(bugprone-incorrect-roundings) lround not constexpr until C++23
      maxHeight = static_cast<uInt32>(baseHeightPAL * 1.05 + 0.5),
      maxLinesVsync = 50,
      initialGarbageFrames = TIAConstants::initialGarbageFrames,
      ystartNTSC = 23,
      ystartPAL = 32
    };

  public:
    FrameManager();
    ~FrameManager() override = default;

    /**
      Set the jitter simulation sensitivity to unstable video signals.
     */
    void setJitterSensitivity(uInt8 sensitivity) override {
      myJitterEmulation.setSensitivity(sensitivity);
    }

    /**
      Set the jitter recovery factor (how quickly jitter simulation recovers).
     */
    void setJitterRecovery(uInt8 factor) override {
      myJitterEmulation.setRecovery(factor);
    }

    /**
      Is jitter simulation enabled?
     */
    bool jitterEnabled() const override { return myJitterEnabled; }

    /**
      Enable or disable jitter simulation.
     */
    void enableJitter(bool enabled) override { myJitterEnabled = enabled; }

    /**
      Is vsync within spec? When jitter is disabled, always true.
     */
    bool vsyncCorrect() const override {
      return !myJitterEnabled || myJitterEmulation.vsyncCorrect();
    }

    /**
      Frame height in visible scanlines.
     */
    uInt32 height() const override { return myHeight; }

    /**
      The current y coordinate (valid only during rendering).
     */
    uInt32 getY() const override { return myY; }

    /**
      The current number of scanlines in the current frame, including
      invisible lines.
     */
    uInt32 scanlines() const override { return myCurrentFrameTotalLines; }

    /**
      The scanline difference between the last two frames.
     */
    Int32 missingScanlines() const override;

    /**
      Set the vcenter offset.
     */
    void setVcenter(Int32 vcenter) override;

    /**
      The configured vcenter offset.
     */
    Int32 vcenter() const override { return myVcenter; }

    /**
      The minimum valid vcenter value for the current layout.
     */
    Int32 minVcenter() const override { return TIAConstants::minVcenter; }

    /**
      The maximum valid vcenter value for the current layout.
     */
    Int32 maxVcenter() const override { return myMaxVcenter; }

    /**
      Set the vertical size adjustment (in scanlines).
     */
    void setAdjustVSize(Int32 adjustVSize) override;

    /**
      The configured vertical size adjustment.
     */
    Int32 adjustVSize() const override { return myVSizeAdjust; }

    /**
      The frame start scanline.
     */
    uInt32 startLine() const override { return myYStart; }

    /**
      Set the frame layout and recalculate derived metrics.
     */
    void setLayout(FrameLayout mode) override { layout(mode); }

  protected:
    /**
      Hook into vblank changes.
     */
    void onSetVblank(uInt64 cycles) override;

    /**
      Hook into vsync changes.
     */
    void onSetVsync(uInt64 cycles) override;

    /**
      Hook into scanline advances.
     */
    void onNextLine() override;

    /**
      Hook into reset.
     */
    void onReset() override;

    /**
      Hook into layout changes to recalculate metrics.
     */
    void onLayoutChange() override;

    /**
      Serialize frame manager state.
     */
    bool onSave(Serializer& out) const override;

    /**
      Restore frame manager state.
     */
    bool onLoad(Serializer& in) override;

  private:
    enum class State: uInt8 {
      // Waiting for VSYNC to go high
      waitForVsyncStart,
      // Waiting for VSYNC to go low
      waitForVsyncEnd,
      // Waiting for the frame to begin (post-vblank)
      waitForFrameStart,
      // Inside a rendered frame
      frame
    };

  private:
    /**
      Transition to a new state.
     */
    void setState(State state);

    /**
      Update myIsRendering from the current state and y position.
     */
    void updateIsRendering();

    /**
      Recalculate layout-dependent metrics (vblank lines, frame height, etc.).
     */
    void recalculateMetrics();

  private:
    // Current frame timing state
    State myState{State::waitForVsyncStart};
    // Scanlines spent in the current state
    uInt32 myLineInState{0};
    // Scanlines counted during the vsync pulse
    uInt32 myVsyncLineCount{0};
    // Current and previous rendered y coordinate
    uInt32 myY{0}, myLastY{0};

    // Expected vblank scanline count for the current layout
    uInt32 myVblankLines{0};
    // Total scanlines per frame for the current layout
    uInt32 myFrameLines{0};
    // Visible frame height after applying vcenter and vsize adjustments
    uInt32 myHeight{0};
    // First visible scanline (derived from layout and vcenter)
    uInt32 myYStart{0};
    // Configured vcenter offset
    Int32 myVcenter{0};
    // Maximum valid vcenter for the current layout
    Int32 myMaxVcenter{0};
    // Vertical size adjustment in scanlines
    Int32 myVSizeAdjust{0};

    // Cycle count at vsync rising edge (used by jitter emulation)
    uInt64 myVsyncStart{0};
    // Cycle count at vsync falling edge
    uInt64 myVsyncEnd{0};
    // Cycle count at vblank rising edge
    uInt64 myVblankStart{0};
    // Total cycles spent in vblank this frame
    uInt64 myVblankCycles{0};
    // Whether jitter simulation is active
    bool myJitterEnabled{false};

    // Jitter emulation state
    JitterEmulation myJitterEmulation;

  private:
    // Following constructors and assignment operators not supported
    FrameManager(const FrameManager&) = delete;
    FrameManager(FrameManager&&) = delete;
    FrameManager& operator=(const FrameManager&) = delete;
    FrameManager& operator=(FrameManager&&) = delete;
};

#endif  // FRAME_MANAGER_HXX
