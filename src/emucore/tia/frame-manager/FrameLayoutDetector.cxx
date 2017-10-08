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

#include "FrameLayoutDetector.hxx"
#include "TIAConstants.hxx"

enum Metrics: uInt32 {
  frameLinesNTSC            = 262,
  frameLinesPAL             = 312,
  waitForVsync              = 100,
  tvModeDetectionTolerance  = 20,
  initialGarbageFrames      = TIAConstants::initialGarbageFrames
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FrameLayout FrameLayoutDetector::detectedLayout() const{
  return myPalFrames > myNtscFrames ? FrameLayout::pal : FrameLayout::ntsc;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameLayoutDetector::onReset()
{
  myState = State::waitForVsyncStart;
  myNtscFrames = myPalFrames = 0;
  myLinesWaitingForVsync = 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameLayoutDetector::onSetVsync()
{
  if (myVsync)
    setState(State::waitForVsyncEnd);
  else
    setState(State::waitForVsyncStart);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameLayoutDetector::onNextLine()
{
  const uInt32 frameLines = layout() == FrameLayout::ntsc ? Metrics::frameLinesNTSC : Metrics::frameLinesPAL;

  switch (myState) {
    case State::waitForVsyncStart:
      if (myCurrentFrameTotalLines > frameLines - 3 || myTotalFrames == 0)
        myLinesWaitingForVsync++;

      if (myLinesWaitingForVsync > Metrics::waitForVsync) setState(State::waitForVsyncEnd);

      break;

    case State::waitForVsyncEnd:
      if (++myLinesWaitingForVsync > Metrics::waitForVsync) setState(State::waitForVsyncStart);

      break;

    default:
      throw runtime_error("cannot happen");
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameLayoutDetector::setState(State state)
{
  if (state == myState) return;

  myState = state;

  switch (myState) {
    case State::waitForVsyncEnd:
      myLinesWaitingForVsync = 0;
      break;

    case State::waitForVsyncStart:
      myLinesWaitingForVsync = 0;

      finalizeFrame();
      notifyFrameStart();
      break;

    default:
      throw new runtime_error("cannot happen");
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameLayoutDetector::finalizeFrame()
{
  notifyFrameComplete();

  if (myTotalFrames <= Metrics::initialGarbageFrames) return;

  const uInt32
    deltaNTSC = abs(Int32(myCurrentFrameFinalLines) - Int32(frameLinesNTSC)),
    deltaPAL =  abs(Int32(myCurrentFrameFinalLines) - Int32(frameLinesPAL));

  if (std::min(deltaNTSC, deltaPAL) <= Metrics::tvModeDetectionTolerance)
    layout(deltaNTSC <= deltaPAL ? FrameLayout::ntsc : FrameLayout::pal);
  else if (
    (myCurrentFrameFinalLines < frameLinesPAL) &&
    (myCurrentFrameFinalLines > frameLinesNTSC) &&
    (myCurrentFrameFinalLines % 2)
  )
    layout(FrameLayout::ntsc);
  else
    layout(deltaNTSC <= deltaPAL ? FrameLayout::ntsc : FrameLayout::pal);

  switch (layout()) {
    case FrameLayout::ntsc:
      myNtscFrames++;
      break;

    case FrameLayout::pal:
      myPalFrames++;
      break;

    default:
      throw runtime_error("cannot happen");
  }
}
