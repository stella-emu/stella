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
// Copyright (c) 1995-2009 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: TIA.hxx,v 1.50 2009-01-13 01:18:25 stephena Exp $
//============================================================================

#ifndef TIA_HXX
#define TIA_HXX

class Console;
class System;
class Settings;

#include "bspf.hxx"
#include "Sound.hxx"
#include "Device.hxx"
#include "MediaSrc.hxx"

/**
  This class is a device that emulates the Television Interface Adapator 
  found in the Atari 2600 and 7800 consoles.  The Television Interface 
  Adapator is an integrated circuit designed to interface between an 
  eight bit microprocessor and a television video modulator. It converts 
  eight bit parallel data into serial outputs for the color, luminosity, 
  and composite sync required by a video modulator.  

  This class outputs the serial data into a frame buffer which can then
  be displayed on screen.

  @author  Bradford W. Mott
  @version $Id: TIA.hxx,v 1.50 2009-01-13 01:18:25 stephena Exp $
*/
class TIA : public Device , public MediaSource
{
  public:
    friend class TIADebug;

    /**
      Create a new TIA for the specified console

      @param console  The console the TIA is associated with
      @param settings The settings object for this TIA device
    */
    TIA(Console& console, Settings& settings);
 
    /**
      Destructor
    */
    virtual ~TIA();

  public:
    /**
      Reset device to its power-on state
    */
    virtual void reset();

    /**
      Reset frame to change XStart/YStart/Width/Height properties
    */
    virtual void frameReset();

    /**
      Notification method invoked by the system right before the
      system resets its cycle counter to zero.  It may be necessary
      to override this method for devices that remember cycle counts.
    */
    virtual void systemCyclesReset();

    /**
      Install TIA in the specified system.  Invoked by the system
      when the TIA is attached to it.

      @param system The system the device should install itself in
    */
    virtual void install(System& system);

    /**
      Install TIA in the specified system and device.  Invoked by
      the system when the TIA is attached to it.  All devices
      which invoke this method take responsibility for chaining
      requests back to *this* device.

      @param system The system the device should install itself in
      @param device The device responsible for this address space
    */
    virtual void install(System& system, Device& device);

    /**
      Save the current state of this device to the given Serializer.

      @param out  The Serializer object to use
      @return  False on any errors, else true
    */
    virtual bool save(Serializer& out) const;

    /**
      Load the current state of this device from the given Deserializer.

      @param in  The Deserializer object to use
      @return  False on any errors, else true
    */
    virtual bool load(Deserializer& in);

    /**
      Get a descriptor for the device name (used in error checking).

      @return The name of the object
    */
    virtual string name() const { return "TIA"; }

  public:
    /**
      Get the byte at the specified address

      @return The byte at the specified address
    */
    virtual uInt8 peek(uInt16 address);

    /**
      Change the byte at the specified address to the given value

      @param address The address where the value should be stored
      @param value The value to be stored at the address
    */
    virtual void poke(uInt16 address, uInt8 value);

  public:
    /**
      This method should be called at an interval corresponding to
      the desired frame rate to update the media source.
    */
    virtual void update();

    /**
      Answers the current frame buffer

      @return Pointer to the current frame buffer
    */
    uInt8* currentFrameBuffer() const { return myCurrentFrameBuffer; }

    /**
      Answers the previous frame buffer

      @return Pointer to the previous frame buffer
    */
    uInt8* previousFrameBuffer() const { return myPreviousFrameBuffer; }

    /**
      Answers the height of the frame buffer

      @return The frame's height
    */
    uInt32 height() const;

    /**
      Answers the width of the frame buffer

      @return The frame's width
    */
    uInt32 width() const;

    /**
      Enables/disables auto-frame calculation.  If enabled, the TIA
      re-adjusts the framerate at regular intervals.

      @param mode  Whether to enable or disable all auto-frame calculation
    */
    void enableAutoFrame(bool mode) { myAutoFrameEnabled = mode; }

    /**
      Answers the total number of scanlines the media source generated
      in producing the current frame buffer. For partial frames, this
      will be the current scanline.

      @return The total number of scanlines generated
    */
    uInt32 scanlines() const;

    /**
      Sets the sound device for the TIA.
    */
    void setSound(Sound& sound);

    /**
      Answers the current color clock we've gotten to on this scanline.

      @return The current color clock
    */
    uInt32 clocksThisLine() const;

