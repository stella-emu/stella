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

  if (myJitter > 0) myJitter = std::max(myJitter - myJitterFactor, 0);
  if (myJitter < 0) myJitter = std::min(myJitter + myJitterFactor, 0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool VblankManager::nextLine(bool isGarbageFrame)
{
  if (!myIsRunning) return false;

  // Make sure that we do the transition check **before** incrementing the line
  // counter. This ensures that, if the transition is caused by VSYNC off during
  // the line, this will continue to trigger the transition in 'locked' mode. Otherwise,
  // the transition would be triggered by the line change **before** the VSYNC
  // and thus detected as a suprious violation. Sigh, this stuff is complicated,
  // isn't it?
  const bool transition = shouldTransition(isGarbageFrame);
  if (transition) myIsRunning = false;

  myCurrentLine++;

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
  // Are we free to transition as per vblank cycle?
  bool shouldTransition = myCurrentLine + 1 >= (myVblank ? myVblankLines : myVblankLines - Metrics::maxUnderscan);

  // Do we **actually** transition? This depends on what mode we are in.
  bool transition = false;

  switch (myMode) {
    // Floating mode: we still are looking for a stable frame start
    case VblankMode::floating:

      // Are we free to transition?
      if (shouldTransition) {
        // Is this same scanline in which the transition ocurred last frame?
        if (!isGarbageFrame && myCurrentLine == myLastVblankLines)
          // Yes? -> Increase the number of stable frames
          myStableVblankFrames++;
        else
          // No? -> Frame start shifted again, set the number of consecutive stable frames to zero
          myStableVblankFrames = 0;

        // Save the transition point for checking on it next frame
        myLastVblankLines = myCurrentLine;

#ifdef TIA_VBLANK_MANAGER_DEBUG_LOG
        (cout << "leaving vblank in floating mode, should transition: " << shouldTransition << "\n").flush();
#endif
        // In floating mode, we transition whenever we can.
        transition = true;
      }

      // Transition to locked mode if we saw enough stable frames in a row.
      if (myStableVblankFrames >= Metrics::minStableVblankFrames) {
        setVblankMode(VblankMode::locked);
        myVblankViolations = 0;
      }

      break;

    // Locked mode: always transition at the same point, but check whether this is actually the
    // detected transition point and revert state if applicable
    case VblankMode::locked:

      // Have we reached the transition point?
      if (myCurrentLine == myLastVblankLines) {

        // Are we free to transition per the algorithm and didn't we observe an violation before?
        // (aka did the algorithm tell us to transition before reaching the actual line)
        if (shouldTransition && !myVblankViolated)
          // Reset the number of irregular frames (if any)
          myVblankViolations = 0;
        else {
          // Record a violation if it wasn't recorded before
          if (!myVblankViolated) myVblankViolations++;
          myVblankViolated = true;
        }

#ifdef TIA_VBLANK_MANAGER_DEBUG_LOG
        (cout << "leaving vblank in locked mode, should transition: " << shouldTransition << "\n").flush();
#endif

        // transition
        transition = true;
      // The algorithm tells us to transition although we haven't reached the trip line before
      } else if (shouldTransition) {
        // Record a violation if it wasn't recorded before
        if (!myVblankViolated) myVblankViolations++;
        myVblankViolated = true;
      }

      // Freeze frame start if the detected value seems to be sufficiently stable
      if (transition && ++myFramesInLockedMode > Metrics::framesUntilFinal) {
        setVblankMode(VblankMode::final);
      // Revert to floating mode if there were too many irregular frames in a row
      } else if (myVblankViolations > Metrics::maxVblankViolations) {
        setVblankMode(VblankMode::floating);
        myStableVblankFrames = 0;
      }

      break;

    // Fixed mode: use external ystart value
    case VblankMode::fixed:
      transition = (Int32)myCurrentLine >=
        std::max<Int32>(myYstart + std::min<Int32>(myJitter, Metrics::maxJitter), 0);
      break;

    // Final mode: use detected ystart value
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
