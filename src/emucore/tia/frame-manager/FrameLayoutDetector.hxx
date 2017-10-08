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

#ifndef TIA_FRAME_LAYOUT_DETECTOR
#define TIA_FRAME_LAYOUT_DETECTOR

#include "AbstractFrameManager.hxx"
#include "FrameLayout.hxx"

class FrameLayoutDetector: public AbstractFrameManager {
  public:

    FrameLayoutDetector() = default;

  public:

    FrameLayout detectedLayout() const;

  protected:

    void onSetVsync() override;

    void onReset() override;

    void onNextLine() override;

  private:

    enum State {
      waitForVsyncStart,
      waitForVsyncEnd
    };


  private:

    void setState(State state);

    void finalizeFrame();

  private:

    State myState;

    uInt32 myNtscFrames, myPalFrames;

    uInt32 myLinesWaitingForVsync;

};

#endif // TIA_FRAME_LAYOUT_DETECTOR