    enum TIABit {
      P0,   // Descriptor for Player 0 Bit
      P1,   // Descriptor for Player 1 Bit
      M0,   // Descriptor for Missle 0 Bit
      M1,   // Descriptor for Missle 1 Bit
      BL,   // Descriptor for Ball Bit
      PF    // Descriptor for Playfield Bit
    };

    /**
      Enables/disables the specified TIA bit.

      @return  Whether the bit was enabled or disabled
    */
    bool enableBit(TIABit b, bool mode) { myBitEnabled[b] = mode; return mode; }

    /**
      Toggles the specified TIA bit.

      @return  Whether the bit was enabled or disabled
    */
    bool toggleBit(TIABit b) { myBitEnabled[b] = !myBitEnabled[b]; return myBitEnabled[b]; }

    /**
      Enables/disables all TIABit bits.

      @param mode  Whether to enable or disable all bits
    */
    void enableBits(bool mode) { for(uInt8 i = 0; i < 6; ++i) myBitEnabled[i] = mode; }

#ifdef DEBUGGER_SUPPORT
    /**
      This method should be called to update the media source with
      a new scanline.
    */
    virtual void updateScanline();

    /**
      This method should be called to update the media source with
      a new partial scanline by stepping one CPU instruction.
    */
    virtual void updateScanlineByStep();

    /**
      This method should be called to update the media source with
      a new partial scanline by tracing to target address.
    */
    virtual void updateScanlineByTrace(int target);
#endif

  private:
    // Compute the ball mask table
    void computeBallMaskTable();

    // Compute the collision decode table
    void computeCollisionTable();

    // Compute the missle mask table
    void computeMissleMaskTable();

    // Compute the player mask table
    void computePlayerMaskTable();

    // Compute the player position reset when table
    void computePlayerPositionResetWhenTable();

    // Compute the player reflect table
    void computePlayerReflectTable();

    // Compute playfield mask table
    void computePlayfieldMaskTable();

  private:
    // Update the current frame buffer up to one scanline
    void updateFrameScanline(uInt32 clocksToUpdate, uInt32 hpos);

    // Update the current frame buffer to the specified color clock
    void updateFrame(Int32 clock);

    // Waste cycles until the current scanline is finished
    void waitHorizontalSync();

    // Grey out current framebuffer from current scanline to bottom
    void greyOutFrame();

    // Clear both internal TIA buffers to black (palette color 0)
    void clearBuffers();

    // Set up bookkeeping for the next frame
    void startFrame();

    // Update bookkeeping at end of frame
    void endFrame();

    // Convert resistance from ports to dumped value
    uInt8 dumpedInputPort(int resistance);

  private:
    // Console the TIA is associated with
    Console& myConsole;

    // Settings object the TIA is associated with
    Settings& mySettings;

    // Sound object the TIA is associated with
    Sound* mySound;

  private:
    // Indicates if color loss should be enabled or disabled.  Color loss
    // occurs on PAL (and maybe SECAM) systems when the previous frame
    // contains an odd number of scanlines.
    bool myColorLossEnabled;

    // Indicates whether we're done with the current frame. poke() clears this
    // when VSYNC is strobed or the max scanlines/frame limit is hit.
    bool myPartialFrameFlag;

  private:
    // Number of frames displayed by this TIA
    int myFrameCounter;

    // Pointer to the current frame buffer
    uInt8* myCurrentFrameBuffer;

    // Pointer to the previous frame buffer
    uInt8* myPreviousFrameBuffer;

    // Pointer to the next pixel that will be drawn in the current frame buffer
    uInt8* myFramePointer;

    // Indicates where the scanline should start being displayed
    uInt32 myFrameXStart;

    // Indicates the width of the scanline 
    uInt32 myFrameWidth;

    // Indicated what scanline the frame should start being drawn at
    uInt32 myFrameYStart;

    // Indicates the height of the frame in scanlines
    uInt32 myFrameHeight;

    // Indicates offset in color clocks when display should begin
    uInt32 myStartDisplayOffset;

    // Indicates offset in color clocks when display should stop
    uInt32 myStopDisplayOffset;

  private:
    // Indicates color clocks when the current frame began
    Int32 myClockWhenFrameStarted;

    // Indicates color clocks when frame should begin to be drawn
    Int32 myClockStartDisplay;

