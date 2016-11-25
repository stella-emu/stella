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

#include "Playfield.hxx"

namespace TIA6502tsCore {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Playfield::Playfield(uInt32 collisionMask)
  : myCollisionMask(collisionMask)
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

  myColor = 0;
  myColorP0 = 0;
  myColorP1 = 0;
  myColorMode = ColorMode::normal;

  collision = 0;

  applyColors();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Playfield::pf0(uInt8 value)
{
  myPattern = (myPattern & 0x000FFFF0) | ((value & 0xF0) >> 4);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Playfield::pf1(uInt8 value)
{
  myPattern = (myPattern & 0x000FF00F)
    | ((value & 0x80) >> 3)
    | ((value & 0x40) >> 1)
    | ((value & 0x20) <<  1)
    | ((value & 0x10) <<  3)
    | ((value & 0x08) <<  5)
    | ((value & 0x04) <<  7)
    | ((value & 0x02) <<  9)
    | ((value & 0x01) <<  11);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Playfield::pf2(uInt8 value)
{
  myPattern = (myPattern & 0x00000FFF) | ((value & 0xFF) << 12);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Playfield::ctrlpf(uInt8 value)
{
  myReflected = (value & 0x01) > 0;
  myColorMode = (value & 0x06) == 0x02 ? ColorMode::score : ColorMode::normal;
  applyColors();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Playfield::setColor(uInt8 color)
{
  myColor = color;
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
void Playfield::tick(uInt32 x)
{
  myX = x;

  if (myX == 80 || myX == 0) myRefp = myReflected;

  if (x & 0x03) return;

  uInt32 currentPixel;

  if (myPattern == 0) {
      currentPixel = 0;
  } else if (x < 80) {
      currentPixel = myPattern & (1 << (x >> 2));
  } else if (myRefp) {
      currentPixel = myPattern & (1 << (39 - (x >> 2)));
  } else {
      currentPixel = myPattern & (1 << ((x >> 2) - 20));
  }

  collision = currentPixel ? 0 : myCollisionMask;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 Playfield::getPixel(uInt8 colorIn) const
{
  if (!collision) return myX < 80 ? myColorLeft : myColorRight;

  return colorIn;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Playfield::applyColors()
{
  switch (myColorMode)
  {
    case ColorMode::normal:
      myColorLeft = myColorRight = myColor;
      break;

    case ColorMode::score:
      myColorLeft = myColorP0;
      myColorRight = myColorP1;
      break;
  }
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

} // namespace TIA6502tsCore
