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

#ifndef ABSTRACT_FRAME_MANAGER_HXX
#define ABSTRACT_FRAME_MANAGER_HXX

#include <functional>

#include "Serializable.hxx"
#include "FrameLayout.hxx"
#include "TIAConstants.hxx"
#include "bspf.hxx"

/**
  Abstract base class for TIA frame managers. Manages the vsync/vblank/scanline
  lifecycle, drives frame-start and frame-complete callbacks, and tracks basic
  frame metrics. Concrete subclasses implement timing rules for layout
  auto-detection (FrameLayoutDetector) or full frame rendering (FrameManager).

  @author  Christian Speckner (DirtyHairy)
*/
class AbstractFrameManager : public Serializable
{
  public:
    using callback = std::function<void()>;

  public:
    AbstractFrameManager();
    ~AbstractFrameManager() override = default;

    /**
      Configure the frame-start and frame-complete handler callbacks.
     */
    void setHandlers(
      const callback& frameStartCallback,
      const callback& frameCompletionCallback
    );

    /**
      Clear the configured handler callbacks.
     */
    void clearHandlers();

    /**
      Reset to initial state.
     */
    void reset();

    /**
      Called by TIA to notify the start of the next scanline.

      @param lineLength  Realized length of the line just finished, in colour
                         clocks. A normal line is TIAConstants::H_CLOCKS; an
                         RSYNC-truncated line is shorter. Used to accumulate the
                         continuous PAL chroma-subcarrier field phase so that
                         RSYNC no longer forces a fake odd/even line-count flip.
     */
    void nextLine(uInt32 lineLength);

    /**
      Called by TIA on VBLANK writes.
     */
    void setVblank(bool vblank, uInt64 cycles);

    /**
      Called by TIA on VSYNC writes.
     */
    void setVsync(bool vsync, uInt64 cycles);

    /**
      Called when a pixel is rendered.
     */
    virtual void pixelColor(uInt8 color) {}

    /**
      Should the TIA render its frame? Buffered in a flag for performance;
      descendants must update it.
     */
    bool isRendering() const { return myIsRendering; }

    /**
      Is vsync on?
     */
    bool vsync() const { return myVsync; }

    /**
      Is vblank on?
     */
    bool vblank() const { return myVblank; }

    /**
      The number of scanlines in the last finished frame.
     */
    uInt32 scanlinesLastFrame() const { return myCurrentFrameFinalLines; }

    /**
      Did the number of scanlines switch between even and odd? Used for
      color loss emulation.
     */
    bool scanlineParityChanged() const {
      return (myPreviousFrameFinalLines & 0x1) != (myCurrentFrameFinalLines & 0x1);
    }

    /**
      Is the PAL chroma subcarrier inverted for the field about to be decoded?

      This is the phase-based replacement for (scanlinesLastFrame() & 1): rather
      than taking the parity of the integer line count, it rounds the last
      frame's realized colour-clock total to the nearest whole line and takes
      the parity of that. With no RSYNC every line is exactly H_CLOCKS, the
      rounding is exact, and this reduces to (scanlinesLastFrame() & 1) — the
      non-RSYNC picture is byte-identical. An RSYNC-shortened line advances the
      field phase by a fraction of a line, so e.g. Fatal Run's "313" lines is
      really ~312.16 → rounds to 312 (even) → colour preserved, matching real
      PAL hardware. A genuine odd-line frame still rounds odd and inverts.

      Because the accumulator keys off realized colour clocks rather than the
      bookkeeping line count, it is also robust to the double-RSYNC phantom
      scanline question: a phantom line contributes only its handful of clocks
      to the phase (not a full line of parity), so whether or not TIA counts it
      as a scanline, the decoded field phase is unaffected.
     */
    bool chromaPhaseInverted() const {
      return (((myChromaClocksLastFrame + TIAConstants::H_CLOCKS / 2)
                / TIAConstants::H_CLOCKS) & 0x1) != 0;
    }

    /**
      The total number of frames rendered.
     */
    uInt32 frameCount() const { return myTotalFrames; }

    /**
      The configured or auto-detected frame layout (PAL / NTSC).
     */
    FrameLayout layout() const { return myLayout; }

    /**
      Save state.
     */
    bool save(Serializer& out) const override;

    /**
      Restore state.
     */
    bool load(Serializer& in) override;

  public:
    // The following methods are implemented as noops and should be overridden as
    // required. All of these are irrelevant if nothing is displayed (during
    // autodetect).

    /**
      Set the jitter simulation sensitivity to unstable video signals.
     */
    virtual void setJitterSensitivity(uInt8 sensitivity) {}

    /**
      Set the jitter recovery factor (how quickly jitter simulation recovers).
     */
    virtual void setJitterRecovery(uInt8 factor) {}

