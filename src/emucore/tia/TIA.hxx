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
#include "DelayQueueIterator.hxx"
#include "FrameManager.hxx"
#include "FrameLayout.hxx"
#include "Background.hxx"
#include "Playfield.hxx"
#include "Missile.hxx"
#include "Player.hxx"
#include "Ball.hxx"
#include "LatchedInput.hxx"
#include "PaddleReader.hxx"
#include "DelayQueueIterator.hxx"

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
    enum DummyRegisters: uInt8 {
      shuffleP0 = 0xF0,
      shuffleP1 = 0xF1,
      shuffleBL = 0xF2
    };

    enum FixedColor {
      NTSC_RED    = 0x30,
      NTSC_ORANGE = 0x38,
      NTSC_YELLOW = 0x1c,
      NTSC_GREEN  = 0xc4,
      NTSC_BLUE   = 0x9e,
      NTSC_PURPLE = 0x66,

      PAL_RED     = 0x62,
      PAL_ORANGE  = 0x4a,
      PAL_YELLOW  = 0x2e,
      PAL_GREEN   = 0x34,
      PAL_BLUE    = 0xbc,
      PAL_PURPLE  = 0xa6,

      BK_GREY      = 0x0a,
      HBLANK_WHITE = 0x0e
    };

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
      Reset device to its power-on state.
    */
    void reset() override;

    /**
      Reset frame to current YStart/Height properties.
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
      Get the byte at the specified address.

      @return The byte at the specified address
    */
    uInt8 peek(uInt16 address) override;

    /**
      Change the byte at the specified address to the given value.

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
      Returns a pointer to the internal frame buffer.
    */
    uInt8* frameBuffer() const { return (uInt8*)(myFramebuffer); }

    /**
      Answers dimensional info about the framebuffer.
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
    bool enableColorLoss(bool enabled);

    /**
      Answers whether colour-loss is applicable for the current frame.

      @return  Colour-loss is active for this frame
    */
    bool colorLossActive() const { return myColorLossActive; }

    /**
      Answers the current color clock we've gotten to on this scanline.

      @return The current color clock
    */
    uInt32 clocksThisLine() const { return myHctr - myHctrDelta; }

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
      Enables/disable/toggle/query 'fixed debug colors' mode.

      @param enable  Whether to enable fixed debug colors mode

      @return  Whether the mode was enabled or disabled
    */
    bool enableFixedColors(bool enable);
    bool toggleFixedColors() { return enableFixedColors(!usingFixedColors()); }
    bool usingFixedColors() const { return myColorHBlank != 0x00; }

    /**
      Sets the color of each object in 'fixed debug colors' mode.
      Note that this doesn't enable/disable fixed colors; it simply
      updates the palette that is used.

      @param colors  Each character in the 6-char string represents the
                     first letter of the color to use for
                     P0/M0/P1/M1/PF/BL, respectively.

      @return  True if colors were changed successfully, else false
    */
    bool setFixedColorPalette(const string& colors);

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
    void setJitterRecoveryFactor(Int32 factor) { myFrameManager.setJitterFactor(factor); }

    /**
      This method should be called to update the TIA with a new scanline.
    */
    TIA& updateScanline();

    /**
      This method should be called to update the TIA with a new partial
      scanline by stepping one CPU instruction.
    */
    TIA& updateScanlineByStep();

    /**
      This method should be called to update the TIA with a new partial
      scanline by tracing to target address.
    */
    TIA& updateScanlineByTrace(int target);

    /**
      Retrieve the last value written to a certain register.
    */
    uInt8 registerValue(uInt8 reg) const;

    /**
      Get the current x value.
    */
    uInt8 getPosition() const {
      uInt8 realHctr = myHctr - myHctrDelta;

      return (realHctr < 68) ? 0 : (realHctr - 68);
    }

    /**
      Flush the line cache after an externally triggered state change
      (e.g. a register write).
    */
    void flushLineCache();

    /**
      Create a new delayQueueIterator for the debugger.
    */
    shared_ptr<DelayQueueIterator> delayQueueIterator() const;

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

    enum FixedObject { P0, M0, P1, M1, PF, BL };
    FixedColor myFixedColorPalette[2][6];

  private:

    void onFrameStart();

    void onRenderingStart();

    void onFrameComplete();

    void onHalt();

    void updateEmulation();

    void cycle(uInt32 colorClocks);

    void tickMovement();

    void tickHblank();

    void tickHframe();

    void applyRsync();

    void updateCollision();

    void renderPixel(uInt32 x, uInt32 y);

    void clearHmoveComb();

    void nextLine();

    void cloneLastLine();

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

    static constexpr unsigned delayQueueLength = 16;
    static constexpr unsigned delayQueueSize = 16;
    DelayQueue<delayQueueLength, delayQueueSize> myDelayQueue;

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

    // Pointer to the internal color-index-based frame buffer
    uInt8 myFramebuffer[160 * FrameManager::frameBufferHeight];

    bool myTIAPinsDriven;

    HState myHstate;

    // Master line counter
    uInt8 myHctr;
    // Delta between master line counter and actual color clock. Nonzero after RSYNC (before the scanline terminates)
    Int32 myHctrDelta;
    // Electron beam x at rendering start (used for blanking out any pixels from the last frame that are not overwritten)
    uInt8 myXAtRenderingStart;

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

    // Indicates if color loss should be enabled or disabled.  Color loss
    // occurs on PAL-like systems when the previous frame contains an odd
    // number of scanlines.
    bool myColorLossEnabled;
    bool myColorLossActive;

  private:
    TIA() = delete;
    TIA(const TIA&) = delete;
    TIA(TIA&&) = delete;
    TIA& operator=(const TIA&) = delete;
    TIA& operator=(TIA&&) = delete;
};

#endif // TIA_TIA
