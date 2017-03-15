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

#ifndef TIA_FRAME_MANAGER
#define TIA_FRAME_MANAGER

#include <functional>

#include "VblankManager.hxx"
#include "Serializable.hxx"
#include "FrameLayout.hxx"
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

    bool isRendering() const { return myState == State::frame; }

    FrameLayout layout() const { return myLayout; }

    bool vblank() const { return myVblankManager.vblank(); }

    bool vsync() const { return myVsync; }

    uInt32 height() const { return myHeight; }

    void setFixedHeight(uInt32 height);

    uInt32 getY() const { return myY; }

    uInt32 scanlines() const { return myCurrentFrameTotalLines; }

    uInt32 scanlinesLastFrame() const { return myCurrentFrameFinalLines; }

    uInt32 missingScanlines() const;

    uInt32 frameCount() const { return myTotalFrames; }

    float frameRate() const { return myFrameRate; }

    void setYstart(uInt32 ystart) { myVblankManager.setYstart(ystart); }

    uInt32 ystart() const { return myVblankManager.ystart(); }
    bool ystartIsAuto(uInt32 line) const { return myVblankManager.ystartIsAuto(line); }

    void autodetectLayout(bool toggle) { myAutodetectLayout = toggle; }

    void setLayout(FrameLayout mode) { if (!myAutodetectLayout) updateLayout(mode); }

    /**
      Serializable methods (see that class for more information).
    */
    bool save(Serializer& out) const override;
    bool load(Serializer& in) override;
    string name() const override { return "TIA_FrameManager"; }

  public:
    static constexpr uInt32 frameBufferHeight = 320;
    static constexpr uInt32 minYStart = 1, maxYStart = 64;
    static constexpr uInt32 minViewableHeight = 210, maxViewableHeight = 256;

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
      final,
      fixed
    };

  private:

    void updateLayout(FrameLayout mode);

    void updateAutodetectedLayout();

    void setState(State state);

    void finalizeFrame();

    void nextLineInVsync();

  private:

    callback myOnFrameStart;
    callback myOnFrameComplete;

    VblankManager myVblankManager;

    FrameLayout myLayout;
    bool myAutodetectLayout;
    State myState;
    uInt32 myLineInState;
    uInt32 myCurrentFrameTotalLines;
    uInt32 myCurrentFrameFinalLines;
    uInt32 myVsyncLines;
    float  myFrameRate;
    uInt32 myY, myLastY;
    bool myFramePending;

    uInt32 myTotalFrames;
    uInt32 myFramesInMode;
    bool myModeConfirmed;

    bool myVsync;

    uInt32 myVblankLines;
    uInt32 myKernelLines;
    uInt32 myOverscanLines;
    uInt32 myFrameLines;
    uInt32 myHeight;
    uInt32 myFixedHeight;

  private:
    FrameManager(const FrameManager&) = delete;
    FrameManager(FrameManager&&) = delete;
    FrameManager& operator=(const FrameManager&) = delete;
    FrameManager& operator=(FrameManager&&) = delete;
};

#endif // TIA_FRAME_MANAGER
