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
// $Id: TIA.cxx,v 1.106 2009-03-16 00:23:42 stephena Exp $
//============================================================================

//#define DEBUG_HMOVE
//#define NO_HMOVE_FIXES

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
  myCurrentFrameBuffer = new uInt8[160 * 300];
  myPreviousFrameBuffer = new uInt8[160 * 300];

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

  myLastHMOVEClock = 0;
  myHMOVEBlankEnabled = false;
  myM0CosmicArkMotionEnabled = false; // FIXME - remove this
  myM0CosmicArkCounter = 0;           // FIXME - remove this

  enableBits(true);

  myDumpEnabled = false;
  myDumpDisabledCycle = 0;

  myFloatTIAOutputPins = mySettings.getBool("tiafloat");

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

  myFrameCounter = 0;

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
  if(myFrameYStart < 0)  myFrameYStart = 0;
  if(myFrameYStart > 64) myFrameYStart = 64;
  myFrameHeight = atoi(myConsole.properties().get(Display_Height).c_str());
  if(myFrameHeight < 210) myFrameHeight = 210;
  if(myFrameHeight > 256) myFrameHeight = 256;

  // Calculate color clock offsets for starting and stoping frame drawing
  myStartDisplayOffset = 228 * myFrameYStart;
  myStopDisplayOffset = myStartDisplayOffset + 228 * myFrameHeight;

  // Reasonable values to start and stop the current frame drawing
  myClockWhenFrameStarted = mySystem->cycles() * 3;
  myClockStartDisplay = myClockWhenFrameStarted + myStartDisplayOffset;
  myClockStopDisplay = myClockWhenFrameStarted + myStopDisplayOffset;
  myClockAtLastUpdate = myClockWhenFrameStarted;
  myClocksToEndOfScanLine = 228;
  myVSYNCFinishClock = 0x7FFFFFFF;
  myScanlineCountForLastFrame = 0;
  myCurrentScanline = 0;
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
  myLastHMOVEClock -= clocks;
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
    out.putInt(myCurrentScanline);
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

    out.putInt(myLastHMOVEClock);
    out.putBool(myHMOVEBlankEnabled);
    out.putBool(myM0CosmicArkMotionEnabled); // FIXME - remove this
    out.putInt(myM0CosmicArkCounter);        // FIXME - remove this

    out.putBool(myDumpEnabled);
    out.putInt(myDumpDisabledCycle);

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
    myScanlineCountForLastFrame = (Int32) in.getInt();
    myCurrentScanline = (Int32) in.getInt();
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
    myHMP0 = (Int8) in.getByte();
    myHMP1 = (Int8) in.getByte();
    myHMM0 = (Int8) in.getByte();
    myHMM1 = (Int8) in.getByte();
    myHMBL = (Int8) in.getByte();
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

    myLastHMOVEClock = (Int32) in.getInt();
    myHMOVEBlankEnabled = in.getBool();
    myM0CosmicArkMotionEnabled = in.getBool();   // FIXME - remove this
    myM0CosmicArkCounter = (uInt32) in.getInt(); // FIXME - remove this

    myDumpEnabled = in.getBool();
    myDumpDisabledCycle = (Int32) in.getInt();

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

  uInt32 totalClocks = (mySystem->cycles() * 3) - myClockWhenFrameStarted;
  myCurrentScanline = totalClocks / 228;

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
  myScanlineCountForLastFrame = myCurrentScanline;

  // Stats counters
  myFrameCounter++;

  // Recalculate framerate. attempting to auto-correct for scanline 'jumps'
  if(myFrameCounter % 8 == 0 && myAutoFrameEnabled)
  {
    myFramerate = (myScanlineCountForLastFrame > 285 ? 15600.0 : 15720.0) /
                   myScanlineCountForLastFrame;
    myConsole.setFramerate(myFramerate);
  }

  myFrameGreyed = false;
}

