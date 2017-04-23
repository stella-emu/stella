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

#ifndef TIA_TIA
#define TIA_TIA

#include "bspf.hxx"
#include "Console.hxx"
#include "Sound.hxx"
#include "Settings.hxx"
#include "Device.hxx"
#include "Serializer.hxx"
#include "TIATypes.hxx"
#include "DelayQueue.hxx"
#include "FrameManager.hxx"
#include "FrameLayout.hxx"
#include "Background.hxx"
#include "Playfield.hxx"
#include "Missile.hxx"
#include "Player.hxx"
#include "Ball.hxx"
#include "LatchedInput.hxx"
#include "PaddleReader.hxx"
#include "PlayfieldPositionProvider.hxx"

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
class TIA : public Device, public PlayfieldPositionProvider
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
    uInt8* currentFrameBuffer() const  { return myCurrentFrameBuffer.get();  }
    uInt8* previousFrameBuffer() const { return myPreviousFrameBuffer.get(); }

    /**
      Answers dimensional info about the framebuffer
    */
    uInt32 width() const  { return 160; }
    uInt32 height() const { return myFrameManager.height(); }
    uInt32 ystart() const { return myFrameManager.ystart(); }
    bool ystartIsAuto(uInt32 line) const { return myFrameManager.ystartIsAuto(line); }

    /**
      Changes the current Height/YStart properties.
      Note that calls to these method(s) must be eventually followed by
      ::frameReset() for the changes to take effect.
    */
    void setHeight(uInt32 height) { myFrameManager.setFixedHeight(height); }
    void setYStart(uInt32 ystart) { myFrameManager.setYstart(ystart); }

    void autodetectLayout(bool toggle) { myFrameManager.autodetectLayout(toggle); }
    void setLayout(FrameLayout layout) { myFrameManager.setLayout(layout); }
    FrameLayout frameLayout() const { return myFrameManager.layout(); }

    /**
      Answers the timing of the console currently in use.
    */
    ConsoleTiming consoleTiming() const { return myConsole.timing(); }

    /**
      Enables/disables auto-frame calculation.  If enabled, the TIA
      re-adjusts the framerate at regular intervals.

      @param enabled  Whether to enable or disable all auto-frame calculation
    */
    void enableAutoFrame(bool enabled) { myAutoFrameEnabled = enabled; }

    /**
      Enables/disables color-loss for PAL modes only.

      @param enabled  Whether to enable or disable PAL color-loss mode
    */
    void enableColorLoss(bool enabled);

    /**
      Answers the current color clock we've gotten to on this scanline.

      @return The current color clock
    */
    uInt32 clocksThisLine() const { return myHctr - myXDelta; }

    /**
      Answers the total number of scanlines the TIA generated in producing
      the current frame buffer. For partial frames, this will be the
      current scanline.

      @return The total number of scanlines generated
    */
    uInt32 scanlines() const { return myFrameManager.scanlines(); }

    /**
      Answers the total number of scanlines the TIA generated in the
      previous frame.

      @return The total number of scanlines generated in the last frame.
    */
    uInt32 scanlinesLastFrame() const { return myFrameManager.scanlinesLastFrame(); }

    /**
      Answers whether the TIA is currently in being rendered
      (we're in between the start and end of drawing a frame).

      @return If the frame is in rendering mode
    */
    bool isRendering() const { return myFrameManager.isRendering(); }

    /**
      Answers the current position of the virtual 'electron beam' used
      when drawing the TIA image in debugger mode.

      @return The x/y coordinates of the scanline electron beam, and whether
              it is in the visible/viewable area of the screen
    */
    bool electronBeamPos(uInt32& x, uInt32& y) const;

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

    /**
      Retrieve the last value written to a certain register
    */
    uInt8 registerValue(uInt8 reg) const;

    /**
      Get the current x value
    */
    uInt8 getPosition() const override {
      return (myHctr < 68) ? 0 : (myHctr - 68 - myXDelta);
    }

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
      P0ColorNTSC = 0x30, // red
      M0ColorNTSC = 0x38, // orange
      P1ColorNTSC = 0x1c, // yellow
      M1ColorNTSC = 0xc4, // green
      BLColorNTSC = 0x9e, // blue
      PFColorNTSC = 0x66, // purple
      BKColorNTSC = 0x0a, // grey

      P0ColorPAL  = 0x62, // red
      M0ColorPAL  = 0x4a, // orange
      P1ColorPAL  = 0x2e, // yellow
      M1ColorPAL  = 0x34, // green
      BLColorPAL  = 0xbc, // blue
      PFColorPAL  = 0xa6, // purple
      BKColorPAL  = 0x0a, // grey

      HBLANKColor = 0x0e  // white
    };

  private:

    void onFrameStart();

    void onFrameComplete();

    void updateEmulation();

    void cycle(uInt32 colorClocks);

    void tickMovement();

    void tickHblank();

    void tickHframe();

    void applyRsync();

    void updateCollision();

    void renderPixel(uInt32 x, uInt32 y, bool lineNotCached);

    void clearHmoveComb();

    void nextLine();

    void delayedWrite(uInt8 address, uInt8 value);

    void updatePaddle(uInt8 idx);

    uInt8 resxCounter();

    void swapBuffers();

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

    DelayQueue myDelayQueue;
    FrameManager myFrameManager;

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

    // Pointer to the current and previous frame buffers
    BytePtr myCurrentFrameBuffer;
    BytePtr myPreviousFrameBuffer;

    bool myTIAPinsDriven;

    HState myHstate;
    bool myIsFreshLine;

    Int32 myHblankCtr;
    Int32 myHctr;
    uInt32 myXDelta;

    bool myCollisionUpdateRequired;
    uInt32 myCollisionMask;

    uInt32 myMovementClock;
    bool myMovementInProgress;
    bool myExtendedHblank;

    uInt32 myLinesSinceChange;

    Priority myPriority;

    uInt8 mySubClock;
    Int32 myLastCycle;

    uInt8 mySpriteEnabledBits;
    uInt8 myCollisionsEnabledBits;

    uInt8 myColorHBlank;

    double myTimestamp;

    uInt8 myShadowRegisters[64];

    // Automatic framerate correction based on number of scanlines
    bool myAutoFrameEnabled;

   private:
    TIA() = delete;
    TIA(const TIA&) = delete;
    TIA(TIA&&) = delete;
    TIA& operator=(const TIA&) = delete;
    TIA& operator=(TIA&&) = delete;
};

#endif // TIA_TIA
