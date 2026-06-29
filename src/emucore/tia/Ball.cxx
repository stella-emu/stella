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

#include "Ball.hxx"
#include "TIA.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Ball::Ball(uInt32 collisionMask)
  : myCollisionMaskDisabled{collisionMask}
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Ball::reset()
{
  myColor = myObjectColor = myDebugColor = 0;
  collision = myCollisionMaskDisabled;
  myIsEnabledOld = false;
  myIsEnabledNew = false;
  myIsEnabled = false;
  myIsDelaying = false;
  mySignalActive = false;
  myHmmClocks = 0;
  myCounter = 0;
  isMoving = false;
  myEffectiveWidth = 1;
  myLastMovementTick = 0;
  myWidth = 1;
  myIsRendering = false;
  myDebugEnabled = false;
  myRenderCounter = 0;
  myInvertedPhaseClock = false;
  myUseInvertedPhaseClock = false;
  myUseShortLateHMove = false;
  myUseLateRespx = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Ball::enabl(uInt8 value)
{
  const auto enabledNewOldValue = myIsEnabledNew;

  myIsEnabledNew = (value & 0x02) > 0;

  if (myIsEnabledNew != enabledNewOldValue && !myIsDelaying) {
    // Without VDEL the new ENABL value is what's actually rendered — flush
    // when it really changes. With VDEL, the live enabled state is the
    // "old" one until shuffleStatus latches the new one, so no flush is
    // needed here. Guarded optimization.
    myTIA->flushLineCache();

    updateEnabled();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Ball::hmbl(uInt8 value)
{
  myHmmClocks = (value >> 4) ^ 0x08;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Ball::resbl(uInt8 counter, bool lateRespxCondition)
{
  if (myUseLateRespx && lateRespxCondition)
    counter = (counter + TIAConstants::H_PIXEL - 1) % TIAConstants::H_PIXEL;

  myCounter = counter;

  myIsRendering = true;
  myRenderCounter = Count::renderCounterOffset + (counter - 157);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Ball::ctrlpf(uInt8 value)
{
  static constexpr std::array<uInt8, 4> ourWidths = { 1, 2, 4, 8 };

  const uInt8 newWidth = ourWidths[(value & 0x30) >> 4];

  if (newWidth != myWidth) {
    // CTRLPF ball width determines how many clocks the signal stays active
    // — cached pixels used the old width.
    myTIA->flushLineCache();
    myWidth = newWidth;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Ball::vdelbl(uInt8 value)
{
  const auto oldIsDelaying = myIsDelaying;

  myIsDelaying = (value & 0x01) > 0;

  if (oldIsDelaying != myIsDelaying) {
    // VDELBL flip switches between myIsEnabledOld and myIsEnabledNew as
    // the live enabled source — visible behaviour changes from here on.
    myTIA->flushLineCache();
    updateEnabled();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Ball::toggleCollisions(bool enabled)
{
  myCollisionMaskEnabled = enabled ? 0xFFFF : (0x8000 | myCollisionMaskDisabled);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Ball::toggleEnabled(bool enabled)
{
  myIsSuppressed = !enabled;
  updateEnabled();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Ball::setColor(uInt8 color)
{
  // Same pattern as Player/Missile::setColor — the "&& myIsEnabled" guard
  // is an optimization (skip flush when the ball isn't emitting).
  if (color != myObjectColor && myIsEnabled) myTIA->flushLineCache();

  myObjectColor = color;
  applyColors();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Ball::setDebugColor(uInt8 color)
{
  // Debug palette override changed.
  myTIA->flushLineCache();
  myDebugColor = color;
  applyColors();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Ball::enableDebugColors(bool enabled)
{
  // Debug color source toggled.
  myTIA->flushLineCache();
  myDebugEnabled = enabled;
  applyColors();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Ball::setInvertedPhaseClock(bool enable)
{
  myUseInvertedPhaseClock = enable;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Ball::setShortLateHMove(bool enable)
{
  myUseShortLateHMove = enable;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Ball::setLateRespx(bool enable)
{
  myUseLateRespx = enable;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Ball::startMovement()
{
  isMoving = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Ball::nextLine()
{
  // Re-evaluate the collision mask in order to properly account for collisions during
  // hblank. Usually, this will be taken care off in the next tick, but there is no
  // next tick before hblank ends.
  mySignalActive = myIsRendering && myRenderCounter >= 0;
  collision = (mySignalActive && myIsEnabled) ? myCollisionMaskEnabled : myCollisionMaskDisabled;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Ball::setENABLOld(bool enabled)
{
  // Debugger-only direct write into the VDEL-old enabled slot.
  myTIA->flushLineCache();

  myIsEnabledOld = enabled;
  updateEnabled();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Ball::shuffleStatus()
{
  const auto oldIsEnabledOld = myIsEnabledOld;

  myIsEnabledOld = myIsEnabledNew;

  // With VDEL active myIsEnabledOld is the live source — latching a
  // different value mid-line changes the visible state from here on.
  if (myIsEnabledOld != oldIsEnabledOld && myIsDelaying) {
    myTIA->flushLineCache();
    updateEnabled();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Ball::updateEnabled()
{
  myIsEnabled = !myIsSuppressed && (myIsDelaying ? myIsEnabledOld : myIsEnabledNew);

  collision = (mySignalActive && myIsEnabled) ? myCollisionMaskEnabled : myCollisionMaskDisabled;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Ball::applyColors()
{
  myColor = myDebugEnabled ? myDebugColor : myObjectColor;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 Ball::getPosition() const
{
  // position =
  //    current playfield x +
  //    (current counter - 156 (the decode clock of copy 0)) +
  //    clock count after decode until first pixel +
  //    1 (it'll take another cycle after the decode for the rendter counter to start ticking)
  //
  // The result may be negative, so we add 160 and do the modulus -> 317 = 156 + TIA::H_PIXEL + 1
  //
  // Mind the sign of renderCounterOffset: it's defined negative above
  return (317 - myCounter - Count::renderCounterOffset + myTIA->getPosition()) % TIAConstants::H_PIXEL;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Ball::setPosition(uInt8 newPosition)
{
  // Debugger-only direct counter move; same reasoning as Player::setPosition.
  myTIA->flushLineCache();

  // See getPosition for an explanation
  myCounter = (317 - newPosition - Count::renderCounterOffset + myTIA->getPosition()) % TIAConstants::H_PIXEL;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Ball::save(Serializer& out) const
{
  try
  {
    out.putInt(collision);
    out.putInt(myCollisionMaskDisabled);
    out.putInt(myCollisionMaskEnabled);

    out.putByte(myColor);
    out.putByte(myObjectColor);
    out.putByte(myDebugColor);
    out.putBool(myDebugEnabled);

    out.putBool(myIsEnabledOld);
    out.putBool(myIsEnabledNew);
    out.putBool(myIsEnabled);
    out.putBool(myIsSuppressed);
    out.putBool(myIsDelaying);
    out.putBool(mySignalActive);

    out.putByte(myHmmClocks);
    out.putByte(myCounter);
    out.putBool(isMoving);
    out.putByte(myWidth);
    out.putByte(myEffectiveWidth);
    out.putByte(myLastMovementTick);

    out.putBool(myIsRendering);
    out.putByte(myRenderCounter);
    out.putBool(myInvertedPhaseClock);
    out.putBool(myUseLateRespx);
  }
  catch(...)
  {
    cerr << "ERROR: TIA_Ball::save\n";
    return false;
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Ball::load(Serializer& in)
{
  try
  {
    collision = in.getInt();
    myCollisionMaskDisabled = in.getInt();
    myCollisionMaskEnabled = in.getInt();

    myColor = in.getByte();
    myObjectColor = in.getByte();
    myDebugColor = in.getByte();
    myDebugEnabled = in.getBool();

    myIsEnabledOld = in.getBool();
    myIsEnabledNew = in.getBool();
    myIsEnabled = in.getBool();
    myIsSuppressed = in.getBool();
    myIsDelaying = in.getBool();
    mySignalActive = in.getBool();

    myHmmClocks = in.getByte();
    myCounter = in.getByte();
    isMoving = in.getBool();
    myWidth = in.getByte();
    myEffectiveWidth = in.getByte();
    myLastMovementTick = in.getByte();

    myIsRendering = in.getBool();
    myRenderCounter = in.getByte();
    myInvertedPhaseClock = in.getBool();
    myUseLateRespx = in.getBool();

    applyColors();
  }
  catch(...)
  {
    cerr << "ERROR: TIA_Ball::load\n";
    return false;
  }

  return true;
}
