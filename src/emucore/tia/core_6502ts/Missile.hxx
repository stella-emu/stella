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

#ifndef TIA_6502TS_CORE_MISSILE
#define TIA_6502TS_CORE_MISSILE

#include "Serializable.hxx"
#include "bspf.hxx"

namespace TIA6502tsCore {

class Missile : public Serializable
{
  public:
    Missile(uInt32 collisionMask);

  public:

    void reset();

    void enam(uInt8 value);

    void hmm(uInt8 value);

    void resm(bool hblank);

    // TODO: resmp

    void nusiz(uInt8 value);

    void startMovement();

    bool movementTick(uInt32 clock, bool apply);

    void render();

    void tick();

    void setColor(uInt8 color);

    uInt8 getPixel(uInt8 colorIn) const;

    /**
      Serializable methods (see that class for more information).
    */
    bool save(Serializer& out) const override;
    bool load(Serializer& in) override;
    string name() const override { return "TIA_Missile"; }

  public:

    uInt32 collision;

  private:

    uInt32 myCollisionMask;

    bool myEnabled;
    bool myEnam;
    uInt8 myResmp;

    uInt8 myHmmClocks;
    uInt8 myCounter;
    bool myIsMoving;
    uInt8 myWidth;

    bool myIsRendering;
    Int8 myRenderCounter;

    const uInt8* myDecodes;

    uInt8 myColor;

  private:
    Missile(const Missile&) = delete;
    Missile(Missile&&) = delete;
    Missile& operator=(const Missile&) = delete;
    Missile& operator=(Missile&&) = delete;
};

} // namespace TIA6502tsCore

#endif // TIA_6502TS_CORE_MISSILE
