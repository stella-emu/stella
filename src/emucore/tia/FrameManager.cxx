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
  myCurrentFrameTotalLines = myCurrentFrameFinalLines = 0;
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

#ifdef TIA_FRAMEMANAGER_DEBUG_LOG
  (cout << "vsync " << myVsync << " -> " << vsync << ": state " << int(myState) << " @ " << myLineInState << "\n").flush();
#endif

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
      if (myVsync) finalizeFrame(State::waitForVsyncEnd);
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
TvMode FrameManager::tvMode() const
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
uInt32 FrameManager::scanlines() const
{
  return myState == State::frame ? myLineInState : myCurrentFrameFinalLines;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameManager::setTvMode(TvMode mode)
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

#ifdef TIA_FRAMEMANAGER_DEBUG_LOG
  (cout << "state change " << myState << " -> " << state << " @ " << myLineInState << "\n").flush();
#endif // TIA_FRAMEMANAGER_DEBUG_LOG

  myState = state;
  myLineInState = 0;

  if (myState == State::frame && myOnFrameStart) myOnFrameStart();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameManager::finalizeFrame(FrameManager::State state)
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

#ifdef TIA_FRAMEMANAGER_DEBUG_LOG
  (cout << "frame complete @ " << myLineInState << " (" << myCurrentFrameFinalLines << " total)" << "\n").flush();
#endif // TIA_FRAMEMANAGER_DEBUG_LOG

  myCurrentFrameFinalLines = myCurrentFrameTotalLines;
  myCurrentFrameTotalLines = 0;
  setState(state);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// TODO: implement this once the class is finalized
bool FrameManager::save(Serializer& out) const
{
  try
  {
    out.putString(name());

    // TODO - save instance variables
  }
  catch(...)
  {
    cerr << "ERROR: TIA_FrameManager::save" << endl;
    return false;
  }

  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// TODO: implement this once the class is finalized
bool FrameManager::load(Serializer& in)
{
  try
  {
    if(in.getString() != name())
      return false;

    // TODO - load instance variables
  }
  catch(...)
  {
    cerr << "ERROR: TIA_FrameManager::load" << endl;
    return false;
  }

  return false;
}