    // Indicates color clocks when frame should stop being drawn
    Int32 myClockStopDisplay;

    // Indicates color clocks when the frame was last updated
    Int32 myClockAtLastUpdate;

    // Indicates how many color clocks remain until the end of 
    // current scanline.  This value is valid during the 
    // displayed portion of the frame.
    Int32 myClocksToEndOfScanLine;

    // Indicates the total number of scanlines generated by the last frame
    Int32 myScanlineCountForLastFrame;

    // Indicates the current scanline during a partial frame.
    Int32 myCurrentScanline;

    // Indicates the maximum number of scanlines to be generated for a frame
    Int32 myMaximumNumberOfScanlines;

    // Color clock when VSYNC ending causes a new frame to be started
    Int32 myVSYNCFinishClock; 

  private:
    uInt8 myVSYNC;        // Holds the VSYNC register value
    uInt8 myVBLANK;       // Holds the VBLANK register value

    uInt8 myNUSIZ0;       // Number and size of player 0 and missle 0
    uInt8 myNUSIZ1;       // Number and size of player 1 and missle 1

    uInt8 myPlayfieldPriorityAndScore;
    uInt32 myColor[4];
    uInt8 myPriorityEncoder[2][256];

    uInt32& myCOLUBK;       // Background color register (replicated 4 times)
    uInt32& myCOLUPF;       // Playfield color register (replicated 4 times)
    uInt32& myCOLUP0;       // Player 0 color register (replicated 4 times)
    uInt32& myCOLUP1;       // Player 1 color register (replicated 4 times)

    uInt8 myCTRLPF;       // Playfield control register

    bool myREFP0;         // Indicates if player 0 is being reflected
    bool myREFP1;         // Indicates if player 1 is being reflected

    uInt32 myPF;          // Playfield graphics (19-12:PF2 11-4:PF1 3-0:PF0)

    uInt8 myGRP0;         // Player 0 graphics register
    uInt8 myGRP1;         // Player 1 graphics register
    
    uInt8 myDGRP0;        // Player 0 delayed graphics register
    uInt8 myDGRP1;        // Player 1 delayed graphics register

    bool myENAM0;         // Indicates if missle 0 is enabled
    bool myENAM1;         // Indicates if missle 0 is enabled

    bool myENABL;         // Indicates if the ball is enabled
    bool myDENABL;        // Indicates if the virtically delayed ball is enabled

    Int8 myHMP0;          // Player 0 horizontal motion register
    Int8 myHMP1;          // Player 1 horizontal motion register
    Int8 myHMM0;          // Missle 0 horizontal motion register
    Int8 myHMM1;          // Missle 1 horizontal motion register
    Int8 myHMBL;          // Ball horizontal motion register

    bool myVDELP0;        // Indicates if player 0 is being virtically delayed
    bool myVDELP1;        // Indicates if player 1 is being virtically delayed
    bool myVDELBL;        // Indicates if the ball is being virtically delayed

    bool myRESMP0;        // Indicates if missle 0 is reset to player 0
    bool myRESMP1;        // Indicates if missle 1 is reset to player 1

    uInt16 myCollision;    // Collision register

    // Note that these position registers contain the color clock 
    // on which the object's serial output should begin (0 to 159)
    Int16 myPOSP0;         // Player 0 position register
    Int16 myPOSP1;         // Player 1 position register
    Int16 myPOSM0;         // Missle 0 position register
    Int16 myPOSM1;         // Missle 1 position register
    Int16 myPOSBL;         // Ball position register

  private:
    // Graphics for Player 0 that should be displayed.  This will be
    // reflected if the player is being reflected.
    uInt8 myCurrentGRP0;

    // Graphics for Player 1 that should be displayed.  This will be
    // reflected if the player is being reflected.
    uInt8 myCurrentGRP1;

    // It's VERY important that the BL, M0, M1, P0 and P1 current
    // mask pointers are always on a uInt32 boundary.  Otherwise,
    // the TIA code will fail on a good number of CPUs.

    // Pointer to the currently active mask array for the ball
    uInt8* myCurrentBLMask;

    // Pointer to the currently active mask array for missle 0
    uInt8* myCurrentM0Mask;

    // Pointer to the currently active mask array for missle 1
    uInt8* myCurrentM1Mask;

    // Pointer to the currently active mask array for player 0
    uInt8* myCurrentP0Mask;