    /**
      Is jitter simulation enabled?
     */
    virtual bool jitterEnabled() const { return false; }

    /**
      Enable or disable jitter simulation.
     */
    virtual void enableJitter(bool enabled) {}

    /**
      Is vsync within spec?
     */
    virtual bool vsyncCorrect() const { return true; }

    /**
      The scanline difference between the last two frames. Used by TIA to
      clear any scanlines that were not repainted.
     */
    virtual Int32 missingScanlines() const { return 0; }

    /**
      Frame height in visible scanlines.
     */
    virtual uInt32 height() const { return 0; }

    /**
      The current y coordinate (valid only during rendering).
     */
    virtual uInt32 getY() const { return 0; }

    /**
      The current number of scanlines in the current frame, including
      invisible lines.
     */
    virtual uInt32 scanlines() const { return 0; }

    /**
      Set the vcenter offset.
     */
    virtual void setVcenter(Int32 vcenter) {}

    /**
      The configured vcenter offset.
     */
    virtual Int32 vcenter() const { return 0; }

    /**
      The calculated minimum vcenter value.
     */
    virtual Int32 minVcenter() const { return 0; }

    /**
      The calculated maximum vcenter value.
     */
    virtual Int32 maxVcenter() const { return 0; }

    /**
      Set the vertical size adjustment (in scanlines).
     */
    virtual void setAdjustVSize(Int32 adjustVSize) {}

    /**
      The configured vertical size adjustment.
     */
    virtual Int32 adjustVSize() const { return 0; }

    /**
      The frame start scanline.
     */
    virtual uInt32 startLine() const { return 0; }

    /**
      Set the frame layout. May be a no-op on the auto-detection manager.
     */
    virtual void setLayout(FrameLayout mode) {}

  protected:
    // The following are template methods that can be implemented to hook into
    // the frame logic.

    /**
      Called when vblank changes.
     */
    virtual void onSetVblank(uInt64 cycles) {}

    /**
      Called when vsync changes.
     */
    virtual void onSetVsync(uInt64 cycles) {}

    /**
      Called when the next line is signalled, after internal bookkeeping
      has been updated.
     */
    virtual void onNextLine() {}

    /**
      Called on reset, after the base class has reset.
     */
    virtual void onReset() {}

    /**
      Called after a frame layout change.
     */
    virtual void onLayoutChange() {}

    /**
      Called during state save, after the base class has serialized its state.
     */
    virtual bool onSave(Serializer& out) const {
      throw std::runtime_error("cannot be serialized");
    }

    /**
      Called during state restore, after the base class has restored its state.
     */
    virtual bool onLoad(Serializer& in) {
      throw std::runtime_error("cannot be serialized");
    }

  protected:
    // These need to be called in order to drive the frame lifecycle of the
    // emulation.

    /**
      Signal frame start to the registered callback.
     */
    void notifyFrameStart();

    /**
      Signal frame completion to the registered callback.
     */
    void notifyFrameComplete();

    /**
      Internal setter to update the frame layout.
     */
    void layout(FrameLayout layout);

  protected:
    // Whether the TIA should currently be rendering (buffered for performance)
    bool myIsRendering{false};

    // Vsync flag
    bool myVsync{false};

    // Vblank flag
    bool myVblank{false};

    // Current scanline count in the current frame
    uInt32 myCurrentFrameTotalLines{0};

    // Total number of scanlines in the last complete frame
    uInt32 myCurrentFrameFinalLines{0};

    // Total number of scanlines in the second last complete frame
    uInt32 myPreviousFrameFinalLines{0};

    // Accumulated realized colour clocks of the current frame. A normal line
    // adds H_CLOCKS (228); an RSYNC-shortened line adds fewer. Drives the PAL
    // chroma field phase (see chromaPhaseInverted()).
    uInt32 myCurrentFrameChromaClocks{0};

    // Snapshot of the above for the last complete frame.
    uInt32 myChromaClocksLastFrame{0};

    // Total frame count
    uInt32 myTotalFrames{0};

  private:
    // Current frame layout
    FrameLayout myLayout{FrameLayout::pal};

    // Frame lifecycle callbacks
    callback myOnFrameStart{nullptr};
    callback myOnFrameComplete{nullptr};

  private:
    // Following constructors and assignment operators not supported
    AbstractFrameManager(const AbstractFrameManager&) = delete;
    AbstractFrameManager(AbstractFrameManager&&) = delete;
    AbstractFrameManager& operator=(const AbstractFrameManager&) = delete;
    AbstractFrameManager& operator=(AbstractFrameManager&&) = delete;
};

#endif  // ABSTRACT_FRAME_MANAGER_HXX
