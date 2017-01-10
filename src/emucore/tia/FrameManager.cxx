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

// #define TIA_FRAMEMANAGER_DEBUG_LOG

#include <algorithm>

#include "FrameManager.hxx"

enum Metrics: uInt32 {
  vblankNTSC                    = 37,
  vblankPAL                     = 45,
  kernelNTSC                    = 192,
  kernelPAL                     = 228,
  overscanNTSC                  = 30,
  overscanPAL                   = 36,
  vsync                         = 3,
  vsyncLimit                    = 30,
  visibleOverscan               = 20,
  maxUnderscan                  = 10,
  tvModeDetectionTolerance      = 20,
  initialGarbageFrames          = 20,
  framesForModeConfirmation     = 5,
  maxVblankViolations           = 2,
  minStableVblankFrames         = 1
};

static constexpr uInt32
  frameLinesNTSC = Metrics::vsync + Metrics::vblankNTSC + Metrics::kernelNTSC + Metrics::overscanNTSC,
  frameLinesPAL = Metrics::vsync + Metrics::vblankPAL + Metrics::kernelPAL + Metrics::overscanPAL;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FrameManager::FrameManager()
  : myMode(TvMode::pal),
    myAutodetectTvMode(true),
    myFixedHeight(0),
    myVblankMode(VblankMode::floating),
    myYstart(0)
{
  updateTvMode(TvMode::ntsc);
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
  myFrameRate = 60.0;
  myLineInState = 0;
  myVsync = false;
  myVblank = false;
  myTotalFrames = 0;
  myFramesInMode = 0;
  myModeConfirmed = false;
  myLastVblankLines = 0;
  myVblankViolations = 0;
  myStableVblankFrames = 0;
  myVblankViolated = false;
  myVsyncLines = 0;

  if (myVblankMode != VblankMode::fixed) myVblankMode = VblankMode::floating;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameManager::nextLine()
{
  myCurrentFrameTotalLines++;
  myLineInState++;

  switch (myState)
  {
    case State::waitForVsyncStart:
      if (Int32(myLineInState) > Int32(myFrameLines - myCurrentFrameFinalLines) || !myCurrentFrameFinalLines)
        myVsyncLines++;

      if (myVsyncLines > Metrics::vsyncLimit) setState(State::waitForFrameStart);

      break;

    case State::waitForVsyncEnd:
      if (++myVsyncLines > Metrics::vsyncLimit)
        setState(State::waitForFrameStart);

      break;

    case State::waitForFrameStart:
      nextLineInVsync();
      break;

    case State::frame:
      if (myLineInState >= (myFixedHeight > 0 ? myFixedHeight : (myKernelLines + Metrics::visibleOverscan)))
        finalizeFrame();
      break;

    default:
      throw runtime_error("frame manager: invalid state");
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameManager::nextLineInVsync()
{
  bool shouldTransition = myLineInState >= (myVblank ? myVblankLines : myVblankLines - Metrics::maxUnderscan);

  switch (myVblankMode) {
    case VblankMode::floating:

      if (shouldTransition) {
        if (myTotalFrames > initialGarbageFrames && myLineInState == myLastVblankLines)
          myStableVblankFrames++;
        else
          myStableVblankFrames = 0;

        myLastVblankLines = myLineInState;

        setState(State::frame);
      }

      if (myStableVblankFrames >= Metrics::minStableVblankFrames) {
        myVblankMode = VblankMode::locked;
        myVblankViolations = 0;
      }

      break;

    case VblankMode::locked:

      if (myLineInState == myLastVblankLines) {
        if (shouldTransition && !myVblankViolated)
          myVblankViolations = 0;
        else {
          if (!myVblankViolated) myVblankViolations++;
          myVblankViolated = true;
        }

        setState(State::frame);
      } else if (shouldTransition){
        if (!myVblankViolated) myVblankViolations++;
        myVblankViolated = true;
      }

      if (myVblankViolations > Metrics::maxVblankViolations)
        myVblankMode = VblankMode::floating;

      break;

    case VblankMode::fixed:
      if (myLineInState > myYstart) setState(State::frame);
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameManager::setVsync(bool vsync)
{
  if (vsync == myVsync) return;

#ifdef TIA_FRAMEMANAGER_DEBUG_LOG
  (cout << "vsync " << myVsync << " -> " << vsync << ": state " << int(myState) << " @ " << myLineInState << "\n").flush();
#endif

  myVsync = vsync;

  switch (myState)
  {
    case State::waitForVsyncStart:
    case State::waitForFrameStart:
      if (myVsync) setState(State::waitForVsyncEnd);
      break;

    case State::waitForVsyncEnd:
      if (!myVsync)
        setState(State::waitForFrameStart);
      break;

    case State::frame:
      if (myVsync) {
        myCurrentFrameTotalLines++;
        finalizeFrame(State::waitForVsyncEnd);
      }
      break;

    default:
      throw runtime_error("frame manager: invalid state");
  }
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

  switch (myState) {
    case State::waitForFrameStart:
      myVsyncLines = 0;
      myVblankViolated = false;
      break;

    case State::frame:
      myVsyncLines = 0;
      if (myOnFrameStart) myOnFrameStart();
      break;

    default:
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameManager::finalizeFrame(FrameManager::State state)
{
  myCurrentFrameFinalLines = myCurrentFrameTotalLines;
  myCurrentFrameTotalLines = 0;
  myTotalFrames++;

  if (myOnFrameComplete) {
    myOnFrameComplete();
  }

#ifdef TIA_FRAMEMANAGER_DEBUG_LOG
  (cout << "frame complete @ " << myLineInState << " (" << myCurrentFrameFinalLines << " total)" << "\n").flush();
#endif // TIA_FRAMEMANAGER_DEBUG_LOG

  setState(state);

  if (myAutodetectTvMode) updateAutodetectedTvMode();

  myFrameRate = (myMode == TvMode::pal ? 15600.0 : 15720.0) /
                myCurrentFrameFinalLines;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameManager::updateAutodetectedTvMode()
{
  if (myTotalFrames <= Metrics::initialGarbageFrames) {
    return;
  }

  const TvMode oldMode = myMode;

  const uInt32
    deltaNTSC = abs(Int32(myCurrentFrameFinalLines) - Int32(frameLinesNTSC)),
    deltaPAL =  abs(Int32(myCurrentFrameFinalLines) - Int32(frameLinesPAL));

  if (std::min(deltaNTSC, deltaPAL) <= Metrics::tvModeDetectionTolerance)
    updateTvMode(deltaNTSC <= deltaPAL ? TvMode::ntsc : TvMode::pal);
  else if (!myModeConfirmed) {
    if (
      (myCurrentFrameFinalLines  < frameLinesPAL) &&
      (myCurrentFrameFinalLines > frameLinesNTSC) &&
      (myCurrentFrameFinalLines % 2)
    )
      updateTvMode(TvMode::ntsc);
    else
      updateTvMode(deltaNTSC <= deltaPAL ? TvMode::ntsc : TvMode::pal);
  }

  if (oldMode == myMode)
    myFramesInMode++;
  else
    myFramesInMode = 0;

  if (myFramesInMode > Metrics::framesForModeConfirmation)
    myModeConfirmed = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameManager::updateTvMode(TvMode mode)
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
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameManager::setYstart(uInt32 ystart)
{
  if (ystart == myYstart) return;

  myYstart = ystart;

  myVblankMode = ystart ? VblankMode::fixed : VblankMode::floating;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 FrameManager::ystart() const {
  return myYstart;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameManager::setVblank(bool vblank)
{
  myVblank = vblank;
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
uInt32 FrameManager::height() const
{
  return myFixedHeight > 0 ? myFixedHeight : (myKernelLines + Metrics::visibleOverscan);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameManager::setFixedHeight(uInt32 height)
{
  myFixedHeight = height;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 FrameManager::currentLine() const
{
  return myState == State::frame ? myLineInState : 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 FrameManager::scanlines() const
{
  return myState == State::frame ? myCurrentFrameTotalLines : myCurrentFrameFinalLines;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameManager::setTvMode(TvMode mode)
{
  if (!myAutodetectTvMode) updateTvMode(mode);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameManager::autodetectTvMode(bool toggle)
{
  myAutodetectTvMode = toggle;
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
