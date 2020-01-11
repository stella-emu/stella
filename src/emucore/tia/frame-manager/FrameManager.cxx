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
// Copyright (c) 1995-2020 by Bradford W. Mott, Stephen Anthony
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
  vsync                         = 3,
  frameSizeNTSC                 = 262,
  frameSizePAL                  = 312,
  baseHeightNTSC                = 240,
  baseHeightPAL                 = 288,
  maxLinesVsync                 = 50,
  initialGarbageFrames          = TIAConstants::initialGarbageFrames,
  ystartNTSC                    = 16,
  ystartPAL                     = 19
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FrameManager::FrameManager()
{
  reset();
  recalculateMetrics();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameManager::onReset()
{
  myState = State::waitForVsyncStart;
  myLineInState = 0;
  myTotalFrames = 0;
  myVsyncLines = 0;
  myY = 0;

  myJitterEmulation.reset();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameManager::onNextLine()
{
  Int32 jitter;

  State previousState = myState;
  ++myLineInState;

  switch (myState)
  {
    case State::waitForVsyncStart:
      if ((myCurrentFrameTotalLines > myFrameLines - 3) || myTotalFrames == 0)
        ++myVsyncLines;

      if (myVsyncLines > Metrics::maxLinesVsync) setState(State::waitForFrameStart);

      break;

    case State::waitForVsyncEnd:
      if (++myVsyncLines > Metrics::maxLinesVsync)
        setState(State::waitForFrameStart);

      break;

    case State::waitForFrameStart:
      jitter =
        (myJitterEnabled && myTotalFrames > Metrics::initialGarbageFrames) ? myJitterEmulation.jitter() : 0;

      if (myLineInState >= (myYStart + jitter)) setState(State::frame);
      break;

    case State::frame:
      if (myLineInState >= myHeight)
      {
        myLastY = myYStart + myY;  // Last line drawn in this frame
        setState(State::waitForVsyncStart);
      }
      break;

    default:
      throw runtime_error("frame manager: invalid state");
  }

  if (myState == State::frame && previousState == State::frame) ++myY;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Int32 FrameManager::missingScanlines() const
{
  if (myLastY == myYStart + myY)
    return 0;
  else {
    return myHeight - myY;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameManager::setVcenter(Int32 vcenter)
{
  if (vcenter < TIAConstants::minVcenter || vcenter > TIAConstants::maxVcenter) return;

  myVcenter = vcenter;
  recalculateMetrics();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameManager::setAdjustScanlines(Int32 adjustScanlines)
{
  myAdjustScanlines = adjustScanlines;
  recalculateMetrics();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameManager::onSetVsync()
{
  if (myState == State::waitForVsyncEnd) setState(State::waitForFrameStart);
  else setState(State::waitForVsyncEnd);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameManager::setState(FrameManager::State state)
{
  if (myState == state) return;

  myState = state;
  myLineInState = 0;

  switch (myState) {
    case State::waitForFrameStart:
      notifyFrameComplete();

      if (myTotalFrames > Metrics::initialGarbageFrames)
        myJitterEmulation.frameComplete(myCurrentFrameFinalLines);

      notifyFrameStart();

      myVsyncLines = 0;
      break;

    case State::frame:
      myVsyncLines = 0;
      myY = 0;
      break;

    default:
      break;
  }

  updateIsRendering();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameManager::onLayoutChange()
{
  recalculateMetrics();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameManager::updateIsRendering() {
  myIsRendering = myState == State::frame;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FrameManager::onSave(Serializer& out) const
{
  if (!myJitterEmulation.save(out)) return false;

  out.putInt(uInt32(myState));
  out.putInt(myLineInState);
  out.putInt(myVsyncLines);
  out.putInt(myY);
  out.putInt(myLastY);

  out.putInt(myVblankLines);
  out.putInt(myFrameLines);
  out.putInt(myHeight);
  out.putInt(myYStart);

  out.putBool(myJitterEnabled);

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FrameManager::onLoad(Serializer& in)
{
  if (!myJitterEmulation.load(in)) return false;

  myState = State(in.getInt());
  myLineInState = in.getInt();
  myVsyncLines = in.getInt();
  myY = in.getInt();
  myLastY = in.getInt();

  myVblankLines = in.getInt();
  myFrameLines = in.getInt();
  myHeight = in.getInt();
  myYStart = in.getInt();

  myJitterEnabled = in.getBool();

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameManager::recalculateMetrics() {
  Int32 ystartBase;
  Int32 baseHeight;

  switch (layout())
  {
    case FrameLayout::ntsc:
      myVblankLines   = Metrics::vblankNTSC;
      myFrameLines    = Metrics::frameSizeNTSC;
      ystartBase      = Metrics::ystartNTSC;
      baseHeight      = Metrics::baseHeightNTSC;
      break;

    case FrameLayout::pal:
      myVblankLines   = Metrics::vblankPAL;
      myFrameLines    = Metrics::frameSizePAL;
      ystartBase      = Metrics::ystartPAL;
      baseHeight      = Metrics::baseHeightPAL;
      break;

    default:
      throw runtime_error("frame manager: invalid TV mode");
  }

  myHeight = BSPF::clamp<uInt32>(baseHeight + myAdjustScanlines * 2, 0, myFrameLines);
  myYStart = BSPF::clamp<uInt32>(ystartBase + (baseHeight - static_cast<Int32>(myHeight)) / 2 - myVcenter, 0, myFrameLines);
  // TODO: why "- 1" here: ???
  myMaxVcenter = BSPF::clamp<Int32>(ystartBase + (baseHeight - static_cast<Int32>(myHeight)) / 2 - 1, 0, TIAConstants::maxVcenter);

  myJitterEmulation.setYStart(myYStart);
}
