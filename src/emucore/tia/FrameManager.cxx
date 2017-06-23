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
  framesForModeConfirmation     = 5,
  minStableFrames               = 10,
  maxStabilizationFrames        = 20,
  minDeltaForJitter             = 3,
  framesForStableHeight         = 2
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
  : myLayout(FrameLayout::pal),
    myAutodetectLayout(true),
    myHeight(0),
    myFixedHeight(0),
    myJitterEnabled(false)
{
  updateLayout(FrameLayout::ntsc);
  reset();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameManager::setHandlers(
  FrameManager::callback frameStartCallback,
  FrameManager::callback frameCompleteCallback,
  FrameManager::callback renderingStartCallback
)
{
  myOnFrameStart = frameStartCallback;
  myOnFrameComplete = frameCompleteCallback;
  myOnRenderingStart = renderingStartCallback;
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
  myStabilizationFrames = 0;
  myStableFrames = 0;
  myHasStabilized = false;

  myStableFrameLines = -1;
  myStableFrameHeightCountdown = 0;
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

      if (myVsyncLines > vsyncLimit(myAutodetectLayout)) setState(State::waitForFrameStart);

      break;

    case State::waitForVsyncEnd:
      if (++myVsyncLines > vsyncLimit(myAutodetectLayout))
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
uInt32 FrameManager::missingScanlines() const
{
  if (myLastY == ystart() + myY)
    return 0;
  else
    return myHeight - myY;
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
      if (!myHasStabilized) {
        myHasStabilized =
          myStableFrames >= Metrics::minStableFrames ||
          myStabilizationFrames >= Metrics::maxStabilizationFrames;

        myStabilizationFrames++;

        if (myVblankManager.isStable())
          myStableFrames++;
        else
          myStableFrames = 0;
      }

      if (myFramePending) finalizeFrame();
      if (myOnFrameStart) myOnFrameStart();

      myVblankManager.start();
      myFramePending = true;

      myVsyncLines = 0;
      break;

    case State::frame:
      if (myOnRenderingStart) myOnRenderingStart();
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

  myPreviousFrameFinalLines = myCurrentFrameFinalLines;
  myCurrentFrameFinalLines = myCurrentFrameTotalLines;
  myCurrentFrameTotalLines = 0;
  myTotalFrames++;

  if (myOnFrameComplete) myOnFrameComplete();

#ifdef TIA_FRAMEMANAGER_DEBUG_LOG
  (cout << "frame complete @ " << myLineInState << " (" << myCurrentFrameFinalLines << " total)" << "\n").flush();
#endif // TIA_FRAMEMANAGER_DEBUG_LOG

  if (myAutodetectLayout) updateAutodetectedLayout();

  myFrameRate = (myLayout == FrameLayout::pal ? 15600.0 : 15720.0) /
                 myCurrentFrameFinalLines;
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
void FrameManager::updateAutodetectedLayout()
{
  if (myTotalFrames <= Metrics::initialGarbageFrames) {
    return;
  }

  const FrameLayout oldLayout = myLayout;

  const uInt32
    deltaNTSC = abs(Int32(myCurrentFrameFinalLines) - Int32(frameLinesNTSC)),
    deltaPAL =  abs(Int32(myCurrentFrameFinalLines) - Int32(frameLinesPAL));

  if (std::min(deltaNTSC, deltaPAL) <= Metrics::tvModeDetectionTolerance)
    updateLayout(deltaNTSC <= deltaPAL ? FrameLayout::ntsc : FrameLayout::pal);
  else if (!myModeConfirmed) {
    if (
      (myCurrentFrameFinalLines < frameLinesPAL) &&
      (myCurrentFrameFinalLines > frameLinesNTSC) &&
      (myCurrentFrameFinalLines % 2)
    )
      updateLayout(FrameLayout::ntsc);
    else
      updateLayout(deltaNTSC <= deltaPAL ? FrameLayout::ntsc : FrameLayout::pal);
  }

  if (oldLayout == myLayout)
    myFramesInMode++;
  else
    myFramesInMode = 0;

  if (myFramesInMode > Metrics::framesForModeConfirmation)
    myModeConfirmed = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameManager::updateLayout(FrameLayout layout)
{
  if (layout == myLayout) return;

#ifdef TIA_FRAMEMANAGER_DEBUG_LOG
  (cout << "TV mode switched to " << int(mode) << "\n").flush();
#endif // TIA_FRAMEMANAGER_DEBUG_LOG

  myLayout = layout;

  switch (myLayout)
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
void FrameManager::setVblank(bool vblank)
{
  if (myState == State::waitForFrameStart) {
    if (myVblankManager.setVblankDuringVblank(vblank, myTotalFrames <= Metrics::initialGarbageFrames)) {
      setState(State::frame);
    }
  } else
    myVblankManager.setVblank(vblank);
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
bool FrameManager::save(Serializer& out) const
{
  try
  {
    out.putString(name());

    if (!myVblankManager.save(out)) return false;

    out.putInt(uInt32(myLayout));
    out.putBool(myAutodetectLayout);
    out.putInt(uInt32(myState));
    out.putInt(myLineInState);
    out.putInt(myCurrentFrameTotalLines);
    out.putInt(myCurrentFrameFinalLines);
    out.putInt(myPreviousFrameFinalLines);
    out.putInt(myVsyncLines);
    out.putDouble(myFrameRate);
    out.putInt(myY);  out.putInt(myLastY);
    out.putBool(myFramePending);

    out.putInt(myTotalFrames);
    out.putInt(myFramesInMode);
    out.putBool(myModeConfirmed);

    out.putInt(myStableFrames);
    out.putInt(myStabilizationFrames);
    out.putBool(myHasStabilized);

    out.putBool(myVsync);

    out.putInt(myVblankLines);
    out.putInt(myKernelLines);
    out.putInt(myOverscanLines);
    out.putInt(myFrameLines);
    out.putInt(myHeight);
    out.putInt(myFixedHeight);

    out.putBool(myJitterEnabled);

    out.putInt(myStableFrameLines);
    out.putInt(myStableFrameHeightCountdown);
  }
  catch(...)
  {
    cerr << "ERROR: TIA_FrameManager::save" << endl;
    return false;
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FrameManager::load(Serializer& in)
{
  try
  {
    if(in.getString() != name())
      return false;

    if (!myVblankManager.load(in)) return false;

    myLayout = FrameLayout(in.getInt());
    myAutodetectLayout = in.getBool();
    myState = State(in.getInt());
    myLineInState = in.getInt();
    myCurrentFrameTotalLines = in.getInt();
    myCurrentFrameFinalLines = in.getInt();
    myPreviousFrameFinalLines = in.getInt();
    myVsyncLines = in.getInt();
    myFrameRate = float(in.getDouble());
    myY = in.getInt();  myLastY = in.getInt();
    myFramePending = in.getBool();

    myTotalFrames = in.getInt();
    myFramesInMode = in.getInt();
    myModeConfirmed = in.getBool();

    myStableFrames = in.getInt();
    myStabilizationFrames = in.getInt();
    myHasStabilized = in.getBool();

    myVsync = in.getBool();

    myVblankLines = in.getInt();
    myKernelLines = in.getInt();
    myOverscanLines = in.getInt();
    myFrameLines = in.getInt();
    myHeight = in.getInt();
    myFixedHeight = in.getInt();

    myJitterEnabled = in.getBool();

    myStableFrameLines = in.getInt();
    myStableFrameHeightCountdown = in.getInt();
  }
  catch(...)
  {
    cerr << "ERROR: TIA_FrameManager::load" << endl;
    return false;
  }

  return true;
}
