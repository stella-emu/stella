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

#include "Player.hxx"
#include "DrawCounterDecodes.hxx"

enum Count: Int8 {
  renderCounterOffset = -5,
};

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
  myIsRendering = false;
  myRenderCounter = 0;
  myPatternOld = 0;
  myPatternNew = 0;
  myIsReflected = 0;
  myIsDelaying = false;
  myColor = myObjectColor = myDebugColor = 0;
  myDebugEnabled = false;
  collision = myCollisionMaskDisabled;
  myDivider = 1;
  mySampleCounter = 0;
  myDividerPending = 0;
  myDividerChangeCounter = -1;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Player::grp(uInt8 pattern)
{
  myPatternNew = pattern;
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

  switch (masked) {
    case 5:
      myDividerPending = 2;
      break;

    case 7:
      myDividerPending = 4;
      break;

    default:
      myDividerPending = 1;
      break;
  }

  myDecodes = DrawCounterDecodes::get().playerDecodes()[masked];

  if (myDividerPending == myDivider) return;

  // The following is an effective description of the effects of NUSIZ during
  // decode and rendering.

  if (myIsRendering) {

    switch ((myDivider << 4) | myDividerPending) {
      case 0x12:
      case 0x14:
        if ((myRenderCounter - Count::renderCounterOffset) < 3)
          myDivider = myDividerPending;
        else
          myDividerChangeCounter = 1;
        break;

      case 0x21:
      case 0x41:
        if ((myRenderCounter - Count::renderCounterOffset) < 3) {
          myDivider = myDividerPending;
        } else if ((myRenderCounter - Count::renderCounterOffset) < 5) {
          myDivider = myDividerPending;
          myRenderCounter--;
        } else {
          myDividerChangeCounter = 1;
        }

        break;

      case 0x42:
      case 0x24:
        if (myRenderCounter < 1)
          myDivider = myDividerPending;
        else
          myDividerChangeCounter = (myDivider - (myRenderCounter - 1) % myDivider);
        break;

      default:
        // should never happen
        myDivider = myDividerPending;
        break;
    }

  } else {
    myDivider = myDividerPending;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Player::resp(uInt8 counter)
{
  myCounter = counter;

  // This tries to account for the effects of RESP during draw counter decode as
  // described in Andrew Towers' notes. Still room for tuning.'
  if (myIsRendering && (myRenderCounter - renderCounterOffset) < 4)
    myRenderCounter = renderCounterOffset + (counter - 157);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Player::refp(uInt8 value)
{
  myIsReflected = (value & 0x08) > 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Player::vdelp(uInt8 value)
{
  const bool oldIsDelaying = myIsDelaying;

  myIsDelaying = (value & 0x01) > 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Player::toggleEnabled(bool enabled)
{
  myIsSuppressed = !enabled;
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
  if (!myIsRendering || myRenderCounter < (myDivider > 1 ? 1 : 0)) {
    collision = myCollisionMaskDisabled;
    return;
  }

  uInt8 pixel =
    (myIsDelaying ? myPatternOld : myPatternNew) &
    (1 << (!myIsReflected ? (7 - mySampleCounter) : mySampleCounter));

  collision = (pixel > 0) ? myCollisionMaskEnabled : myCollisionMaskDisabled;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Player::tick()
{
  if (myDecodes[myCounter]) {
    myIsRendering = true;
    mySampleCounter = 0;
    myRenderCounter = Count::renderCounterOffset;
  } else if (myIsRendering) {
    myRenderCounter++;

    if (myDivider == 1) {
      if (myRenderCounter > 0) {
        mySampleCounter++;
      }

      if (myRenderCounter >= 0 && myDividerChangeCounter >= 0 && myDividerChangeCounter-- == 0)
        myDivider = myDividerPending;
    } else {
      if (myRenderCounter > 1 && (((myRenderCounter - 1) % myDivider) == 0))
        mySampleCounter++;

      if (myRenderCounter > 0 && myDividerChangeCounter >= 0 && myDividerChangeCounter-- == 0)
        myDivider = myDividerPending;
    }

    if (mySampleCounter > 7) myIsRendering = false;
  }

  if (++myCounter >= 160) myCounter = 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Player::shufflePatterns()
{
  const uInt8 oldPatternOld = myPatternOld;

  myPatternOld = myPatternNew;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 Player::getRespClock() const
{
  switch (myDivider)
  {
    case 1:
      return (myCounter + 160 - 5) % 160;

    case 2:
      return (myCounter + 160 - 9) % 160;

    case 4:
      return (myCounter + 160 - 12) % 160;

    default:
      throw runtime_error("invalid width");
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
