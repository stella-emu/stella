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

namespace TIA6502tsCore {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Ball::Ball(uInt32 collisionMask)
  : myCollisionMask(collisionMask)
{
  reset();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Ball::reset()
{
  myColor = 0;
  collision = myCollisionMask;
  myEnabledOld = false;
  myEnabledNew = false;
  myEnabled = false;
  myIsDelaying = false;
  myHmmClocks = 0;
  myCounter = 0;
  myIsMoving = false;
  myWidth = 1;
  myIsRendering = false;
  myRenderCounter = 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Ball::enabl(uInt8 value)
{
  myEnabledNew = (value & 0x02) > 0;
  updateEnabled();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Ball::hmbl(uInt8 value)
{
  myHmmClocks = (value >> 4) ^ 0x08;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Ball::resbl(bool hblank)
{
  myCounter = hblank ? 159 : 157;

  if (!hblank) {
    myIsRendering = true;
    myRenderCounter = Count::renderCounterOffset;
  }
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
void Ball::setColor(uInt8 color)
{
  myColor = color;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Ball::startMovement()
{
  myIsMoving = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Ball::movementTick(uInt32 clock, bool apply)
{
  if (clock == myHmmClocks) myIsMoving = false;

  if (myIsMoving && apply) {
    render();
    tick();
  }

  return myIsMoving;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Ball::render()
{
  collision = (myIsRendering && myRenderCounter >= 0 && myEnabled) ? 0 : myCollisionMask;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Ball::tick()
{
  if (myCounter == 156) {
    myIsRendering = true;
    myRenderCounter = Count::renderCounterOffset;
  }
  else if (myIsRendering && ++myRenderCounter >= myWidth)
    myIsRendering = false;

  if (++myCounter >= 160)
      myCounter = 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Ball::shuffleStatus()
{
  myEnabledOld = myEnabledNew;
  updateEnabled();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Ball::updateEnabled()
{
  myEnabled = myIsDelaying ? myEnabledOld : myEnabledNew;
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

} // namespace TIA6502tsCore
