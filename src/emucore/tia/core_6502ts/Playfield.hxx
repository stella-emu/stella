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

#ifndef TIA_6502TS_CORE_PLAYFIELD
#define TIA_6502TS_CORE_PLAYFIELD

#include "bspf.hxx"

namespace TIA6502tsCore {

class Playfield
{
  public:
    Playfield(uInt32 collisionMask);

  public:

    void reset();

    void pf0(uInt8 value);

    void pf1(uInt8 value);

    void pf2(uInt8 value);

    void ctrlpf(uInt8 value);

    void setColor(uInt8 color);

    void setColorP0(uInt8 color);

    void setColorP1(uInt8 color);

    void tick(uInt32 x);

    uInt8 getPixel(uInt8 colorIn) const;

  public:

    uInt32 collision;

  private:

    enum ColorMode {normal, score};

  private:

    void applyColors();

  private:

    uInt8 myColorLeft;
    uInt8 myColorRight;
    uInt8 myColorP0;
    uInt8 myColorP1;
    uInt8 myColor;
    ColorMode myColorMode;

    uInt32 myPattern;
    bool myRefp;
    bool myReflected;

    uInt8 myPf0;
    uInt8 myPf1;
    uInt8 myPf2;

    uInt32 myX;

    uInt32 myCollisionMask;

  private:
    Playfield() = delete;
    Playfield(const Playfield&) = delete;
    Playfield(Playfield&&) = delete;
    Playfield& operator=(const Playfield&) = delete;
    Playfield& operator=(Playfield&&) = delete;
};

} // namespace TIA6502tsCore

#endif // TIA_6502TS_CORE_PLAYFIELD
