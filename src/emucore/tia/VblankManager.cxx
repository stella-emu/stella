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

#include "VblankManager.hxx"

enum Metrics: uInt32 {
  maxUnderscan          = 10,
  maxVblankViolations   = 2,
  minStableVblankFrames = 1,
  framesUntilFinal = 30
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
VblankManager::VblankManager()
  : myVblankLines(0),
    myYstart(0),
    myMode(VblankMode::floating)
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

  if (myMode != VblankMode::fixed) setVblankMode(VblankMode::floating);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void VblankManager::start()
{
  myCurrentLine = 0;
  myIsRunning = true;
  myVblankViolated = false;

  if (myMode == VblankMode::locked && ++myFramesInLockedMode > Metrics::framesUntilFinal)
    setVblankMode(VblankMode::final);
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
      transition = myCurrentLine >= myYstart;
      break;

    case VblankMode::final:
      transition = myCurrentLine >= myLastVblankLines;
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
// TODO: implement this once the class is finalized
bool VblankManager::save(Serializer& out) const
{
  try
  {
    out.putString(name());

    // TODO - save instance variables
  }
  catch(...)
  {
    cerr << "ERROR: TIA_VblankManager::save" << endl;
    return false;
  }

  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// TODO: implement this once the class is finalized
bool VblankManager::load(Serializer& in)
{
  try
  {
    if(in.getString() != name())
      return false;

    // TODO - load instance variables
  }
  catch(...)
  {
    cerr << "ERROR: TIA_VblankManager::load" << endl;
    return false;
  }

  return false;
}
