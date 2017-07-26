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

// #define TIA_VBLANK_MANAGER_DEBUG_LOG

#include <algorithm>

#include "VblankManager.hxx"

enum Metrics: uInt32 {
  maxUnderscan          = 10,
  maxVblankViolations   = 2,
  minStableVblankFrames = 1,
  framesUntilFinal = 30,
  maxJitter = 50
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
VblankManager::VblankManager()
  : myVblankLines(0),
    myYstart(0),
    myMode(VblankMode::floating),
    myJitter(0),
    myJitterFactor(2)
{
  reset();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VblankManager::reset()
{
  myVblank = false;
  myCurrentLine = 0;
  myVblankViolations = 0;
  myStableVblankFrames = 0;
  myVblankViolated = false;
  myLastVblankLines = 0;
  myIsRunning = false;
  myJitter = 0;

  if (myMode != VblankMode::fixed) setVblankMode(VblankMode::floating);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VblankManager::setJitter(Int32 jitter) {
  jitter = std::min<Int32>(jitter, Metrics::maxJitter);

  if (myMode == VblankMode::final) jitter = std::max<Int32>(jitter, -myLastVblankLines);
  if (myMode == VblankMode::fixed) jitter = std::max<Int32>(jitter, -myYstart);

  if (jitter > 0) jitter += myJitterFactor;
  if (jitter < 0) jitter -= myJitterFactor;

  if (abs(jitter) > abs(myJitter)) myJitter = jitter;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VblankManager::start()
{
  myCurrentLine = 0;
  myIsRunning = true;
  myVblankViolated = false;

  if (myMode == VblankMode::locked && ++myFramesInLockedMode > Metrics::framesUntilFinal)
    setVblankMode(VblankMode::final);

  if (myJitter > 0) myJitter = std::max(myJitter - myJitterFactor, 0);
  if (myJitter < 0) myJitter = std::min(myJitter + myJitterFactor, 0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool VblankManager::nextLine(bool isGarbageFrame)
{
  if (!myIsRunning) return false;

  myCurrentLine++;

  const bool transition = shouldTransition(isGarbageFrame);
  if (transition) myIsRunning = false;

  return transition;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VblankManager::setYstart(uInt32 ystart)
{
  if (ystart == myYstart) return;

  myYstart = ystart;

  setVblankMode(ystart ? VblankMode::fixed : VblankMode::floating);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool VblankManager::setVblankDuringVblank(bool vblank, bool isGarbageFrame)
{
  const bool oldVblank = myVblank;

  myVblank = vblank;
  if (!myIsRunning || vblank || oldVblank == myVblank) return false;

  const bool transition = shouldTransition(isGarbageFrame);
  if (transition) myIsRunning = false;

  return transition;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool VblankManager::shouldTransition(bool isGarbageFrame)
{
  bool shouldTransition = myCurrentLine >= (myVblank ? myVblankLines : myVblankLines - Metrics::maxUnderscan);
  bool transition = false;

  switch (myMode) {
    case VblankMode::floating:

      if (shouldTransition) {
        if (!isGarbageFrame && myCurrentLine == myLastVblankLines)
          myStableVblankFrames++;
        else
          myStableVblankFrames = 0;

        myLastVblankLines = myCurrentLine;

#ifdef TIA_VBLANK_MANAGER_DEBUG_LOG
        (cout << "leaving vblank in floating mode, should transition: " << shouldTransition << "\n").flush();
#endif
        transition = true;
      }

      if (myStableVblankFrames >= Metrics::minStableVblankFrames) {
        setVblankMode(VblankMode::locked);
        myVblankViolations = 0;
      }

      break;

    case VblankMode::locked:

      if (myCurrentLine == myLastVblankLines) {

        if (shouldTransition && !myVblankViolated)
          myVblankViolations = 0;
        else {
          if (!myVblankViolated) myVblankViolations++;
          myVblankViolated = true;
        }

#ifdef TIA_VBLANK_MANAGER_DEBUG_LOG
        (cout << "leaving vblank in locked mode, should transition: " << shouldTransition << "\n").flush();
#endif

        transition = true;
      } else if (shouldTransition){
        if (!myVblankViolated) myVblankViolations++;
        myVblankViolated = true;
      }

      if (myVblankViolations > Metrics::maxVblankViolations) {
        setVblankMode(VblankMode::floating);
        myStableVblankFrames = 0;
      }

      break;

    case VblankMode::fixed:
      transition = (Int32)myCurrentLine >=
        std::max<Int32>(myYstart + std::min<Int32>(myJitter, Metrics::maxJitter), 0);
      break;

    case VblankMode::final:
      transition = (Int32)myCurrentLine >=
        std::max<Int32>(myLastVblankLines + std::min<Int32>(myJitter, Metrics::maxJitter), 0);
      break;
  }

  return transition;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VblankManager::setVblankMode(VblankMode mode)
{
  if (myMode == mode) return;

  myMode = mode;

  switch (myMode) {
    case VblankMode::locked:
      myFramesInLockedMode = 0;
      break;

    default:
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool VblankManager::save(Serializer& out) const
{
  try
  {
    out.putString(name());

    out.putInt(myVblankLines);
    out.putInt(myYstart);
    out.putBool(myVblank);
    out.putInt(myCurrentLine);

    out.putInt(int(myMode));
    out.putInt(myLastVblankLines);
    out.putByte(myVblankViolations);
    out.putByte(myStableVblankFrames);
    out.putBool(myVblankViolated);
    out.putByte(myFramesInLockedMode);

    out.putInt(myJitter);
    out.putByte(myJitterFactor);

    out.putBool(myIsRunning);
  }
  catch(...)
  {
    cerr << "ERROR: TIA_VblankManager::save" << endl;
    return false;
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool VblankManager::load(Serializer& in)
{
  try
  {
    if(in.getString() != name())
      return false;

    myVblankLines = in.getInt();
    myYstart = in.getInt();
    myVblank = in.getBool();
    myCurrentLine = in.getInt();

    myMode = VblankMode(in.getInt());
    myLastVblankLines = in.getInt();
    myVblankViolations = in.getByte();
    myStableVblankFrames = in.getByte();
    myVblankViolated = in.getBool();
    myFramesInLockedMode = in.getByte();

    myJitter = in.getInt();
    myJitterFactor = in.getByte();

    myIsRunning = in.getBool();
  }
  catch(...)
  {
    cerr << "ERROR: TIA_VblankManager::load" << endl;
    return false;
  }

  return true;
}
