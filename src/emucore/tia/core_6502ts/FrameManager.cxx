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
// Copyright (c) 1995-2016 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

#include "FrameManager.hxx"

enum Metrics: uInt32 {
  vblankNTSC               = 40,
  vblankPAL                = 48,
  kernelNTSC               = 192,
  kernelPAL                = 228,
  overscanNTSC             = 30,
  overscanPAL              = 36,
  vsync                    = 3,
  visibleOverscan          = 20,
  maxUnderscan             = 10,
  maxFramesWithoutVsync    = 50,
  tvModeDetectionTolerance = 20
};

static constexpr uInt32
  frameLinesNTSC = Metrics::vsync + Metrics::vblankNTSC + Metrics::kernelNTSC + Metrics::overscanNTSC,
  frameLinesPAL = Metrics::vsync + Metrics::vblankPAL + Metrics::kernelPAL + Metrics::overscanPAL;

namespace TIA6502tsCore {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FrameManager::FrameManager()
  : myMode(TvMode::pal)
{
  setTvMode(TvMode::ntsc);
  reset();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameManager::setHandlers(
  FrameManager::callback frameStartCallback,
  FrameManager::callback frameCompleteCallback
)
{
  myOnFrameStart = frameStartCallback;
  myOnFrameComplete = frameCompleteCallback;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameManager::reset()
{
  myState = State::waitForVsyncStart;
  myCurrentFrameTotalLines = 0;
  myLineInState = 0;
  myLinesWithoutVsync = 0;
  myWaitForVsync = true;
  myVsync = false;
  myVblank = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameManager::nextLine()
{
  myCurrentFrameTotalLines++;
  myLineInState++;

  switch (myState)
  {
    case State::waitForVsyncStart:
    case State::waitForVsyncEnd:
      if (myLinesWithoutVsync > myMaxLinesWithoutVsync) {
        myWaitForVsync = false;
        setState(State::waitForFrameStart);
      }
      break;

    case State::waitForFrameStart:
      if (myWaitForVsync) {
        if (myLineInState >= (myVblank ? myVblankLines : myVblankLines - Metrics::maxUnderscan))
          setState(State::frame);
      } else {
        if (!myVblank) {
          setState(State::frame);
        }
      }
      break;

    case State::frame:
      if (myLineInState >= myKernelLines + Metrics::visibleOverscan) {
        finalizeFrame();
      }
      break;

    case State::overscan:
      if (myLineInState >= myOverscanLines - Metrics::visibleOverscan) {
        setState(myWaitForVsync ? State::waitForVsyncStart : State::waitForFrameStart);
      }
      break;

    default:
      throw runtime_error("frame manager: invalid state");
  }

  if (myWaitForVsync) myLinesWithoutVsync++;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameManager::setVblank(bool vblank)
{
  myVblank = vblank;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameManager::setVsync(bool vsync)
{
  if (!myWaitForVsync || vsync == myVsync) return;

  myVsync = vsync;

  switch (myState)
  {
    case State::waitForVsyncStart:
    case State::waitForFrameStart:
    case State::overscan:
      if (myVsync) setState(State::waitForVsyncEnd);
      break;

    case State::waitForVsyncEnd:
      if (!myVsync) {
        setState(State::waitForFrameStart);
        myLinesWithoutVsync = 0;
      }
      break;

    case State::frame:
      if (myVsync) finalizeFrame();
      break;

    default:
      throw runtime_error("frame manager: invalid state");
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FrameManager::isRendering() const
{
  return myState == State::frame;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FrameManager::TvMode FrameManager::tvMode() const
{
  return myMode;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FrameManager::vblank() const
{
  return myVblank;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 FrameManager::height() const
{
  return myKernelLines + Metrics::visibleOverscan;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 FrameManager::currentLine() const
{
  return myState == State::frame ? myLineInState : 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameManager::setTvMode(FrameManager::TvMode mode)
{
  if (mode == myMode) return;

  myMode = mode;

  switch (myMode)
  {
    case TvMode::ntsc:
      myVblankLines     = Metrics::vblankNTSC;
      myKernelLines     = Metrics::kernelNTSC;
      myOverscanLines   = Metrics::overscanNTSC;
      break;

    case TvMode::pal:
      myVblankLines     = Metrics::vblankPAL;
      myKernelLines     = Metrics::kernelPAL;
      myOverscanLines   = Metrics::overscanPAL;
      break;

    default:
      throw runtime_error("frame manager: invalid TV mode");
  }

  myFrameLines = Metrics::vsync + myVblankLines + myKernelLines + myOverscanLines;
  myMaxLinesWithoutVsync = myFrameLines * Metrics::maxFramesWithoutVsync;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameManager::setState(FrameManager::State state)
{
  if (myState == state) return;

  myState = state;
  myLineInState = 0;

  if (myState == State::frame && myOnFrameStart) myOnFrameStart();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameManager::finalizeFrame()
{
  const uInt32
    deltaNTSC = abs(Int32(myCurrentFrameTotalLines) - Int32(frameLinesNTSC)),
    deltaPAL =  abs(Int32(myCurrentFrameTotalLines) - Int32(frameLinesPAL));

  if (std::min(deltaNTSC, deltaPAL) <= Metrics::tvModeDetectionTolerance) {
    setTvMode(deltaNTSC <= deltaPAL ? TvMode::ntsc : TvMode::pal);
  }

  if (myOnFrameComplete) {
    myOnFrameComplete();
  }

  myCurrentFrameTotalLines = 0;
  setState(State::overscan);
}

} // namespace TIA6502tsCore
