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

#ifndef TIA_6502TS_CORE_BALL
#define TIA_6502TS_CORE_BALL

#include "Serializable.hxx"
#include "bspf.hxx"

class Ball : public Serializable
{
  public:

    Ball(uInt32 collisionMask);

  public:

    void reset();

    void enabl(uInt8 value);
    bool enabl() const { return myIsEnabled; }

    void hmbl(uInt8 value);
    uInt8 hmbl() const { return myHmmClocks; }

    void resbl(uInt8 counter);

    void ctrlpf(uInt8 value);

    void vdelbl(uInt8 value);
    bool vdelbl() const { return myIsDelaying; }

    void toggleCollisions(bool enabled);

    void toggleEnabled(bool enabled);

    void setColor(uInt8 color);

    void setDebugColor(uInt8 color);
    void enableDebugColors(bool enabled);

    void startMovement();

    bool movementTick(uInt32 clock, bool apply);

    void render();

    void tick(bool isReceivingMclock = true);

    uInt8 getPixel(uInt8 colorIn) const {
      return (collision & 0x8000) ? myColor : colorIn;
    }

    void shuffleStatus();

    /**
      Serializable methods (see that class for more information).
    */
    bool save(Serializer& out) const override;
    bool load(Serializer& in) override;
    string name() const override { return "TIA_Ball"; }

  public:

    uInt32 collision;

  private:

    void updateEnabled();
    void applyColors();

  private:

    uInt32 myCollisionMaskDisabled;
    uInt32 myCollisionMaskEnabled;

    uInt8 myColor;
    uInt8 myObjectColor, myDebugColor;
    bool myDebugEnabled;

    bool myIsEnabledOld;
    bool myIsEnabledNew;
    bool myIsEnabled;
    bool myIsSuppressed;
    bool myIsDelaying;

    uInt8 myHmmClocks;
    uInt8 myCounter;
    bool myIsMoving;
    uInt8 myWidth;
    uInt8 myEffectiveWidth;
    uInt8 myLastMovementTick;

    bool myIsRendering;
    Int8 myRenderCounter;

  private:
    Ball() = delete;
    Ball(const Ball&) = delete;
    Ball(Ball&&) = delete;
    Ball& operator=(const Ball&) = delete;
    Ball& operator=(Ball&&);
};

#endif // TIA_6502TS_CORE_BALL
