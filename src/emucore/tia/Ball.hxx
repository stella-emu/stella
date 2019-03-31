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
// Copyright (c) 1995-2019 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#ifndef TIA_BALL
#define TIA_BALL

#include "Serializable.hxx"
#include "bspf.hxx"
#include "TIAConstants.hxx"

class TIA;

class Ball : public Serializable
{
  public:

    explicit Ball(uInt32 collisionMask);

  public:

    void setTIA(TIA* tia) { myTIA = tia; }

    void reset();

    void enabl(uInt8 value);

    void hmbl(uInt8 value);

    void resbl(uInt8 counter);

    void ctrlpf(uInt8 value);

    void vdelbl(uInt8 value);

    void toggleCollisions(bool enabled);

    void toggleEnabled(bool enabled);

    void setColor(uInt8 color);

    void setDebugColor(uInt8 color);
    void enableDebugColors(bool enabled);

    void applyColorLoss();

    void setInvertedPhaseClock(bool enable);

    void startMovement();

    void nextLine();

    bool isOn() const { return (collision & 0x8000); }
    uInt8 getColor() const { return myColor; }

    void shuffleStatus();

    uInt8 getPosition() const;
    void setPosition(uInt8 newPosition);

    bool getENABLOld() const { return myIsEnabledOld; }
    bool getENABLNew() const { return myIsEnabledNew; }

    void setENABLOld(bool enabled);

    /**
      Serializable methods (see that class for more information).
    */
    bool save(Serializer& out) const override;
    bool load(Serializer& in) override;

    void movementTick(uInt32 clock, bool hblank)
    {
      myLastMovementTick = myCounter;

      if (clock == myHmmClocks)
        isMoving = false;

      if(isMoving)
      {
        if (hblank) tick(false);
        myInvertedPhaseClock = !hblank;
      }
    }

    void tick(bool isReceivingMclock = true)
    {
      if(myUseInvertedPhaseClock && myInvertedPhaseClock)
      {
        myInvertedPhaseClock = false;
        return;
      }

      myIsVisible = myIsRendering && myRenderCounter >= 0;
      collision = (myIsVisible && myIsEnabled) ? myCollisionMaskEnabled : myCollisionMaskDisabled;

      bool starfieldEffect = isMoving && isReceivingMclock;

      if (myCounter == 156) {
        myIsRendering = true;
        myRenderCounter = renderCounterOffset;

        uInt8 starfieldDelta = (myCounter + TIAConstants::H_PIXEL - myLastMovementTick) % 4;
        if (starfieldEffect && starfieldDelta == 3 && myWidth < 4) ++myRenderCounter;

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

      } else if (myIsRendering && ++myRenderCounter >= (starfieldEffect ? myEffectiveWidth : myWidth))
        myIsRendering = false;

      if (++myCounter >= TIAConstants::H_PIXEL)
          myCounter = 0;
    }

  public:

    uInt32 collision;
    bool isMoving;

  private:

    void updateEnabled();
    void applyColors();

  private:

    enum Count: Int8 {
      renderCounterOffset = -4
    };

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
    bool myIsVisible;

    uInt8 myHmmClocks;
    uInt8 myCounter;
    uInt8 myWidth;
    uInt8 myEffectiveWidth;
    uInt8 myLastMovementTick;

    bool myIsRendering;
    Int8 myRenderCounter;

    bool myInvertedPhaseClock;
    bool myUseInvertedPhaseClock;

    TIA* myTIA;

  private:
    Ball() = delete;
    Ball(const Ball&) = delete;
    Ball(Ball&&) = delete;
    Ball& operator=(const Ball&) = delete;
    Ball& operator=(Ball&&);
};

#endif // TIA_BALL