    // Pointer to the currently active mask array for player 1
    uInt8* myCurrentP1Mask;

    // Pointer to the currently active mask array for the playfield
    uInt32* myCurrentPFMask;

    // Audio values. Only used by TIADebug.
    uInt8 myAUDV0;
    uInt8 myAUDV1;
    uInt8 myAUDC0;
    uInt8 myAUDC1;
    uInt8 myAUDF0;
    uInt8 myAUDF1;

  private:
    // Indicates when the dump for paddles was last set
    Int32 myDumpDisabledCycle;

    // Indicates if the dump is current enabled for the paddles
    bool myDumpEnabled;

  private:
    // Color clock when last HMOVE occured
    Int32 myLastHMOVEClock;

    // Indicates if HMOVE blanks are currently enabled
    bool myHMOVEBlankEnabled;

    // Indicates if we're allowing HMOVE blanks to be enabled
    bool myAllowHMOVEBlanks;

    // Indicates if unused TIA pins are floating on a peek
    bool myFloatTIAOutputPins;
    
    // TIA M0 "bug" used for stars in Cosmic Ark flag
    bool myM0CosmicArkMotionEnabled;

    // Counter used for TIA M0 "bug" 
    uInt32 myM0CosmicArkCounter;

    // Bitmap of the objects that should be considered while drawing
    uInt8 myEnabledObjects;

    // Answers whether specified bits (from TIABit) are enabled or disabled
    bool myBitEnabled[6];

    // Has current frame been "greyed out" (has updateScanline() been run?)
    bool myFrameGreyed;

    // Automatic framerate correction based on number of scanlines
    bool myAutoFrameEnabled;

    // The framerate currently in use by the Console
    float myFramerate;

  private:
    enum {  // TODO - convert these to match TIA.cs
      myP0Bit     = 0x01,    // Bit for Player 0
      myM0Bit     = 0x02,    // Bit for Missle 0
      myP1Bit     = 0x04,    // Bit for Player 1
      myM1Bit     = 0x08,    // Bit for Missle 1
      myBLBit     = 0x10,    // Bit for Ball
      myPFBit     = 0x20,    // Bit for Playfield
      ScoreBit    = 0x40,    // Bit for Playfield score mode
      PriorityBit = 0x80     // Bit for Playfield priority
    };

    // TIA Write/Read register names
    enum {
      VSYNC   = 0x00,  // Write: vertical sync set-clear (D1)
      VBLANK  = 0x01,  // Write: vertical blank set-clear (D7-6,D1)
      WSYNC   = 0x02,  // Write: wait for leading edge of hrz. blank (strobe)
      RSYNC   = 0x03,  // Write: reset hrz. sync counter (strobe)
      NUSIZ0  = 0x04,  // Write: number-size player-missle 0 (D5-0)
      NUSIZ1  = 0x05,  // Write: number-size player-missle 1 (D5-0)
      COLUP0  = 0x06,  // Write: color-lum player 0 (D7-1)
      COLUP1  = 0x07,  // Write: color-lum player 1 (D7-1)
      COLUPF  = 0x08,  // Write: color-lum playfield (D7-1)
      COLUBK  = 0x09,  // Write: color-lum background (D7-1)
      CTRLPF  = 0x0a,  // Write: cntrl playfield ballsize & coll. (D5-4,D2-0)
      REFP0   = 0x0b,  // Write: reflect player 0 (D3)
      REFP1   = 0x0c,  // Write: reflect player 1 (D3)
      PF0     = 0x0d,  // Write: playfield register byte 0 (D7-4)
      PF1     = 0x0e,  // Write: playfield register byte 1 (D7-0)
      PF2     = 0x0f,  // Write: playfield register byte 2 (D7-0)
      RESP0   = 0x10,  // Write: reset player 0 (strobe)
      RESP1   = 0x11,  // Write: reset player 1 (strobe)
      RESM0   = 0x12,  // Write: reset missle 0 (strobe)
      RESM1   = 0x13,  // Write: reset missle 1 (strobe)
      RESBL   = 0x14,  // Write: reset ball (strobe)
      AUDC0   = 0x15,  // Write: audio control 0 (D3-0)
      AUDC1   = 0x16,  // Write: audio control 1 (D4-0)
      AUDF0   = 0x17,  // Write: audio frequency 0 (D4-0)
      AUDF1   = 0x18,  // Write: audio frequency 1 (D3-0)
      AUDV0   = 0x19,  // Write: audio volume 0 (D3-0)
      AUDV1   = 0x1a,  // Write: audio volume 1 (D3-0)
      GRP0    = 0x1b,  // Write: graphics player 0 (D7-0)
      GRP1    = 0x1c,  // Write: graphics player 1 (D7-0)
      ENAM0   = 0x1d,  // Write: graphics (enable) missle 0 (D1)
      ENAM1   = 0x1e,  // Write: graphics (enable) missle 1 (D1)
      ENABL   = 0x1f,  // Write: graphics (enable) ball (D1)
      HMP0    = 0x20,  // Write: horizontal motion player 0 (D7-4)
      HMP1    = 0x21,  // Write: horizontal motion player 1 (D7-4)
      HMM0    = 0x22,  // Write: horizontal motion missle 0 (D7-4)
      HMM1    = 0x23,  // Write: horizontal motion missle 1 (D7-4)
      HMBL    = 0x24,  // Write: horizontal motion ball (D7-4)
      VDELP0  = 0x25,  // Write: vertical delay player 0 (D0)
      VDELP1  = 0x26,  // Write: vertical delay player 1 (D0)
      VDELBL  = 0x27,  // Write: vertical delay ball (D0)
      RESMP0  = 0x28,  // Write: reset missle 0 to player 0 (D1)
      RESMP1  = 0x29,  // Write: reset missle 1 to player 1 (D1)
      HMOVE   = 0x2a,  // Write: apply horizontal motion (strobe)
      HMCLR   = 0x2b,  // Write: clear horizontal motion registers (strobe)
      CXCLR   = 0x2c,  // Write: clear collision latches (strobe)

