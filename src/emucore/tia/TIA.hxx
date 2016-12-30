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
*/
class TIA : public Device
{
  public:
    friend class TIADebug;
    friend class RiotDebug;

    /**
      Create a new TIA for the specified console

      @param console   The console the TIA is associated with
      @param sound     The sound object the TIA is associated with
      @param settings  The settings object for this TIA device
    */
    TIA(Console& console, Sound& sound, Settings& settings);
    virtual ~TIA() = default;

  public:
    /**
      Reset device to its power-on state
    */
    void reset() override;

    /**
      Reset frame to current YStart/Height properties
    */
    void frameReset();

    /**
      Notification method invoked by the system right before the
      system resets its cycle counter to zero.
    */
    void systemCyclesReset() override;

    /**
      Install TIA in the specified system.  Invoked by the system
      when the TIA is attached to it.

      @param system The system the device should install itself in
    */
    void install(System& system) override;

    /**
      Get the byte at the specified address

      @return The byte at the specified address
    */
    uInt8 peek(uInt16 address) override;

    /**
      Change the byte at the specified address to the given value

      @param address  The address where the value should be stored
      @param value    The value to be stored at the address

      @return  True if the poke changed the device address space, else false
    */
    bool poke(uInt16 address, uInt8 value) override;

    /**
      Install TIA in the specified system and device.  Invoked by
      the system when the TIA is attached to it.  All devices
      which invoke this method take responsibility for chaining
      requests back to *this* device.

      @param system  The system the device should install itself in
      @param device  The device responsible for this address space
    */
    void installDelegate(System& system, Device& device);

    /**
      The following are very similar to save() and load(), except they
      do a 'deeper' save of the display data itself.

      Normally, the internal framebuffer doesn't need to be saved to
      a state file, since the file already contains all the information
      needed to re-create it, starting from scanline 0.  In effect, when a
      state is loaded, the framebuffer is empty, and the next call to
      update() generates valid framebuffer data.

      However, state files saved from the debugger need more information,
      such as the exact state of the internal framebuffer itself *before*
      we call update(), including if the display was in partial frame mode.

      Essentially, a normal state save has 'frame resolution', whereas
      the debugger state save has 'cycle resolution', and hence needs
      more information.  The methods below save/load this extra info,
      and eliminate having to save approx. 50K to normal state files.
    */
    bool saveDisplay(Serializer& out) const;
    bool loadDisplay(Serializer& in);

    /**
      This method should be called at an interval corresponding to the
      desired frame rate to update the TIA.  Invoking this method will update
      the graphics buffer and generate the corresponding audio samples.
    */
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

    /**
      Enables/disables auto-frame calculation.  If enabled, the TIA
      re-adjusts the framerate at regular intervals.

      @param enabled  Whether to enable or disable all auto-frame calculation
    */
    void enableAutoFrame(bool enabled);

    /**
      Enables/disables color-loss for PAL modes only.

      @param enabled  Whether to enable or disable PAL color-loss mode
    */
    void enableColorLoss(bool enabled);

    /**
      Answers whether this TIA runs at NTSC or PAL scanrates.
    */
    bool isPAL() const;

    /**
      Answers the current color clock we've gotten to on this scanline.

      @return The current color clock
    */
    uInt32 clocksThisLine() const;

    /**
      Answers the total number of scanlines the TIA generated in producing
      the current frame buffer. For partial frames, this will be the
      current scanline.

      @return The total number of scanlines generated
    */
    uInt32 scanlines() const;

    /**
      Answers whether the TIA is currently in 'partial frame' mode
      (we're in between the start and end of drawing a frame).

      @return If we're in partial frame mode
    */
    bool partialFrame() const;

    /**
      Answers the current position of the virtual 'electron beam' used to
      draw the TIA image.  If not in partial frame mode, the position is
      defined to be in the lower right corner (@ width/height of the screen).
      Note that the coordinates are with respect to currentFrameBuffer(),
      taking any YStart values into account.

      @return The x/y coordinates of the scanline electron beam, and whether
              it is in the visible/viewable area of the screen
    */
    bool scanlinePos(uInt16& x, uInt16& y) const;

    /**
      Enables/disable/toggle the specified (or all) TIA bit(s).  Note that
      disabling a graphical object also disables its collisions.

      @param mode  1/0 indicates on/off, and values greater than 1 mean
                   flip the bit from its current state

      @return  Whether the bit was enabled or disabled
    */
    bool toggleBit(TIABit b, uInt8 mode = 2);
    bool toggleBits();

    /**
      Enables/disable/toggle the specified (or all) TIA bit collision(s).

      @param mode  1/0 indicates on/off, and values greater than 1 mean
                   flip the collision from its current state

      @return  Whether the collision was enabled or disabled
    */
    bool toggleCollision(TIABit b, uInt8 mode = 2);
    bool toggleCollisions();

    /**
      Enables/disable/toggle 'fixed debug colors' mode.

      @param mode  1/0 indicates on/off, otherwise flip from
                   its current state

      @return  Whether the mode was enabled or disabled
    */
    bool toggleFixedColors(uInt8 mode = 2);

    /**
      Enable/disable/query state of 'undriven/floating TIA pins'.

      @param mode  1/0 indicates on/off, otherwise return the current state

      @return  Whether the mode was enabled or disabled
    */
    bool driveUnusedPinsRandom(uInt8 mode = 2);

    /**
      Enables/disable/toggle 'scanline jittering' mode, and set the
      recovery 'factor'.

      @param mode  1/0 indicates on/off, otherwise flip from
                   its current state

      @return  Whether the mode was enabled or disabled
    */
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

    void delayedWrite(uInt8 address, uInt8 value);

    void updatePaddle(uInt8 idx);

    uInt8 resxCounter();

    /**
      Get the result of the specified collision register.
    */
    uInt8 collCXM0P() const;
    uInt8 collCXM1P() const;
    uInt8 collCXP0FB() const;
    uInt8 collCXP1FB() const;
    uInt8 collCXM0FB() const;
    uInt8 collCXM1FB() const;
    uInt8 collCXPPMM() const;
    uInt8 collCXBLPF() const;

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
    uInt8 myCtrlPF;  // needed for the debugger

    uInt8 mySubClock;
    Int32 myLastCycle;

    uInt8 mySpriteEnabledBits;
    uInt8 myCollisionsEnabledBits;

    uInt8 myColorHBlank;

    double myTimestamp;

    // Automatic framerate correction based on number of scanlines
    bool myAutoFrameEnabled;

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
