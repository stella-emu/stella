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

#include "Missile.hxx"
#include "DrawCounterDecodes.hxx"

enum Count: Int8 {
  renderCounterOffset = -4
};

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
  myLastMovementTick = 0;
  myWidth = 1;
  myEffectiveWidth = 1;
  myIsRendering = false;
  myRenderCounter = 0;
  myColor = myObjectColor = myDebugColor = 0;
  myDebugEnabled = false;
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
void Missile::resm(uInt8 counter)
{
  myCounter = counter;

  if (myIsRendering) {
    if (myRenderCounter < 0) {
      myRenderCounter = Count::renderCounterOffset + (counter - 157);

    } else {
      // The following is an effective description of the behavior of missile width after a
      // RESMx during draw. It would be much simpler without the HBLANK cases :)
      switch (myWidth) {
        case 8:
          myRenderCounter = (counter != 157 && myRenderCounter >= 4) ? 7 : (counter - 157);
          break;

        case 4:
          myRenderCounter = counter - 157;
          break;

        case 2:
          if (counter != 157) {
            if (myRenderCounter == 1 || myRenderCounter == 0) myIsRendering = false;
          } else {
            if (myRenderCounter == 0) myRenderCounter++;
          }

          break;

        default:
          if (counter != 157) {
            if (myRenderCounter == 0) myIsRendering = false;
          }
          break;
      }
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Missile::resmp(uInt8 value, const Player& player)
{
  const uInt8 resmp = value & 0x02;

  if (resmp == myResmp)
    return;

  myResmp = resmp;

  if (!myResmp)
    myCounter = player.getRespClock();

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
  myLastMovementTick = myCounter;

  if (clock == myHmmClocks) myIsMoving = false;

  if (myIsMoving && apply) {
    render();
    tick(false);
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
void Missile::tick(bool isReceivingMclock)
{
  bool starfieldEffect = myIsMoving && isReceivingMclock;

  if (myDecodes[myCounter]) {
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

  } else if (
    myIsRendering && ++myRenderCounter >= (starfieldEffect ? myEffectiveWidth : myWidth)
  )
    myIsRendering = false;

  if (++myCounter >= 160) myCounter = 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Missile::setColor(uInt8 color)
{
  myObjectColor = color;
  applyColors();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Missile::setDebugColor(uInt8 color)
{
  myDebugColor = color;
  applyColors();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Missile::enableDebugColors(bool enabled)
{
  myDebugEnabled = enabled;
  applyColors();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Missile::updateEnabled()
{
  myIsEnabled = !myIsSuppressed && myEnam && !myResmp;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Missile::applyColors()
{
  myColor = myDebugEnabled ? myDebugColor : myObjectColor;
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
