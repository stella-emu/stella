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

#ifndef TIA_YSTART_DETECTOR
#define TIA_YSTART_DETECTOR

#include "AbstractFrameManager.hxx"

class YStartDetector: public AbstractFrameManager {

  public:

    YStartDetector() = default;

  public:

    uInt32 detectedYStart() const;

    void setLayout(FrameLayout layout) override { this->layout(layout); }

  protected:

    void onSetVsync() override;

    void onReset() override;

    void onNextLine() override;

  private:

    enum State {
      waitForVsyncStart,
      waitForVsyncEnd,
      waitForFrameStart
    };

    enum VblankMode {
      locked,
      floating
    };

  private:

    void setState(State state);

    bool shouldTransitionToFrame();

  private:

    State myState;

    VblankMode myVblankMode;

    uInt32 myLinesWaitingForVsyncToStart;

    uInt32 myCurrentVblankLines;
    uInt32 myLastVblankLines;
    uInt32 myVblankViolations;
    uInt32 myStableVblankFrames;

    bool myVblankViolated;

  private:

    YStartDetector(const YStartDetector&) = delete;
    YStartDetector(YStartDetector&&) = delete;
    YStartDetector& operator=(const YStartDetector&) = delete;
    YStartDetector& operator=(YStartDetector&&) = delete;
};

#endif // TIA_YSTART_DETECTOR
