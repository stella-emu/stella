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

#ifndef TIA_PLAYER
#define TIA_PLAYER

#include "Serializable.hxx"
#include "bspf.hxx"

class Player : public Serializable
{
  public:
    Player(uInt32 collisionMask);

  public:

    void reset();

    void grp(uInt8 value);
    uInt8 grp() const { return myPatternNew; }

    void hmp(uInt8 value);
    uInt8 hmp() const { return myHmmClocks; }

    void nusiz(uInt8 value);

    void resp(uInt8 counter);

    void refp(uInt8 value);
    bool refp() const { return myIsReflected; }

    void vdelp(uInt8 value);
    bool vdelp() const { return myIsDelaying; }

    void toggleEnabled(bool enabled);

    void toggleCollisions(bool enabled);

    void setColor(uInt8 color);
    uInt8 getColor() const { return myObjectColor; }

    void setDebugColor(uInt8 color);
    void enableDebugColors(bool enabled);

    void startMovement();

    bool movementTick(uInt32 clock, bool apply);

    void render();

    void tick();
    uInt8 getClock() const { return myCounter; }

    uInt8 getPixel(uInt8 colorIn) const {
      return (collision & 0x8000) ? myColor : colorIn;
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
    void applyColors();
    void setDivider(uInt8 divider);

  private:

    uInt32 myCollisionMaskDisabled;
    uInt32 myCollisionMaskEnabled;
    uInt8 myColor;
    uInt8 myObjectColor, myDebugColor;
    bool myDebugEnabled;

    bool myIsSuppressed;

    uInt8 myHmmClocks;
    uInt8 myCounter;
    bool myIsMoving;

    bool myIsRendering;
    Int8 myRenderCounter;
    Int8 myRenderCounterTripPoint;
    uInt8 myDivider;
    uInt8 myDividerPending;
    uInt8 mySampleCounter;
    Int8 myDividerChangeCounter;

    const uInt8* myDecodes;

    uInt8 myPatternOld;
    uInt8 myPatternNew;
    uInt8 myPattern;

    bool myIsReflected;
    bool myIsDelaying;

  private:
    Player(const Player&) = delete;
    Player(Player&&) = delete;
    Player& operator=(const Player&) = delete;
    Player& operator=(Player&&) = delete;
};

#endif // TIA_PLAYER
