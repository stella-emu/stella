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

#include "Missile.hxx"
#include "DrawCounterDecodes.hxx"

enum Count: Int8 {
  renderCounterOffset = -4
};

namespace TIA6502tsCore {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Missile::Missile(uInt32 collisionMask)
  : myCollisionMaskDisabled(collisionMask),
    myCollisionMaskEnabled(0xFFFF),
    myIsSuppressed(false)
{
  reset();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Missile::reset()
{
  myDecodes = DrawCounterDecodes::get().missileDecodes()[0];
  myIsEnabled = false;
  myEnam = false;
  myResmp = 0;
  myHmmClocks = 0;
  myCounter = 0;
  myIsMoving = false;
  myWidth = 1;
  myIsRendering = false;
  myRenderCounter = 0;
  myColor = 0;
  collision = myCollisionMaskDisabled;

  updateEnabled();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Missile::enam(uInt8 value)
{
  myEnam = (value & 0x02) > 0;
  updateEnabled();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Missile::hmm(uInt8 value)
{
  myHmmClocks = (value >> 4) ^ 0x08;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Missile::resm(bool hblank)
{
  myCounter = hblank ? 159 : 157;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Missile::resmp(uInt8 value, const Player& player)
{
  const uInt8 resmp = value & 0x02;

  if (resmp == myResmp) return;

  myResmp = resmp;

  if (!myResmp) myCounter = player.getRespClock();

  updateEnabled();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Missile::toggleCollisions(bool enabled)
{
  myCollisionMaskEnabled = enabled ? 0xFFFF : (0x8000 | myCollisionMaskDisabled);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Missile::toggleEnabled(bool enabled)
{
  myIsSuppressed = !enabled;
  updateEnabled();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Missile::nusiz(uInt8 value)
{
  static constexpr uInt8 ourWidths[] = { 1, 2, 4, 8 };

  myWidth = ourWidths[(value & 0x30) >> 4];
  myDecodes = DrawCounterDecodes::get().missileDecodes()[value & 0x07];

  if (myIsRendering && myRenderCounter >= myWidth)
    myIsRendering = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Missile::startMovement()
{
  myIsMoving = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Missile::movementTick(uInt32 clock, bool apply)
{
  if (clock == myHmmClocks) myIsMoving = false;

  if (myIsMoving && apply) {
    render();
    tick();
  }

  return myIsMoving;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Missile::render()
{
  collision = (myIsRendering && myRenderCounter >= 0 && myIsEnabled) ?
    myCollisionMaskEnabled :
    myCollisionMaskDisabled;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Missile::tick()
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
void Missile::setColor(uInt8 color)
{
  myColor = color;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Missile::updateEnabled()
{
    myIsEnabled = !myIsSuppressed && myEnam && !myResmp;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// TODO: implement this once the class is finalized
bool Missile::save(Serializer& out) const
{
  try
  {
    out.putString(name());

    // TODO - save instance variables
  }
  catch(...)
  {
    cerr << "ERROR: TIA_Missile::save" << endl;
    return false;
  }

  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// TODO: implement this once the class is finalized
bool Missile::load(Serializer& in)
{
  try
  {
    if(in.getString() != name())
      return false;

    // TODO - load instance variables
  }
  catch(...)
  {
    cerr << "ERROR: TIA_Missile::load" << endl;
    return false;
  }

  return false;
}

} // namespace TIA6502tsCore
