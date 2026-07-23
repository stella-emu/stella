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
// Copyright (c) 1995-2026 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

// #define TIA_FRAMEMANAGER_DEBUG_LOG

#include <cmath>

#include "FrameManager.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FrameManager::FrameManager()
{
  // Establish the reset state without invoking the virtual-dispatching reset()
  // from the constructor: doing so lets GCC's LTO devirtualizer speculatively
  // inline a sibling override (FrameLayoutDetector::onReset) and trip a bogus
  // -Wstringop-overflow.  All members are NSDMI-initialized, so the statically
  // bound onReset() plus recalculateMetrics() establishes the same state.
  FrameManager::onReset();
  recalculateMetrics();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameManager::onReset()
{
  myState = State::waitForVsyncStart;
  myLineInState = 0;
  myTotalFrames = 0;
  myVsyncLineCount = 0;
  myY = 0;
  myVsyncPending = false;
  myVsyncPendingLines = 0;

  myJitterEmulation.reset();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameManager::onNextLine()
{
  const State previousState = myState;
  ++myLineInState;

  // Promote a pending VSYNC to real once it has lasted the minimum number of
  // scanlines.  This matches real TV behaviour: pulses shorter than 2 full
  // scanlines are completely invisible to the state machine.
  if (myVsyncPending && ++myVsyncPendingLines >= 2) {
    myVsyncPending = false;
    setState(State::waitForVsyncEnd);
  }

  switch (myState)
  {
    case State::waitForVsyncStart:
      if ((myCurrentFrameTotalLines > myFrameLines - 3) || myTotalFrames == 0)
      {
        // if vertical blank is not enabled, bail out after too many frame  lines:
        if (myVblank || myCurrentFrameTotalLines > Metrics::frameSizePAL * 2)
          ++myVsyncLineCount;
      }

      if (myVsyncLineCount > Metrics::maxLinesVsync) setState(State::waitForFrameStart);

      break;

    case State::waitForVsyncEnd:
      // if vertical blank is not enabled, bail out after too many frame lines:
      if (myVblank || myCurrentFrameTotalLines > Metrics::frameSizePAL * 2)
        ++myVsyncLineCount;
      if (myVsyncLineCount > Metrics::maxLinesVsync)
        setState(State::waitForFrameStart);

      break;

    case State::waitForFrameStart:
    {
      const Int32 jitter =
        (myJitterEnabled && myTotalFrames > Metrics::initialGarbageFrames) ? myJitterEmulation.jitter() : 0;

      if (static_cast<Int32>(myLineInState) >= static_cast<Int32>(myYStart) + jitter) setState(State::frame);
      break;
    }

    case State::frame:
      if (myLineInState >= myHeight)
      {
        myLastY = myYStart + myY;  // Last line drawn in this frame
        setState(State::waitForVsyncStart);
      }
      break;

    default:
      throw std::runtime_error("frame manager: invalid state");
  }

  if (myState == State::frame && previousState == State::frame) ++myY;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Int32 FrameManager::missingScanlines() const
{
  if (myLastY == myYStart + myY) return 0;
  return myHeight - myY;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameManager::setVcenter(Int32 vcenter)
{
  if (vcenter < TIAConstants::minVcenter || vcenter > TIAConstants::maxVcenter) return;

  myVcenter = vcenter;
  recalculateMetrics();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameManager::setAdjustVSize(Int32 adjustVSize)
{
  myVSizeAdjust = adjustVSize;
  recalculateMetrics();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameManager::onSetVblank(uInt64 cycles)
{
  if (myState == State::waitForVsyncEnd)
  {
    if (myVblank) // VBLANK switched on
    {
      myVblankStart = cycles;
    }
    else // VBLANK switched off
    {
      myVblankCycles += cycles - myVblankStart;
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameManager::onSetVsync(uInt64 cycles)
{
  if (myVsync) {
    // VSYNC rising edge.  Don't commit to waitForVsyncEnd immediately — real
    // TVs require at least 2 full scanlines of VSYNC before locking.  Record
    // timing state and let onNextLine() promote this to waitForVsyncEnd once
    // the minimum scanline count is reached.  Pulses shorter than 2 scanlines
    // are ignored entirely.
    if (myState == State::waitForVsyncEnd || myVsyncPending) return;

    myVsyncStart = cycles;
    myVblankStart = myVblank ? cycles : INT64_MAX;
    myVblankCycles = 0;
    myVsyncPending = true;
    myVsyncPendingLines = 0;
  }
  else {
    // VSYNC falling edge.
    myVsyncPending = false;

    if (myState != State::waitForVsyncEnd)
      return;  // pulse never reached 2 scanlines

    myVsyncEnd = cycles;
    if(myVblankStart != INT64_MAX)
      myVblankCycles += cycles - myVblankStart;
    setState(State::waitForFrameStart);
  }
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
        myJitterEmulation.frameComplete(myCurrentFrameFinalLines,
            static_cast<Int32>(myVsyncEnd - myVsyncStart), static_cast<Int32>(myVblankCycles));

      notifyFrameStart();

      myVsyncLineCount = 0;
      break;

    case State::frame:
      myVsyncLineCount = 0;
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

  out.putInt(std::to_underlying(myState));
  out.putInt(myLineInState);
  out.putInt(myVsyncLineCount);
  out.putInt(myY);
  out.putInt(myLastY);

  out.putInt(myVcenter);
  out.putInt(myVSizeAdjust);

  out.putBool(myJitterEnabled);
  out.putBool(myVsyncPending);
  out.putInt(myVsyncPendingLines);

  // Cycle stamps used to measure VSYNC/VBLANK duration for jitter emulation.
  // A state may legally be saved mid-VSYNC (myVsyncPending set), so these must
  // be preserved or the next frame's jitter is computed from stale values
  out.putLong(myVsyncStart);
  out.putLong(myVsyncEnd);
  out.putLong(myVblankStart);
  out.putLong(myVblankCycles);

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FrameManager::onLoad(Serializer& in)
{
  if (!myJitterEmulation.load(in)) return false;

  myState = static_cast<State>(in.getInt());
  myLineInState = in.getInt();
  myVsyncLineCount = in.getInt();
  myY = in.getInt();
  myLastY = in.getInt();

  myVcenter = in.getInt();
  myVSizeAdjust = in.getInt();

  myJitterEnabled = in.getBool();
  myVsyncPending = in.getBool();
  myVsyncPendingLines = in.getInt();

  myVsyncStart = in.getLong();
  myVsyncEnd = in.getLong();
  myVblankStart = in.getLong();
  myVblankCycles = in.getLong();

  recalculateMetrics();
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FrameManager::recalculateMetrics() {
  Int32 ystartBase = 0;
  Int32 baseHeight = 0;

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
      throw std::runtime_error("frame manager: invalid TV mode");
  }

  myHeight = BSPF::clamp<uInt32>(roundf(static_cast<float>(baseHeight) * (1.F - myVSizeAdjust / 100.F)), 0, myFrameLines);
  myYStart = BSPF::clamp<uInt32>(ystartBase + (baseHeight - static_cast<Int32>(myHeight)) / 2 - myVcenter, 0, myFrameLines);
  // The - 1 keeps myYStart >= 1 when vcenter is at its maximum, preventing
  // waitForFrameStart from exiting on scanline 0 when a negative vsizeadjust
  // makes myHeight exceed baseHeight and reduces centerOffset below maxVcenter.
  myMaxVcenter = BSPF::clamp<Int32>(ystartBase + (baseHeight - static_cast<Int32>(myHeight)) / 2 - 1, 0, TIAConstants::maxVcenter);

  //cout << "myVSizeAdjust " << myVSizeAdjust << " " << myHeight << '\n' << std::flush;

  myJitterEmulation.setYStart(myYStart);
}
