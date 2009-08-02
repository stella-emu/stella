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
// $Id$
//============================================================================

#include <cassert>
#include <cstdlib>
#include <cstring>

#include "bspf.hxx"

#include "Console.hxx"
#include "Control.hxx"
#include "Deserializer.hxx"
#include "M6502.hxx"
#include "Serializer.hxx"
#include "Settings.hxx"
#include "Sound.hxx"
#include "System.hxx"
#include "TIATables.hxx"

#include "TIA.hxx"

#define HBLANK 68
#define USE_MMR_LATCHES
static int P0suppress = 0;
static int P1suppress = 0;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TIA::TIA(Console& console, Sound& sound, Settings& settings)
  : myConsole(console),
    mySound(sound),
    mySettings(settings),
    myMaximumNumberOfScanlines(262),
    myCOLUBK(myColor[0]),
    myCOLUPF(myColor[1]),
    myCOLUP0(myColor[2]),
    myCOLUP1(myColor[3]),
    myColorLossEnabled(false),
    myPartialFrameFlag(false),
    myFrameGreyed(false),
    myAutoFrameEnabled(false),
    myFrameCounter(0)
{
  // Allocate buffers for two frame buffers
  myCurrentFrameBuffer = new uInt8[160 * 320];
  myPreviousFrameBuffer = new uInt8[160 * 320];

  // Make sure all TIA bits are enabled
  enableBits(true);

  for(uInt16 x = 0; x < 2; ++x)
  {
    for(uInt16 enabled = 0; enabled < 256; ++enabled)
    {
      if(enabled & PriorityBit)
      {
        uInt8 color = 0;

        if((enabled & (P1Bit | M1Bit)) != 0)
          color = 3;
        if((enabled & (P0Bit | M0Bit)) != 0)
          color = 2;
        if((enabled & BLBit) != 0)
          color = 1;
        if((enabled & PFBit) != 0)
          color = 1;  // NOTE: Playfield has priority so ScoreBit isn't used

        myPriorityEncoder[x][enabled] = color;
      }
      else
      {
        uInt8 color = 0;

        if((enabled & BLBit) != 0)
          color = 1;
        if((enabled & PFBit) != 0)
          color = (enabled & ScoreBit) ? ((x == 0) ? 2 : 3) : 1;
        if((enabled & (P1Bit | M1Bit)) != 0)
          color = (color != 2) ? 3 : 2;
        if((enabled & (P0Bit | M0Bit)) != 0)
          color = 2;

        myPriorityEncoder[x][enabled] = color;
      }
    }
  }

  // Compute all of the mask tables
  TIATables::computeAllTables();

  // Zero audio registers
  myAUDV0 = myAUDV1 = myAUDF0 = myAUDF1 = myAUDC0 = myAUDC1 = 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TIA::~TIA()
{
  delete[] myCurrentFrameBuffer;
  delete[] myPreviousFrameBuffer;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIA::reset()
{
  // Reset the sound device
  mySound.reset();

  // Currently no objects are enabled
  myEnabledObjects = 0;

  // Some default values for the registers
  myVSYNC = myVBLANK = 0;
  myNUSIZ0 = myNUSIZ1 = 0;
  myCOLUP0 = 0;
  myCOLUP1 = 0;
  myCOLUPF = 0;
  myPlayfieldPriorityAndScore = 0;
  myCOLUBK = 0;
  myCTRLPF = 0;
  myREFP0 = myREFP1 = false;
  myPF = 0;
  myGRP0 = myGRP1 = myDGRP0 = myDGRP1 = 0;
  myENAM0 = myENAM1 = myENABL = myDENABL = false;
  myHMP0 = myHMP1 = myHMM0 = myHMM1 = myHMBL = 0;
  myVDELP0 = myVDELP1 = myVDELBL = myRESMP0 = myRESMP1 = false;
  myCollision = 0;
  myPOSP0 = myPOSP1 = myPOSM0 = myPOSM1 = myPOSBL = 0;

  // Some default values for the "current" variables
  myCurrentGRP0 = 0;
  myCurrentGRP1 = 0;
  myCurrentBLMask = TIATables::BLMask[0][0];
  myCurrentM0Mask = TIATables::MxMask[0][0][0];
  myCurrentM1Mask = TIATables::MxMask[0][0][0];
  myCurrentP0Mask = TIATables::PxMask[0][0][0];
  myCurrentP1Mask = TIATables::PxMask[0][0][0];
  myCurrentPFMask = TIATables::PFMask[0];

  myMotionClockP0 = 0;
  myMotionClockP1 = 0;
  myMotionClockM0 = 0;
  myMotionClockM1 = 0;
  myMotionClockBL = 0;

  myHMP0mmr = myHMP1mmr = myHMM0mmr = myHMM1mmr = myHMBLmmr = false;

  myCurrentHMOVEPos = myPreviousHMOVEPos = 0x7FFFFFFF;
  myHMOVEBlankEnabled = false;

  enableBits(true);

  myDumpEnabled = false;
  myDumpDisabledCycle = 0;

  myFloatTIAOutputPins = mySettings.getBool("tiafloat");

  myFrameCounter = 0;
  myScanlineCountForLastFrame = 0;
  myAutoFrameEnabled = (mySettings.getInt("framerate") <= 0);
  myFramerate = myConsole.getFramerate();

  if(myFramerate > 55.0)  // NTSC
  {
    myColorLossEnabled = false;
    myMaximumNumberOfScanlines = 290;
  }
  else
  {
    myColorLossEnabled = true;
    myMaximumNumberOfScanlines = 342;
  }

  // Recalculate the size of the display
  frameReset();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIA::frameReset()
{
  // Clear frame buffers
  clearBuffers();

  // Reset pixel pointer and drawing flag
  myFramePointer = myCurrentFrameBuffer;

  // Make sure all these are within bounds
  myFrameWidth  = 160;
  myFrameYStart = atoi(myConsole.properties().get(Display_YStart).c_str());
  if(myFrameYStart < 0)       myFrameYStart = 0;
  else if(myFrameYStart > 64) myFrameYStart = 64;
  myFrameHeight = atoi(myConsole.properties().get(Display_Height).c_str());
  if(myFrameHeight < 210)      myFrameHeight = 210;
  else if(myFrameHeight > 256) myFrameHeight = 256;

  // Calculate color clock offsets for starting and stopping frame drawing
  // Note that although we always start drawing at scanline zero, the
  // framebuffer that is exposed outside the class actually starts at 'ystart'
  myFramePointerOffset = myFrameWidth * myFrameYStart;
  myStartDisplayOffset = 0;

  // NTSC screens will process at least 262 scanlines,
  // while PAL will have at least 312
  // In any event, at most 320 lines can be processed
  uInt32 scanlines = myFrameYStart + myFrameHeight;
  if(myMaximumNumberOfScanlines == 290)
    scanlines = BSPF_max(scanlines, 262u);  // NTSC
  else
    scanlines = BSPF_max(scanlines, 312u);  // PAL
  myStopDisplayOffset = 228 * BSPF_min(scanlines, 320u);

  // Reasonable values to start and stop the current frame drawing
  myClockWhenFrameStarted = mySystem->cycles() * 3;
  myClockStartDisplay = myClockWhenFrameStarted + myStartDisplayOffset;
  myClockStopDisplay = myClockWhenFrameStarted + myStopDisplayOffset;
  myClockAtLastUpdate = myClockWhenFrameStarted;
  myClocksToEndOfScanLine = 228;
  myVSYNCFinishClock = 0x7FFFFFFF;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIA::systemCyclesReset()
{
  // Get the current system cycle
  uInt32 cycles = mySystem->cycles();

  // Adjust the sound cycle indicator
  mySound.adjustCycleCounter(-1 * cycles);

  // Adjust the dump cycle
  myDumpDisabledCycle -= cycles;

  // Get the current color clock the system is using
  uInt32 clocks = cycles * 3;

  // Adjust the clocks by this amount since we're reseting the clock to zero
  myClockWhenFrameStarted -= clocks;
  myClockStartDisplay -= clocks;
  myClockStopDisplay -= clocks;
  myClockAtLastUpdate -= clocks;
  myVSYNCFinishClock -= clocks;
}
 
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIA::install(System& system)
{
  install(system, *this);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIA::install(System& system, Device& device)
{
  // Remember which system I'm installed in
  mySystem = &system;

  uInt16 shift = mySystem->pageShift();
  mySystem->resetCycles();

  // All accesses are to the given device
  System::PageAccess access;
  access.directPeekBase = 0;
  access.directPokeBase = 0;
  access.device = &device;

  // We're installing in a 2600 system
  for(uInt32 i = 0; i < 8192; i += (1 << shift))
  {
    if((i & 0x1080) == 0x0000)
    {
      mySystem->setPageAccess(i >> shift, access);
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool TIA::save(Serializer& out) const
{
  string device = name();

  try
  {
    out.putString(device);

    out.putInt(myClockWhenFrameStarted);
    out.putInt(myClockStartDisplay);
    out.putInt(myClockStopDisplay);
    out.putInt(myClockAtLastUpdate);
    out.putInt(myClocksToEndOfScanLine);
    out.putInt(myScanlineCountForLastFrame);
    out.putInt(myVSYNCFinishClock);

    out.putByte((char)myEnabledObjects);

    out.putByte((char)myVSYNC);
    out.putByte((char)myVBLANK);
    out.putByte((char)myNUSIZ0);
    out.putByte((char)myNUSIZ1);

    out.putInt(myCOLUP0);
    out.putInt(myCOLUP1);
    out.putInt(myCOLUPF);
    out.putInt(myCOLUBK);

    out.putByte((char)myCTRLPF);
    out.putByte((char)myPlayfieldPriorityAndScore);
    out.putBool(myREFP0);
    out.putBool(myREFP1);
    out.putInt(myPF);
    out.putByte((char)myGRP0);
    out.putByte((char)myGRP1);
    out.putByte((char)myDGRP0);
    out.putByte((char)myDGRP1);
    out.putBool(myENAM0);
    out.putBool(myENAM1);
    out.putBool(myENABL);
    out.putBool(myDENABL);
    out.putByte((char)myHMP0);
    out.putByte((char)myHMP1);
    out.putByte((char)myHMM0);
    out.putByte((char)myHMM1);
    out.putByte((char)myHMBL);
    out.putBool(myVDELP0);
    out.putBool(myVDELP1);
    out.putBool(myVDELBL);
    out.putBool(myRESMP0);
    out.putBool(myRESMP1);
    out.putInt(myCollision);
    out.putInt(myPOSP0);
    out.putInt(myPOSP1);
    out.putInt(myPOSM0);
    out.putInt(myPOSM1);
    out.putInt(myPOSBL);

    out.putByte((char)myCurrentGRP0);
    out.putByte((char)myCurrentGRP1);

// pointers
//  myCurrentBLMask = TIATables::BLMask[0][0];
//  myCurrentM0Mask = TIATables::MxMask[0][0][0];
//  myCurrentM1Mask = TIATables::MxMask[0][0][0];
//  myCurrentP0Mask = TIATables::PxMask[0][0][0];
//  myCurrentP1Mask = TIATables::PxMask[0][0][0];
//  myCurrentPFMask = TIATables::PFMask[0];

    out.putBool(myDumpEnabled);
    out.putInt(myDumpDisabledCycle);

    out.putInt(myFrameCounter);
    out.putBool(myPartialFrameFlag);
    out.putBool(myFrameGreyed);

    out.putInt(myMotionClockP0);
    out.putInt(myMotionClockP1);
    out.putInt(myMotionClockM0);
    out.putInt(myMotionClockM1);
    out.putInt(myMotionClockBL);

    out.putBool(myHMP0mmr);
    out.putBool(myHMP1mmr);
    out.putBool(myHMM0mmr);
    out.putBool(myHMM1mmr);
    out.putBool(myHMBLmmr);

    out.putInt(myCurrentHMOVEPos);
    out.putInt(myPreviousHMOVEPos);
    out.putBool(myHMOVEBlankEnabled);

    // Save the sound sample stuff ...
    mySound.save(out);
  }
  catch(char *msg)
  {
    cerr << msg << endl;
    return false;
  }
  catch(...)
  {
    cerr << "Unknown error in save state for " << device << endl;
    return false;
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool TIA::load(Deserializer& in)
{
  string device = name();

  try
  {
    if(in.getString() != device)
      return false;

    myClockWhenFrameStarted = (Int32) in.getInt();
    myClockStartDisplay = (Int32) in.getInt();
    myClockStopDisplay = (Int32) in.getInt();
    myClockAtLastUpdate = (Int32) in.getInt();
    myClocksToEndOfScanLine = (Int32) in.getInt();
    myScanlineCountForLastFrame = (uInt32) in.getInt();
    myVSYNCFinishClock = (Int32) in.getInt();

    myEnabledObjects = (uInt8) in.getByte();

    myVSYNC = (uInt8) in.getByte();
    myVBLANK = (uInt8) in.getByte();
    myNUSIZ0 = (uInt8) in.getByte();
    myNUSIZ1 = (uInt8) in.getByte();

    myCOLUP0 = (uInt32) in.getInt();
    myCOLUP1 = (uInt32) in.getInt();
    myCOLUPF = (uInt32) in.getInt();
    myCOLUBK = (uInt32) in.getInt();

    myCTRLPF = (uInt8) in.getByte();
    myPlayfieldPriorityAndScore = (uInt8) in.getByte();
    myREFP0 = in.getBool();
    myREFP1 = in.getBool();
    myPF = (uInt32) in.getInt();
    myGRP0 = (uInt8) in.getByte();
    myGRP1 = (uInt8) in.getByte();
    myDGRP0 = (uInt8) in.getByte();
    myDGRP1 = (uInt8) in.getByte();
    myENAM0 = in.getBool();
    myENAM1 = in.getBool();
    myENABL = in.getBool();
    myDENABL = in.getBool();
    myHMP0 = (uInt8) in.getByte();
    myHMP1 = (uInt8) in.getByte();
    myHMM0 = (uInt8) in.getByte();
    myHMM1 = (uInt8) in.getByte();
    myHMBL = (uInt8) in.getByte();
    myVDELP0 = in.getBool();
    myVDELP1 = in.getBool();
    myVDELBL = in.getBool();
    myRESMP0 = in.getBool();
    myRESMP1 = in.getBool();
    myCollision = (uInt16) in.getInt();
    myPOSP0 = (Int16) in.getInt();
    myPOSP1 = (Int16) in.getInt();
    myPOSM0 = (Int16) in.getInt();
    myPOSM1 = (Int16) in.getInt();
    myPOSBL = (Int16) in.getInt();

    myCurrentGRP0 = (uInt8) in.getByte();
    myCurrentGRP1 = (uInt8) in.getByte();

// pointers
//  myCurrentBLMask = TIATables::BLMask[0][0];
//  myCurrentM0Mask = TIATables::MxMask[0][0][0];
//  myCurrentM1Mask = TIATables::MxMask[0][0][0];
//  myCurrentP0Mask = TIATables::PxMask[0][0][0];
//  myCurrentP1Mask = TIATables::PxMask[0][0][0];
//  myCurrentPFMask = TIATables::PFMask[0];

    myDumpEnabled = in.getBool();
    myDumpDisabledCycle = (Int32) in.getInt();

    myFrameCounter = (Int32) in.getInt();
    myPartialFrameFlag = in.getBool();
    myFrameGreyed = in.getBool();

    myMotionClockP0 = (Int32) in.getInt();
    myMotionClockP1 = (Int32) in.getInt();
    myMotionClockM0 = (Int32) in.getInt();
    myMotionClockM1 = (Int32) in.getInt();
    myMotionClockBL = (Int32) in.getInt();

    myHMP0mmr = in.getBool();
    myHMP1mmr = in.getBool();
    myHMM0mmr = in.getBool();
    myHMM1mmr = in.getBool();
    myHMBLmmr = in.getBool();

    myCurrentHMOVEPos = (Int32) in.getInt();
    myPreviousHMOVEPos = (Int32) in.getInt();
    myHMOVEBlankEnabled = in.getBool();

    // Load the sound sample stuff ...
    mySound.load(in);

    // Reset TIA bits to be on
    enableBits(true);
  }
  catch(char *msg)
  {
    cerr << msg << endl;
    return false;
  }
  catch(...)
  {
    cerr << "Unknown error in load state for " << device << endl;
    return false;
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIA::update()
{
  // if we've finished a frame, start a new one
  if(!myPartialFrameFlag)
    startFrame();

  // Partial frame flag starts out true here. When then 6502 strobes VSYNC,
  // TIA::poke() will set this flag to false, so we'll know whether the
  // frame got finished or interrupted by the debugger hitting a break/trap.
  myPartialFrameFlag = true;

  // Execute instructions until frame is finished, or a breakpoint/trap hits
  mySystem->m6502().execute(25000);

  // TODO: have code here that handles errors....

  if(myPartialFrameFlag)
  {
    // Grey out old frame contents
    if(!myFrameGreyed)
      greyOutFrame();
    myFrameGreyed = true;
  }
  else
    endFrame();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
inline void TIA::startFrame()
{
  // This stuff should only happen at the beginning of a new frame.
  uInt8* tmp = myCurrentFrameBuffer;
  myCurrentFrameBuffer = myPreviousFrameBuffer;
  myPreviousFrameBuffer = tmp;

  // Remember the number of clocks which have passed on the current scanline
  // so that we can adjust the frame's starting clock by this amount.  This
  // is necessary since some games position objects during VSYNC and the
  // TIA's internal counters are not reset by VSYNC.
  uInt32 clocks = ((mySystem->cycles() * 3) - myClockWhenFrameStarted) % 228;

  // Ask the system to reset the cycle count so it doesn't overflow
  mySystem->resetCycles();

  // Setup clocks that'll be used for drawing this frame
  myClockWhenFrameStarted = -1 * clocks;
  myClockStartDisplay = myClockWhenFrameStarted + myStartDisplayOffset;
  myClockStopDisplay = myClockWhenFrameStarted + myStopDisplayOffset;
  myClockAtLastUpdate = myClockStartDisplay;
  myClocksToEndOfScanLine = 228;

  // Reset frame buffer pointer
  myFramePointer = myCurrentFrameBuffer;

  // If color loss is enabled then update the color registers based on
  // the number of scanlines in the last frame that was generated
  if(myColorLossEnabled)
  {
    if(myScanlineCountForLastFrame & 0x01)
    {
      myCOLUP0 |= 0x01010101;
      myCOLUP1 |= 0x01010101;
      myCOLUPF |= 0x01010101;
      myCOLUBK |= 0x01010101;
    }
    else
    {
      myCOLUP0 &= 0xfefefefe;
      myCOLUP1 &= 0xfefefefe;
      myCOLUPF &= 0xfefefefe;
      myCOLUBK &= 0xfefefefe;
    }
  }   

  myFrameGreyed = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
inline void TIA::endFrame()
{
  // This stuff should only happen at the end of a frame
  // Compute the number of scanlines in the frame
  myScanlineCountForLastFrame = scanlines();

  // Stats counters
  myFrameCounter++;

  // Recalculate framerate. attempting to auto-correct for scanline 'jumps'
  if(myFrameCounter % 8 == 0 && myAutoFrameEnabled)
  {
    myFramerate = (myScanlineCountForLastFrame > 285 ? 15600.0 : 15720.0) /
                   myScanlineCountForLastFrame;
    myConsole.setFramerate(myFramerate);

    // Adjust end-of-frame pointer
    // We always accommodate the highest # of scanlines, up to the maximum
    // size of the buffer (currently, 320 lines)
    uInt32 offset = 228 * myScanlineCountForLastFrame;
    if(offset > myStopDisplayOffset && offset <= 228 * 320)
      myStopDisplayOffset = offset;
  }

  myFrameGreyed = false;
}

#ifdef DEBUGGER_SUPPORT
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIA::updateScanline()
{
  // Start a new frame if the old one was finished
  if(!myPartialFrameFlag)
    startFrame();

  // grey out old frame contents
  if(!myFrameGreyed)
    greyOutFrame();
  myFrameGreyed = true;

  // true either way:
  myPartialFrameFlag = true;

  int totalClocks = (mySystem->cycles() * 3) - myClockWhenFrameStarted;
  int endClock = ((totalClocks + 228) / 228) * 228;

  int clock;
  do {
	  mySystem->m6502().execute(1);
	  clock = mySystem->cycles() * 3;
	  updateFrame(clock);
  } while(clock < endClock);

  // if we finished the frame, get ready for the next one
  if(!myPartialFrameFlag)
    endFrame();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIA::updateScanlineByStep()
{
  // Start a new frame if the old one was finished
  if(!myPartialFrameFlag)
    startFrame();

  // grey out old frame contents
  if(!myFrameGreyed) greyOutFrame();
  myFrameGreyed = true;

  // true either way:
  myPartialFrameFlag = true;

  // Update frame by one CPU instruction/color clock
  mySystem->m6502().execute(1);
  updateFrame(mySystem->cycles() * 3);

  // if we finished the frame, get ready for the next one
  if(!myPartialFrameFlag)
    endFrame();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIA::updateScanlineByTrace(int target)
{
  // Start a new frame if the old one was finished
  if(!myPartialFrameFlag)
    startFrame();

  // grey out old frame contents
  if(!myFrameGreyed) greyOutFrame();
  myFrameGreyed = true;

  // true either way:
  myPartialFrameFlag = true;

  while(mySystem->m6502().getPC() != target)
  {
    mySystem->m6502().execute(1);
    updateFrame(mySystem->cycles() * 3);
  }

  // if we finished the frame, get ready for the next one
  if(!myPartialFrameFlag)
    endFrame();
}
#endif

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
inline void TIA::updateFrameScanline(uInt32 clocksToUpdate, uInt32 hpos)
{
  // Calculate the ending frame pointer value
  uInt8* ending = myFramePointer + clocksToUpdate;

  // See if we're in the vertical blank region
  if(myVBLANK & 0x02)
  {
    memset(myFramePointer, 0, clocksToUpdate);
  }
  // Handle all other possible combinations
  else
  {
    // Update masks
    myCurrentBLMask = &TIATables::BLMask[myPOSBL & 0x03]
        [(myCTRLPF & 0x30) >> 4][160 - (myPOSBL & 0xFC)];
    myCurrentP0Mask = &TIATables::PxMask[myPOSP0 & 0x03]
        [P0suppress][myNUSIZ0 & 0x07][160 - (myPOSP0 & 0xFC)];
    myCurrentP1Mask = &TIATables::PxMask[myPOSP1 & 0x03]
        [P1suppress][myNUSIZ1 & 0x07][160 - (myPOSP1 & 0xFC)];
    myCurrentM0Mask = &TIATables::MxMask[myPOSM0 & 0x03]
        [myNUSIZ0 & 0x07][(myNUSIZ0 & 0x30) >> 4][160 - (myPOSM0 & 0xFC)];
    myCurrentM1Mask = &TIATables::MxMask[myPOSM1 & 0x03]
        [myNUSIZ1 & 0x07][(myNUSIZ1 & 0x30) >> 4][160 - (myPOSM1 & 0xFC)];

    switch(myEnabledObjects | myPlayfieldPriorityAndScore)
    {
      // Background 
      case 0x00:
      case 0x00 | ScoreBit:
      case 0x00 | PriorityBit:
      case 0x00 | PriorityBit | ScoreBit:
      {
        memset(myFramePointer, myCOLUBK, clocksToUpdate);
        break;
      }

      // Playfield is enabled and the priority bit is set (score bit is overridden)
      case PFBit: 
      case PFBit | PriorityBit:
      case PFBit | PriorityBit | ScoreBit:
      {
        uInt32* mask = &myCurrentPFMask[hpos];

        // Update a uInt8 at a time until reaching a uInt32 boundary
        for(; ((uintptr_t)myFramePointer & 0x03) && (myFramePointer < ending);
            ++myFramePointer, ++mask)
        {
          *myFramePointer = (myPF & *mask) ? myCOLUPF : myCOLUBK;
        }

        // Now, update a uInt32 at a time
        for(; myFramePointer < ending; myFramePointer += 4, mask += 4)
        {
          *((uInt32*)myFramePointer) = (myPF & *mask) ? myCOLUPF : myCOLUBK;
        }
        break;
      }

      // Playfield is enabled and the score bit is set (without priority bit)
      case PFBit | ScoreBit:
      {
        uInt32* mask = &myCurrentPFMask[hpos];

        // Update a uInt8 at a time until reaching a uInt32 boundary
        for(; ((uintptr_t)myFramePointer & 0x03) && (myFramePointer < ending); 
            ++myFramePointer, ++mask, ++hpos)
        {
          *myFramePointer = (myPF & *mask) ? 
              (hpos < 80 ? myCOLUP0 : myCOLUP1) : myCOLUBK;
        }

        // Now, update a uInt32 at a time
        for(; myFramePointer < ending; 
            myFramePointer += 4, mask += 4, hpos += 4)
        {
          *((uInt32*)myFramePointer) = (myPF & *mask) ?
              (hpos < 80 ? myCOLUP0 : myCOLUP1) : myCOLUBK;
        }
        break;
      }

      // Player 0 is enabled
      case P0Bit:
      case P0Bit | ScoreBit:
      case P0Bit | PriorityBit:
      case P0Bit | ScoreBit | PriorityBit:
      {
        uInt8* mP0 = &myCurrentP0Mask[hpos];

        while(myFramePointer < ending)
        {
          if(!((uintptr_t)myFramePointer & 0x03) && !*(uInt32*)mP0)
          {
            *(uInt32*)myFramePointer = myCOLUBK;
            mP0 += 4; myFramePointer += 4;
          }
          else
          {
            *myFramePointer = (myCurrentGRP0 & *mP0) ? myCOLUP0 : myCOLUBK;
            ++mP0; ++myFramePointer;
          }
        }
        break;
      }

      // Player 1 is enabled
      case P1Bit:
      case P1Bit | ScoreBit:
      case P1Bit | PriorityBit:
      case P1Bit | ScoreBit | PriorityBit:
      {
        uInt8* mP1 = &myCurrentP1Mask[hpos];

        while(myFramePointer < ending)
        {
          if(!((uintptr_t)myFramePointer & 0x03) && !*(uInt32*)mP1)
          {
            *(uInt32*)myFramePointer = myCOLUBK;
            mP1 += 4; myFramePointer += 4;
          }
          else
          {
            *myFramePointer = (myCurrentGRP1 & *mP1) ? myCOLUP1 : myCOLUBK;
            ++mP1; ++myFramePointer;
          }
        }
        break;
      }

      // Player 0 and 1 are enabled
      case P0Bit | P1Bit:
      case P0Bit | P1Bit | ScoreBit:
      case P0Bit | P1Bit | PriorityBit:
      case P0Bit | P1Bit | ScoreBit | PriorityBit:
      {
        uInt8* mP0 = &myCurrentP0Mask[hpos];
        uInt8* mP1 = &myCurrentP1Mask[hpos];

        while(myFramePointer < ending)
        {
          if(!((uintptr_t)myFramePointer & 0x03) && !*(uInt32*)mP0 &&
              !*(uInt32*)mP1)
          {
            *(uInt32*)myFramePointer = myCOLUBK;
            mP0 += 4; mP1 += 4; myFramePointer += 4;
          }
          else
          {
            *myFramePointer = (myCurrentGRP0 & *mP0) ? 
                myCOLUP0 : ((myCurrentGRP1 & *mP1) ? myCOLUP1 : myCOLUBK);

            if((myCurrentGRP0 & *mP0) && (myCurrentGRP1 & *mP1))
              myCollision |= TIATables::CollisionMask[P0Bit | P1Bit];

            ++mP0; ++mP1; ++myFramePointer;
          }
        }
        break;
      }

      // Missle 0 is enabled
      case M0Bit:
      case M0Bit | ScoreBit:
      case M0Bit | PriorityBit:
      case M0Bit | ScoreBit | PriorityBit:
      {
        uInt8* mM0 = &myCurrentM0Mask[hpos];

        while(myFramePointer < ending)
        {
          if(!((uintptr_t)myFramePointer & 0x03) && !*(uInt32*)mM0)
          {
            *(uInt32*)myFramePointer = myCOLUBK;
            mM0 += 4; myFramePointer += 4;
          }
          else
          {
            *myFramePointer = *mM0 ? myCOLUP0 : myCOLUBK;
            ++mM0; ++myFramePointer;
          }
        }
        break;
      }

      // Missle 1 is enabled
      case M1Bit:
      case M1Bit | ScoreBit:
      case M1Bit | PriorityBit:
      case M1Bit | ScoreBit | PriorityBit:
      {
        uInt8* mM1 = &myCurrentM1Mask[hpos];

        while(myFramePointer < ending)
        {
          if(!((uintptr_t)myFramePointer & 0x03) && !*(uInt32*)mM1)
          {
            *(uInt32*)myFramePointer = myCOLUBK;
            mM1 += 4; myFramePointer += 4;
          }
          else
          {
            *myFramePointer = *mM1 ? myCOLUP1 : myCOLUBK;
            ++mM1; ++myFramePointer;
          }
        }
        break;
      }

      // Ball is enabled
      case BLBit:
      case BLBit | ScoreBit:
      case BLBit | PriorityBit:
      case BLBit | ScoreBit | PriorityBit:
      {
        uInt8* mBL = &myCurrentBLMask[hpos];

        while(myFramePointer < ending)
        {
          if(!((uintptr_t)myFramePointer & 0x03) && !*(uInt32*)mBL)
          {
            *(uInt32*)myFramePointer = myCOLUBK;
            mBL += 4; myFramePointer += 4;
          }
          else
          {
            *myFramePointer = *mBL ? myCOLUPF : myCOLUBK;
            ++mBL; ++myFramePointer;
          }
        }
        break;
      }

      // Missle 0 and 1 are enabled
      case M0Bit | M1Bit:
      case M0Bit | M1Bit | ScoreBit:
      case M0Bit | M1Bit | PriorityBit:
      case M0Bit | M1Bit | ScoreBit | PriorityBit:
      {
        uInt8* mM0 = &myCurrentM0Mask[hpos];
        uInt8* mM1 = &myCurrentM1Mask[hpos];

        while(myFramePointer < ending)
        {
          if(!((uintptr_t)myFramePointer & 0x03) && !*(uInt32*)mM0 && !*(uInt32*)mM1)
          {
            *(uInt32*)myFramePointer = myCOLUBK;
            mM0 += 4; mM1 += 4; myFramePointer += 4;
          }
          else
          {
            *myFramePointer = *mM0 ? myCOLUP0 : (*mM1 ? myCOLUP1 : myCOLUBK);

            if(*mM0 && *mM1)
              myCollision |= TIATables::CollisionMask[M0Bit | M1Bit];

            ++mM0; ++mM1; ++myFramePointer;
          }
        }
        break;
      }

      // Ball and Missle 0 are enabled and playfield priority is not set
      case BLBit | M0Bit:
      case BLBit | M0Bit | ScoreBit:
      {
        uInt8* mBL = &myCurrentBLMask[hpos];
        uInt8* mM0 = &myCurrentM0Mask[hpos];

        while(myFramePointer < ending)
        {
          if(!((uintptr_t)myFramePointer & 0x03) && !*(uInt32*)mBL && !*(uInt32*)mM0)
          {
            *(uInt32*)myFramePointer = myCOLUBK;
            mBL += 4; mM0 += 4; myFramePointer += 4;
          }
          else
          {
            *myFramePointer = (*mM0 ? myCOLUP0 : (*mBL ? myCOLUPF : myCOLUBK));

            if(*mBL && *mM0)
              myCollision |= TIATables::CollisionMask[BLBit | M0Bit];

            ++mBL; ++mM0; ++myFramePointer;
          }
        }
        break;
      }

      // Ball and Missle 0 are enabled and playfield priority is set
      case BLBit | M0Bit | PriorityBit:
      case BLBit | M0Bit | ScoreBit | PriorityBit:
      {
        uInt8* mBL = &myCurrentBLMask[hpos];
        uInt8* mM0 = &myCurrentM0Mask[hpos];

        while(myFramePointer < ending)
        {
          if(!((uintptr_t)myFramePointer & 0x03) && !*(uInt32*)mBL && !*(uInt32*)mM0)
          {
            *(uInt32*)myFramePointer = myCOLUBK;
            mBL += 4; mM0 += 4; myFramePointer += 4;
          }
          else
          {
            *myFramePointer = (*mBL ? myCOLUPF : (*mM0 ? myCOLUP0 : myCOLUBK));

            if(*mBL && *mM0)
              myCollision |= TIATables::CollisionMask[BLBit | M0Bit];

            ++mBL; ++mM0; ++myFramePointer;
          }
        }
        break;
      }

      // Ball and Missle 1 are enabled and playfield priority is not set
      case BLBit | M1Bit:
      case BLBit | M1Bit | ScoreBit:
      {
        uInt8* mBL = &myCurrentBLMask[hpos];
        uInt8* mM1 = &myCurrentM1Mask[hpos];

        while(myFramePointer < ending)
        {
          if(!((uintptr_t)myFramePointer & 0x03) && !*(uInt32*)mBL && 
              !*(uInt32*)mM1)
          {
            *(uInt32*)myFramePointer = myCOLUBK;
            mBL += 4; mM1 += 4; myFramePointer += 4;
          }
          else
          {
            *myFramePointer = (*mM1 ? myCOLUP1 : (*mBL ? myCOLUPF : myCOLUBK));

            if(*mBL && *mM1)
              myCollision |= TIATables::CollisionMask[BLBit | M1Bit];

            ++mBL; ++mM1; ++myFramePointer;
          }
        }
        break;
      }

      // Ball and Missle 1 are enabled and playfield priority is set
      case BLBit | M1Bit | PriorityBit:
      case BLBit | M1Bit | ScoreBit | PriorityBit:
      {
        uInt8* mBL = &myCurrentBLMask[hpos];
        uInt8* mM1 = &myCurrentM1Mask[hpos];

        while(myFramePointer < ending)
        {
          if(!((uintptr_t)myFramePointer & 0x03) && !*(uInt32*)mBL && 
              !*(uInt32*)mM1)
          {
            *(uInt32*)myFramePointer = myCOLUBK;
            mBL += 4; mM1 += 4; myFramePointer += 4;
          }
          else
          {
            *myFramePointer = (*mBL ? myCOLUPF : (*mM1 ? myCOLUP1 : myCOLUBK));

            if(*mBL && *mM1)
              myCollision |= TIATables::CollisionMask[BLBit | M1Bit];

            ++mBL; ++mM1; ++myFramePointer;
          }
        }
        break;
      }

      // Ball and Player 1 are enabled and playfield priority is not set
      case BLBit | P1Bit:
      case BLBit | P1Bit | ScoreBit:
      {
        uInt8* mBL = &myCurrentBLMask[hpos];
        uInt8* mP1 = &myCurrentP1Mask[hpos];

        while(myFramePointer < ending)
        {
          if(!((uintptr_t)myFramePointer & 0x03) && !*(uInt32*)mP1 && !*(uInt32*)mBL)
          {
            *(uInt32*)myFramePointer = myCOLUBK;
            mBL += 4; mP1 += 4; myFramePointer += 4;
          }
          else
          {
            *myFramePointer = (myCurrentGRP1 & *mP1) ? myCOLUP1 : 
                (*mBL ? myCOLUPF : myCOLUBK);

            if(*mBL && (myCurrentGRP1 & *mP1))
              myCollision |= TIATables::CollisionMask[BLBit | P1Bit];

            ++mBL; ++mP1; ++myFramePointer;
          }
        }
        break;
      }

      // Ball and Player 1 are enabled and playfield priority is set
      case BLBit | P1Bit | PriorityBit:
      case BLBit | P1Bit | PriorityBit | ScoreBit:
      {
        uInt8* mBL = &myCurrentBLMask[hpos];
        uInt8* mP1 = &myCurrentP1Mask[hpos];

        while(myFramePointer < ending)
        {
          if(!((uintptr_t)myFramePointer & 0x03) && !*(uInt32*)mP1 && !*(uInt32*)mBL)
          {
            *(uInt32*)myFramePointer = myCOLUBK;
            mBL += 4; mP1 += 4; myFramePointer += 4;
          }
          else
          {
            *myFramePointer = *mBL ? myCOLUPF : 
                ((myCurrentGRP1 & *mP1) ? myCOLUP1 : myCOLUBK);

            if(*mBL && (myCurrentGRP1 & *mP1))
              myCollision |= TIATables::CollisionMask[BLBit | P1Bit];

            ++mBL; ++mP1; ++myFramePointer;
          }
        }
        break;
      }

      // Playfield and Player 0 are enabled and playfield priority is not set
      case PFBit | P0Bit:
      {
        uInt32* mPF = &myCurrentPFMask[hpos];
        uInt8* mP0 = &myCurrentP0Mask[hpos];

        while(myFramePointer < ending)
        {
          if(!((uintptr_t)myFramePointer & 0x03) && !*(uInt32*)mP0)
          {
            *(uInt32*)myFramePointer = (myPF & *mPF) ? myCOLUPF : myCOLUBK;
            mPF += 4; mP0 += 4; myFramePointer += 4;
          }
          else
          {
            *myFramePointer = (myCurrentGRP0 & *mP0) ? 
                  myCOLUP0 : ((myPF & *mPF) ? myCOLUPF : myCOLUBK);

            if((myPF & *mPF) && (myCurrentGRP0 & *mP0))
              myCollision |= TIATables::CollisionMask[PFBit | P0Bit];

            ++mPF; ++mP0; ++myFramePointer;
          }
        }
        break;
      }

      // Playfield and Player 0 are enabled and playfield priority is set
      case PFBit | P0Bit | PriorityBit:
      {
        uInt32* mPF = &myCurrentPFMask[hpos];
        uInt8* mP0 = &myCurrentP0Mask[hpos];

        while(myFramePointer < ending)
        {
          if(!((uintptr_t)myFramePointer & 0x03) && !*(uInt32*)mP0)
          {
            *(uInt32*)myFramePointer = (myPF & *mPF) ? myCOLUPF : myCOLUBK;
            mPF += 4; mP0 += 4; myFramePointer += 4;
          }
          else
          {
            *myFramePointer = (myPF & *mPF) ? myCOLUPF : 
                ((myCurrentGRP0 & *mP0) ? myCOLUP0 : myCOLUBK);

            if((myPF & *mPF) && (myCurrentGRP0 & *mP0))
              myCollision |= TIATables::CollisionMask[PFBit | P0Bit];

            ++mPF; ++mP0; ++myFramePointer;
          }
        }
        break;
      }

      // Playfield and Player 1 are enabled and playfield priority is not set
      case PFBit | P1Bit:
      {
        uInt32* mPF = &myCurrentPFMask[hpos];
        uInt8* mP1 = &myCurrentP1Mask[hpos];

        while(myFramePointer < ending)
        {
          if(!((uintptr_t)myFramePointer & 0x03) && !*(uInt32*)mP1)
          {
            *(uInt32*)myFramePointer = (myPF & *mPF) ? myCOLUPF : myCOLUBK;
            mPF += 4; mP1 += 4; myFramePointer += 4;
          }
          else
          {
            *myFramePointer = (myCurrentGRP1 & *mP1) ? 
                  myCOLUP1 : ((myPF & *mPF) ? myCOLUPF : myCOLUBK);

            if((myPF & *mPF) && (myCurrentGRP1 & *mP1))
              myCollision |= TIATables::CollisionMask[PFBit | P1Bit];

            ++mPF; ++mP1; ++myFramePointer;
          }
        }
        break;
      }

      // Playfield and Player 1 are enabled and playfield priority is set
      case PFBit | P1Bit | PriorityBit:
      {
        uInt32* mPF = &myCurrentPFMask[hpos];
        uInt8* mP1 = &myCurrentP1Mask[hpos];

        while(myFramePointer < ending)
        {
          if(!((uintptr_t)myFramePointer & 0x03) && !*(uInt32*)mP1)
          {
            *(uInt32*)myFramePointer = (myPF & *mPF) ? myCOLUPF : myCOLUBK;
            mPF += 4; mP1 += 4; myFramePointer += 4;
          }
          else
          {
            *myFramePointer = (myPF & *mPF) ? myCOLUPF : 
                ((myCurrentGRP1 & *mP1) ? myCOLUP1 : myCOLUBK);

            if((myPF & *mPF) && (myCurrentGRP1 & *mP1))
              myCollision |= TIATables::CollisionMask[PFBit | P1Bit];

            ++mPF; ++mP1; ++myFramePointer;
          }
        }
        break;
      }

      // Playfield and Ball are enabled
      case PFBit | BLBit:
      case PFBit | BLBit | PriorityBit:
      {
        uInt32* mPF = &myCurrentPFMask[hpos];
        uInt8* mBL = &myCurrentBLMask[hpos];

        while(myFramePointer < ending)
        {
          if(!((uintptr_t)myFramePointer & 0x03) && !*(uInt32*)mBL)
          {
            *(uInt32*)myFramePointer = (myPF & *mPF) ? myCOLUPF : myCOLUBK;
            mPF += 4; mBL += 4; myFramePointer += 4;
          }
          else
          {
            *myFramePointer = ((myPF & *mPF) || *mBL) ? myCOLUPF : myCOLUBK;

            if((myPF & *mPF) && *mBL)
              myCollision |= TIATables::CollisionMask[PFBit | BLBit];

            ++mPF; ++mBL; ++myFramePointer;
          }
        }
        break;
      }

      // Handle all of the other cases
      default:
      {
        for(; myFramePointer < ending; ++myFramePointer, ++hpos)
        {
          uInt8 enabled = (myPF & myCurrentPFMask[hpos]) ? PFBit : 0;

          if((myEnabledObjects & BLBit) && myCurrentBLMask[hpos])
            enabled |= BLBit;

          if(myCurrentGRP1 & myCurrentP1Mask[hpos])
            enabled |= P1Bit;

          if((myEnabledObjects & M1Bit) && myCurrentM1Mask[hpos])
            enabled |= M1Bit;

          if(myCurrentGRP0 & myCurrentP0Mask[hpos])
            enabled |= P0Bit;

          if((myEnabledObjects & M0Bit) && myCurrentM0Mask[hpos])
            enabled |= M0Bit;

          myCollision |= TIATables::CollisionMask[enabled];
          *myFramePointer = myColor[myPriorityEncoder[hpos < 80 ? 0 : 1]
              [enabled | myPlayfieldPriorityAndScore]];
        }
        break;  
      }
    }
  }
  myFramePointer = ending;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIA::updateFrame(Int32 clock)
{
  // See if we've already updated this portion of the screen
  if((clock < myClockStartDisplay) ||
     (myClockAtLastUpdate >= myClockStopDisplay) ||
     (myClockAtLastUpdate >= clock))
    return;

  // Truncate the number of cycles to update to the stop display point
  if(clock > myClockStopDisplay)
    clock = myClockStopDisplay;

  // Determine how many scanlines to process
  // It's easier to think about this in scanlines rather than color clocks
  uInt32 startLine = (myClockAtLastUpdate - myClockWhenFrameStarted) / 228;
  uInt32 endLine = (clock - myClockWhenFrameStarted) / 228;

  // Update frame one scanline at a time
  for(uInt32 line = startLine; line <= endLine; ++line)
  {
    // Only check for inter-line changes after the current scanline
    // The ideas for much of the following code was inspired by MESS
    // (used with permission from Wilbert Pol)
    if(line != startLine)
    {
      // We're no longer concerned with previously issued HMOVE's
      myPreviousHMOVEPos = 0x7FFFFFFF;
      bool posChanged = false;

      // Apply pending motion clocks from a HMOVE initiated during the scanline
      if(myCurrentHMOVEPos != 0x7FFFFFFF)
      {
        if(myCurrentHMOVEPos >= 97 && myCurrentHMOVEPos < 157)
        {
          myPOSP0 -= myMotionClockP0;
          myPOSP1 -= myMotionClockP1;
          myPOSM0 -= myMotionClockM0;
          myPOSM1 -= myMotionClockM1;
          myPOSBL -= myMotionClockBL;
          myPreviousHMOVEPos = myCurrentHMOVEPos;
          posChanged = true;
        }
        // Indicate that the HMOVE has been completed
        myCurrentHMOVEPos = 0x7FFFFFFF;
      }
#ifdef USE_MMR_LATCHES
      // Apply extra clocks for 'more motion required/mmr'
      if(myHMP0mmr) { myPOSP0 -= 17; posChanged = true; }
      if(myHMP1mmr) { myPOSP1 -= 17; posChanged = true; }
      if(myHMM0mmr) { myPOSM0 -= 17; posChanged = true; }
      if(myHMM1mmr) { myPOSM1 -= 17; posChanged = true; }
      if(myHMBLmmr) { myPOSBL -= 17; posChanged = true; }
#endif
      // Make sure positions are in range
      if(posChanged)
      {
        if(myPOSP0 < 0) { myPOSP0 += 160; }  myPOSP0 %= 160;
        if(myPOSP1 < 0) { myPOSP1 += 160; }  myPOSP1 %= 160;
        if(myPOSM0 < 0) { myPOSM0 += 160; }  myPOSM0 %= 160;
        if(myPOSM1 < 0) { myPOSM1 += 160; }  myPOSM1 %= 160;
        if(myPOSBL < 0) { myPOSBL += 160; }  myPOSBL %= 160;
      }
    }

    // Compute the number of clocks we're going to update
    Int32 clocksToUpdate = 0;

    // Remember how many clocks we are from the left side of the screen
    Int32 clocksFromStartOfScanLine = 228 - myClocksToEndOfScanLine;

    // See if we're updating more than the current scanline
    if(clock > (myClockAtLastUpdate + myClocksToEndOfScanLine))
    {
      // Yes, we have more than one scanline to update so finish current one
      clocksToUpdate = myClocksToEndOfScanLine;
      myClocksToEndOfScanLine = 228;
      myClockAtLastUpdate += clocksToUpdate;
    }
    else
    {
      // No, so do as much of the current scanline as possible
      clocksToUpdate = clock - myClockAtLastUpdate;
      myClocksToEndOfScanLine -= clocksToUpdate;
      myClockAtLastUpdate = clock;
    }

    Int32 startOfScanLine = HBLANK;

    // Skip over as many horizontal blank clocks as we can
    if(clocksFromStartOfScanLine < startOfScanLine)
    {
      uInt32 tmp;

      if((startOfScanLine - clocksFromStartOfScanLine) < clocksToUpdate)
        tmp = startOfScanLine - clocksFromStartOfScanLine;
      else
        tmp = clocksToUpdate;

      clocksFromStartOfScanLine += tmp;
      clocksToUpdate -= tmp;
    }

    // Remember frame pointer in case HMOVE blanks need to be handled
    uInt8* oldFramePointer = myFramePointer;

    // Update as much of the scanline as we can
    if(clocksToUpdate != 0)
      updateFrameScanline(clocksToUpdate, clocksFromStartOfScanLine - HBLANK);

    // Handle HMOVE blanks if they are enabled
    if(myHMOVEBlankEnabled && (startOfScanLine < HBLANK + 8) &&
        (clocksFromStartOfScanLine < (HBLANK + 8)))
    {
      Int32 blanks = (HBLANK + 8) - clocksFromStartOfScanLine;
      memset(oldFramePointer, 0, blanks);

      if((clocksToUpdate + clocksFromStartOfScanLine) >= (HBLANK + 8))
        myHMOVEBlankEnabled = false;
    }

    // See if we're at the end of a scanline
    if(myClocksToEndOfScanLine == 228)
    {
      // Yes, so set PF mask based on current CTRLPF reflection state 
      myCurrentPFMask = TIATables::PFMask[myCTRLPF & 0x01];

      // TODO: These should be reset right after the first copy of the player
      // has passed.  However, for now we'll just reset at the end of the 
      // scanline since the other way would be to slow (01/21/99).
P0suppress = 0;
P1suppress = 0;
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
inline void TIA::waitHorizontalSync()
{
  uInt32 cyclesToEndOfLine = 76 - ((mySystem->cycles() - 
      (myClockWhenFrameStarted / 3)) % 76);

  if(cyclesToEndOfLine < 76)
    mySystem->incrementCycles(cyclesToEndOfLine);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIA::greyOutFrame()
{
  uInt32 c = scanlines();
  if(c < myFrameYStart) c = myFrameYStart;

  uInt8* buffer = myCurrentFrameBuffer + myFramePointerOffset;
  for(uInt32 s = c; s < (myFrameHeight + myFrameYStart); ++s)
  {
    for(uInt32 i = 0; i < 160; ++i)
    {
      uInt32 idx = (s - myFrameYStart) * 160 + i;
      buffer[idx] = ((buffer[idx] & 0x0f) >> 1);
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIA::clearBuffers()
{
  memset(myCurrentFrameBuffer, 0, 160 * 320);
  memset(myPreviousFrameBuffer, 0, 160 * 320);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
inline uInt8 TIA::dumpedInputPort(int resistance)
{
  if(resistance == Controller::minimumResistance)
  {
    return 0x80;
  }
  else if((resistance == Controller::maximumResistance) || myDumpEnabled)
  {
    return 0x00;
  }
  else
  {
    // Constant here is derived from '1.6 * 0.01e-6 * 228 / 3'
    uInt32 needed = (uInt32)
      (1.216e-6 * resistance * myScanlineCountForLastFrame * myFramerate);
    if((mySystem->cycles() - myDumpDisabledCycle) > needed)
      return 0x80;
    else
      return 0x00;
  }
  return 0x00;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 TIA::peek(uInt16 addr)
{
  // Update frame to current color clock before we look at anything!
  updateFrame(mySystem->cycles() * 3);

  uInt8 value = 0x00;

  switch(addr & 0x000f)
  {
    case CXM0P:
      value = ((myCollision & Cx_M0P1) ? 0x80 : 0x00) |
              ((myCollision & Cx_M0P0) ? 0x40 : 0x00);
      break;

    case CXM1P:
      value = ((myCollision & Cx_M1P0) ? 0x80 : 0x00) |
              ((myCollision & Cx_M1P1) ? 0x40 : 0x00);
      break;

    case CXP0FB:
      value = ((myCollision & Cx_P0PF) ? 0x80 : 0x00) |
              ((myCollision & Cx_P0BL) ? 0x40 : 0x00);
      break;

    case CXP1FB:
      value = ((myCollision & Cx_P1PF) ? 0x80 : 0x00) |
              ((myCollision & Cx_P1BL) ? 0x40 : 0x00);
      break;

    case CXM0FB:
      value = ((myCollision & Cx_M0PF) ? 0x80 : 0x00) |
              ((myCollision & Cx_M0BL) ? 0x40 : 0x00);
      break;

    case CXM1FB:
      value = ((myCollision & Cx_M1PF) ? 0x80 : 0x00) |
              ((myCollision & Cx_M1BL) ? 0x40 : 0x00);
      break;

    case CXBLPF:
      value = (myCollision & Cx_BLPF) ? 0x80 : 0x00;
      break;

    case CXPPMM:
      value = ((myCollision & Cx_P0P1) ? 0x80 : 0x00) |
              ((myCollision & Cx_M0M1) ? 0x40 : 0x00);
      break;

    case INPT0:
      value = dumpedInputPort(myConsole.controller(Controller::Left).read(Controller::Nine));
      break;

    case INPT1:
      value = dumpedInputPort(myConsole.controller(Controller::Left).read(Controller::Five));
      break;

    case INPT2:
      value = dumpedInputPort(myConsole.controller(Controller::Right).read(Controller::Nine));
      break;

    case INPT3:
      value = dumpedInputPort(myConsole.controller(Controller::Right).read(Controller::Five));
      break;

    case INPT4:
      value = myConsole.controller(Controller::Left).read(Controller::Six) ? 0x80 : 0x00;
      break;

    case INPT5:
      value = myConsole.controller(Controller::Right).read(Controller::Six) ? 0x80 : 0x00;
      break;

    default:
      break;
  }

  // On certain CMOS EPROM chips the unused TIA pins on a read are not
  // floating but pulled high. Programmers might want to check their
  // games for compatibility, so we make this optional. 
  value |= myFloatTIAOutputPins ? (mySystem->getDataBusState() & 0x3F) : 0x3F;

  return value;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIA::poke(uInt16 addr, uInt8 value)
{
  addr = addr & 0x003f;

  Int32 clock = mySystem->cycles() * 3;
  Int16 delay = TIATables::PokeDelay[addr];

  // See if this is a poke to a PF register
  if(delay == -1)
  {
    static uInt32 d[4] = {4, 5, 2, 3};
    Int32 x = ((clock - myClockWhenFrameStarted) % 228);
    delay = d[(x / 3) & 3];
  }

  // Update frame to current CPU cycle before we make any changes!
  updateFrame(clock + delay);

  // If a VSYNC hasn't been generated in time go ahead and end the frame
  if(((clock - myClockWhenFrameStarted) / 228) > (Int32)myMaximumNumberOfScanlines)
  {
    mySystem->m6502().stop();
    myPartialFrameFlag = false;
  }

  switch(addr)
  {
    case VSYNC:    // Vertical sync set-clear
    {
      myVSYNC = value;

      if(myVSYNC & 0x02)
      {
        // Indicate when VSYNC should be finished.  This should really 
        // be 3 * 228 according to Atari's documentation, however, some 
        // games don't supply the full 3 scanlines of VSYNC.
        myVSYNCFinishClock = clock + 228;
      }
      else if(!(myVSYNC & 0x02) && (clock >= myVSYNCFinishClock))
      {
        // We're no longer interested in myVSYNCFinishClock
        myVSYNCFinishClock = 0x7FFFFFFF;

        // Since we're finished with the frame tell the processor to halt
        mySystem->m6502().stop();
        myPartialFrameFlag = false;
      }
      break;
    }

    case VBLANK:  // Vertical blank set-clear
    {
      // Is the dump to ground path being set for I0, I1, I2, and I3?
      if(!(myVBLANK & 0x80) && (value & 0x80))
      {
        myDumpEnabled = true;
      }

      // Is the dump to ground path being removed from I0, I1, I2, and I3?
      if((myVBLANK & 0x80) && !(value & 0x80))
      {
        myDumpEnabled = false;
        myDumpDisabledCycle = mySystem->cycles();
      }

      myVBLANK = value;
      break;
    }

    case WSYNC:   // Wait for leading edge of HBLANK
    {
      // It appears that the 6507 only halts during a read cycle so
      // we test here for follow-on writes which should be ignored as
      // far as halting the processor is concerned.
      //
      // TODO - 08-30-2006: This halting isn't correct since it's 
      // still halting on the original write.  The 6507 emulation
      // should be expanded to include a READY line.
      if(mySystem->m6502().lastAccessWasRead())
      {
        // Tell the cpu to waste the necessary amount of time
        waitHorizontalSync();
      }
      break;
    }

    case RSYNC:   // Reset horizontal sync counter
    {
//      cerr << "TIA Poke: " << hex << addr << endl;
      break;
    }

    case NUSIZ0:  // Number-size of player-missle 0
    {
//cerr << "NUSIZ0 set: " << (int)myNUSIZ0 << " => " << (int)value << " @ " << (clock + delay) << ", p0 pos = " << myPOSP0 << endl;
      myNUSIZ0 = value;
P0suppress = 0;
      break;
    }

    case NUSIZ1:  // Number-size of player-missle 1
    {
//cerr << "NUSIZ1 set: " << (int)myNUSIZ1 << " => " << (int)value << " @ " << (clock + delay) << endl;
      myNUSIZ1 = value;
P1suppress = 0;
      break;
    }

    case COLUP0:  // Color-Luminance Player 0
    {
      uInt32 color = (uInt32)(value & 0xfe);
      if(myColorLossEnabled && (myScanlineCountForLastFrame & 0x01))
      {
        color |= 0x01;
      }
      myCOLUP0 = (((((color << 8) | color) << 8) | color) << 8) | color;
      break;
    }

    case COLUP1:  // Color-Luminance Player 1
    {
      uInt32 color = (uInt32)(value & 0xfe);
      if(myColorLossEnabled && (myScanlineCountForLastFrame & 0x01))
      {
        color |= 0x01;
      }
      myCOLUP1 = (((((color << 8) | color) << 8) | color) << 8) | color;
      break;
    }

    case COLUPF:  // Color-Luminance Playfield
    {
      uInt32 color = (uInt32)(value & 0xfe);
      if(myColorLossEnabled && (myScanlineCountForLastFrame & 0x01))
      {
        color |= 0x01;
      }
      myCOLUPF = (((((color << 8) | color) << 8) | color) << 8) | color;
      break;
    }

    case COLUBK:  // Color-Luminance Background
    {
      uInt32 color = (uInt32)(value & 0xfe);
      if(myColorLossEnabled && (myScanlineCountForLastFrame & 0x01))
      {
        color |= 0x01;
      }
      myCOLUBK = (((((color << 8) | color) << 8) | color) << 8) | color;
      break;
    }

    case CTRLPF:  // Control Playfield, Ball size, Collisions
    {
      myCTRLPF = value;

      // The playfield priority and score bits from the control register
      // are accessed when the frame is being drawn.  We precompute the 
      // necessary value here so we can save time while drawing.
      myPlayfieldPriorityAndScore = ((myCTRLPF & 0x06) << 5);

      // Update the playfield mask based on reflection state if 
      // we're still on the left hand side of the playfield
      if(((clock - myClockWhenFrameStarted) % 228) < (68 + 79))
        myCurrentPFMask = TIATables::PFMask[myCTRLPF & 0x01];

      myCurrentBLMask = &TIATables::BLMask[myPOSBL & 0x03]
          [(myCTRLPF & 0x30) >> 4][160 - (myPOSBL & 0xFC)];

      break;
    }

    case REFP0:   // Reflect Player 0
    {
      // See if the reflection state of the player is being changed
      if(((value & 0x08) && !myREFP0) || (!(value & 0x08) && myREFP0))
      {
        myREFP0 = (value & 0x08);
        myCurrentGRP0 = TIATables::GRPReflect[myCurrentGRP0];
      }
      break;
    }

    case REFP1:   // Reflect Player 1
    {
      // See if the reflection state of the player is being changed
      if(((value & 0x08) && !myREFP1) || (!(value & 0x08) && myREFP1))
      {
        myREFP1 = (value & 0x08);
        myCurrentGRP1 = TIATables::GRPReflect[myCurrentGRP1];
      }
      break;
    }

    case PF0:     // Playfield register byte 0
    {
      myPF = (myPF & 0x000FFFF0) | ((value >> 4) & 0x0F);

      if(myBitEnabled[TIA::PF] == 0x00 || myPF == 0)
        myEnabledObjects &= ~PFBit;
      else
        myEnabledObjects |= PFBit;

      break;
    }

    case PF1:     // Playfield register byte 1
    {
      myPF = (myPF & 0x000FF00F) | ((uInt32)value << 4);

      if(myBitEnabled[TIA::PF] == 0x00 || myPF == 0)
        myEnabledObjects &= ~PFBit;
      else
        myEnabledObjects |= PFBit;

      break;
    }

    case PF2:     // Playfield register byte 2
    {
      myPF = (myPF & 0x00000FFF) | ((uInt32)value << 12);

      if(myBitEnabled[TIA::PF] == 0x00 || myPF == 0)
        myEnabledObjects &= ~PFBit;
      else
        myEnabledObjects |= PFBit;

      break;
    }

    case RESP0:   // Reset Player 0
    {
      Int32 hpos = (clock - myClockWhenFrameStarted) % 228 - HBLANK;
      Int16 newx;

      // Check if HMOVE is currently active
      if(myCurrentHMOVEPos != 0x7FFFFFFF)
      {
        newx = hpos < 7 ? 3 : ((hpos + 5) % 160);
        // If HMOVE is active, adjust for any remaining horizontal move clocks
        applyActiveHMOVEMotion(hpos, newx, myMotionClockP0);
      }
      else
      {
        newx = hpos < -2 ? 3 : ((hpos + 5) % 160);
        applyPreviousHMOVEMotion(hpos, newx, myHMP0);
      }
      if(newx != myPOSP0)
      {
//        myPOSP0 = newx;
//        myStartP0 = 0;
      }

      // Find out under what condition the player is being reset
      Int8 when = TIATables::PxPosResetWhen[myNUSIZ0 & 7][myPOSP0][newx];

      // Player is being reset during the display of one of its copies
      if(when == 1)
      {
        // So we go ahead and update the display before moving the player
        // TODO: The 11 should depend on how much of the player has already
        // been displayed.  Probably change table to return the amount to
        // delay by instead of just 1 (01/21/99).
//        updateFrame(clock + 11);

        myPOSP0 = newx;
P0suppress = 1;
      }
      // Player is being reset in neither the delay nor display section
      else if(when == 0)
      {
        myPOSP0 = newx;
P0suppress = 1;
      }
      // Player is being reset during the delay section of one of its copies
      else if(when == -1)
      {
        myPOSP0 = newx;
P0suppress = 0;
      }
      break;
    }

    case RESP1:   // Reset Player 1
    {
      Int32 hpos = (clock - myClockWhenFrameStarted) % 228 - HBLANK;
      Int16 newx;

      // Check if HMOVE is currently active
      if(myCurrentHMOVEPos != 0x7FFFFFFF)
      {
        newx = hpos < 7 ? 3 : ((hpos + 5) % 160);
        // If HMOVE is active, adjust for any remaining horizontal move clocks
        applyActiveHMOVEMotion(hpos, newx, myMotionClockP1);
      }
      else
      {
        newx = hpos < -2 ? 3 : ((hpos + 5) % 160);
        applyPreviousHMOVEMotion(hpos, newx, myHMP1);
      }
      if(newx != myPOSP1)
      {
//        myPOSP1 = newx;
//        myStartP1 = 0;
      }

      // Find out under what condition the player is being reset
      Int8 when = TIATables::PxPosResetWhen[myNUSIZ1 & 7][myPOSP1][newx];

      // Player is being reset during the display of one of its copies
      if(when == 1)
      {
        // So we go ahead and update the display before moving the player
        // TODO: The 11 should depend on how much of the player has already
        // been displayed.  Probably change table to return the amount to
        // delay by instead of just 1 (01/21/99).
//        updateFrame(clock + 11);

        myPOSP1 = newx;
P1suppress = 1;
      }
      // Player is being reset in neither the delay nor display section
      else if(when == 0)
      {
        myPOSP1 = newx;
P1suppress = 1;
      }
      // Player is being reset during the delay section of one of its copies
      else if(when == -1)
      {
        myPOSP1 = newx;
P1suppress = 0;
      }
      break;
    }

    case RESM0:   // Reset Missle 0
    {
      Int32 hpos = (clock - myClockWhenFrameStarted) % 228 - HBLANK;
      Int16 newx;

      // Check if HMOVE is currently active
      if(myCurrentHMOVEPos != 0x7FFFFFFF)
      {
        newx = hpos < 7 ? 2 : ((hpos + 4) % 160);
        // If HMOVE is active, adjust for any remaining horizontal move clocks
        applyActiveHMOVEMotion(hpos, newx, myMotionClockM0);
      }
      else
      {
        newx = hpos < -1 ? 2 : ((hpos + 4) % 160);
        applyPreviousHMOVEMotion(hpos, newx, myHMM0);
      }
      if(newx != myPOSM0)
      {
        // myStartM0 = skipM0delay ? 1 : 0;
        myPOSM0 = newx;
      }
      break;
    }

    case RESM1:   // Reset Missle 1
    {
      Int32 hpos = (clock - myClockWhenFrameStarted) % 228 - HBLANK;
      Int16 newx;

      // Check if HMOVE is currently active
      if(myCurrentHMOVEPos != 0x7FFFFFFF)
      {
        newx = hpos < 7 ? 2 : ((hpos + 4) % 160);
        // If HMOVE is active, adjust for any remaining horizontal move clocks
        applyActiveHMOVEMotion(hpos, newx, myMotionClockM1);
      }
      else
      {
        newx = hpos < -1 ? 2 : ((hpos + 4) % 160);
        applyPreviousHMOVEMotion(hpos, newx, myHMM1);
      }
      if(newx != myPOSM1)
      {
        // myStartM1 = skipM1delay ? 1 : 0;
        myPOSM1 = newx;
      }
      break;
    }

    case RESBL:   // Reset Ball
    {
      Int32 hpos = (clock - myClockWhenFrameStarted) % 228 - HBLANK;

      // Check if HMOVE is currently active
      if(myCurrentHMOVEPos != 0x7FFFFFFF)
      {
        myPOSBL = hpos < 7 ? 2 : ((hpos + 4) % 160);
        // If HMOVE is active, adjust for any remaining horizontal move clocks
        applyActiveHMOVEMotion(hpos, myPOSBL, myMotionClockBL);
      }
      else
      {
        myPOSBL = hpos < 0 ? 2 : ((hpos + 4) % 160);
        applyPreviousHMOVEMotion(hpos, myPOSBL, myHMBL);
      }
      break;
    }

    case AUDC0:   // Audio control 0
    {
      myAUDC0 = value & 0x0f;
      mySound.set(addr, value, mySystem->cycles());
      break;
    }
  
    case AUDC1:   // Audio control 1
    {
      myAUDC1 = value & 0x0f;
      mySound.set(addr, value, mySystem->cycles());
      break;
    }
  
    case AUDF0:   // Audio frequency 0
    {
      myAUDF0 = value & 0x1f;
      mySound.set(addr, value, mySystem->cycles());
      break;
    }
  
    case AUDF1:   // Audio frequency 1
    {
      myAUDF1 = value & 0x1f;
      mySound.set(addr, value, mySystem->cycles());
      break;
    }
  
    case AUDV0:   // Audio volume 0
    {
      myAUDV0 = value & 0x0f;
      mySound.set(addr, value, mySystem->cycles());
      break;
    }
  
    case AUDV1:   // Audio volume 1
    {
      myAUDV1 = value & 0x0f;
      mySound.set(addr, value, mySystem->cycles());
      break;
    }

    case GRP0:    // Graphics Player 0
    {
      // Set player 0 graphics
      myGRP0 = value & myBitEnabled[TIA::P0];

      // Copy player 1 graphics into its delayed register
      myDGRP1 = myGRP1;

      // Get the "current" data for GRP0 base on delay register and reflect
      uInt8 grp0 = myVDELP0 ? myDGRP0 : myGRP0;
      myCurrentGRP0 = myREFP0 ? TIATables::GRPReflect[grp0] : grp0; 

      // Get the "current" data for GRP1 base on delay register and reflect
      uInt8 grp1 = myVDELP1 ? myDGRP1 : myGRP1;
      myCurrentGRP1 = myREFP1 ? TIATables::GRPReflect[grp1] : grp1; 

      // Set enabled object bits
      if(myCurrentGRP0 != 0)
        myEnabledObjects |= P0Bit;
      else
        myEnabledObjects &= ~P0Bit;

      if(myCurrentGRP1 != 0)
        myEnabledObjects |= P1Bit;
      else
        myEnabledObjects &= ~P1Bit;

      break;
    }

    case GRP1:    // Graphics Player 1
    {
      // Set player 1 graphics
      myGRP1 = value & myBitEnabled[TIA::P1];

      // Copy player 0 graphics into its delayed register
      myDGRP0 = myGRP0;

      // Copy ball graphics into its delayed register
      myDENABL = myENABL;

      // Get the "current" data for GRP0 base on delay register
      uInt8 grp0 = myVDELP0 ? myDGRP0 : myGRP0;
      myCurrentGRP0 = myREFP0 ? TIATables::GRPReflect[grp0] : grp0; 

      // Get the "current" data for GRP1 base on delay register
      uInt8 grp1 = myVDELP1 ? myDGRP1 : myGRP1;
      myCurrentGRP1 = myREFP1 ? TIATables::GRPReflect[grp1] : grp1; 

      // Set enabled object bits
      if(myCurrentGRP0 != 0)
        myEnabledObjects |= P0Bit;
      else
        myEnabledObjects &= ~P0Bit;

      if(myCurrentGRP1 != 0)
        myEnabledObjects |= P1Bit;
      else
        myEnabledObjects &= ~P1Bit;

      if(myVDELBL ? myDENABL : myENABL)
        myEnabledObjects |= BLBit;
      else
        myEnabledObjects &= ~BLBit;

      break;
    }

    case ENAM0:   // Enable Missile 0 graphics
    {
      myENAM0 = (value & 0x02) & myBitEnabled[TIA::M0];

      if(myENAM0 && !myRESMP0)
        myEnabledObjects |= M0Bit;
      else
        myEnabledObjects &= ~M0Bit;
      break;
    }

    case ENAM1:   // Enable Missile 1 graphics
    {
      myENAM1 = (value & 0x02) & myBitEnabled[TIA::M1];

      if(myENAM1 && !myRESMP1)
        myEnabledObjects |= M1Bit;
      else
        myEnabledObjects &= ~M1Bit;
      break;
    }

    case ENABL:   // Enable Ball graphics
    {
      myENABL = (value & 0x02) & myBitEnabled[TIA::BL];

      if(myVDELBL ? myDENABL : myENABL)
        myEnabledObjects |= BLBit;
      else
        myEnabledObjects &= ~BLBit;

      break;
    }

    case HMP0:    // Horizontal Motion Player 0
    {
      pokeHMP0(value, clock);
      break;
    }

    case HMP1:    // Horizontal Motion Player 1
    {
      pokeHMP1(value, clock);
      break;
    }

    case HMM0:    // Horizontal Motion Missle 0
    {
      pokeHMM0(value, clock);
      break;
    }

    case HMM1:    // Horizontal Motion Missle 1
    {
      pokeHMM1(value, clock);
      break;
    }

    case HMBL:    // Horizontal Motion Ball
    {
      pokeHMBL(value, clock);
      break;
    }

    case VDELP0:  // Vertical Delay Player 0
    {
      myVDELP0 = value & 0x01;

      uInt8 grp0 = myVDELP0 ? myDGRP0 : myGRP0;
      myCurrentGRP0 = myREFP0 ? TIATables::GRPReflect[grp0] : grp0; 

      if(myCurrentGRP0 != 0)
        myEnabledObjects |= P0Bit;
      else
        myEnabledObjects &= ~P0Bit;
      break;
    }

    case VDELP1:  // Vertical Delay Player 1
    {
      myVDELP1 = value & 0x01;

      uInt8 grp1 = myVDELP1 ? myDGRP1 : myGRP1;
      myCurrentGRP1 = myREFP1 ? TIATables::GRPReflect[grp1] : grp1; 

      if(myCurrentGRP1 != 0)
        myEnabledObjects |= P1Bit;
      else
        myEnabledObjects &= ~P1Bit;
      break;
    }

    case VDELBL:  // Vertical Delay Ball
    {
      myVDELBL = value & 0x01;

      if(myVDELBL ? myDENABL : myENABL)
        myEnabledObjects |= BLBit;
      else
        myEnabledObjects &= ~BLBit;
      break;
    }

    case RESMP0:  // Reset missle 0 to player 0
    {
      if(myRESMP0 && !(value & 0x02))
      {
        uInt16 middle = 4;
        switch(myNUSIZ0 & 0x07)
        {
          case 0x05: middle = 8;  break;  // double size
          case 0x07: middle = 16; break;  // quad size
        }
        myPOSM0 = myPOSP0 + middle;
        if(myCurrentHMOVEPos != 0x7FFFFFFF)
        {
          myPOSM0 -= (8 - myMotionClockP0);
          myPOSM0 += (8 - myMotionClockM0);
          if(myPOSM0 < 0)  myPOSM0 += 160;
        }
        myPOSM0 %= 160;
      }
      myRESMP0 = value & 0x02;

      if(myENAM0 && !myRESMP0)
        myEnabledObjects |= M0Bit;
      else
        myEnabledObjects &= ~M0Bit;

      break;
    }

    case RESMP1:  // Reset missle 1 to player 1
    {
      if(myRESMP1 && !(value & 0x02))
      {
        uInt16 middle = 4;
        switch(myNUSIZ1 & 0x07)
        {
          case 0x05: middle = 8;  break;  // double size
          case 0x07: middle = 16; break;  // quad size
        }
        myPOSM1 = myPOSP1 + middle;
        if(myCurrentHMOVEPos != 0x7FFFFFFF)
        {
          myPOSM1 -= (8 - myMotionClockP1);
          myPOSM1 += (8 - myMotionClockM1);
          if(myPOSM1 < 0)  myPOSM1 += 160;
        }
        myPOSM1 %= 160;
      }
      myRESMP1 = value & 0x02;

      if(myENAM1 && !myRESMP1)
        myEnabledObjects |= M1Bit;
      else
        myEnabledObjects &= ~M1Bit;
      break;
    }

    case HMOVE:   // Apply horizontal motion
    {
      int hpos = (clock - myClockWhenFrameStarted) % 228 - HBLANK;
      myCurrentHMOVEPos = hpos;

      // Figure out what cycle we're at
      Int32 x = ((clock - myClockWhenFrameStarted) % 228) / 3;

      // See if we need to enable the HMOVE blank bug
      myHMOVEBlankEnabled = TIATables::HMOVEBlankEnableCycles[x];

#ifdef USE_MMR_LATCHES
      // Do we have to undo some of the already applied cycles from an
      // active graphics latch?
      if(hpos + HBLANK < 17 * 4)
      {
        Int16 cycle_fix = 17 - ((hpos + VBLANK + 7) / 4);
        if(myHMP0mmr)  myPOSP0 = (myPOSP0 + cycle_fix) % 160;
        if(myHMP1mmr)  myPOSP1 = (myPOSP1 + cycle_fix) % 160;
        if(myHMM0mmr)  myPOSM0 = (myPOSM0 + cycle_fix) % 160;
        if(myHMM1mmr)  myPOSM1 = (myPOSM1 + cycle_fix) % 160;
        if(myHMBLmmr)  myPOSBL = (myPOSBL + cycle_fix) % 160;
      }
      myHMP0mmr = myHMP1mmr = myHMM0mmr = myHMM1mmr = myHMBLmmr = false;
#endif
      // Can HMOVE activities be ignored?
      if(hpos >= -5 && hpos < 97 )
      {
        myMotionClockP0 = 0;
        myMotionClockP1 = 0;
        myMotionClockM0 = 0;
        myMotionClockM1 = 0;
        myMotionClockBL = 0;
        myHMOVEBlankEnabled = false;
        myCurrentHMOVEPos = 0x7FFFFFFF;
        break;
      }

      myMotionClockP0 = (myHMP0 ^ 0x80) >> 4;
      myMotionClockP1 = (myHMP1 ^ 0x80) >> 4;
      myMotionClockM0 = (myHMM0 ^ 0x80) >> 4;
      myMotionClockM1 = (myHMM1 ^ 0x80) >> 4;
      myMotionClockBL = (myHMBL ^ 0x80) >> 4;

      // Adjust number of graphics motion clocks for active display
      if(hpos >= 97 && hpos < 151)
      {
        Int16 skip_motclks = (160 - myCurrentHMOVEPos - 6) >> 2;
        myMotionClockP0 -= skip_motclks;
        myMotionClockP1 -= skip_motclks;
        myMotionClockM0 -= skip_motclks;
        myMotionClockM1 -= skip_motclks;
        myMotionClockBL -= skip_motclks;
        if(myMotionClockP0 < 0)  myMotionClockP0 = 0;
        if(myMotionClockP1 < 0)  myMotionClockP1 = 0;
        if(myMotionClockM0 < 0)  myMotionClockM0 = 0;
        if(myMotionClockM1 < 0)  myMotionClockM1 = 0;
        if(myMotionClockBL < 0)  myMotionClockBL = 0;
      }

      if(hpos >= -56 && hpos < -5)
      {
        Int16 max_motclks = (7 - (myCurrentHMOVEPos + 5)) >> 2;
        if(myMotionClockP0 > max_motclks)  myMotionClockP0 = max_motclks;
        if(myMotionClockP1 > max_motclks)  myMotionClockP1 = max_motclks;
        if(myMotionClockM0 > max_motclks)  myMotionClockM0 = max_motclks;
        if(myMotionClockM1 > max_motclks)  myMotionClockM1 = max_motclks;
        if(myMotionClockBL > max_motclks)  myMotionClockBL = max_motclks;
      }

      // Apply horizontal motion
      if(hpos < -5 || hpos >= 157)
      {
        myPOSP0 += 8 - myMotionClockP0;
        myPOSP1 += 8 - myMotionClockP1;
        myPOSM0 += 8 - myMotionClockM0;
        myPOSM1 += 8 - myMotionClockM1;
        myPOSBL += 8 - myMotionClockBL;
      }

      // Make sure positions are in range
      if(myPOSP0 < 0) { myPOSP0 += 160; }  myPOSP0 %= 160;
      if(myPOSP1 < 0) { myPOSP1 += 160; }  myPOSP1 %= 160;
      if(myPOSM0 < 0) { myPOSM0 += 160; }  myPOSM0 %= 160;
      if(myPOSM1 < 0) { myPOSM1 += 160; }  myPOSM1 %= 160;
      if(myPOSBL < 0) { myPOSBL += 160; }  myPOSBL %= 160;

P0suppress = 0;
P1suppress = 0;
      break;
    }

    case HMCLR:   // Clear horizontal motion registers
    {
      pokeHMP0(0, clock);
      pokeHMP1(0, clock);
      pokeHMM0(0, clock);
      pokeHMM1(0, clock);
      pokeHMBL(0, clock);
      break;
    }

    case CXCLR:   // Clear collision latches
    {
      myCollision = 0;
      break;
    }

    default:
    {
#ifdef DEBUG_ACCESSES
      cerr << "BAD TIA Poke: " << hex << addr << endl;
#endif
      break;
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Note that the following methods to change the horizontal motion registers
// are not completely accurate.  We should be taking care of the following
// explanation from A. Towers Hardware Notes:
//
//   Much more interesting is this: if the counter has not yet
//   reached the value in HMxx (or has reached it but not yet
//   commited the comparison) and a value with at least one bit
//   in common with all remaining internal counter states is
//   written (zeros or ones), the stopping condition will never be
//   reached and the object will be moved a full 15 pixels left.
//   In addition to this, the HMOVE will complete without clearing
//   the "more movement required" latch, and so will continue to send
//   an additional clock signal every 4 CLK (during visible and
//   non-visible parts of the scanline) until another HMOVE operation
//   clears the latch. The HMCLR command does not reset these latches.
//
// This condition is what causes the 'starfield effect' in Cosmic Ark,
// and the 'snow' in Stay Frosty.  Ideally, we'd trace the counter and
// do a compare every colour clock, updating the horizontal positions
// when applicable.  We can save time by cheating, and noting that the
// effect only occurs for 'magic numbers' 0x70 and 0x80.
//
// Most of the ideas in these methods come from MESS.
// (used with permission from Wilbert Pol)
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIA::pokeHMP0(uInt8 value, Int32 clock)
{
  value &= 0xF0;
  if(myHMP0 == value)
    return;

  int hpos  = (clock - myClockWhenFrameStarted) % 228 - HBLANK;

  // Check if HMOVE is currently active
  if(myCurrentHMOVEPos != 0x7FFFFFFF &&
     hpos < BSPF_min(myCurrentHMOVEPos + 6 + myMotionClockP0 * 4, 7))
  {
    Int32 newMotion = (value ^ 0x80) >> 4;
    // Check if new horizontal move can still be applied normally
    if(newMotion > myMotionClockP0 ||
       hpos <= BSPF_min(myCurrentHMOVEPos + 6 + newMotion * 4, 7))
    {
      myPOSP0 -= (newMotion - myMotionClockP0);
      myMotionClockP0 = newMotion;
    }
    else
    {
      myPOSP0 -= (15 - myMotionClockP0);
      myMotionClockP0 = 15;
      if(value != 0x70 && value != 0x80)
        myHMP0mmr = true;
    }
    if(myPOSP0 < 0) { myPOSP0 += 160; }  myPOSP0 %= 160;
  }
  myHMP0 = value;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIA::pokeHMP1(uInt8 value, Int32 clock)
{
  value &= 0xF0;
  if(myHMP1 == value)
    return;

  int hpos  = (clock - myClockWhenFrameStarted) % 228 - HBLANK;

  // Check if HMOVE is currently active
  if(myCurrentHMOVEPos != 0x7FFFFFFF &&
     hpos < BSPF_min(myCurrentHMOVEPos + 6 + myMotionClockP1 * 4, 7))
  {
    Int32 newMotion = (value ^ 0x80) >> 4;
    // Check if new horizontal move can still be applied normally
    if(newMotion > myMotionClockP1 ||
       hpos <= BSPF_min(myCurrentHMOVEPos + 6 + newMotion * 4, 7))
    {
      myPOSP1 -= (newMotion - myMotionClockP1);
      myMotionClockP1 = newMotion;
    }
    else
    {
      myPOSP1 -= (15 - myMotionClockP1);
      myMotionClockP1 = 15;
      if(value != 0x70 && value != 0x80)
        myHMP1mmr = true;
    }
    if(myPOSP1 < 0) { myPOSP1 += 160; }  myPOSP1 %= 160;
  }
  myHMP1 = value;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIA::pokeHMM0(uInt8 value, Int32 clock)
{
  value &= 0xF0;
  if(myHMM0 == value)
    return;

  int hpos  = (clock - myClockWhenFrameStarted) % 228 - HBLANK;

  // Check if HMOVE is currently active
  if(myCurrentHMOVEPos != 0x7FFFFFFF &&
     hpos < BSPF_min(myCurrentHMOVEPos + 6 + myMotionClockM0 * 4, 7))
  {
    Int32 newMotion = (value ^ 0x80) >> 4;
    // Check if new horizontal move can still be applied normally
    if(newMotion > myMotionClockM0 ||
       hpos <= BSPF_min(myCurrentHMOVEPos + 6 + newMotion * 4, 7))
    {
      myPOSM0 -= (newMotion - myMotionClockM0);
      myMotionClockM0 = newMotion;
    }
    else
    {
      myPOSM0 -= (15 - myMotionClockM0);
      myMotionClockM0 = 15;
      if(value != 0x70 && value != 0x80)
        myHMM0mmr = true;
    }
    if(myPOSM0 < 0) { myPOSM0 += 160; }  myPOSM0 %= 160;
  }
  myHMM0 = value;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIA::pokeHMM1(uInt8 value, Int32 clock)
{
  value &= 0xF0;
  if(myHMM1 == value)
    return;

  int hpos  = (clock - myClockWhenFrameStarted) % 228 - HBLANK;

  // Check if HMOVE is currently active
  if(myCurrentHMOVEPos != 0x7FFFFFFF &&
     hpos < BSPF_min(myCurrentHMOVEPos + 6 + myMotionClockM1 * 4, 7))
  {
    Int32 newMotion = (value ^ 0x80) >> 4;
    // Check if new horizontal move can still be applied normally
    if(newMotion > myMotionClockM1 ||
       hpos <= BSPF_min(myCurrentHMOVEPos + 6 + newMotion * 4, 7))
    {
      myPOSM1 -= (newMotion - myMotionClockM1);
      myMotionClockM1 = newMotion;
    }
    else
    {
      myPOSM1 -= (15 - myMotionClockM1);
      myMotionClockM1 = 15;
      if(value != 0x70 && value != 0x80)
        myHMM1mmr = true;
    }
    if(myPOSM1 < 0) { myPOSM1 += 160; }  myPOSM1 %= 160;
  }
  myHMM1 = value;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIA::pokeHMBL(uInt8 value, Int32 clock)
{
  value &= 0xF0;
  if(myHMBL == value)
    return;

  int hpos  = (clock - myClockWhenFrameStarted) % 228 - HBLANK;

  // Check if HMOVE is currently active
  if(myCurrentHMOVEPos != 0x7FFFFFFF &&
     hpos < BSPF_min(myCurrentHMOVEPos + 6 + myMotionClockBL * 4, 7))
  {
    Int32 newMotion = (value ^ 0x80) >> 4;
    // Check if new horizontal move can still be applied normally
    if(newMotion > myMotionClockBL ||
       hpos <= BSPF_min(myCurrentHMOVEPos + 6 + newMotion * 4, 7))
    {
      myPOSBL -= (newMotion - myMotionClockBL);
      myMotionClockBL = newMotion;
    }
    else
    {
      myPOSBL -= (15 - myMotionClockBL);
      myMotionClockBL = 15;
      if(value != 0x70 && value != 0x80)
        myHMBLmmr = true;
    }
    if(myPOSBL < 0) { myPOSBL += 160; }  myPOSBL %= 160;
  }
  myHMBL = value;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// The following two methods apply extra clocks when a horizontal motion
// register (HMxx) is modified during an HMOVE, before waiting for the
// documented time of at least 24 CPU cycles.  The applicable explanation
// from A. Towers Hardware Notes is as follows:
//
//   In theory then the side effects of modifying the HMxx registers
//   during HMOVE should be quite straight-forward. If the internal
//   counter has not yet reached the value in HMxx, a new value greater
//   than this (in 0-15 terms) will work normally. Conversely, if
//   the counter has already reached the value in HMxx, new values
//   will have no effect because the latch will have been cleared.
//
// Most of the ideas in these methods come from MESS.
// (used with permission from Wilbert Pol)
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
inline void TIA::applyActiveHMOVEMotion(int hpos, Int16& pos, Int32 motionClock)
{
  if(hpos < BSPF_min(myCurrentHMOVEPos + 6 + 16 * 4, 7))
  {
    Int32 decrements_passed = (hpos - (myCurrentHMOVEPos + 4)) >> 2;
    pos += 8;
    if((motionClock - decrements_passed) > 0)
    {
      pos -= (motionClock - decrements_passed);
      if(pos < 0)  pos += 160;
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
inline void TIA::applyPreviousHMOVEMotion(int hpos, Int16& pos, uInt8 motion)
{
  if(myPreviousHMOVEPos != 0x7FFFFFFF)
  {
    uInt8 motclk = (motion ^ 0x80) >> 4;
    if(hpos <= myPreviousHMOVEPos - 228 + 5 + motclk * 4)
    {
      uInt8 motclk_passed = (hpos - (myPreviousHMOVEPos - 228 + 6)) >> 2;
      pos -= (motclk - motclk_passed);
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TIA::TIA(const TIA& c)
  : myConsole(c.myConsole),
    mySound(c.mySound),
    mySettings(c.mySettings),
    myCOLUBK(myColor[0]),
    myCOLUPF(myColor[1]),
    myCOLUP0(myColor[2]),
    myCOLUP1(myColor[3])
{
  assert(false);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TIA& TIA::operator = (const TIA&)
{
  assert(false);
  return *this;
}
