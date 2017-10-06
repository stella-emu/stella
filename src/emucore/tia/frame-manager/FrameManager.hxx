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

#include "AbstractFrameManager.hxx"
#include "VblankManager.hxx"
#include "TIAConstants.hxx"
#include "bspf.hxx"

class FrameManager: public AbstractFrameManager {
  public:

    FrameManager();

  public:

    void setJitterFactor(uInt8 factor) override { myVblankManager.setJitterFactor(factor); }

    bool jitterEnabled() const override { return myJitterEnabled; }

    void enableJitter(bool enabled) override;

    uInt32 height() const override { return myHeight; };

    void setFixedHeight(uInt32 height) override;

    uInt32 getY() const override { return myY; }

    uInt32 scanlines() const override { return myCurrentFrameTotalLines; }

    Int32 missingScanlines() const override;

    void setYstart(uInt32 ystart) override { myVblankManager.setYstart(ystart); }

    uInt32 ystart() const override { return myVblankManager.ystart(); }

    bool ystartIsAuto(uInt32 line) const override { return myVblankManager.ystartIsAuto(line); };

    void autodetectLayout(bool toggle) override { myAutodetectLayout = toggle; }

    void setLayout(FrameLayout mode) override { if (!myAutodetectLayout) layout(mode); }

    void onSetVblank() override;

    void onSetVsync() override;

    void onNextLine() override;

    void onReset() override;

    void onLayoutChange() override;

    bool onSave(Serializer& out) const override;

    bool onLoad(Serializer& in) override;

    string name() const override { return "TIA_FrameManager"; }

  private:

    enum State {
      waitForVsyncStart,
      waitForVsyncEnd,
      waitForFrameStart,
      frame
    };

  private:

    void updateAutodetectedLayout();

    void setState(State state);

    void finalizeFrame();

    void nextLineInVsync();

    void handleJitter(Int32 scanlineDifference);

    void updateIsRendering();

  private:

    VblankManager myVblankManager;

    bool myAutodetectLayout;
    State myState;
    uInt32 myLineInState;
    uInt32 myVsyncLines;
    uInt32 myY, myLastY;
    bool myFramePending;

    uInt32 myFramesInMode;
    bool myModeConfirmed;

    uInt32 myStableFrames;
    uInt32 myStabilizationFrames;
    bool myHasStabilized;

    uInt32 myVblankLines;
    uInt32 myKernelLines;
    uInt32 myOverscanLines;
    uInt32 myFrameLines;
    uInt32 myHeight;
    uInt32 myFixedHeight;

    bool myJitterEnabled;

    Int32 myStableFrameLines;
    uInt8 myStableFrameHeightCountdown;

  private:

    FrameManager(const FrameManager&) = delete;
    FrameManager(FrameManager&&) = delete;
    FrameManager& operator=(const FrameManager&) = delete;
    FrameManager& operator=(FrameManager&&) = delete;
};

#endif // TIA_FRAME_MANAGER
