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
// Copyright (c) 1995-2016 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

#include "Player.hxx"
#include "DrawCounterDecodes.hxx"

enum Count: Int8 {
  renderCounterOffset = -5
};

namespace TIA6502tsCore {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Player::Player(uInt32 collisionMask)
  : myCollisionMaskDisabled(collisionMask),
    myCollisionMaskEnabled(0xFFFF),
    myIsSuppressed(false)
{
  reset();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Player::reset()
{
  myDecodes = DrawCounterDecodes::get().playerDecodes()[0];
  myHmmClocks = 0;
  myCounter = 0;
  myIsMoving = false;
  myWidth = 8;
  myIsRendering = false;
  myRenderCounter = 0;
  myPatternOld = 0;
  myPatternNew = 0;
  myPattern = 0;
  myIsReflected = 0;
  myIsDelaying = false;
  myColor = myObjectColor = myDebugColor = 0;
  myDebugEnabled = false;
  collision = myCollisionMaskDisabled;

  updatePattern();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Player::grp(uInt8 pattern)
{
  myPatternNew = pattern;

  if (!myIsDelaying) updatePattern();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Player::hmp(uInt8 value)
{
  myHmmClocks = (value >> 4) ^ 0x08;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Player::nusiz(uInt8 value)
{
  const uInt8 masked = value & 0x07;
  const uInt8 oldWidth = myWidth;

  if (masked == 5)
    myWidth = 16;
  else if (masked == 7)
    myWidth = 32;
  else
    myWidth = 8;

  myDecodes = DrawCounterDecodes::get().playerDecodes()[masked];

  if (myIsRendering && myRenderCounter >= myWidth)
    myIsRendering = false;

  if (oldWidth != myWidth) updatePattern();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Player::resp(bool hblank)
{
  myCounter = hblank ? 159 : 157;

  if (myIsRendering && myRenderCounter < -1)
    myRenderCounter = Count::renderCounterOffset;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Player::refp(uInt8 value)
{
  const bool oldIsReflected = myIsReflected;

  myIsReflected = (value & 0x08) > 0;

  if (myIsReflected != oldIsReflected) updatePattern();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Player::vdelp(uInt8 value)
{
  const bool oldIsDelaying = myIsDelaying;

  myIsDelaying = (value & 0x01) > 0;

  if (myIsDelaying != oldIsDelaying) updatePattern();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Player::toggleEnabled(bool enabled)
{
  myIsSuppressed = !enabled;
  updatePattern();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Player::toggleCollisions(bool enabled)
{
  myCollisionMaskEnabled = enabled ? 0xFFFF : (0x8000 | myCollisionMaskDisabled);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Player::setColor(uInt8 color)
{
  myObjectColor = color;
  applyColors();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Player::setDebugColor(uInt8 color)
{
  myDebugColor = color;
  applyColors();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Player::enableDebugColors(bool enabled)
{
  myDebugEnabled = enabled;
  applyColors();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Player::startMovement()
{
  myIsMoving = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Player::movementTick(uInt32 clock, bool apply)
{
  if (clock == myHmmClocks) {
    myIsMoving = false;
  }

  if (myIsMoving && apply) {
    render();
    tick();
  }

  return myIsMoving;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Player::render()
{
  collision = (
    myIsRendering &&
    myRenderCounter >= 0 &&
    (myPattern & (1 << (myWidth - myRenderCounter - 1)))
  ) ? myCollisionMaskEnabled : myCollisionMaskDisabled;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Player::tick()
{
  if (myDecodes[myCounter]) {
    myIsRendering = true;
    myRenderCounter = Count::renderCounterOffset;
  } else if (myIsRendering && ++myRenderCounter >= myWidth) {
    myIsRendering = false;
  }

  if (++myCounter >= 160) myCounter = 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Player::shufflePatterns()
{
  const uInt8 oldPatternOld = myPatternOld;

  myPatternOld = myPatternNew;

  if (myIsDelaying && oldPatternOld != myPatternOld) updatePattern();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 Player::getRespClock() const
{
  switch (myWidth)
  {
    case 8:
      return (myCounter + 160 - 3) % 160;

    case 16:
      return (myCounter + 160 - 9) % 160;

    case 32:
      return (myCounter + 160 - 13) % 160;

    default:
      throw runtime_error("invalid width");
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Player::updatePattern()
{
  if (myIsSuppressed) {
    myPattern = 0;
    return;
  }

  const uInt32 pattern = myIsDelaying ? myPatternOld : myPatternNew;

  switch (myWidth)
  {
    case 8:
      if (myIsReflected) {
        myPattern =
          ((pattern & 0x01) << 7) |
          ((pattern & 0x02) << 5) |
          ((pattern & 0x04) << 3) |
          ((pattern & 0x08) << 1) |
          ((pattern & 0x10) >> 1) |
          ((pattern & 0x20) >> 3) |
          ((pattern & 0x40) >> 5) |
          ((pattern & 0x80) >> 7);
      } else {
        myPattern = pattern;
      }
      break;

    case 16:
      if (myIsReflected) {
        myPattern =
          ((3 * (pattern & 0x01)) << 14) |
          ((3 * (pattern & 0x02)) << 11) |
          ((3 * (pattern & 0x04)) << 8)  |
          ((3 * (pattern & 0x08)) << 5)  |
          ((3 * (pattern & 0x10)) << 2)  |
          ((3 * (pattern & 0x20)) >> 1)  |
          ((3 * (pattern & 0x40)) >> 4)  |
          ((3 * (pattern & 0x80)) >> 7);
      } else {
        myPattern =
          ((3 * (pattern & 0x01)))       |
          ((3 * (pattern & 0x02)) << 1)  |
          ((3 * (pattern & 0x04)) << 2)  |
          ((3 * (pattern & 0x08)) << 3)  |
          ((3 * (pattern & 0x10)) << 4)  |
          ((3 * (pattern & 0x20)) << 5)  |
          ((3 * (pattern & 0x40)) << 6)  |
          ((3 * (pattern & 0x80)) << 7);
      }
      break;

    case 32:
      if (myIsReflected) {
        myPattern =
          ((0xF * (pattern & 0x01)) << 28) |
          ((0xF * (pattern & 0x02)) << 23) |
          ((0xF * (pattern & 0x04)) << 18) |
          ((0xF * (pattern & 0x08)) << 13) |
          ((0xF * (pattern & 0x10)) << 8)  |
          ((0xF * (pattern & 0x20)) << 3)  |
          ((0xF * (pattern & 0x40)) >> 2)  |
          ((0xF * (pattern & 0x80)) >> 7);
      } else {
        myPattern =
          ((0xF * (pattern & 0x01)))       |
          ((0xF * (pattern & 0x02)) << 3)  |
          ((0xF * (pattern & 0x04)) << 6)  |
          ((0xF * (pattern & 0x08)) << 9)  |
          ((0xF * (pattern & 0x10)) << 12) |
          ((0xF * (pattern & 0x20)) << 15) |
          ((0xF * (pattern & 0x40)) << 18) |
          ((0xF * (pattern & 0x80)) << 21);
      }
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Player::applyColors()
{
  myColor = myDebugEnabled ? myDebugColor : myObjectColor;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// TODO: implement this once the class is finalized
bool Player::save(Serializer& out) const
{
  try
  {
    out.putString(name());

    // TODO - save instance variables
  }
  catch(...)
  {
    cerr << "ERROR: TIA_Player::save" << endl;
    return false;
  }

  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// TODO: implement this once the class is finalized
bool Player::load(Serializer& in)
{
  try
  {
    if(in.getString() != name())
      return false;

    // TODO - load instance variables
  }
  catch(...)
  {
    cerr << "ERROR: TIA_Player::load" << endl;
    return false;
  }

  return false;
}

} // namespace TIA6502tsCore
