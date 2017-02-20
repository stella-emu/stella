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
  maxLinesVsync                 = 32,
  maxLinesVsyncDuringAutodetect = 100,
  visibleOverscan               = 20,
  maxUnderscan                  = 10,
  tvModeDetectionTolerance      = 20,
  initialGarbageFrames          = 10,
  framesForModeConfirmation     = 5
};

static constexpr uInt32
  frameLinesNTSC = Metrics::vsync + Metrics::vblankNTSC + Metrics::kernelNTSC + Metrics::overscanNTSC,
  frameLinesPAL = Metrics::vsync + Metrics::vblankPAL + Metrics::kernelPAL + Metrics::overscanPAL;

inline static uInt32 vsyncLimit(bool autodetect) {
  return autodetect ? maxLinesVsyncDuringAutodetect : maxLinesVsync;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 FrameManager::initialGarbageFrames()
{
  return Metrics::initialGarbageFrames;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FrameManager::FrameManager()
  : myMode(TvMode::pal),
    myAutodetectTvMode(true),
    myFixedHeight(0)
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
  myVblankManager.reset();

  myState = State::waitForVsyncStart;
  myCurrentFrameTotalLines = myCurrentFrameFinalLines = 0;
  myFrameRate = 60.0;
  myLineInState = 0;
  myVsync = false;
  myTotalFrames = 0;
  myFramesInMode = 0;
  myModeConfirmed = false;
  myVsyncLines = 0;
  myY = 0;
  myFramePending = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameManager::nextLine()
{
  State previousState = myState;

  myCurrentFrameTotalLines++;
  myLineInState++;

  switch (myState)
  {
    case State::waitForVsyncStart:
      if ((myCurrentFrameTotalLines > myFrameLines - 3) || myTotalFrames == 0)
        myVsyncLines++;

      if (myVsyncLines > vsyncLimit(myAutodetectTvMode)) setState(State::waitForFrameStart);

      break;

    case State::waitForVsyncEnd:
      if (++myVsyncLines > vsyncLimit(myAutodetectTvMode))
        setState(State::waitForFrameStart);

      break;

    case State::waitForFrameStart:
      if (myVblankManager.nextLine(myTotalFrames <= Metrics::initialGarbageFrames))
        setState(State::frame);
      break;

    case State::frame:
      if (myLineInState >= (myFixedHeight > 0 ? myFixedHeight : (myKernelLines + Metrics::visibleOverscan)))
        setState(State::waitForVsyncStart);
      break;

    default:
      throw runtime_error("frame manager: invalid state");
  }

  if (myState == State::frame && previousState == State::frame) myY++;
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
      if (!myVsync) setState(State::waitForFrameStart);
      break;

    case State::frame:
      if (myVsync) setState(State::waitForVsyncEnd);
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
      if (myFramePending) finalizeFrame();
      if (myOnFrameStart) myOnFrameStart();
      myVblankManager.start();
      myFramePending = true;

      myVsyncLines = 0;
      break;

    case State::frame:
      myVsyncLines = 0;
      myY = 0;
      break;

    default:
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameManager::finalizeFrame()
{
  myCurrentFrameFinalLines = myCurrentFrameTotalLines;
  myCurrentFrameTotalLines = 0;
  myTotalFrames++;

  if (myOnFrameComplete) myOnFrameComplete();

#ifdef TIA_FRAMEMANAGER_DEBUG_LOG
  (cout << "frame complete @ " << myLineInState << " (" << myCurrentFrameFinalLines << " total)" << "\n").flush();
#endif // TIA_FRAMEMANAGER_DEBUG_LOG

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
      (myCurrentFrameFinalLines < frameLinesPAL) &&
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

#ifdef TIA_FRAMEMANAGER_DEBUG_LOG
  (cout << "TV mode switched to " << int(mode) << "\n").flush();
#endif // TIA_FRAMEMANAGER_DEBUG_LOG

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

  myVblankManager.setVblankLines(myVblankLines);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameManager::setYstart(uInt32 ystart)
{
  myVblankManager.setYstart(ystart);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 FrameManager::ystart() const
{
  return myVblankManager.ystart();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameManager::setVblank(bool vblank)
{
  myVblankManager.setVblank(vblank);
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
uInt32 FrameManager::scanlines() const
{
  return  myCurrentFrameTotalLines;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 FrameManager::scanlinesLastFrame() const
{
  return  myCurrentFrameFinalLines;
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

    if (!myVblankManager.save(out)) return false;

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

    if (!myVblankManager.load(in)) return false;

    // TODO - load instance variables
  }
  catch(...)
  {
    cerr << "ERROR: TIA_FrameManager::load" << endl;
    return false;
  }

  return false;
}