      CXM0P   = 0x00,  // Read collision: D7=(M0,P1); D6=(M0,P0)
      CXM1P   = 0x01,  // Read collision: D7=(M1,P0); D6=(M1,P1)
      CXP0FB  = 0x02,  // Read collision: D7=(P0,PF); D6=(P0,BL)
      CXP1FB  = 0x03,  // Read collision: D7=(P1,PF); D6=(P1,BL)
      CXM0FB  = 0x04,  // Read collision: D7=(M0,PF); D6=(M0,BL)
      CXM1FB  = 0x05,  // Read collision: D7=(M1,PF); D6=(M1,BL)
      CXBLPF  = 0x06,  // Read collision: D7=(BL,PF); D6=(unused)
      CXPPMM  = 0x07,  // Read collision: D7=(P0,P1); D6=(M0,M1)
      INPT0   = 0x08,  // Read pot port: D7
      INPT1   = 0x09,  // Read pot port: D7
      INPT2   = 0x0a,  // Read pot port: D7
      INPT3   = 0x0b,  // Read pot port: D7
      INPT4   = 0x0c,  // Read P1 joystick trigger: D7
      INPT5   = 0x0d   // Read P2 joystick trigger: D7
    };

    // Ball mask table (entries are true or false)
    static uInt8 ourBallMaskTable[4][4][320];

    // Used to set the collision register to the correct value
    static uInt16 ourCollisionTable[64];

    // A mask table which can be used when an object is disabled
    static uInt8 ourDisabledMaskTable[640];

    // Indicates the update delay associated with poking at a TIA address
    static const Int16 ourPokeDelayTable[64];

    // Missle mask table (entries are true or false)
    static uInt8 ourMissleMaskTable[4][8][4][320];

    // Used to convert value written in a motion register into 
    // its internal representation
    static const Int32 ourCompleteMotionTable[76][16];

    // Indicates if HMOVE blanks should occur for the corresponding cycle
    static const bool ourHMOVEBlankEnableCycles[76];

    // Player mask table
    static uInt8 ourPlayerMaskTable[4][2][8][320];

    // Indicates if player is being reset during delay, display or other times
    static Int8 ourPlayerPositionResetWhenTable[8][160][160];

    // Used to reflect a players graphics
    static uInt8 ourPlayerReflectTable[256];

    // Playfield mask table for reflected and non-reflected playfields
    static uInt32 ourPlayfieldTable[2][160];

  private:
    // Copy constructor isn't supported by this class so make it private
    TIA(const TIA&);

    // Assignment operator isn't supported by this class so make it private
    TIA& operator = (const TIA&);
};

#endif
