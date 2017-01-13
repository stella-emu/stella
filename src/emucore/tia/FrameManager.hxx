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
// Copyright (c) 1995-2017 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#ifndef TIA_6502TS_CORE_FRAME_MANAGER
#define TIA_6502TS_CORE_FRAME_MANAGER

#include <functional>

#include "Serializable.hxx"
#include "TvMode.hxx"
#include "bspf.hxx"

class FrameManager : public Serializable
{
  public:

    using callback = std::function<void()>;

  public:

    FrameManager();

  public:

    static uInt8 initialGarbageFrames();

    void setHandlers(callback frameStartCallback, callback frameCompletionCallback);

    void reset();

    void nextLine();

    void setVblank(bool vblank);

    void setVsync(bool vsync);

    bool isRendering() const;

    TvMode tvMode() const;

    bool vblank() const { return myVblank; }

    bool vsync() const { return myVsync; }

    uInt32 height() const;

    void setFixedHeight(uInt32 height);

    uInt32 currentLine() const;

    uInt32 scanlines() const;

    uInt32 frameCount() const { return myTotalFrames; }

    float frameRate() const { return myFrameRate; }

    void setYstart(uInt32 ystart);

    uInt32 ystart() const;

    void autodetectTvMode(bool toggle);

    void setTvMode(TvMode mode);

    /**
      Serializable methods (see that class for more information).
    */
    bool save(Serializer& out) const override;
    bool load(Serializer& in) override;
    string name() const override { return "TIA_FrameManager"; }

  public:

    static constexpr uInt32 frameBufferHeight = 320;

  private:

    enum State {
      waitForVsyncStart,
      waitForVsyncEnd,
      waitForFrameStart,
      frame
    };

    enum VblankMode {
      locked,
      floating,
      fixed
    };

  private:

    void updateTvMode(TvMode mode);

    void updateAutodetectedTvMode();

    void setState(State state);

    void finalizeFrame(State state = State::waitForVsyncStart);

    void nextLineInVsync();

  private:

    callback myOnFrameStart;
    callback myOnFrameComplete;

    TvMode myMode;
    bool myAutodetectTvMode;
    State myState;
    uInt32 myLineInState;
    uInt32 myCurrentFrameTotalLines;
    uInt32 myCurrentFrameFinalLines;
    uInt32 myVsyncLines;
    float  myFrameRate;

    uInt32 myTotalFrames;
    uInt32 myFramesInMode;
    bool myModeConfirmed;

    bool myVsync;
    bool myVblank;

    uInt32 myVblankLines;
    uInt32 myKernelLines;
    uInt32 myOverscanLines;
    uInt32 myFrameLines;
    uInt32 myFixedHeight;

    VblankMode myVblankMode;
    uInt32 myLastVblankLines;
    uInt32 myYstart;
    uInt8 myVblankViolations;
    uInt8 myStableVblankFrames;
    bool myVblankViolated;

  private:
    FrameManager(const FrameManager&) = delete;
    FrameManager(FrameManager&&) = delete;
    FrameManager& operator=(const FrameManager&) = delete;
    FrameManager& operator=(FrameManager&&) = delete;
};

#endif // TIA_6502TS_CORE_FRAME_MANAGER
