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

#include "Playfield.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Playfield::Playfield(uInt32 collisionMask)
  : myCollisionMaskDisabled(collisionMask),
    myCollisionMaskEnabled(0xFFFF),
    myIsSuppressed(false)
{
  reset();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Playfield::reset()
{
  myPattern = 0;
  myReflected = false;
  myRefp = false;

  myPf0 = 0;
  myPf1 = 0;
  myPf2 = 0;

  myObjectColor = myDebugColor = 0;
  myColorP0 = 0;
  myColorP1 = 0;
  myColorMode = ColorMode::normal;
  myDebugEnabled = false;

  collision = 0;

  applyColors();
  updatePattern();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Playfield::pf0(uInt8 value)
{
  myPattern = (myPattern & 0x000FFFF0) | (value >> 4);
  myPf0 = value;

  updatePattern();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Playfield::pf1(uInt8 value)
{
  myPattern = (myPattern & 0x000FF00F)
    | ((value & 0x80) >> 3)
    | ((value & 0x40) >> 1)
    | ((value & 0x20) << 1)
    | ((value & 0x10) << 3)
    | ((value & 0x08) << 5)
    | ((value & 0x04) << 7)
    | ((value & 0x02) << 9)
    | ((value & 0x01) << 11);

  myPf1 = value;
  updatePattern();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Playfield::pf2(uInt8 value)
{
  myPattern = (myPattern & 0x00000FFF) | (value << 12);
  myPf2 = value;

  updatePattern();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Playfield::ctrlpf(uInt8 value)
{
  myReflected = (value & 0x01) > 0;
  myColorMode = (value & 0x06) == 0x02 ? ColorMode::score : ColorMode::normal;
  applyColors();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Playfield::toggleEnabled(bool enabled)
{
  myIsSuppressed = !enabled;

  updatePattern();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Playfield::toggleCollisions(bool enabled)
{
  myCollisionMaskEnabled = enabled ? 0xFFFF : (0x8000 | myCollisionMaskDisabled);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Playfield::setColor(uInt8 color)
{
  myObjectColor = color;
  applyColors();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Playfield::setColorP0(uInt8 color)
{
  myColorP0 = color;
  applyColors();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Playfield::setColorP1(uInt8 color)
{
  myColorP1 = color;
  applyColors();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Playfield::setDebugColor(uInt8 color)
{
  myDebugColor = color;
  applyColors();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Playfield::enableDebugColors(bool enabled)
{
  myDebugEnabled = enabled;
  applyColors();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Playfield::tick(uInt32 x)
{
  myX = x;

  if (myX == 80 || myX == 0) myRefp = myReflected;

  if (x & 0x03) return;

  uInt32 currentPixel;

  if (myEffectivePattern == 0) {
      currentPixel = 0;
  } else if (x < 80) {
      currentPixel = myEffectivePattern & (1 << (x >> 2));
  } else if (myRefp) {
      currentPixel = myEffectivePattern & (1 << (39 - (x >> 2)));
  } else {
      currentPixel = myEffectivePattern & (1 << ((x >> 2) - 20));
  }

  collision = currentPixel ? myCollisionMaskEnabled : myCollisionMaskDisabled;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Playfield::applyColors()
{
  if (myDebugEnabled)
    myColorLeft = myColorRight = myDebugColor;
  else
  {
    switch (myColorMode)
    {
      case ColorMode::normal:
        myColorLeft = myColorRight = myObjectColor;
        break;

      case ColorMode::score:
        myColorLeft = myColorP0;
        myColorRight = myColorP1;
        break;
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Playfield::updatePattern()
{
  myEffectivePattern = myIsSuppressed ? 0 : myPattern;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// TODO: implement this once the class is finalized
bool Playfield::save(Serializer& out) const
{
  try
  {
    out.putString(name());

    // TODO - save instance variables
  }
  catch(...)
  {
    cerr << "ERROR: TIA_Playfield::save" << endl;
    return false;
  }

  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// TODO: implement this once the class is finalized
bool Playfield::load(Serializer& in)
{
  try
  {
    if(in.getString() != name())
      return false;

    // TODO - load instance variables
  }
  catch(...)
  {
    cerr << "ERROR: TIA_Playfield::load" << endl;
    return false;
  }

  return false;
}