#ifdef DEBUGGER_SUPPORT
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIA::updateScanline()
{
  // Start a new frame if the old one was finished
  if(!myPartialFrameFlag) {
    startFrame();
  }

  // grey out old frame contents
  if(!myFrameGreyed) greyOutFrame();
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

  totalClocks = (mySystem->cycles() * 3) - myClockWhenFrameStarted;
  myCurrentScanline = totalClocks / 228;

  // if we finished the frame, get ready for the next one
  if(!myPartialFrameFlag)
    endFrame();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIA::updateScanlineByStep()
{
  // Start a new frame if the old one was finished
  if(!myPartialFrameFlag) {
    startFrame();
  }

  // grey out old frame contents
  if(!myFrameGreyed) greyOutFrame();
  myFrameGreyed = true;

  // true either way:
  myPartialFrameFlag = true;

  int totalClocks = (mySystem->cycles() * 3) - myClockWhenFrameStarted;

  // Update frame by one CPU instruction/color clock
  mySystem->m6502().execute(1);
  updateFrame(mySystem->cycles() * 3);

  totalClocks = (mySystem->cycles() * 3) - myClockWhenFrameStarted;
  myCurrentScanline = totalClocks / 228;

  // if we finished the frame, get ready for the next one
  if(!myPartialFrameFlag)
    endFrame();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIA::updateScanlineByTrace(int target)
{
  // Start a new frame if the old one was finished
  if(!myPartialFrameFlag) {
    startFrame();
  }

  // grey out old frame contents
  if(!myFrameGreyed) greyOutFrame();
  myFrameGreyed = true;

  // true either way:
  myPartialFrameFlag = true;

  int totalClocks = (mySystem->cycles() * 3) - myClockWhenFrameStarted;

  while(mySystem->m6502().getPC() != target)
  {
    mySystem->m6502().execute(1);
    updateFrame(mySystem->cycles() * 3);
  }

  totalClocks = (mySystem->cycles() * 3) - myClockWhenFrameStarted;
  myCurrentScanline = totalClocks / 228;

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
  // See if we're in the nondisplayable portion of the screen or if
  // we've already updated this portion of the screen
  if((clock < myClockStartDisplay) || 
      (myClockAtLastUpdate >= myClockStopDisplay) ||  
      (myClockAtLastUpdate >= clock))
  {
    return;
  }

  // Truncate the number of cycles to update to the stop display point
  if(clock > myClockStopDisplay)
  {
    clock = myClockStopDisplay;
  }

  // Update frame one scanline at a time
  do
  {
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
    {
      updateFrameScanline(clocksToUpdate, clocksFromStartOfScanLine - HBLANK);
    }

    // Handle HMOVE blanks if they are enabled
    if(myHMOVEBlankEnabled && (startOfScanLine < HBLANK + 8) &&
        (clocksFromStartOfScanLine < (HBLANK + 8)))
    {
      Int32 blanks = (HBLANK + 8) - clocksFromStartOfScanLine;
      memset(oldFramePointer, 0, blanks);

      if((clocksToUpdate + clocksFromStartOfScanLine) >= (HBLANK + 8))
      {
        myHMOVEBlankEnabled = false;
      }
    }

    // See if we're at the end of a scanline
    if(myClocksToEndOfScanLine == 228)
    {
      // Yes, so set PF mask based on current CTRLPF reflection state 
      myCurrentPFMask = TIATables::PFMask[myCTRLPF & 0x01];

      // TODO: These should be reset right after the first copy of the player
      // has passed.  However, for now we'll just reset at the end of the 
      // scanline since the other way would be to slow (01/21/99).
      myCurrentP0Mask = &TIATables::PxMask[myPOSP0 & 0x03]
          [0][myNUSIZ0 & 0x07][160 - (myPOSP0 & 0xFC)];
      myCurrentP1Mask = &TIATables::PxMask[myPOSP1 & 0x03]
          [0][myNUSIZ1 & 0x07][160 - (myPOSP1 & 0xFC)];

#ifndef NO_HMOVE_FIXES
      // Handle the "Cosmic Ark" TIA bug if it's enabled
      if(myM0CosmicArkMotionEnabled)
      {
        // Movement table associated with the bug
        static uInt32 m[4] = {18, 33, 0, 17};

        myM0CosmicArkCounter = (myM0CosmicArkCounter + 1) & 3;
        myPOSM0 -= m[myM0CosmicArkCounter];

        if(myPOSM0 >= 160)
          myPOSM0 -= 160;
        else if(myPOSM0 < 0)
          myPOSM0 += 160;

        if(myM0CosmicArkCounter == 1)
        {
          // Stretch this missle so it's at least 2 pixels wide
          myCurrentM0Mask = &TIATables::MxMask[myPOSM0 & 0x03]
              [myNUSIZ0 & 0x07][((myNUSIZ0 & 0x30) >> 4) | 0x01]
              [160 - (myPOSM0 & 0xFC)];
        }
        else if(myM0CosmicArkCounter == 2)
        {
          // Missle is disabled on this line 
          myCurrentM0Mask = &TIATables::DisabledMask[0];
        }
        else
        {
          myCurrentM0Mask = &TIATables::MxMask[myPOSM0 & 0x03]
              [myNUSIZ0 & 0x07][(myNUSIZ0 & 0x30) >> 4][160 - (myPOSM0 & 0xFC)];
        }
      }
#endif
    }
  } 
  while(myClockAtLastUpdate < clock);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
inline void TIA::waitHorizontalSync()
{
  uInt32 cyclesToEndOfLine = 76 - ((mySystem->cycles() - 
      (myClockWhenFrameStarted / 3)) % 76);

  if(cyclesToEndOfLine < 76)
  {
    mySystem->incrementCycles(cyclesToEndOfLine);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIA::greyOutFrame()
{
  uInt32 c = scanlines();
  if(c < myFrameYStart) c = myFrameYStart;

  for(uInt32 s = c; s < (myFrameHeight + myFrameYStart); ++s)
  {
    for(uInt32 i = 0; i < 160; ++i)
    {
      uInt8 tmp = myCurrentFrameBuffer[ (s - myFrameYStart) * 160 + i] & 0x0f;
      tmp >>= 1;
      myCurrentFrameBuffer[ (s - myFrameYStart) * 160 + i] = tmp;
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIA::clearBuffers()
{
  memset(myCurrentFrameBuffer, 0, 160 * 300);
  memset(myPreviousFrameBuffer, 0, 160 * 300);
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
  // TODO - convert all constants to enums (TIA.cs/530)

  // Update frame to current color clock before we look at anything!
  updateFrame(mySystem->cycles() * 3);

  uInt8 value = 0x00;

  switch(addr & 0x000f)
  {
    case CXM0P:
      value = ((myCollision & 0x0001) ? 0x80 : 0x00) |
              ((myCollision & 0x0002) ? 0x40 : 0x00);
      break;

    case CXM1P:
      value = ((myCollision & 0x0004) ? 0x80 : 0x00) |
              ((myCollision & 0x0008) ? 0x40 : 0x00);
      break;

    case CXP0FB:
      value = ((myCollision & 0x0010) ? 0x80 : 0x00) |
              ((myCollision & 0x0020) ? 0x40 : 0x00);
      break;

    case CXP1FB:
      value = ((myCollision & 0x0040) ? 0x80 : 0x00) |
              ((myCollision & 0x0080) ? 0x40 : 0x00);
      break;

    case CXM0FB:
      value = ((myCollision & 0x0100) ? 0x80 : 0x00) |
              ((myCollision & 0x0200) ? 0x40 : 0x00);
      break;

    case CXM1FB:
      value = ((myCollision & 0x0400) ? 0x80 : 0x00) |
              ((myCollision & 0x0800) ? 0x40 : 0x00);
      break;

    case CXBLPF:
      value = (myCollision & 0x1000) ? 0x80 : 0x00;
      break;

    case CXPPMM:
      value = ((myCollision & 0x2000) ? 0x80 : 0x00) |
              ((myCollision & 0x4000) ? 0x40 : 0x00);
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

    case 0x0e:  // TODO - document this address
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
  if(((clock - myClockWhenFrameStarted) / 228) > myMaximumNumberOfScanlines)
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
      myNUSIZ0 = value;

      // TODO: Technically the "enable" part, [0], should depend on the current
      // enabled or disabled state.  This mean we probably need a data member
      // to maintain that state (01/21/99).
      myCurrentP0Mask = &TIATables::PxMask[myPOSP0 & 0x03]
          [0][myNUSIZ0 & 0x07][160 - (myPOSP0 & 0xFC)];

      myCurrentM0Mask = &TIATables::MxMask[myPOSM0 & 0x03]
          [myNUSIZ0 & 0x07][(myNUSIZ0 & 0x30) >> 4][160 - (myPOSM0 & 0xFC)];

      break;
    }

    case NUSIZ1:  // Number-size of player-missle 1
    {
      myNUSIZ1 = value;

      // TODO: Technically the "enable" part, [0], should depend on the current
      // enabled or disabled state.  This mean we probably need a data member
      // to maintain that state (01/21/99).
      myCurrentP1Mask = &TIATables::PxMask[myPOSP1 & 0x03]
          [0][myNUSIZ1 & 0x07][160 - (myPOSP1 & 0xFC)];

      myCurrentM1Mask = &TIATables::MxMask[myPOSM1 & 0x03]
          [myNUSIZ1 & 0x07][(myNUSIZ1 & 0x30) >> 4][160 - (myPOSM1 & 0xFC)];

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
      {
        myCurrentPFMask = TIATables::PFMask[myCTRLPF & 0x01];
      }

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
      Int32 hpos = (clock - myClockWhenFrameStarted) % 228;
      Int32 newx = hpos < HBLANK ? 3 : (((hpos - HBLANK) + 5) % 160);

      // Find out under what condition the player is being reset
      Int8 when = TIATables::PxPosResetWhen[myNUSIZ0 & 7][myPOSP0][newx];

#ifdef DEBUG_HMOVE
      if((clock - myLastHMOVEClock) < (24 * 3))
        cerr << "Reset Player 0 within 24 cycles of HMOVE: "
             << ((clock - myLastHMOVEClock)/3)
             << "  hpos: " << hpos << ", newx = " << newx << endl;
#endif

      // Player is being reset during the display of one of its copies
      if(when == 1)
      {
        // So we go ahead and update the display before moving the player
        // TODO: The 11 should depend on how much of the player has already
        // been displayed.  Probably change table to return the amount to
        // delay by instead of just 1 (01/21/99).
        updateFrame(clock + 11);

        myPOSP0 = newx;

        // Setup the mask to skip the first copy of the player
        myCurrentP0Mask = &TIATables::PxMask[myPOSP0 & 0x03]
            [1][myNUSIZ0 & 0x07][160 - (myPOSP0 & 0xFC)];
      }
      // Player is being reset in neither the delay nor display section
      else if(when == 0)
      {
        myPOSP0 = newx;

        // So we setup the mask to skip the first copy of the player
        myCurrentP0Mask = &TIATables::PxMask[myPOSP0 & 0x03]
            [1][myNUSIZ0 & 0x07][160 - (myPOSP0 & 0xFC)];
      }
      // Player is being reset during the delay section of one of its copies
      else if(when == -1)
      {
        myPOSP0 = newx;

        // So we setup the mask to display all copies of the player
        myCurrentP0Mask = &TIATables::PxMask[myPOSP0 & 0x03]
            [0][myNUSIZ0 & 0x07][160 - (myPOSP0 & 0xFC)];
      }
      break;
    }

    case RESP1:   // Reset Player 1
    {
      Int32 hpos = (clock - myClockWhenFrameStarted) % 228;
      Int32 newx = hpos < HBLANK ? 3 : (((hpos - HBLANK) + 5) % 160);

      // Find out under what condition the player is being reset
      Int8 when = TIATables::PxPosResetWhen[myNUSIZ1 & 7][myPOSP1][newx];

#ifdef DEBUG_HMOVE
      if((clock - myLastHMOVEClock) < (24 * 3))
        cerr << "Reset Player 1 within 24 cycles of HMOVE: "
             << ((clock - myLastHMOVEClock)/3)
             << "  hpos: " << hpos << ", newx = " << newx << endl;
#endif

      // Player is being reset during the display of one of its copies
      if(when == 1)
      {
        // So we go ahead and update the display before moving the player
        // TODO: The 11 should depend on how much of the player has already
        // been displayed.  Probably change table to return the amount to
        // delay by instead of just 1 (01/21/99).
        updateFrame(clock + 11);

        myPOSP1 = newx;

        // Setup the mask to skip the first copy of the player
        myCurrentP1Mask = &TIATables::PxMask[myPOSP1 & 0x03]
            [1][myNUSIZ1 & 0x07][160 - (myPOSP1 & 0xFC)];
      }
      // Player is being reset in neither the delay nor display section
      else if(when == 0)
      {
        myPOSP1 = newx;

        // So we setup the mask to skip the first copy of the player
        myCurrentP1Mask = &TIATables::PxMask[myPOSP1 & 0x03]
            [1][myNUSIZ1 & 0x07][160 - (myPOSP1 & 0xFC)];
      }
      // Player is being reset during the delay section of one of its copies
      else if(when == -1)
      {
        myPOSP1 = newx;

        // So we setup the mask to display all copies of the player
        myCurrentP1Mask = &TIATables::PxMask[myPOSP1 & 0x03]
            [0][myNUSIZ1 & 0x07][160 - (myPOSP1 & 0xFC)];
      }
      break;
    }

    case RESM0:   // Reset Missle 0
    {
      int hpos = (clock - myClockWhenFrameStarted) % 228;
      myPOSM0 = hpos < HBLANK ? 2 : (((hpos - HBLANK) + 4) % 160);

#ifdef DEBUG_HMOVE
      if((clock - myLastHMOVEClock) < (24 * 3))
        cerr << "Reset Missle 0 within 24 cycles of HMOVE: "
             << ((clock - myLastHMOVEClock)/3)
             << "  hpos: " << hpos << ", myPOSM0 = " << myPOSM0 << endl;
#endif

#ifndef NO_HMOVE_FIXES
      // TODO: Remove the following special hack for Dolphin by
      // figuring out what really happens when Reset Missle 
      // occurs 20 cycles after an HMOVE (04/13/02).
      if(((clock - myLastHMOVEClock) == (20 * 3)) && (hpos == 69))
      {
        myPOSM0 = 8;
      }
      // TODO: Remove the following special hack for Solaris by
      // figuring out what really happens when Reset Missle 
      // occurs 9 cycles after an HMOVE (04/11/08).
      else if(((clock - myLastHMOVEClock) == (9 * 3)) && (hpos == 36))
      {
        myPOSM0 = 8;
      }
#endif
      myCurrentM0Mask = &TIATables::MxMask[myPOSM0 & 0x03]
          [myNUSIZ0 & 0x07][(myNUSIZ0 & 0x30) >> 4][160 - (myPOSM0 & 0xFC)];
      break;
    }

    case RESM1:   // Reset Missle 1
    {
      int hpos = (clock - myClockWhenFrameStarted) % 228;
      myPOSM1 = hpos < HBLANK ? 2 : (((hpos - HBLANK) + 4) % 160);

#ifdef DEBUG_HMOVE
      if((clock - myLastHMOVEClock) < (24 * 3))
        cerr << "Reset Missle 1 within 24 cycles of HMOVE: "
             << ((clock - myLastHMOVEClock)/3)
             << "  hpos: " << hpos << ", myPOSM1 = " << myPOSM1 << endl;
#endif

#ifndef NO_HMOVE_FIXES
      // TODO: Remove the following special hack for Pitfall II by
      // figuring out what really happens when Reset Missle 
      // occurs 3 cycles after an HMOVE (04/13/02).
      if(((clock - myLastHMOVEClock) == (3 * 3)) && (hpos == 18))
      {
        myPOSM1 = 3;
      }
#endif
      myCurrentM1Mask = &TIATables::MxMask[myPOSM1 & 0x03]
          [myNUSIZ1 & 0x07][(myNUSIZ1 & 0x30) >> 4][160 - (myPOSM1 & 0xFC)];
      break;
    }

    case RESBL:   // Reset Ball
    {
      int hpos = (clock - myClockWhenFrameStarted) % 228 ;
      myPOSBL = hpos < HBLANK ? 2 : (((hpos - HBLANK) + 4) % 160);

#ifdef DEBUG_HMOVE
      if((clock - myLastHMOVEClock) < (24 * 3))
        cerr << "Reset Ball within 24 cycles of HMOVE: "
             << ((clock - myLastHMOVEClock)/3)
             << "  hpos: " << hpos << ", myPOSBL = " << myPOSBL << endl;
#endif

#ifndef NO_HMOVE_FIXES
      // TODO: Remove the following special hack by figuring out what
      // really happens when Reset Ball occurs 18 cycles after an HMOVE.
      if((clock - myLastHMOVEClock) == (18 * 3))
      {
        // Escape from the Mindmaster (01/09/99)
        if((hpos == 60) || (hpos == 69))
          myPOSBL = 10;
        // Mission Survive (04/11/08)
        else if(hpos == 63)
          myPOSBL = 7;
      }
      // TODO: Remove the following special hack for Escape from the
      // Mindmaster by figuring out what really happens when Reset Ball 
      // occurs 15 cycles after an HMOVE (04/11/08).
      else if(((clock - myLastHMOVEClock) == (15 * 3)) && (hpos == 60))
      {
        myPOSBL = 10;
      } 
      // TODO: Remove the following special hack for Decathlon by
      // figuring out what really happens when Reset Ball 
      // occurs 3 cycles after an HMOVE (04/13/02).
      else if(((clock - myLastHMOVEClock) == (3 * 3)) && (hpos == 18))
      {
        myPOSBL = 3;
      } 
      // TODO: Remove the following special hack for Robot Tank by
      // figuring out what really happens when Reset Ball 
      // occurs 7 cycles after an HMOVE (04/13/02).
      else if(((clock - myLastHMOVEClock) == (7 * 3)) && (hpos == 30))
      {
        myPOSBL = 6;
      } 
      // TODO: Remove the following special hack for Hole Hunter by
      // figuring out what really happens when Reset Ball 
      // occurs 6 cycles after an HMOVE (04/13/02).
      else if(((clock - myLastHMOVEClock) == (6 * 3)) && (hpos == 27))
      {
        myPOSBL = 5;
      }
      // TODO: Remove the following special hack for Swoops! by
      // figuring out what really happens when Reset Ball 
      // occurs 9 cycles after an HMOVE (04/11/08).
      else if(((clock - myLastHMOVEClock) == (9 * 3)) && (hpos == 36))
      {
        myPOSBL = 7;
      }
      // TODO: Remove the following special hack for Solaris by
      // figuring out what really happens when Reset Ball 
      // occurs 12 cycles after an HMOVE (04/11/08).
      else if(((clock - myLastHMOVEClock) == (12 * 3)) && (hpos == 45))
      {
        myPOSBL = 8;
      }
#endif
      myCurrentBLMask = &TIATables::BLMask[myPOSBL & 0x03]
          [(myCTRLPF & 0x30) >> 4][160 - (myPOSBL & 0xFC)];
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
      myHMP0 = value >> 4;
      break;
    }

    case HMP1:    // Horizontal Motion Player 1
    {
      myHMP1 = value >> 4;
      break;
    }

    case HMM0:    // Horizontal Motion Missle 0
    {
      Int8 tmp = value >> 4;

#ifndef NO_HMOVE_FIXES
      // Should we enabled TIA M0 "bug" used for stars in Cosmic Ark?
      if((clock == (myLastHMOVEClock + 21 * 3)) && (myHMM0 == 7) && (tmp == 6))
      {
        myM0CosmicArkMotionEnabled = true;
        myM0CosmicArkCounter = 0;
      }
#endif
      myHMM0 = tmp;
      break;
    }

    case HMM1:    // Horizontal Motion Missle 1
    {
      myHMM1 = value >> 4;
      break;
    }

    case HMBL:    // Horizontal Motion Ball
    {
      myHMBL = value >> 4;
      break;
    }

    case VDELP0:  // Vertial Delay Player 0
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

    case VDELP1:  // Vertial Delay Player 1
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

    case VDELBL:  // Vertial Delay Ball
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
        uInt16 middle;

        if((myNUSIZ0 & 0x07) == 0x05)
          middle = 8;
        else if((myNUSIZ0 & 0x07) == 0x07)
          middle = 16;
        else
          middle = 4;

        myPOSM0 = (myPOSP0 + middle) % 160;
        myCurrentM0Mask = &TIATables::MxMask[myPOSM0 & 0x03]
            [myNUSIZ0 & 0x07][(myNUSIZ0 & 0x30) >> 4][160 - (myPOSM0 & 0xFC)];
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
        uInt16 middle;

        if((myNUSIZ1 & 0x07) == 0x05)
          middle = 8;
        else if((myNUSIZ1 & 0x07) == 0x07)
          middle = 16;
        else
          middle = 4;

        myPOSM1 = (myPOSP1 + middle) % 160;
        myCurrentM1Mask = &TIATables::MxMask[myPOSM1 & 0x03]
            [myNUSIZ1 & 0x07][(myNUSIZ1 & 0x30) >> 4][160 - (myPOSM1 & 0xFC)];
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
      // Figure out what cycle we're at
      Int32 x = ((clock - myClockWhenFrameStarted) % 228) / 3;

      // See if we need to enable the HMOVE blank bug
      if(TIATables::HMOVEBlankEnableCycles[x])
      {
        // TODO: Allow this to be turned off using properties...
        myHMOVEBlankEnabled = true;
      }

      myPOSP0 += TIATables::CompleteMotion[x][myHMP0];
      myPOSP1 += TIATables::CompleteMotion[x][myHMP1];
      myPOSM0 += TIATables::CompleteMotion[x][myHMM0];
      myPOSM1 += TIATables::CompleteMotion[x][myHMM1];
      myPOSBL += TIATables::CompleteMotion[x][myHMBL];

      if(myPOSP0 >= 160)
        myPOSP0 -= 160;
      else if(myPOSP0 < 0)
        myPOSP0 += 160;

      if(myPOSP1 >= 160)
        myPOSP1 -= 160;
      else if(myPOSP1 < 0)
        myPOSP1 += 160;

      if(myPOSM0 >= 160)
        myPOSM0 -= 160;
      else if(myPOSM0 < 0)
        myPOSM0 += 160;

      if(myPOSM1 >= 160)
        myPOSM1 -= 160;
      else if(myPOSM1 < 0)
        myPOSM1 += 160;

      if(myPOSBL >= 160)
        myPOSBL -= 160;
      else if(myPOSBL < 0)
        myPOSBL += 160;

      myCurrentBLMask = &TIATables::BLMask[myPOSBL & 0x03]
          [(myCTRLPF & 0x30) >> 4][160 - (myPOSBL & 0xFC)];

      myCurrentP0Mask = &TIATables::PxMask[myPOSP0 & 0x03]
          [0][myNUSIZ0 & 0x07][160 - (myPOSP0 & 0xFC)];
      myCurrentP1Mask = &TIATables::PxMask[myPOSP1 & 0x03]
          [0][myNUSIZ1 & 0x07][160 - (myPOSP1 & 0xFC)];

      myCurrentM0Mask = &TIATables::MxMask[myPOSM0 & 0x03]
          [myNUSIZ0 & 0x07][(myNUSIZ0 & 0x30) >> 4][160 - (myPOSM0 & 0xFC)];
      myCurrentM1Mask = &TIATables::MxMask[myPOSM1 & 0x03]
          [myNUSIZ1 & 0x07][(myNUSIZ1 & 0x30) >> 4][160 - (myPOSM1 & 0xFC)];

      // Remember what clock HMOVE occured at
      myLastHMOVEClock = clock;

      // Disable TIA M0 "bug" used for stars in Cosmic ark
      myM0CosmicArkMotionEnabled = false;
      break;
    }

    case HMCLR:   // Clear horizontal motion registers
    {
      myHMP0 = 0;
      myHMP1 = 0;
      myHMM0 = 0;
      myHMM1 = 0;
      myHMBL = 0;
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
