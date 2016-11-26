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

#ifndef TIA_6502TS_CORE_PLAYER
#define TIA_6502TS_CORE_PLAYER

#include "Serializable.hxx"
#include "bspf.hxx"

namespace TIA6502tsCore {

class Player : public Serializable
{
  public:
    Player(uInt32 collisionMask);

  public:

    void reset();

    void grp(uInt8 value);

    void hmp(uInt8 value);

    void nusiz(uInt8 value);

    void resp(bool hblank);

    void refp(uInt8 value);

    void vdelp(uInt8 value);

    void setColor(uInt8 color);

    void startMovement();

    bool movementTick(uInt32 clock, bool apply);

    void render();

    void tick();

    uInt8 getPixel(uInt8 colorIn) const {
      return collision ? colorIn : myColor;
    }

    void shufflePatterns();

    uInt8 getRespClock() const;

    /**
      Serializable methods (see that class for more information).
    */
    bool save(Serializer& out) const override;
    bool load(Serializer& in) override;
    string name() const override { return "TIA_Player"; }

  public:

    uInt32 collision;

  private:

    void updatePattern();

  private:

    uInt32 myCollisionMask;
    uInt8 myColor;

    uInt8 myHmmClocks;
    uInt8 myCounter;
    bool myIsMoving;
    uInt8 myWidth;

    bool myIsRendering;
    Int8 myRenderCounter;

    const uInt8* myDecodes;

    uInt8 myPatternOld;
    uInt8 myPatternNew;
    uInt32 myPattern;

    bool myIsReflected;
    bool myIsDelaying;

  private:
    Player(const Player&) = delete;
    Player(Player&&) = delete;
    Player& operator=(const Player&) = delete;
    Player& operator=(Player&&) = delete;
};

} // namespace TIA6502tsCore

#endif // TIA_6502TS_CORE_PLAYER
