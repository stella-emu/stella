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
// Copyright (c) 1995-2002 by Bradford W. Mott
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: TIA.hxx,v 1.7 2002-10-09 04:38:12 bwmott Exp $
//============================================================================

#ifndef TIA_HXX
#define TIA_HXX

class Console;
class System;
class Serializer;
class Deserializer;

#include <string>

#include "bspf.hxx"
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
  be displayed on screen it also creates audio samples and places them
  in a bounded queue.

  @author  Bradford W. Mott
  @version $Id: TIA.hxx,v 1.7 2002-10-09 04:38:12 bwmott Exp $
*/
class TIA : public Device , public MediaSource
{
  public:
    /**
      Create a new TIA for the specified console

      @param console The console the TIA is associated with
      @param sampleRate The sample rate to create audio samples at
    */
    TIA(const Console& console, uInt32 sampleRate);
 
    /**
      Destructor
    */
    virtual ~TIA();

  public:
    /**
      Get a null terminated string which is the device's name (i.e. "M6532")

      @return The name of the device
    */
    virtual const char* name() const;

    /**
      Reset device to its power-on state
    */
    virtual void reset();

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
      Saves the current state of this device to the given Serializer.

      @param out The serializer device to save to.
      @return The result of the save.  True on success, false on failure.
    */
    virtual bool save(Serializer& out);

    /**
      Loads the current state of this device from the given Deserializer.

      @param in The deserializer device to load from.
      @return The result of the load.  True on success, false on failure.
    */
    virtual bool load(Deserializer& in);

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
      This method should be called to cause further calls to 'update'
      to be ignored until an unpause is given.  Will also send a mute to
      the Sound device.

      @return Status of the pause, success (true) or failure (false)
    */
    virtual bool pause(bool state);

    /**
      Inserts the given message into the framebuffer for the given
      number of frames.
    */
    virtual void showMessage(string& message, Int32 duration);

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
      Get the palette which maps frame data to RGB values.

      @return Array of integers which represent the palette (RGB)
    */
    virtual const uInt32* palette() const;

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
      Answers the total number of scanlines the media source generated
      in producing the current frame buffer.

      @return The total number of scanlines generated
    */
    uInt32 scanlines() const;

    /**
      Dequeues all of the samples in the audio sample queue.
    */
    void clearAudioSamples();

    /**
      Dequeues up to the specified number of samples from the audio sample
      queue into the buffer.  If the requested number of samples are not
      available then all of samples are dequeued.  The method returns the
      actual number of samples removed from the queue.

      @return The actual number of samples which were dequeued.
    */
    uInt32 dequeueAudioSamples(uInt8* buffer, int size);

    /**
      Answers the number of samples currently available in the audio
      sample queue.

      @return The number of samples in the audio sample queue.
    */
    uInt32 numberOfAudioSamples() const;

    /**
      Returns the type of audio samples which are being stored in the audio
      sample queue.  Currently, only unsigned 8-bit audio samples are created,
      however, in the future this will be extended to support stereo samples.

      @return The type of audio sample stored in the sample queue.
    */
    MediaSource::AudioSampleType typeOfAudioSamples() const;

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

    // Draw message to framebuffer
    void drawMessageText();

  private:
    /**
      A bounded queue class used to hold audio samples after they are
      produced by the TIA sound emulation.
    */
    class SampleQueue
    {
      public:
        /**
          Create a new SampleQueue instance which can hold the specified
          number of samples.  If the queue ever reaches its capacity then
          older samples are discarded.
        */
        SampleQueue(uInt32 capacity);

        /**
          Destroy this SampleQueue instance.
        */
        virtual ~SampleQueue();

      public:
        /**
          Clear any samples stored in the queue.
        */
        void clear();

        /**
          Dequeue the upto the specified number of samples and store them
          in the buffer.  Returns the actual number of samples removed from
          the queue.

          @return the actual number of samples removed from the queue.
        */
        uInt32 dequeue(uInt8* buffer, uInt32 size);

        /**
          Enqueue the specified number of samples from the buffer.
        */
        void enqueue(uInt8* buffer, uInt32 size);

        /**
          Answers the number of samples currently in the queue.

          @return The number of samples in the queue.
        */
        uInt32 size() const;

      private:
        const uInt32 myCapacity;
        uInt8* myBuffer;
        uInt32 mySize;
        uInt32 myHead;
        uInt32 myTail;
    };

    // Invoked to create and store audio samples into the sample queue
    // whenever a TIA audio register is changed or a frame is finished
    void createAudioSamples(uInt16 addr, uInt8 value);
    
  private:
    // Console the TIA is associated with
    const Console& myConsole;

    // Indicates whether the emulation is paused or not
    bool myPauseState;

    // Message timer
    Int32 myMessageTime;

    // Message text
    string myMessageText;

  private:
    // Indicates the CPU cycle when a TIA sound register was last updated
    Int32 myLastSoundUpdateCycle;

    // Indicates if color loss should be enabled or disabled.  Color loss
    // occurs on PAL (and maybe SECAM) systems when the previous frame
    // contains an odd number of scanlines.
    bool myColorLossEnabled;

  private:
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

  private:
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

  private:
    // Color clock when VSYNC ending causes a new frame to be started
    Int32 myVSYNCFinishClock; 

  private:
    enum
    {
      myP0Bit = 0x01,         // Bit for Player 0
      myM0Bit = 0x02,         // Bit for Missle 0
      myP1Bit = 0x04,         // Bit for Player 1
      myM1Bit = 0x08,         // Bit for Missle 1
      myBLBit = 0x10,         // Bit for Ball
      myPFBit = 0x20,         // Bit for Playfield
      ScoreBit = 0x40,        // Bit for Playfield score mode
      PriorityBit = 0x080     // Bit for Playfield priority
    };

    // Bitmap of the objects that should be considered while drawing
    uInt8 myEnabledObjects;

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

    uInt32 myPF;           // Playfield graphics (19-12:PF2 11-4:PF1 3-0:PF0)

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

    // TIA M0 "bug" used for stars in Cosmic Ark flag
    bool myM0CosmicArkMotionEnabled;

    // Counter used for TIA M0 "bug" 
    uInt32 myM0CosmicArkCounter;

  private:
    // Sample queue instance for storing audio samples
    SampleQueue mySampleQueue;

    // Sample rate requested for the audio samples
    uInt32 mySampleRate;

  private:
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

    // Table of RGB values for NTSC
    static const uInt32 ourNTSCPalette[256];

    // Table of RGB values for PAL.  NOTE: The odd numbered entries in
    // this array are always shades of grey.  This is used to implement
    // the PAL color loss effect.
    static const uInt32 ourPALPalette[256];

    // Table of bitmapped fonts.  Holds A..Z and 0..9.
    static const uInt32 ourFontData[36];

  private:
    // Copy constructor isn't supported by this class so make it private
    TIA(const TIA&);

    // Assignment operator isn't supported by this class so make it private
    TIA& operator = (const TIA&);
};

#endif

