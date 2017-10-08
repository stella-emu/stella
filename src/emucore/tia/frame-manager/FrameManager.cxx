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
  visibleOverscan               = 20,
  tvModeDetectionTolerance      = 20,
  initialGarbageFrames          = TIAConstants::initialGarbageFrames,
  minStableFrames               = 10,
  maxStabilizationFrames        = 20,
  minDeltaForJitter             = 3,
  framesForStableHeight         = 2
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FrameManager::FrameManager() :
  myHeight(0)
{
  onLayoutChange();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameManager::onReset()
{
  myVblankManager.reset();

  myState = State::waitForVsyncStart;
  myLineInState = 0;
  myTotalFrames = 0;
  myVsyncLines = 0;
  myY = 0;
  myFramePending = false;
  myStabilizationFrames = 0;
  myStableFrames = 0;
  myHasStabilized = false;

  myStableFrameLines = -1;
  myStableFrameHeightCountdown = 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameManager::onNextLine()
{
  State previousState = myState;
  myLineInState++;

  switch (myState)
  {
    case State::waitForVsyncStart:
      if ((myCurrentFrameTotalLines > myFrameLines - 3) || myTotalFrames == 0)
        myVsyncLines++;

      if (myVsyncLines > Metrics::maxLinesVsync) setState(State::waitForFrameStart);

      break;

    case State::waitForVsyncEnd:
      if (++myVsyncLines > Metrics::maxLinesVsync)
        setState(State::waitForFrameStart);

      break;

    case State::waitForFrameStart:
      if (myVblankManager.nextLine(myTotalFrames <= Metrics::initialGarbageFrames))
        setState(State::frame);
      break;

    case State::frame:
      if (myLineInState >= myHeight)
      {
        myLastY = ystart() + myY;  // Last line drawn in this frame
        setState(State::waitForVsyncStart);
      }
      break;

    default:
      throw runtime_error("frame manager: invalid state");
  }

  if (myState == State::frame && previousState == State::frame) myY++;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Int32 FrameManager::missingScanlines() const
{
  if (myLastY == ystart() + myY)
    return 0;
  else {
    return myHeight - myY;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameManager::onSetVsync()
{
#ifdef TIA_FRAMEMANAGER_DEBUG_LOG
  (cout << "vsync " << !myVsync << " -> " << myVsync << ": state " << int(myState) << " @ " << myLineInState << "\n").flush();
#endif

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
      if (!myHasStabilized) {
        myHasStabilized =
          myStableFrames >= Metrics::minStableFrames ||
          myStabilizationFrames >= Metrics::maxStabilizationFrames;

        updateIsRendering();

        myStabilizationFrames++;

        if (myVblankManager.isStable())
          myStableFrames++;
        else
          myStableFrames = 0;
      }

      if (myFramePending) finalizeFrame();
      notifyFrameStart();

      myVblankManager.start();
      myFramePending = true;

      myVsyncLines = 0;
      break;

    case State::frame:
      notifyRenderingStart();
      myVsyncLines = 0;
      myY = 0;
      break;

    default:
      break;
  }

  updateIsRendering();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameManager::finalizeFrame()
{
  if (myCurrentFrameTotalLines != (uInt32)myStableFrameLines) {
    if (myCurrentFrameTotalLines == myCurrentFrameFinalLines) {

      if (++myStableFrameHeightCountdown >= Metrics::framesForStableHeight) {
        if (myStableFrameLines >= 0) {
          handleJitter(myCurrentFrameTotalLines - myStableFrameLines);
        }

        myStableFrameLines = myCurrentFrameTotalLines;
      }

    }
    else myStableFrameHeightCountdown = 0;
  }

  notifyFrameComplete();

#ifdef TIA_FRAMEMANAGER_DEBUG_LOG
  (cout << "frame complete @ " << myLineInState << " (" << myCurrentFrameFinalLines << " total)" << "\n").flush();
#endif // TIA_FRAMEMANAGER_DEBUG_LOG
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameManager::handleJitter(Int32 scanlineDifference)
{
  if (
    (uInt32)abs(scanlineDifference) < Metrics::minDeltaForJitter ||
    !myJitterEnabled ||
    myTotalFrames < Metrics::initialGarbageFrames
  ) return;

  myVblankManager.setJitter(scanlineDifference);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// TODO: kill this with fire once frame manager refactoring is complete
void FrameManager::onLayoutChange()
{
#ifdef TIA_FRAMEMANAGER_DEBUG_LOG
  (cout << "TV mode switched to " << int(layout()) << "\n").flush();
#endif // TIA_FRAMEMANAGER_DEBUG_LOG

  switch (layout())
  {
    case FrameLayout::ntsc:
      myVblankLines   = Metrics::vblankNTSC;
      myKernelLines   = Metrics::kernelNTSC;
      myOverscanLines = Metrics::overscanNTSC;
      break;

    case FrameLayout::pal:
      myVblankLines   = Metrics::vblankPAL;
      myKernelLines   = Metrics::kernelPAL;
      myOverscanLines = Metrics::overscanPAL;
      break;

    default:
      throw runtime_error("frame manager: invalid TV mode");
  }

  myFrameLines = Metrics::vsync + myVblankLines + myKernelLines + myOverscanLines;
  if (myFixedHeight == 0)
    myHeight = myKernelLines + Metrics::visibleOverscan;

  myVblankManager.setVblankLines(myVblankLines);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameManager::onSetVblank()
{
  #ifdef TIA_FRAMEMANAGER_DEBUG_LOG
    (cout << "vblank change " << !myVblank << " -> " << myVblank << "@" << myLineInState << "\n").flush();
  #endif // TIA_FRAMEMANAGER_DEBUG_LOG

  if (myState == State::waitForFrameStart) {
    if (myVblankManager.setVblankDuringVblank(myVblank, myTotalFrames <= Metrics::initialGarbageFrames)) {
      setState(State::frame);
    }
  } else
    myVblankManager.setVblank(myVblank);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameManager::setFixedHeight(uInt32 height)
{
  myFixedHeight = height;
  myHeight = myFixedHeight > 0 ? myFixedHeight : (myKernelLines + Metrics::visibleOverscan);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameManager::enableJitter(bool enabled)
{
  myJitterEnabled = enabled;

  if (!enabled) myVblankManager.setJitter(0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameManager::updateIsRendering() {
  myIsRendering = myState == State::frame && myHasStabilized;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FrameManager::onSave(Serializer& out) const
{
  if (!myVblankManager.save(out)) return false;

  out.putInt(uInt32(myState));
  out.putInt(myLineInState);
  out.putInt(myVsyncLines);
  out.putInt(myY);
  out.putInt(myLastY);
  out.putBool(myFramePending);

  out.putInt(myStableFrames);
  out.putInt(myStabilizationFrames);
  out.putBool(myHasStabilized);

  out.putInt(myVblankLines);
  out.putInt(myKernelLines);
  out.putInt(myOverscanLines);
  out.putInt(myFrameLines);
  out.putInt(myHeight);
  out.putInt(myFixedHeight);

  out.putBool(myJitterEnabled);

  out.putInt(myStableFrameLines);
  out.putInt(myStableFrameHeightCountdown);

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FrameManager::onLoad(Serializer& in)
{
  if (!myVblankManager.load(in)) return false;

  myState = State(in.getInt());
  myLineInState = in.getInt();
  myVsyncLines = in.getInt();
  myY = in.getInt();
  myLastY = in.getInt();
  myFramePending = in.getBool();

  myStableFrames = in.getInt();
  myStabilizationFrames = in.getInt();
  myHasStabilized = in.getBool();

  myVblankLines = in.getInt();
  myKernelLines = in.getInt();
  myOverscanLines = in.getInt();
  myFrameLines = in.getInt();
  myHeight = in.getInt();
  myFixedHeight = in.getInt();

  myJitterEnabled = in.getBool();

  myStableFrameLines = in.getInt();
  myStableFrameHeightCountdown = in.getInt();

  return true;
}
