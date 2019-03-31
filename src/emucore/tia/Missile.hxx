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

#ifndef TIA_MISSILE
#define TIA_MISSILE

#include "Serializable.hxx"
#include "bspf.hxx"
#include "Player.hxx"
#include "TIAConstants.hxx"

class TIA;

class Missile : public Serializable
{
  public:

    explicit Missile(uInt32 collisionMask);

  public:

    void setTIA(TIA* tia) { myTIA = tia; }

    void reset();

    void enam(uInt8 value);

    void hmm(uInt8 value);

    void resm(uInt8 counter, bool hblank);

    void resmp(uInt8 value, const Player& player);

    void nusiz(uInt8 value);

    void startMovement();

    void nextLine();

    void setColor(uInt8 color);

    void setDebugColor(uInt8 color);
    void enableDebugColors(bool enabled);

    void applyColorLoss();

    void setInvertedPhaseClock(bool enable);

    void toggleCollisions(bool enabled);

    void toggleEnabled(bool enabled);

    bool isOn() const { return (collision & 0x8000); }
    uInt8 getColor() const { return myColor; }

    uInt8 getPosition() const;
    void setPosition(uInt8 newPosition);

    /**
      Serializable methods (see that class for more information).
    */
    bool save(Serializer& out) const override;
    bool load(Serializer& in) override;

    void movementTick(uInt8 clock, uInt8 hclock, bool hblank)
    {
      if(clock == myHmmClocks) isMoving = false;

      if (isMoving)
      {
        if (hblank) tick(hclock, false);
        myInvertedPhaseClock = !hblank;
      }
    }

    void tick(uInt8 hclock, bool isReceivingMclock = true)
    {
      if(myUseInvertedPhaseClock && myInvertedPhaseClock)
      {
        myInvertedPhaseClock = false;
        return;
      }

      myIsVisible =
        myIsRendering &&
        (myRenderCounter >= 0 || (isMoving && isReceivingMclock && myRenderCounter == -1 && myWidth < 4 && ((hclock + 1) % 4 == 3)));

      collision = (myIsVisible && myIsEnabled) ? myCollisionMaskEnabled : myCollisionMaskDisabled;

      if (myDecodes[myCounter] && !myResmp) {
        myIsRendering = true;
        myRenderCounter = renderCounterOffset;
      } else if (myIsRendering) {

          if (myRenderCounter == -1) {
            if (isMoving && isReceivingMclock) {
              switch ((hclock + 1) % 4) {
                case 3:
                  myEffectiveWidth = myWidth == 1 ? 2 : myWidth;
                  if (myWidth < 4) ++myRenderCounter;
                  break;

                case 2:
                  myEffectiveWidth = 0;
                  break;

                default:
                  myEffectiveWidth = myWidth;
                  break;
              }
            } else {
              myEffectiveWidth = myWidth;
            }
          }

          if (++myRenderCounter >= (isMoving ? myEffectiveWidth : myWidth)) myIsRendering = false;
      }

      if (++myCounter >= TIAConstants::H_PIXEL) myCounter = 0;
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

    bool myIsEnabled;
    bool myIsSuppressed;
    bool myEnam;
    uInt8 myResmp;

    uInt8 myHmmClocks;
    uInt8 myCounter;

    uInt8 myWidth;
    uInt8 myEffectiveWidth;

    bool myIsRendering;
    bool myIsVisible;
    Int8 myRenderCounter;

    const uInt8* myDecodes;
    uInt8 myDecodesOffset;  // needed for state saving

    uInt8 myColor;
    uInt8 myObjectColor, myDebugColor;
    bool myDebugEnabled;

    bool myInvertedPhaseClock;
    bool myUseInvertedPhaseClock;

    TIA *myTIA;

  private:
    Missile(const Missile&) = delete;
    Missile(Missile&&) = delete;
    Missile& operator=(const Missile&) = delete;
    Missile& operator=(Missile&&) = delete;
};

#endif // TIA_MISSILE
