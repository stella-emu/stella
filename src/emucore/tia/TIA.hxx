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
#include "Sound.hxx"
#include "Settings.hxx"
#include "Device.hxx"
#include "Serializer.hxx"
#include "TIATypes.hxx"
#include "DelayQueue.hxx"
#include "FrameManager.hxx"
#include "Background.hxx"
#include "Playfield.hxx"
#include "Missile.hxx"
#include "Player.hxx"
#include "Ball.hxx"
#include "LatchedInput.hxx"
#include "PaddleReader.hxx"

class Console;

/**
  This class is a device that emulates the Television Interface Adaptor
  found in the Atari 2600 and 7800 consoles.  The Television Interface
  Adaptor is an integrated circuit designed to interface between an
  eight bit microprocessor and a television video modulator. It converts
  eight bit parallel data into serial outputs for the color, luminosity,
  and composite sync required by a video modulator.

  This class outputs the serial data into a frame buffer which can then
  be displayed on screen.

  @author  Christian Speckner (DirtyHairy) and Stephen Anthony
  @version $Id$
*/
class TIA : public Device
{
  public:
    friend class TIADebug;
    friend class RiotDebug;

    /**
      Create a new TIA for the specified console

      @param console  The console the TIA is associated with
      @param sound    The sound object the TIA is associated with
      @param settings The settings object for this TIA device
    */
    TIA(Console& console, Sound& sound, Settings& settings);
    virtual ~TIA() = default;

  public:

    void reset() override;

    /**
      Reset frame to current YStart/Height properties
    */
    void frameReset();

    void systemCyclesReset() override;

    void install(System& system) override;

    uInt8 peek(uInt16 address) override;

    bool poke(uInt16 address, uInt8 value) override;

    void installDelegate(System& system, Device& device);

    bool saveDisplay(Serializer& out) const;

    bool loadDisplay(Serializer& in);

    void update();

    /**
      Answers the current and previous frame buffer pointers
    */
    uInt8* currentFrameBuffer() const {
      return myCurrentFrameBuffer.get();
    }
    uInt8* previousFrameBuffer() const {
      return myPreviousFrameBuffer.get();
    }

    /**
      Answers dimensional info about the framebuffer
    */
    uInt32 width() const { return 160; }
    uInt32 height() const;
    uInt32 ystart() const;

    /**
      Changes the current Height/YStart properties.
      Note that calls to these method(s) must be eventually followed by
      ::frameReset() for the changes to take effect.
    */
    void setHeight(uInt32 height);
    void setYStart(uInt32 ystart);

    void enableAutoFrame(bool enabled);

    void enableColorLoss(bool enabled);

    bool isPAL() const;

    uInt32 clocksThisLine() const;

    uInt32 scanlines() const;

    bool partialFrame() const;

    uInt32 startScanline() const;

    bool scanlinePos(uInt16& x, uInt16& y) const;

    bool toggleBit(TIABit b, uInt8 mode = 2);

    bool toggleBits();

    bool toggleCollision(TIABit b, uInt8 mode = 2);

    bool toggleCollisions();

    bool toggleFixedColors(uInt8 mode = 2);

    bool driveUnusedPinsRandom(uInt8 mode = 2);

    bool toggleJitter(uInt8 mode = 2);

    void setJitterRecoveryFactor(Int32 f);

    // Clear both internal TIA buffers to black (palette color 0)
    void clearBuffers();

  #ifdef DEBUGGER_SUPPORT
    /**
      This method should be called to update the TIA with a new scanline.
    */
    void updateScanline();

    /**
      This method should be called to update the TIA with a new partial
      scanline by stepping one CPU instruction.
    */
    void updateScanlineByStep();

    /**
      This method should be called to update the TIA with a new partial
      scanline by tracing to target address.
    */
    void updateScanlineByTrace(int target);
  #endif

    /**
      Save the current state of this device to the given Serializer.

      @param out  The Serializer object to use
      @return  False on any errors, else true
    */
    bool save(Serializer& out) const override;

    /**
      Load the current state of this device from the given Serializer.

      @param in  The Serializer object to use
      @return  False on any errors, else true
    */
    bool load(Serializer& in) override;

    /**
      Get a descriptor for the device name (used in error checking).

      @return The name of the object
    */
    string name() const override { return "TIA"; }

  private:
    enum HState {blank, frame};
    enum Priority {pfp, score, normal};

    enum FixedColors {
      P0ColorNTSC = 0x30,
      P1ColorNTSC = 0x16,
      M0ColorNTSC = 0x38,
      M1ColorNTSC = 0x12,
      BLColorNTSC = 0x7e,
      PFColorNTSC = 0x76,
      BKColorNTSC = 0x0a,
      P0ColorPAL  = 0x62,
      P1ColorPAL  = 0x26,
      M0ColorPAL  = 0x68,
      M1ColorPAL  = 0x2e,
      BLColorPAL  = 0xde,
      PFColorPAL  = 0xd8,
      BKColorPAL  = 0x1c,
      HBLANKColor = 0x0e
    };

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

    void updatePaddle(uInt8 idx);

  private:

    Console& myConsole;
    Sound& mySound;
    Settings& mySettings;

    bool myTIAPinsDriven;

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

    uInt8 mySpriteEnabledBits;
    uInt8 myCollisionsEnabledBits;

    uInt8 myColorHBlank;

    double myTimestamp;

    // Pointer to the current and previous frame buffers
    BytePtr myCurrentFrameBuffer;
    BytePtr myPreviousFrameBuffer;

    Background myBackground;
    Playfield myPlayfield;
    Missile myMissile0;
    Missile myMissile1;
    Player myPlayer0;
    Player myPlayer1;
    Ball myBall;
    PaddleReader myPaddleReaders[4];

    LatchedInput myInput0;
    LatchedInput myInput1;

   private:
    TIA() = delete;
    TIA(const TIA&) = delete;
    TIA(TIA&&) = delete;
    TIA& operator=(const TIA&) = delete;
    TIA& operator=(TIA&&) = delete;
};

#endif // TIA_6502TS_CORE_TIA
