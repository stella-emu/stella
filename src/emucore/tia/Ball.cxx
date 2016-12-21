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

#include "Ball.hxx"

enum Count: Int8 {
  renderCounterOffset = -4
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Ball::Ball(uInt32 collisionMask)
  : myCollisionMaskDisabled(collisionMask),
    myCollisionMaskEnabled(0xFFFF),
    myIsSuppressed(false)
{
  reset();
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
  myHmmClocks = 0;
  myCounter = 0;
  myIsMoving = false;
  myEffectiveWidth = 1;
  myLastMovementTick = 0;
  myWidth = 1;
  myIsRendering = false;
  myDebugEnabled = false;
  myRenderCounter = 0;

  updateEnabled();
  applyColors();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Ball::enabl(uInt8 value)
{
  myIsEnabledNew = (value & 0x02) > 0;
  updateEnabled();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Ball::hmbl(uInt8 value)
{
  myHmmClocks = (value >> 4) ^ 0x08;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Ball::resbl(uInt8 counter)
{
  myCounter = counter;

  myIsRendering = true;
  myRenderCounter = Count::renderCounterOffset + (counter - 157);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Ball::ctrlpf(uInt8 value)
{
  static constexpr uInt8 ourWidths[] = {1, 2, 4, 8};

  myWidth = ourWidths[(value & 0x30) >> 4];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Ball::vdelbl(uInt8 value)
{
  myIsDelaying = (value & 0x01) > 0;
  updateEnabled();
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
  myObjectColor = color;
  applyColors();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Ball::setDebugColor(uInt8 color)
{
  myDebugColor = color;
  applyColors();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Ball::enableDebugColors(bool enabled)
{
  myDebugEnabled = enabled;
  applyColors();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Ball::startMovement()
{
  myIsMoving = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Ball::movementTick(uInt32 clock, bool apply)
{
  myLastMovementTick = myCounter;

  if (clock == myHmmClocks) myIsMoving = false;

  if (myIsMoving && apply) {
    render();
    tick(false);
  }

  return myIsMoving;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Ball::render()
{
  collision = (myIsRendering && myRenderCounter >= 0 && myIsEnabled) ?
    myCollisionMaskEnabled :
    myCollisionMaskDisabled;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Ball::tick(bool isReceivingMclock)
{
  bool starfieldEffect = myIsMoving && isReceivingMclock;

  if (myCounter == 156) {
    myIsRendering = true;
    myRenderCounter = Count::renderCounterOffset;

    uInt8 starfieldDelta = (myCounter + 160 - myLastMovementTick) % 4;
    if (starfieldEffect && starfieldDelta == 3 && myWidth < 4) myRenderCounter++;

    switch (starfieldDelta) {
      case 3:
        myEffectiveWidth = myWidth == 1 ? 2 : myWidth;
        break;

      case 2:
        myEffectiveWidth = 0;
        break;

      default:
        myEffectiveWidth = myWidth;
        break;
    }

  } else if (myIsRendering && ++myRenderCounter >= (starfieldEffect ? myEffectiveWidth : myWidth))
    myIsRendering = false;

  if (++myCounter >= 160)
      myCounter = 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Ball::shuffleStatus()
{
  myIsEnabledOld = myIsEnabledNew;
  updateEnabled();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Ball::updateEnabled()
{
  myIsEnabled = !myIsSuppressed && (myIsDelaying ? myIsEnabledOld : myIsEnabledNew);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Ball::applyColors()
{
  myColor = myDebugEnabled ? myDebugColor : myObjectColor;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// TODO: implement this once the class is finalized
bool Ball::save(Serializer& out) const
{
  try
  {
    out.putString(name());

    // TODO - save instance variables
  }
  catch(...)
  {
    cerr << "ERROR: TIA_Ball::save" << endl;
    return false;
  }

  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// TODO: implement this once the class is finalized
bool Ball::load(Serializer& in)
{
  try
  {
    if(in.getString() != name())
      return false;

    // TODO - load instance variables
  }
  catch(...)
  {
    cerr << "ERROR: TIA_Ball::load" << endl;
    return false;
  }

  return false;
}
