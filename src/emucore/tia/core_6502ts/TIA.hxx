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

#ifndef TIA_6502TS_CORE_TIA
#define TIA_6502TS_CORE_TIA

#include "bspf.hxx"
#include "AbstractTIA.hxx"
#include "Sound.hxx"
#include "Settings.hxx"
#include "TIATypes.hxx"
#include "DelayQueue.hxx"
#include "FrameManager.hxx"
#include "Playfield.hxx"
#include "Missile.hxx"
#include "Player.hxx"
#include "Ball.hxx"

class Console;

namespace TIA6502tsCore {

class TIA : public AbstractTIA
{
  public:
    TIA(Console& console, Sound& sound, Settings& settings);
    virtual ~TIA() = default;

  public:

    void reset() override;

    void systemCyclesReset() override;

    void install(System& system) override;

    bool save(Serializer& out) const override;

    bool load(Serializer& in) override;

    string name() const override
    {
      return "TIA";
    }

    uInt8 peek(uInt16 address) override;

    bool poke(uInt16 address, uInt8 value) override;

    void installDelegate(System& system, Device& device) override;

    void frameReset() override;

    bool saveDisplay(Serializer& out) const override;

    bool loadDisplay(Serializer& in) override;

    void update() override;

    uInt8* currentFrameBuffer() const override;

    uInt8* previousFrameBuffer() const override;

    uInt32 height() const override;

    uInt32 ystart() const override;

    void setHeight(uInt32 height) override;

    void setYStart(uInt32 ystart) override;

    void enableAutoFrame(bool enabled) override;

    void enableColorLoss(bool enabled) override;

    bool isPAL() const override;

    uInt32 clocksThisLine() const override;

    uInt32 scanlines() const override;

    bool partialFrame() const override;

    uInt32 startScanline() const override;

    bool scanlinePos(uInt16& x, uInt16& y) const override;

    bool toggleBit(TIABit b, uInt8 mode = 2) override;

    bool toggleBits() override;

    bool toggleCollision(TIABit b, uInt8 mode = 2) override;

    bool toggleCollisions() override;

    bool toggleHMOVEBlank() override;

    bool toggleFixedColors(uInt8 mode = 2) override;

    bool driveUnusedPinsRandom(uInt8 mode = 2) override;

    bool toggleJitter(uInt8 mode = 2) override;

    void setJitterRecoveryFactor(Int32 f) override;

  private:

    enum HState {blank, frame};

    enum Priority {normal, inverted};

  private:

    void updateEmulation();

    void cycle(uInt32 colorClocks);

    void tickMovement();

    void tickHblank();

    void tickHframe();

    void updateCollision();

    void renderSprites();

    void tickSprites();

    void renderPixel(uInt32 x, uInt32 y, bool lineNotCached);

    void clearHmoveComb();

    void nextLine();

    void onFrameComplete();

    void onFrameStart();

    void delayedWrite(uInt8 address, uInt8 value);

  private:

    Console& myConsole;
    Sound& mySound;
    Settings& mySettings;

    DelayQueue myDelayQueue;
    FrameManager myFrameManager;

    HState myHstate;
    bool myIsFreshLine;

    Int32 myHblankCtr;
    Int32 myHctr;

    bool myCollisionUpdateRequired;
    uInt32 myCollisionMask;

    uInt32 myMovementClock;
    bool myMovementInProgress;
    bool myExtendedHblank;

    uInt32 myLinesSinceChange;

    Priority myPriority;

    uInt8 mySubClock;
    uInt32 myLastCycle;

    uInt8 myColorBk;

    BytePtr myCurrentFrameBuffer;
    BytePtr myPreviousFrameBuffer;

    Playfield myPlayfield;
    Missile myMissile0;
    Missile myMissile1;
    Player myPlayer0;
    Player myPlayer1;
    Ball myBall;

   private:
    TIA() = delete;
    TIA(const TIA&) = delete;
    TIA(TIA&&) = delete;
    TIA& operator=(const TIA&) = delete;
    TIA& operator=(TIA&&) = delete;
};

} // namespace TIA6502tsCore

#endif // TIA_6502TS_CORE_TIA
