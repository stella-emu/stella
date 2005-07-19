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
// Copyright (c) 1995-2005 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: TIA.cxx,v 1.55 2005-07-19 02:24:13 urchlay Exp $
//============================================================================

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <iostream>

#include "Console.hxx"
#include "Control.hxx"
#include "M6502.hxx"
#include "System.hxx"
#include "TIA.hxx"
#include "Serializer.hxx"
#include "Deserializer.hxx"
#include "Settings.hxx"
#include "Sound.hxx"

#define HBLANK 68

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TIA::TIA(const Console& console, Settings& settings)
    : myConsole(console),
      mySettings(settings),
      mySound(NULL),
      myColorLossEnabled(false),
      myMaximumNumberOfScanlines(262),
      myCOLUBK(myColor[0]),
      myCOLUPF(myColor[1]),
      myCOLUP0(myColor[2]),
      myCOLUP1(myColor[3])
{
  // Allocate buffers for two frame buffers
  myCurrentFrameBuffer = new uInt8[160 * 300];
  myPreviousFrameBuffer = new uInt8[160 * 300];

  myFrameGreyed = false;

  for(uInt32 i = 0; i < 6; ++i)
    myBitEnabled[i] = true;

  for(uInt16 x = 0; x < 2; ++x)
  {
    for(uInt16 enabled = 0; enabled < 256; ++enabled)
    {
      if(enabled & PriorityBit)
      {
        uInt8 color = 0;

        if((enabled & (myP1Bit | myM1Bit)) != 0)
          color = 3;
        if((enabled & (myP0Bit | myM0Bit)) != 0)
          color = 2;
        if((enabled & myBLBit) != 0)
          color = 1;
        if((enabled & myPFBit) != 0)
          color = 1;  // NOTE: Playfield has priority so ScoreBit isn't used

        myPriorityEncoder[x][enabled] = color;
      }
      else
      {
        uInt8 color = 0;

        if((enabled & myBLBit) != 0)
          color = 1;
        if((enabled & myPFBit) != 0)
          color = (enabled & ScoreBit) ? ((x == 0) ? 2 : 3) : 1;
        if((enabled & (myP1Bit | myM1Bit)) != 0)
          color = 3;
        if((enabled & (myP0Bit | myM0Bit)) != 0)
          color = 2;

        myPriorityEncoder[x][enabled] = color;
      }
    }
  }

  for(uInt32 i = 0; i < 640; ++i)
  {
    ourDisabledMaskTable[i] = 0;
  }

  // Compute all of the mask tables
  computeBallMaskTable();
  computeCollisionTable();
  computeMissleMaskTable();
  computePlayerMaskTable();
  computePlayerPositionResetWhenTable();
  computePlayerReflectTable();
  computePlayfieldMaskTable();

  // Init stats counters
  myFrameCounter = 0;

  myAUDV0 = myAUDV1 = myAUDF0 = myAUDF1 = myAUDC0 = myAUDC1 = 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TIA::~TIA()
{
  delete[] myCurrentFrameBuffer;
  delete[] myPreviousFrameBuffer;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const char* TIA::name() const
{
  return "TIA";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIA::reset()
{
  // Reset the sound device
  mySound->reset();

  // Clear frame buffers
  clearBuffers();

  // Reset pixel pointer and drawing flag
  myFramePointer = myCurrentFrameBuffer;

  // Calculate color clock offsets for starting and stoping frame drawing
  myStartDisplayOffset = 228 * 
      atoi(myConsole.properties().get("Display.YStart").c_str());
  myStopDisplayOffset = myStartDisplayOffset + 228 *
      atoi(myConsole.properties().get("Display.Height").c_str());

  // Reasonable values to start and stop the current frame drawing
  myClockWhenFrameStarted = mySystem->cycles() * 3;
  myClockStartDisplay = myClockWhenFrameStarted + myStartDisplayOffset;
  myClockStopDisplay = myClockWhenFrameStarted + myStopDisplayOffset;
  myClockAtLastUpdate = myClockWhenFrameStarted;
  myClocksToEndOfScanLine = 228;
  myVSYNCFinishClock = 0x7FFFFFFF;
  myScanlineCountForLastFrame = 0;
  myCurrentScanline = 0;

  // Currently no objects are enabled
  myEnabledObjects = 0;

  // Some default values for the registers
  myVSYNC = 0;
  myVBLANK = 0;
  myNUSIZ0 = 0;
  myNUSIZ1 = 0;
  myCOLUP0 = 0;
  myCOLUP1 = 0;
  myCOLUPF = 0;
  myPlayfieldPriorityAndScore = 0;
  myCOLUBK = 0;
  myCTRLPF = 0;
  myREFP0 = false;
  myREFP1 = false;
  myPF = 0;
  myGRP0 = 0;
  myGRP1 = 0;
  myDGRP0 = 0;
  myDGRP1 = 0;
  myENAM0 = false;
  myENAM1 = false;
  myENABL = false;
  myDENABL = false;
  myHMP0 = 0;
  myHMP1 = 0;
  myHMM0 = 0;
  myHMM1 = 0;
  myHMBL = 0;
  myVDELP0 = false;
  myVDELP1 = false;
  myVDELBL = false;
  myRESMP0 = false;
  myRESMP1 = false;
  myCollision = 0;
  myPOSP0 = 0;
  myPOSP1 = 0;
  myPOSM0 = 0;
  myPOSM1 = 0;
  myPOSBL = 0;

  // Some default values for the "current" variables
  myCurrentGRP0 = 0;
  myCurrentGRP1 = 0;
  myCurrentBLMask = ourBallMaskTable[0][0];
  myCurrentM0Mask = ourMissleMaskTable[0][0][0];
  myCurrentM1Mask = ourMissleMaskTable[0][0][0];
  myCurrentP0Mask = ourPlayerMaskTable[0][0][0];
  myCurrentP1Mask = ourPlayerMaskTable[0][0][0];
  myCurrentPFMask = ourPlayfieldTable[0];

  myLastHMOVEClock = 0;
  myHMOVEBlankEnabled = false;
  myM0CosmicArkMotionEnabled = false;
  myM0CosmicArkCounter = 0;

  for(uInt32 i = 0; i < 6; ++i)
    myBitEnabled[i] = true;

  myDumpEnabled = false;
  myDumpDisabledCycle = 0;

  myAllowHMOVEBlanks = 
      (myConsole.properties().get("Emulation.HmoveBlanks") == "Yes");

  myFrameXStart = atoi(myConsole.properties().get("Display.XStart").c_str());
  myFrameWidth = atoi(myConsole.properties().get("Display.Width").c_str());
  myFrameYStart = atoi(myConsole.properties().get("Display.YStart").c_str());
  myFrameHeight = atoi(myConsole.properties().get("Display.Height").c_str());

  // Make sure the starting x and width values are reasonable
  if((myFrameXStart + myFrameWidth) > 160)
  {
    // Values are illegal so reset to default values
    myFrameXStart = 0;
    myFrameWidth = 160;
  }

  if(myConsole.properties().get("Display.Format") == "PAL")
  {
    myColorLossEnabled = true;
    myMaximumNumberOfScanlines = 342;
  }
  else
  {
    myColorLossEnabled = false;
    myMaximumNumberOfScanlines = 290;
  }

  enableBits(true);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIA::systemCyclesReset()
{
  // Get the current system cycle
  uInt32 cycles = mySystem->cycles();

  // Adjust the sound cycle indicator
  mySound->adjustCycleCounter(-1 * cycles);

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
  // Remember which system I'm installed in
  mySystem = &system;

  uInt16 shift = mySystem->pageShift();
  mySystem->resetCycles();

  // All accesses are to this device
  System::PageAccess access;
  access.directPeekBase = 0;
  access.directPokeBase = 0;
  access.device = this;

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
bool TIA::save(Serializer& out)
{
  string device = name();

  try
  {
    out.putString(device);

    out.putLong(myClockWhenFrameStarted);
    out.putLong(myClockStartDisplay);
    out.putLong(myClockStopDisplay);
    out.putLong(myClockAtLastUpdate);
    out.putLong(myClocksToEndOfScanLine);
    out.putLong(myScanlineCountForLastFrame);
    out.putLong(myCurrentScanline);
    out.putLong(myVSYNCFinishClock);

    out.putLong(myEnabledObjects);

    out.putLong(myVSYNC);
    out.putLong(myVBLANK);
    out.putLong(myNUSIZ0);
    out.putLong(myNUSIZ1);

    out.putLong(myCOLUP0);
    out.putLong(myCOLUP1);
    out.putLong(myCOLUPF);
    out.putLong(myCOLUBK);

    out.putLong(myCTRLPF);
    out.putLong(myPlayfieldPriorityAndScore);
    out.putBool(myREFP0);
    out.putBool(myREFP1);
    out.putLong(myPF);
    out.putLong(myGRP0);
    out.putLong(myGRP1);
    out.putLong(myDGRP0);
    out.putLong(myDGRP1);
    out.putBool(myENAM0);
    out.putBool(myENAM1);
    out.putBool(myENABL);
    out.putBool(myDENABL);
    out.putLong(myHMP0);
    out.putLong(myHMP1);
    out.putLong(myHMM0);
    out.putLong(myHMM1);
    out.putLong(myHMBL);
    out.putBool(myVDELP0);
    out.putBool(myVDELP1);
    out.putBool(myVDELBL);
    out.putBool(myRESMP0);
    out.putBool(myRESMP1);
    out.putLong(myCollision);
    out.putLong(myPOSP0);
    out.putLong(myPOSP1);
    out.putLong(myPOSM0);
    out.putLong(myPOSM1);
    out.putLong(myPOSBL);

    out.putLong(myCurrentGRP0);
    out.putLong(myCurrentGRP1);

// pointers
//  myCurrentBLMask = ourBallMaskTable[0][0];
//  myCurrentM0Mask = ourMissleMaskTable[0][0][0];
//  myCurrentM1Mask = ourMissleMaskTable[0][0][0];
//  myCurrentP0Mask = ourPlayerMaskTable[0][0][0];
//  myCurrentP1Mask = ourPlayerMaskTable[0][0][0];
//  myCurrentPFMask = ourPlayfieldTable[0];

    out.putLong(myLastHMOVEClock);
    out.putBool(myHMOVEBlankEnabled);
    out.putBool(myM0CosmicArkMotionEnabled);
    out.putLong(myM0CosmicArkCounter);

    out.putBool(myDumpEnabled);
    out.putLong(myDumpDisabledCycle);

    // Save the sound sample stuff ...
    mySound->save(out);
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

    myClockWhenFrameStarted = (Int32) in.getLong();
    myClockStartDisplay = (Int32) in.getLong();
    myClockStopDisplay = (Int32) in.getLong();
    myClockAtLastUpdate = (Int32) in.getLong();
    myClocksToEndOfScanLine = (Int32) in.getLong();
    myScanlineCountForLastFrame = (Int32) in.getLong();
    myCurrentScanline = (Int32) in.getLong();
    myVSYNCFinishClock = (Int32) in.getLong();

    myEnabledObjects = (uInt8) in.getLong();

    myVSYNC = (uInt8) in.getLong();
    myVBLANK = (uInt8) in.getLong();
    myNUSIZ0 = (uInt8) in.getLong();
    myNUSIZ1 = (uInt8) in.getLong();

    myCOLUP0 = (uInt32) in.getLong();
    myCOLUP1 = (uInt32) in.getLong();
    myCOLUPF = (uInt32) in.getLong();
    myCOLUBK = (uInt32) in.getLong();

    myCTRLPF = (uInt8) in.getLong();
    myPlayfieldPriorityAndScore = (uInt8) in.getLong();
    myREFP0 = in.getBool();
    myREFP1 = in.getBool();
    myPF = (uInt32) in.getLong();
    myGRP0 = (uInt8) in.getLong();
    myGRP1 = (uInt8) in.getLong();
    myDGRP0 = (uInt8) in.getLong();
    myDGRP1 = (uInt8) in.getLong();
    myENAM0 = in.getBool();
    myENAM1 = in.getBool();
    myENABL = in.getBool();
    myDENABL = in.getBool();
    myHMP0 = (Int8) in.getLong();
    myHMP1 = (Int8) in.getLong();
    myHMM0 = (Int8) in.getLong();
    myHMM1 = (Int8) in.getLong();
    myHMBL = (Int8) in.getLong();
    myVDELP0 = in.getBool();
    myVDELP1 = in.getBool();
    myVDELBL = in.getBool();
    myRESMP0 = in.getBool();
    myRESMP1 = in.getBool();
    myCollision = (uInt16) in.getLong();
    myPOSP0 = (Int16) in.getLong();
    myPOSP1 = (Int16) in.getLong();
    myPOSM0 = (Int16) in.getLong();
    myPOSM1 = (Int16) in.getLong();
    myPOSBL = (Int16) in.getLong();

    myCurrentGRP0 = (uInt8) in.getLong();
    myCurrentGRP1 = (uInt8) in.getLong();

// pointers
//  myCurrentBLMask = ourBallMaskTable[0][0];
//  myCurrentM0Mask = ourMissleMaskTable[0][0][0];
//  myCurrentM1Mask = ourMissleMaskTable[0][0][0];
//  myCurrentP0Mask = ourPlayerMaskTable[0][0][0];
//  myCurrentP1Mask = ourPlayerMaskTable[0][0][0];
//  myCurrentPFMask = ourPlayfieldTable[0];

    myLastHMOVEClock = (Int32) in.getLong();
    myHMOVEBlankEnabled = in.getBool();
    myM0CosmicArkMotionEnabled = in.getBool();
    myM0CosmicArkCounter = (uInt32) in.getLong();

    myDumpEnabled = in.getBool();
    myDumpDisabledCycle = (Int32) in.getLong();

    // Load the sound sample stuff ...
    mySound->load(in);

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

  if(!myPartialFrameFlag)
    endFrame();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
inline void TIA::startFrame() {
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
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
inline void TIA::endFrame() {
  // This stuff should only happen at the end of a frame
  // Compute the number of scanlines in the frame
  myScanlineCountForLastFrame = myCurrentScanline;

  // Stats counters
  myFrameCounter++;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIA::updateScanline()
{
  // Start a new frame if the old one was finished
  if(!myPartialFrameFlag) {
    startFrame();
	 myFrameGreyed = false;
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
const uInt32* TIA::palette() const
{
  // See which palette we should be using
  string type   = mySettings.getString("palette");
  string format = myConsole.properties().get("Display.Format");

  if(type == "standard")
    return (format == "PAL") ? ourPALPalette : ourNTSCPalette;
  else if(type == "original")
    return (format == "PAL") ? ourPALPalette11 : ourNTSCPalette11;
  else if(type == "z26")
    return (format == "PAL") ? ourPALPaletteZ26 : ourNTSCPaletteZ26;
  else  // return normal palette by default
    return (format == "PAL") ? ourPALPalette : ourNTSCPalette;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 TIA::width() const 
{
  return myFrameWidth; 
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 TIA::height() const 
{
  return myFrameHeight; 
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 TIA::scanlines() const
{
  // calculate the current scanline
  uInt32 totalClocks = (mySystem->cycles() * 3) - myClockWhenFrameStarted;
  return totalClocks/228;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIA::setSound(Sound& sound)
{
  mySound = &sound;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIA::computeBallMaskTable()
{
  // First, calculate masks for alignment 0
  for(Int32 size = 0; size < 4; ++size)
  {
    Int32 x;

    // Set all of the masks to false to start with
    for(x = 0; x < 160; ++x)
    {
      ourBallMaskTable[0][size][x] = false;
    }

    // Set the necessary fields true
    for(x = 0; x < 160 + 8; ++x)
    {
      if((x >= 0) && (x < (1 << size)))
      {
        ourBallMaskTable[0][size][x % 160] = true;
      }
    }

    // Copy fields into the wrap-around area of the mask
    for(x = 0; x < 160; ++x)
    {
      ourBallMaskTable[0][size][x + 160] = ourBallMaskTable[0][size][x];
    }
  }

  // Now, copy data for alignments of 1, 2 and 3
  for(uInt32 align = 1; align < 4; ++align)
  {
    for(uInt32 size = 0; size < 4; ++size)
    {
      for(uInt32 x = 0; x < 320; ++x)
      {
        ourBallMaskTable[align][size][x] = 
            ourBallMaskTable[0][size][(x + 320 - align) % 320];
      }
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIA::computeCollisionTable()
{
  for(uInt8 i = 0; i < 64; ++i)
  { 
    ourCollisionTable[i] = 0;

    if((i & myM0Bit) && (i & myP1Bit))    // M0-P1
      ourCollisionTable[i] |= 0x0001;

    if((i & myM0Bit) && (i & myP0Bit))    // M0-P0
      ourCollisionTable[i] |= 0x0002;

    if((i & myM1Bit) && (i & myP0Bit))    // M1-P0
      ourCollisionTable[i] |= 0x0004;

    if((i & myM1Bit) && (i & myP1Bit))    // M1-P1
      ourCollisionTable[i] |= 0x0008;

    if((i & myP0Bit) && (i & myPFBit))    // P0-PF
      ourCollisionTable[i] |= 0x0010;

    if((i & myP0Bit) && (i & myBLBit))    // P0-BL
      ourCollisionTable[i] |= 0x0020;

    if((i & myP1Bit) && (i & myPFBit))    // P1-PF
      ourCollisionTable[i] |= 0x0040;

    if((i & myP1Bit) && (i & myBLBit))    // P1-BL
      ourCollisionTable[i] |= 0x0080;

    if((i & myM0Bit) && (i & myPFBit))    // M0-PF
      ourCollisionTable[i] |= 0x0100;

    if((i & myM0Bit) && (i & myBLBit))    // M0-BL
      ourCollisionTable[i] |= 0x0200;

    if((i & myM1Bit) && (i & myPFBit))    // M1-PF
      ourCollisionTable[i] |= 0x0400;

    if((i & myM1Bit) && (i & myBLBit))    // M1-BL
      ourCollisionTable[i] |= 0x0800;

    if((i & myBLBit) && (i & myPFBit))    // BL-PF
      ourCollisionTable[i] |= 0x1000;

    if((i & myP0Bit) && (i & myP1Bit))    // P0-P1
      ourCollisionTable[i] |= 0x2000;

    if((i & myM0Bit) && (i & myM1Bit))    // M0-M1
      ourCollisionTable[i] |= 0x4000;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIA::computeMissleMaskTable()
{
  // First, calculate masks for alignment 0
  Int32 x, size, number;

  // Clear the missle table to start with
  for(number = 0; number < 8; ++number)
    for(size = 0; size < 4; ++size)
      for(x = 0; x < 160; ++x)
        ourMissleMaskTable[0][number][size][x] = false;

  for(number = 0; number < 8; ++number)
  {
    for(size = 0; size < 4; ++size)
    {
      for(x = 0; x < 160 + 72; ++x)
      {
        // Only one copy of the missle
        if((number == 0x00) || (number == 0x05) || (number == 0x07))
        {
          if((x >= 0) && (x < (1 << size)))
            ourMissleMaskTable[0][number][size][x % 160] = true;
        }
        // Two copies - close
        else if(number == 0x01)
        {
          if((x >= 0) && (x < (1 << size)))
            ourMissleMaskTable[0][number][size][x % 160] = true;
          else if(((x - 16) >= 0) && ((x - 16) < (1 << size)))
            ourMissleMaskTable[0][number][size][x % 160] = true;
        }
        // Two copies - medium
        else if(number == 0x02)
        {
          if((x >= 0) && (x < (1 << size)))
            ourMissleMaskTable[0][number][size][x % 160] = true;
          else if(((x - 32) >= 0) && ((x - 32) < (1 << size)))
            ourMissleMaskTable[0][number][size][x % 160] = true;
        }
        // Three copies - close
        else if(number == 0x03)
        {
          if((x >= 0) && (x < (1 << size)))
            ourMissleMaskTable[0][number][size][x % 160] = true;
          else if(((x - 16) >= 0) && ((x - 16) < (1 << size)))
            ourMissleMaskTable[0][number][size][x % 160] = true;
          else if(((x - 32) >= 0) && ((x - 32) < (1 << size)))
            ourMissleMaskTable[0][number][size][x % 160] = true;
        }
        // Two copies - wide
        else if(number == 0x04)
        {
          if((x >= 0) && (x < (1 << size)))
            ourMissleMaskTable[0][number][size][x % 160] = true;
          else if(((x - 64) >= 0) && ((x - 64) < (1 << size)))
            ourMissleMaskTable[0][number][size][x % 160] = true;
        }
        // Three copies - medium
        else if(number == 0x06)
        {
          if((x >= 0) && (x < (1 << size)))
            ourMissleMaskTable[0][number][size][x % 160] = true;
          else if(((x - 32) >= 0) && ((x - 32) < (1 << size)))
            ourMissleMaskTable[0][number][size][x % 160] = true;
          else if(((x - 64) >= 0) && ((x - 64) < (1 << size)))
            ourMissleMaskTable[0][number][size][x % 160] = true;
        }
      }

      // Copy data into wrap-around area
      for(x = 0; x < 160; ++x)
        ourMissleMaskTable[0][number][size][x + 160] = 
          ourMissleMaskTable[0][number][size][x];
    }
  }

  // Now, copy data for alignments of 1, 2 and 3
  for(uInt32 align = 1; align < 4; ++align)
  {
    for(number = 0; number < 8; ++number)
    {
      for(size = 0; size < 4; ++size)
      {
        for(x = 0; x < 320; ++x)
        {
          ourMissleMaskTable[align][number][size][x] = 
            ourMissleMaskTable[0][number][size][(x + 320 - align) % 320];
        }
      }
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIA::computePlayerMaskTable()
{
  // First, calculate masks for alignment 0
  Int32 x, enable, mode;

  // Set the player mask table to all zeros
  for(enable = 0; enable < 2; ++enable)
    for(mode = 0; mode < 8; ++mode)
      for(x = 0; x < 160; ++x)
        ourPlayerMaskTable[0][enable][mode][x] = 0x00;

  // Now, compute the player mask table
  for(enable = 0; enable < 2; ++enable)
  {
    for(mode = 0; mode < 8; ++mode)
    {
      for(x = 0; x < 160 + 72; ++x)
      {
        if(mode == 0x00)
        {
          if((enable == 0) && (x >= 0) && (x < 8))
            ourPlayerMaskTable[0][enable][mode][x % 160] = 0x80 >> x;
        }
        else if(mode == 0x01)
        {
          if((enable == 0) && (x >= 0) && (x < 8))
            ourPlayerMaskTable[0][enable][mode][x % 160] = 0x80 >> x;
          else if(((x - 16) >= 0) && ((x - 16) < 8))
            ourPlayerMaskTable[0][enable][mode][x % 160] = 0x80 >> (x - 16);
        }
        else if(mode == 0x02)
        {
          if((enable == 0) && (x >= 0) && (x < 8))
            ourPlayerMaskTable[0][enable][mode][x % 160] = 0x80 >> x;
          else if(((x - 32) >= 0) && ((x - 32) < 8))
            ourPlayerMaskTable[0][enable][mode][x % 160] = 0x80 >> (x - 32);
        }
        else if(mode == 0x03)
        {
          if((enable == 0) && (x >= 0) && (x < 8))
            ourPlayerMaskTable[0][enable][mode][x % 160] = 0x80 >> x;
          else if(((x - 16) >= 0) && ((x - 16) < 8))
            ourPlayerMaskTable[0][enable][mode][x % 160] = 0x80 >> (x - 16);
          else if(((x - 32) >= 0) && ((x - 32) < 8))
            ourPlayerMaskTable[0][enable][mode][x % 160] = 0x80 >> (x - 32);
        }
        else if(mode == 0x04)
        {
          if((enable == 0) && (x >= 0) && (x < 8))
            ourPlayerMaskTable[0][enable][mode][x % 160] = 0x80 >> x;
          else if(((x - 64) >= 0) && ((x - 64) < 8))
            ourPlayerMaskTable[0][enable][mode][x % 160] = 0x80 >> (x - 64);
        }
        else if(mode == 0x05)
        {
          // For some reason in double size mode the player's output
          // is delayed by one pixel thus we use > instead of >=
          if((enable == 0) && (x > 0) && (x <= 16))
            ourPlayerMaskTable[0][enable][mode][x % 160] = 0x80 >> ((x - 1)/2);
        }
        else if(mode == 0x06)
        {
          if((enable == 0) && (x >= 0) && (x < 8))
            ourPlayerMaskTable[0][enable][mode][x % 160] = 0x80 >> x;
          else if(((x - 32) >= 0) && ((x - 32) < 8))
            ourPlayerMaskTable[0][enable][mode][x % 160] = 0x80 >> (x - 32);
          else if(((x - 64) >= 0) && ((x - 64) < 8))
            ourPlayerMaskTable[0][enable][mode][x % 160] = 0x80 >> (x - 64);
        }
        else if(mode == 0x07)
        {
          // For some reason in quad size mode the player's output
          // is delayed by one pixel thus we use > instead of >=
          if((enable == 0) && (x > 0) && (x <= 32))
            ourPlayerMaskTable[0][enable][mode][x % 160] = 0x80 >> ((x - 1)/4);
        }
      }
  
      // Copy data into wrap-around area
      for(x = 0; x < 160; ++x)
      {
        ourPlayerMaskTable[0][enable][mode][x + 160] = 
            ourPlayerMaskTable[0][enable][mode][x];
      }
    }
  }

  // Now, copy data for alignments of 1, 2 and 3
  for(uInt32 align = 1; align < 4; ++align)
  {
    for(enable = 0; enable < 2; ++enable)
    {
      for(mode = 0; mode < 8; ++mode)
      {
        for(x = 0; x < 320; ++x)
        {
          ourPlayerMaskTable[align][enable][mode][x] =
              ourPlayerMaskTable[0][enable][mode][(x + 320 - align) % 320];
        }
      }
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIA::computePlayerPositionResetWhenTable()
{
  uInt32 mode, oldx, newx;

  // Loop through all player modes, all old player positions, and all new
  // player positions and determine where the new position is located:
  // 1 means the new position is within the display of an old copy of the
  // player, -1 means the new position is within the delay portion of an
  // old copy of the player, and 0 means it's neither of these two
  for(mode = 0; mode < 8; ++mode)
  {
    for(oldx = 0; oldx < 160; ++oldx)
    {
      // Set everything to 0 for non-delay/non-display section
      for(newx = 0; newx < 160; ++newx)
      {
        ourPlayerPositionResetWhenTable[mode][oldx][newx] = 0;
      }

      // Now, we'll set the entries for non-delay/non-display section
      for(newx = 0; newx < 160 + 72 + 5; ++newx)
      {
        if(mode == 0x00)
        {
          if((newx >= oldx) && (newx < (oldx + 4)))
            ourPlayerPositionResetWhenTable[mode][oldx][newx % 160] = -1;

          if((newx >= oldx + 4) && (newx < (oldx + 4 + 8)))
            ourPlayerPositionResetWhenTable[mode][oldx][newx % 160] = 1;
        }
        else if(mode == 0x01)
        {
          if((newx >= oldx) && (newx < (oldx + 4)))
            ourPlayerPositionResetWhenTable[mode][oldx][newx % 160] = -1;
          else if((newx >= (oldx + 16)) && (newx < (oldx + 16 + 4)))
            ourPlayerPositionResetWhenTable[mode][oldx][newx % 160] = -1;

          if((newx >= oldx + 4) && (newx < (oldx + 4 + 8)))
            ourPlayerPositionResetWhenTable[mode][oldx][newx % 160] = 1;
          else if((newx >= oldx + 16 + 4) && (newx < (oldx + 16 + 4 + 8)))
            ourPlayerPositionResetWhenTable[mode][oldx][newx % 160] = 1;
        }
        else if(mode == 0x02)
        {
          if((newx >= oldx) && (newx < (oldx + 4)))
            ourPlayerPositionResetWhenTable[mode][oldx][newx % 160] = -1;
          else if((newx >= (oldx + 32)) && (newx < (oldx + 32 + 4)))
            ourPlayerPositionResetWhenTable[mode][oldx][newx % 160] = -1;

          if((newx >= oldx + 4) && (newx < (oldx + 4 + 8)))
            ourPlayerPositionResetWhenTable[mode][oldx][newx % 160] = 1;
          else if((newx >= oldx + 32 + 4) && (newx < (oldx + 32 + 4 + 8)))
            ourPlayerPositionResetWhenTable[mode][oldx][newx % 160] = 1;
        }
        else if(mode == 0x03)
        {
          if((newx >= oldx) && (newx < (oldx + 4)))
            ourPlayerPositionResetWhenTable[mode][oldx][newx % 160] = -1;
          else if((newx >= (oldx + 16)) && (newx < (oldx + 16 + 4)))
            ourPlayerPositionResetWhenTable[mode][oldx][newx % 160] = -1;
          else if((newx >= (oldx + 32)) && (newx < (oldx + 32 + 4)))
            ourPlayerPositionResetWhenTable[mode][oldx][newx % 160] = -1;

          if((newx >= oldx + 4) && (newx < (oldx + 4 + 8)))
            ourPlayerPositionResetWhenTable[mode][oldx][newx % 160] = 1;
          else if((newx >= oldx + 16 + 4) && (newx < (oldx + 16 + 4 + 8)))
            ourPlayerPositionResetWhenTable[mode][oldx][newx % 160] = 1;
          else if((newx >= oldx + 32 + 4) && (newx < (oldx + 32 + 4 + 8)))
            ourPlayerPositionResetWhenTable[mode][oldx][newx % 160] = 1;
        }
        else if(mode == 0x04)
        {
          if((newx >= oldx) && (newx < (oldx + 4)))
            ourPlayerPositionResetWhenTable[mode][oldx][newx % 160] = -1;
          else if((newx >= (oldx + 64)) && (newx < (oldx + 64 + 4)))
            ourPlayerPositionResetWhenTable[mode][oldx][newx % 160] = -1;

          if((newx >= oldx + 4) && (newx < (oldx + 4 + 8)))
            ourPlayerPositionResetWhenTable[mode][oldx][newx % 160] = 1;
          else if((newx >= oldx + 64 + 4) && (newx < (oldx + 64 + 4 + 8)))
            ourPlayerPositionResetWhenTable[mode][oldx][newx % 160] = 1;
        }
        else if(mode == 0x05)
        {
          if((newx >= oldx) && (newx < (oldx + 4)))
            ourPlayerPositionResetWhenTable[mode][oldx][newx % 160] = -1;

          if((newx >= oldx + 4) && (newx < (oldx + 4 + 16)))
            ourPlayerPositionResetWhenTable[mode][oldx][newx % 160] = 1;
        }
        else if(mode == 0x06)
        {
          if((newx >= oldx) && (newx < (oldx + 4)))
            ourPlayerPositionResetWhenTable[mode][oldx][newx % 160] = -1;
          else if((newx >= (oldx + 32)) && (newx < (oldx + 32 + 4)))
            ourPlayerPositionResetWhenTable[mode][oldx][newx % 160] = -1;
          else if((newx >= (oldx + 64)) && (newx < (oldx + 64 + 4)))
            ourPlayerPositionResetWhenTable[mode][oldx][newx % 160] = -1;

          if((newx >= oldx + 4) && (newx < (oldx + 4 + 8)))
            ourPlayerPositionResetWhenTable[mode][oldx][newx % 160] = 1;
          else if((newx >= oldx + 32 + 4) && (newx < (oldx + 32 + 4 + 8)))
            ourPlayerPositionResetWhenTable[mode][oldx][newx % 160] = 1;
          else if((newx >= oldx + 64 + 4) && (newx < (oldx + 64 + 4 + 8)))
            ourPlayerPositionResetWhenTable[mode][oldx][newx % 160] = 1;
        }
        else if(mode == 0x07)
        {
          if((newx >= oldx) && (newx < (oldx + 4)))
            ourPlayerPositionResetWhenTable[mode][oldx][newx % 160] = -1;

          if((newx >= oldx + 4) && (newx < (oldx + 4 + 32)))
            ourPlayerPositionResetWhenTable[mode][oldx][newx % 160] = 1;
        }
      }

      // Let's do a sanity check on our table entries
      uInt32 s1 = 0, s2 = 0;
      for(newx = 0; newx < 160; ++newx)
      {
        if(ourPlayerPositionResetWhenTable[mode][oldx][newx] == -1)
          ++s1;
        if(ourPlayerPositionResetWhenTable[mode][oldx][newx] == 1)
          ++s2;
      }
      assert((s1 % 4 == 0) && (s2 % 8 == 0));
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIA::computePlayerReflectTable()
{
  for(uInt16 i = 0; i < 256; ++i)
  {
    uInt8 r = 0;

    for(uInt16 t = 1; t <= 128; t *= 2)
    {
      r = (r << 1) | ((i & t) ? 0x01 : 0x00);
    }

    ourPlayerReflectTable[i] = r;
  } 
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIA::computePlayfieldMaskTable()
{
  Int32 x;

  // Compute playfield mask table for non-reflected mode
  for(x = 0; x < 160; ++x)
  {
    if(x < 16)
      ourPlayfieldTable[0][x] = 0x00001 << (x / 4);
    else if(x < 48)
      ourPlayfieldTable[0][x] = 0x00800 >> ((x - 16) / 4);
    else if(x < 80) 
      ourPlayfieldTable[0][x] = 0x01000 << ((x - 48) / 4);
    else if(x < 96) 
      ourPlayfieldTable[0][x] = 0x00001 << ((x - 80) / 4);
    else if(x < 128)
      ourPlayfieldTable[0][x] = 0x00800 >> ((x - 96) / 4);
    else if(x < 160) 
      ourPlayfieldTable[0][x] = 0x01000 << ((x - 128) / 4);
  }

  // Compute playfield mask table for reflected mode
  for(x = 0; x < 160; ++x)
  {
    if(x < 16)
      ourPlayfieldTable[1][x] = 0x00001 << (x / 4);
    else if(x < 48)
      ourPlayfieldTable[1][x] = 0x00800 >> ((x - 16) / 4);
    else if(x < 80) 
      ourPlayfieldTable[1][x] = 0x01000 << ((x - 48) / 4);
    else if(x < 112) 
      ourPlayfieldTable[1][x] = 0x80000 >> ((x - 80) / 4);
    else if(x < 144) 
      ourPlayfieldTable[1][x] = 0x00010 << ((x - 112) / 4);
    else if(x < 160) 
      ourPlayfieldTable[1][x] = 0x00008 >> ((x - 144) / 4);
  }
}

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

      // Playfield is enabled and the score bit is not set
      case myPFBit: 
      case myPFBit | PriorityBit:
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

      // Playfield is enabled and the score bit is set
      case myPFBit | ScoreBit:
      case myPFBit | ScoreBit | PriorityBit:
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
      case myP0Bit:
      case myP0Bit | ScoreBit:
      case myP0Bit | PriorityBit:
      case myP0Bit | ScoreBit | PriorityBit:
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
      case myP1Bit:
      case myP1Bit | ScoreBit:
      case myP1Bit | PriorityBit:
      case myP1Bit | ScoreBit | PriorityBit:
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
      case myP0Bit | myP1Bit:
      case myP0Bit | myP1Bit | ScoreBit:
      case myP0Bit | myP1Bit | PriorityBit:
      case myP0Bit | myP1Bit | ScoreBit | PriorityBit:
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
              myCollision |= ourCollisionTable[myP0Bit | myP1Bit];

            ++mP0; ++mP1; ++myFramePointer;
          }
        }
        break;
      }

      // Missle 0 is enabled
      case myM0Bit:
      case myM0Bit | ScoreBit:
      case myM0Bit | PriorityBit:
      case myM0Bit | ScoreBit | PriorityBit:
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
      case myM1Bit:
      case myM1Bit | ScoreBit:
      case myM1Bit | PriorityBit:
      case myM1Bit | ScoreBit | PriorityBit:
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
      case myBLBit:
      case myBLBit | ScoreBit:
      case myBLBit | PriorityBit:
      case myBLBit | ScoreBit | PriorityBit:
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
      case myM0Bit | myM1Bit:
      case myM0Bit | myM1Bit | ScoreBit:
      case myM0Bit | myM1Bit | PriorityBit:
      case myM0Bit | myM1Bit | ScoreBit | PriorityBit:
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
              myCollision |= ourCollisionTable[myM0Bit | myM1Bit];

            ++mM0; ++mM1; ++myFramePointer;
          }
        }
        break;
      }

      // Ball and Missle 0 are enabled and playfield priority is not set
      case myBLBit | myM0Bit:
      case myBLBit | myM0Bit | ScoreBit:
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
              myCollision |= ourCollisionTable[myBLBit | myM0Bit];

            ++mBL; ++mM0; ++myFramePointer;
          }
        }
        break;
      }

      // Ball and Missle 0 are enabled and playfield priority is set
      case myBLBit | myM0Bit | PriorityBit:
      case myBLBit | myM0Bit | ScoreBit | PriorityBit:
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
              myCollision |= ourCollisionTable[myBLBit | myM0Bit];

            ++mBL; ++mM0; ++myFramePointer;
          }
        }
        break;
      }

      // Ball and Missle 1 are enabled and playfield priority is not set
      case myBLBit | myM1Bit:
      case myBLBit | myM1Bit | ScoreBit:
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
              myCollision |= ourCollisionTable[myBLBit | myM1Bit];

            ++mBL; ++mM1; ++myFramePointer;
          }
        }
        break;
      }

      // Ball and Missle 1 are enabled and playfield priority is set
      case myBLBit | myM1Bit | PriorityBit:
      case myBLBit | myM1Bit | ScoreBit | PriorityBit:
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
              myCollision |= ourCollisionTable[myBLBit | myM1Bit];

            ++mBL; ++mM1; ++myFramePointer;
          }
        }
        break;
      }

      // Ball and Player 1 are enabled and playfield priority is not set
      case myBLBit | myP1Bit:
      case myBLBit | myP1Bit | ScoreBit:
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
              myCollision |= ourCollisionTable[myBLBit | myP1Bit];

            ++mBL; ++mP1; ++myFramePointer;
          }
        }
        break;
      }

      // Ball and Player 1 are enabled and playfield priority is set
      case myBLBit | myP1Bit | PriorityBit:
      case myBLBit | myP1Bit | PriorityBit | ScoreBit:
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
              myCollision |= ourCollisionTable[myBLBit | myP1Bit];

            ++mBL; ++mP1; ++myFramePointer;
          }
        }
        break;
      }

      // Playfield and Player 0 are enabled and playfield priority is not set
      case myPFBit | myP0Bit:
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
              myCollision |= ourCollisionTable[myPFBit | myP0Bit];

            ++mPF; ++mP0; ++myFramePointer;
          }
        }

        break;
      }

      // Playfield and Player 0 are enabled and playfield priority is set
      case myPFBit | myP0Bit | PriorityBit:
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
              myCollision |= ourCollisionTable[myPFBit | myP0Bit];

            ++mPF; ++mP0; ++myFramePointer;
          }
        }

        break;
      }

      // Playfield and Player 1 are enabled and playfield priority is not set
      case myPFBit | myP1Bit:
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
              myCollision |= ourCollisionTable[myPFBit | myP1Bit];

            ++mPF; ++mP1; ++myFramePointer;
          }
        }

        break;
      }

      // Playfield and Player 1 are enabled and playfield priority is set
      case myPFBit | myP1Bit | PriorityBit:
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
              myCollision |= ourCollisionTable[myPFBit | myP1Bit];

            ++mPF; ++mP1; ++myFramePointer;
          }
        }

        break;
      }

      // Playfield and Ball are enabled
      case myPFBit | myBLBit:
      case myPFBit | myBLBit | PriorityBit:
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
              myCollision |= ourCollisionTable[myPFBit | myBLBit];

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
          uInt8 enabled = (myPF & myCurrentPFMask[hpos]) ? myPFBit : 0;

          if((myEnabledObjects & myBLBit) && myCurrentBLMask[hpos])
            enabled |= myBLBit;

          if(myCurrentGRP1 & myCurrentP1Mask[hpos])
            enabled |= myP1Bit;

          if((myEnabledObjects & myM1Bit) && myCurrentM1Mask[hpos])
            enabled |= myM1Bit;

          if(myCurrentGRP0 & myCurrentP0Mask[hpos])
            enabled |= myP0Bit;

          if((myEnabledObjects & myM0Bit) && myCurrentM0Mask[hpos])
            enabled |= myM0Bit;

          myCollision |= ourCollisionTable[enabled];
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
inline void TIA::updateFrame(Int32 clock)
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

    Int32 startOfScanLine = HBLANK + myFrameXStart;

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
      myFramePointer -= (160 - myFrameWidth - myFrameXStart);

      // Yes, so set PF mask based on current CTRLPF reflection state 
      myCurrentPFMask = ourPlayfieldTable[myCTRLPF & 0x01];

      // TODO: These should be reset right after the first copy of the player
      // has passed.  However, for now we'll just reset at the end of the 
      // scanline since the other way would be to slow (01/21/99).
      myCurrentP0Mask = &ourPlayerMaskTable[myPOSP0 & 0x03]
          [0][myNUSIZ0 & 0x07][160 - (myPOSP0 & 0xFC)];
      myCurrentP1Mask = &ourPlayerMaskTable[myPOSP1 & 0x03]
          [0][myNUSIZ1 & 0x07][160 - (myPOSP1 & 0xFC)];

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
          myCurrentM0Mask = &ourMissleMaskTable[myPOSM0 & 0x03]
              [myNUSIZ0 & 0x07][((myNUSIZ0 & 0x30) >> 4) | 0x01]
              [160 - (myPOSM0 & 0xFC)];
        }
        else if(myM0CosmicArkCounter == 2)
        {
          // Missle is disabled on this line 
          myCurrentM0Mask = &ourDisabledMaskTable[0];
        }
        else
        {
          myCurrentM0Mask = &ourMissleMaskTable[myPOSM0 & 0x03]
              [myNUSIZ0 & 0x07][(myNUSIZ0 & 0x30) >> 4][160 - (myPOSM0 & 0xFC)];
        }
      } 
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
	/*
  for(int s = scanlines() + 1; s < 300; s++)
    for(int i = 0; i < 160; i++)
      myCurrentFrameBuffer[s * 160 + i] = 0;
		*/
  for(int s = scanlines(); s < 300; s++)
	  for(int i = 0; i < 160; i++) {
		  uInt8 tmp = myCurrentFrameBuffer[s * 160 + i] & 0x0f;
		  tmp >>= 1;
		  myCurrentFrameBuffer[s * 160 + i] = tmp;
	  }

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIA::clearBuffers()
{
  for(uInt32 i = 0; i < 160 * 300; ++i)
  {
    myCurrentFrameBuffer[i] = myPreviousFrameBuffer[i] = 0;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 TIA::peek(uInt16 addr)
{
  // Update frame to current color clock before we look at anything!
  updateFrame(mySystem->cycles() * 3);

  uInt8 noise = mySystem->getDataBusState() & 0x3F;

  switch(addr & 0x000f)
  {
    case 0x00:    // CXM0P
      return ((myCollision & 0x0001) ? 0x80 : 0x00) | 
          ((myCollision & 0x0002) ? 0x40 : 0x00) | noise;

    case 0x01:    // CXM1P
      return ((myCollision & 0x0004) ? 0x80 : 0x00) | 
          ((myCollision & 0x0008) ? 0x40 : 0x00) | noise;

    case 0x02:    // CXP0FB
      return ((myCollision & 0x0010) ? 0x80 : 0x00) | 
          ((myCollision & 0x0020) ? 0x40 : 0x00) | noise;

    case 0x03:    // CXP1FB
      return ((myCollision & 0x0040) ? 0x80 : 0x00) | 
          ((myCollision & 0x0080) ? 0x40 : 0x00) | noise;

    case 0x04:    // CXM0FB
      return ((myCollision & 0x0100) ? 0x80 : 0x00) | 
          ((myCollision & 0x0200) ? 0x40 : 0x00) | noise;

    case 0x05:    // CXM1FB
      return ((myCollision & 0x0400) ? 0x80 : 0x00) | 
          ((myCollision & 0x0800) ? 0x40 : 0x00) | noise;

    case 0x06:    // CXBLPF
      return ((myCollision & 0x1000) ? 0x80 : 0x00) | noise;

    case 0x07:    // CXPPMM
      return ((myCollision & 0x2000) ? 0x80 : 0x00) | 
          ((myCollision & 0x4000) ? 0x40 : 0x00) | noise;

    case 0x08:    // INPT0
    {
      Int32 r = myConsole.controller(Controller::Left).read(Controller::Nine);
      if(r == Controller::minimumResistance)
      {
        return 0x80 | noise;
      }
      else if((r == Controller::maximumResistance) || myDumpEnabled)
      {
        return noise;
      }
      else
      {
        double t = (1.6 * r * 0.01E-6);
        uInt32 needed = (uInt32)(t * 1.19E6);
        if(mySystem->cycles() > (myDumpDisabledCycle + needed))
        {
          return 0x80 | noise;
        }
        else
        {
          return noise;
        }
      }
    }

    case 0x09:    // INPT1
    {
      Int32 r = myConsole.controller(Controller::Left).read(Controller::Five);
      if(r == Controller::minimumResistance)
      {
        return 0x80 | noise;
      }
      else if((r == Controller::maximumResistance) || myDumpEnabled)
      {
        return noise;
      }
      else
      {
        double t = (1.6 * r * 0.01E-6);
        uInt32 needed = (uInt32)(t * 1.19E6);
        if(mySystem->cycles() > (myDumpDisabledCycle + needed))
        {
          return 0x80 | noise;
        }
        else
        {
          return noise;
        }
      }
    }

    case 0x0A:    // INPT2
    {
      Int32 r = myConsole.controller(Controller::Right).read(Controller::Nine);
      if(r == Controller::minimumResistance)
      {
        return 0x80 | noise;
      }
      else if((r == Controller::maximumResistance) || myDumpEnabled)
      {
        return noise;
      }
      else
      {
        double t = (1.6 * r * 0.01E-6);
        uInt32 needed = (uInt32)(t * 1.19E6);
        if(mySystem->cycles() > (myDumpDisabledCycle + needed))
        {
          return 0x80 | noise;
        }
        else
        {
          return noise;
        }
      }
    }

    case 0x0B:    // INPT3
    {
      Int32 r = myConsole.controller(Controller::Right).read(Controller::Five);
      if(r == Controller::minimumResistance)
      {
        return 0x80 | noise;
      }
      else if((r == Controller::maximumResistance) || myDumpEnabled)
      {
        return noise;
      }
      else
      {
        double t = (1.6 * r * 0.01E-6);
        uInt32 needed = (uInt32)(t * 1.19E6);
        if(mySystem->cycles() > (myDumpDisabledCycle + needed))
        {
          return 0x80 | noise;
        }
        else
        {
          return noise;
        }
      }
    }

    case 0x0C:    // INPT4
      return myConsole.controller(Controller::Left).read(Controller::Six) ?
          (0x80 | noise) : noise;

    case 0x0D:    // INPT5
      return myConsole.controller(Controller::Right).read(Controller::Six) ?
          (0x80 | noise) : noise;

    case 0x0e:
      return noise;

    default:
      return noise;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIA::poke(uInt16 addr, uInt8 value)
{
  addr = addr & 0x003f;

  Int32 clock = mySystem->cycles() * 3;
  Int16 delay = ourPokeDelayTable[addr];

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
    case 0x00:    // Vertical sync set-clear
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

    case 0x01:    // Vertical blank set-clear
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

    case 0x02:    // Wait for leading edge of HBLANK
    {
      // Tell the cpu to waste the necessary amount of time
      waitHorizontalSync();
      break;
    }

    case 0x03:    // Reset horizontal sync counter
    {
//      cerr << "TIA Poke: " << hex << addr << endl;
      break;
    }

    case 0x04:    // Number-size of player-missle 0
    {
      myNUSIZ0 = value;

      // TODO: Technically the "enable" part, [0], should depend on the current
      // enabled or disabled state.  This mean we probably need a data member
      // to maintain that state (01/21/99).
      myCurrentP0Mask = &ourPlayerMaskTable[myPOSP0 & 0x03]
          [0][myNUSIZ0 & 0x07][160 - (myPOSP0 & 0xFC)];

      myCurrentM0Mask = &ourMissleMaskTable[myPOSM0 & 0x03]
          [myNUSIZ0 & 0x07][(myNUSIZ0 & 0x30) >> 4][160 - (myPOSM0 & 0xFC)];

      break;
    }

    case 0x05:    // Number-size of player-missle 1
    {
      myNUSIZ1 = value;

      // TODO: Technically the "enable" part, [0], should depend on the current
      // enabled or disabled state.  This mean we probably need a data member
      // to maintain that state (01/21/99).
      myCurrentP1Mask = &ourPlayerMaskTable[myPOSP1 & 0x03]
          [0][myNUSIZ1 & 0x07][160 - (myPOSP1 & 0xFC)];

      myCurrentM1Mask = &ourMissleMaskTable[myPOSM1 & 0x03]
          [myNUSIZ1 & 0x07][(myNUSIZ1 & 0x30) >> 4][160 - (myPOSM1 & 0xFC)];

      break;
    }

    case 0x06:    // Color-Luminance Player 0
    {
      uInt32 color = (uInt32)(value & 0xfe);
      if(myColorLossEnabled && (myScanlineCountForLastFrame & 0x01))
      {
        color |= 0x01;
      }
      myCOLUP0 = (((((color << 8) | color) << 8) | color) << 8) | color;
      break;
    }

    case 0x07:    // Color-Luminance Player 1
    {
      uInt32 color = (uInt32)(value & 0xfe);
      if(myColorLossEnabled && (myScanlineCountForLastFrame & 0x01))
      {
        color |= 0x01;
      }
      myCOLUP1 = (((((color << 8) | color) << 8) | color) << 8) | color;
      break;
    }

    case 0x08:    // Color-Luminance Playfield
    {
      uInt32 color = (uInt32)(value & 0xfe);
      if(myColorLossEnabled && (myScanlineCountForLastFrame & 0x01))
      {
        color |= 0x01;
      }
      myCOLUPF = (((((color << 8) | color) << 8) | color) << 8) | color;
      break;
    }

    case 0x09:    // Color-Luminance Background
    {
      uInt32 color = (uInt32)(value & 0xfe);
      if(myColorLossEnabled && (myScanlineCountForLastFrame & 0x01))
      {
        color |= 0x01;
      }
      myCOLUBK = (((((color << 8) | color) << 8) | color) << 8) | color;
      break;
    }

    case 0x0A:    // Control Playfield, Ball size, Collisions
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
        myCurrentPFMask = ourPlayfieldTable[myCTRLPF & 0x01];
      }

      myCurrentBLMask = &ourBallMaskTable[myPOSBL & 0x03]
          [(myCTRLPF & 0x30) >> 4][160 - (myPOSBL & 0xFC)];

      break;
    }

    case 0x0B:    // Reflect Player 0
    {
      // See if the reflection state of the player is being changed
      if(((value & 0x08) && !myREFP0) || (!(value & 0x08) && myREFP0))
      {
        myREFP0 = (value & 0x08);
        myCurrentGRP0 = ourPlayerReflectTable[myCurrentGRP0];
      }
      break;
    }

    case 0x0C:    // Reflect Player 1
    {
      // See if the reflection state of the player is being changed
      if(((value & 0x08) && !myREFP1) || (!(value & 0x08) && myREFP1))
      {
        myREFP1 = (value & 0x08);
        myCurrentGRP1 = ourPlayerReflectTable[myCurrentGRP1];
      }
      break;
    }

    case 0x0D:    // Playfield register byte 0
    {
      myPF = (myPF & 0x000FFFF0) | ((value >> 4) & 0x0F);

      if(!myBitEnabled[TIA::PF] || myPF == 0)
        myEnabledObjects &= ~myPFBit;
      else
        myEnabledObjects |= myPFBit;

      break;
    }

    case 0x0E:    // Playfield register byte 1
    {
      myPF = (myPF & 0x000FF00F) | ((uInt32)value << 4);

      if(!myBitEnabled[TIA::PF] || myPF == 0)
        myEnabledObjects &= ~myPFBit;
      else
        myEnabledObjects |= myPFBit;

      break;
    }

    case 0x0F:    // Playfield register byte 2
    {
      myPF = (myPF & 0x00000FFF) | ((uInt32)value << 12);

      if(!myBitEnabled[TIA::PF] || myPF == 0)
        myEnabledObjects &= ~myPFBit;
      else
        myEnabledObjects |= myPFBit;

      break;
    }

    case 0x10:    // Reset Player 0
    {
      Int32 hpos = (clock - myClockWhenFrameStarted) % 228;
      Int32 newx = hpos < HBLANK ? 3 : (((hpos - HBLANK) + 5) % 160);

      // Find out under what condition the player is being reset
      Int8 when = ourPlayerPositionResetWhenTable[myNUSIZ0 & 7][myPOSP0][newx];

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
        myCurrentP0Mask = &ourPlayerMaskTable[myPOSP0 & 0x03]
            [1][myNUSIZ0 & 0x07][160 - (myPOSP0 & 0xFC)];
      }
      // Player is being reset in neither the delay nor display section
      else if(when == 0)
      {
        myPOSP0 = newx;

        // So we setup the mask to skip the first copy of the player
        myCurrentP0Mask = &ourPlayerMaskTable[myPOSP0 & 0x03]
            [1][myNUSIZ0 & 0x07][160 - (myPOSP0 & 0xFC)];
      }
      // Player is being reset during the delay section of one of its copies
      else if(when == -1)
      {
        myPOSP0 = newx;

        // So we setup the mask to display all copies of the player
        myCurrentP0Mask = &ourPlayerMaskTable[myPOSP0 & 0x03]
            [0][myNUSIZ0 & 0x07][160 - (myPOSP0 & 0xFC)];
      }
      break;
    }

    case 0x11:    // Reset Player 1
    {
      Int32 hpos = (clock - myClockWhenFrameStarted) % 228;
      Int32 newx = hpos < HBLANK ? 3 : (((hpos - HBLANK) + 5) % 160);

      // Find out under what condition the player is being reset
      Int8 when = ourPlayerPositionResetWhenTable[myNUSIZ1 & 7][myPOSP1][newx];

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
        myCurrentP1Mask = &ourPlayerMaskTable[myPOSP1 & 0x03]
            [1][myNUSIZ1 & 0x07][160 - (myPOSP1 & 0xFC)];
      }
      // Player is being reset in neither the delay nor display section
      else if(when == 0)
      {
        myPOSP1 = newx;

        // So we setup the mask to skip the first copy of the player
        myCurrentP1Mask = &ourPlayerMaskTable[myPOSP1 & 0x03]
            [1][myNUSIZ1 & 0x07][160 - (myPOSP1 & 0xFC)];
      }
      // Player is being reset during the delay section of one of its copies
      else if(when == -1)
      {
        myPOSP1 = newx;

        // So we setup the mask to display all copies of the player
        myCurrentP1Mask = &ourPlayerMaskTable[myPOSP1 & 0x03]
            [0][myNUSIZ1 & 0x07][160 - (myPOSP1 & 0xFC)];
      }
      break;
    }

    case 0x12:    // Reset Missle 0
    {
      int hpos = (clock - myClockWhenFrameStarted) % 228;
      myPOSM0 = hpos < HBLANK ? 2 : (((hpos - HBLANK) + 4) % 160);

      // TODO: Remove the following special hack for Dolphin by
      // figuring out what really happens when Reset Missle 
      // occurs 20 cycles after an HMOVE (04/13/02).
      if(((clock - myLastHMOVEClock) == (20 * 3)) && (hpos == 69))
      {
        myPOSM0 = 8;
      }
 
      myCurrentM0Mask = &ourMissleMaskTable[myPOSM0 & 0x03]
          [myNUSIZ0 & 0x07][(myNUSIZ0 & 0x30) >> 4][160 - (myPOSM0 & 0xFC)];
      break;
    }

    case 0x13:    // Reset Missle 1
    {
      int hpos = (clock - myClockWhenFrameStarted) % 228;
      myPOSM1 = hpos < HBLANK ? 2 : (((hpos - HBLANK) + 4) % 160);

      // TODO: Remove the following special hack for Pitfall II by
      // figuring out what really happens when Reset Missle 
      // occurs 3 cycles after an HMOVE (04/13/02).
      if(((clock - myLastHMOVEClock) == (3 * 3)) && (hpos == 18))
      {
        myPOSM1 = 3;
      }
 
      myCurrentM1Mask = &ourMissleMaskTable[myPOSM1 & 0x03]
          [myNUSIZ1 & 0x07][(myNUSIZ1 & 0x30) >> 4][160 - (myPOSM1 & 0xFC)];
      break;
    }

    case 0x14:    // Reset Ball
    {
      int hpos = (clock - myClockWhenFrameStarted) % 228 ;
      myPOSBL = hpos < HBLANK ? 2 : (((hpos - HBLANK) + 4) % 160);

      // TODO: Remove the following special hack for Escape from the 
      // Mindmaster by figuring out what really happens when Reset Ball 
      // occurs 18 cycles after an HMOVE (01/09/99).
      if(((clock - myLastHMOVEClock) == (18 * 3)) && 
          ((hpos == 60) || (hpos == 69)))
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
 
      myCurrentBLMask = &ourBallMaskTable[myPOSBL & 0x03]
          [(myCTRLPF & 0x30) >> 4][160 - (myPOSBL & 0xFC)];
      break;
    }

    case 0x15:    // Audio control 0
    {
      myAUDC0 = value & 0x0f;
      mySound->set(addr, value, mySystem->cycles());
      break;
    }
  
    case 0x16:    // Audio control 1
    {
      myAUDC1 = value & 0x0f;
      mySound->set(addr, value, mySystem->cycles());
      break;
    }
  
    case 0x17:    // Audio frequency 0
    {
      myAUDF0 = value & 0x1f;
      mySound->set(addr, value, mySystem->cycles());
      break;
    }
  
    case 0x18:    // Audio frequency 1
    {
      myAUDF1 = value & 0x1f;
      mySound->set(addr, value, mySystem->cycles());
      break;
    }
  
    case 0x19:    // Audio volume 0
    {
      myAUDV0 = value & 0x0f;
      mySound->set(addr, value, mySystem->cycles());
      break;
    }
  
    case 0x1A:    // Audio volume 1
    {
      myAUDV1 = value & 0x0f;
      mySound->set(addr, value, mySystem->cycles());
      break;
    }

    case 0x1B:    // Graphics Player 0
    {
      // Set player 0 graphics
      myGRP0 = (myBitEnabled[TIA::P0] ? value : 0);

      // Copy player 1 graphics into its delayed register
      myDGRP1 = myGRP1;

      // Get the "current" data for GRP0 base on delay register and reflect
      uInt8 grp0 = myVDELP0 ? myDGRP0 : myGRP0;
      myCurrentGRP0 = myREFP0 ? ourPlayerReflectTable[grp0] : grp0; 

      // Get the "current" data for GRP1 base on delay register and reflect
      uInt8 grp1 = myVDELP1 ? myDGRP1 : myGRP1;
      myCurrentGRP1 = myREFP1 ? ourPlayerReflectTable[grp1] : grp1; 

      // Set enabled object bits
      if(myCurrentGRP0 != 0)
        myEnabledObjects |= myP0Bit;
      else
        myEnabledObjects &= ~myP0Bit;

      if(myCurrentGRP1 != 0)
        myEnabledObjects |= myP1Bit;
      else
        myEnabledObjects &= ~myP1Bit;

      break;
    }

    case 0x1C:    // Graphics Player 1
    {
      // Set player 1 graphics
      myGRP1 = (myBitEnabled[TIA::P1] ? value : 0);

      // Copy player 0 graphics into its delayed register
      myDGRP0 = myGRP0;

      // Copy ball graphics into its delayed register
      myDENABL = myENABL;

      // Get the "current" data for GRP0 base on delay register
      uInt8 grp0 = myVDELP0 ? myDGRP0 : myGRP0;
      myCurrentGRP0 = myREFP0 ? ourPlayerReflectTable[grp0] : grp0; 

      // Get the "current" data for GRP1 base on delay register
      uInt8 grp1 = myVDELP1 ? myDGRP1 : myGRP1;
      myCurrentGRP1 = myREFP1 ? ourPlayerReflectTable[grp1] : grp1; 

      // Set enabled object bits
      if(myCurrentGRP0 != 0)
        myEnabledObjects |= myP0Bit;
      else
        myEnabledObjects &= ~myP0Bit;

      if(myCurrentGRP1 != 0)
        myEnabledObjects |= myP1Bit;
      else
        myEnabledObjects &= ~myP1Bit;

      if(myVDELBL ? myDENABL : myENABL)
        myEnabledObjects |= myBLBit;
      else
        myEnabledObjects &= ~myBLBit;

      break;
    }

    case 0x1D:    // Enable Missile 0 graphics
    {
      myENAM0 = (myBitEnabled[TIA::M0] ? value & 0x02 : 0);

      if(myENAM0 && !myRESMP0)
        myEnabledObjects |= myM0Bit;
      else
        myEnabledObjects &= ~myM0Bit;
      break;
    }

    case 0x1E:    // Enable Missile 1 graphics
    {
      myENAM1 = (myBitEnabled[TIA::M1] ? value & 0x02 : 0);

      if(myENAM1 && !myRESMP1)
        myEnabledObjects |= myM1Bit;
      else
        myEnabledObjects &= ~myM1Bit;
      break;
    }

    case 0x1F:    // Enable Ball graphics
    {
      myENABL = (myBitEnabled[TIA::BL] ? value & 0x02 : 0);

      if(myVDELBL ? myDENABL : myENABL)
        myEnabledObjects |= myBLBit;
      else
        myEnabledObjects &= ~myBLBit;

      break;
    }

    case 0x20:    // Horizontal Motion Player 0
    {
      myHMP0 = value >> 4;
      break;
    }

    case 0x21:    // Horizontal Motion Player 1
    {
      myHMP1 = value >> 4;
      break;
    }

    case 0x22:    // Horizontal Motion Missle 0
    {
      Int8 tmp = value >> 4;

      // Should we enabled TIA M0 "bug" used for stars in Cosmic Ark?
      if((clock == (myLastHMOVEClock + 21 * 3)) && (myHMM0 == 7) && (tmp == 6))
      {
        myM0CosmicArkMotionEnabled = true;
        myM0CosmicArkCounter = 0;
      }

      myHMM0 = tmp;
      break;
    }

    case 0x23:    // Horizontal Motion Missle 1
    {
      myHMM1 = value >> 4;
      break;
    }

    case 0x24:    // Horizontal Motion Ball
    {
      myHMBL = value >> 4;
      break;
    }

    case 0x25:    // Vertial Delay Player 0
    {
      myVDELP0 = value & 0x01;

      uInt8 grp0 = myVDELP0 ? myDGRP0 : myGRP0;
      myCurrentGRP0 = myREFP0 ? ourPlayerReflectTable[grp0] : grp0; 

      if(myCurrentGRP0 != 0)
        myEnabledObjects |= myP0Bit;
      else
        myEnabledObjects &= ~myP0Bit;
      break;
    }

    case 0x26:    // Vertial Delay Player 1
    {
      myVDELP1 = value & 0x01;

      uInt8 grp1 = myVDELP1 ? myDGRP1 : myGRP1;
      myCurrentGRP1 = myREFP1 ? ourPlayerReflectTable[grp1] : grp1; 

      if(myCurrentGRP1 != 0)
        myEnabledObjects |= myP1Bit;
      else
        myEnabledObjects &= ~myP1Bit;
      break;
    }

    case 0x27:    // Vertial Delay Ball
    {
      myVDELBL = value & 0x01;

      if(myVDELBL ? myDENABL : myENABL)
        myEnabledObjects |= myBLBit;
      else
        myEnabledObjects &= ~myBLBit;
      break;
    }

    case 0x28:    // Reset missle 0 to player 0
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
        myCurrentM0Mask = &ourMissleMaskTable[myPOSM0 & 0x03]
            [myNUSIZ0 & 0x07][(myNUSIZ0 & 0x30) >> 4][160 - (myPOSM0 & 0xFC)];
      }

      myRESMP0 = value & 0x02;

      if(myENAM0 && !myRESMP0)
        myEnabledObjects |= myM0Bit;
      else
        myEnabledObjects &= ~myM0Bit;

      break;
    }

    case 0x29:    // Reset missle 1 to player 1
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
        myCurrentM1Mask = &ourMissleMaskTable[myPOSM1 & 0x03]
            [myNUSIZ1 & 0x07][(myNUSIZ1 & 0x30) >> 4][160 - (myPOSM1 & 0xFC)];
      }

      myRESMP1 = value & 0x02;

      if(myENAM1 && !myRESMP1)
        myEnabledObjects |= myM1Bit;
      else
        myEnabledObjects &= ~myM1Bit;
      break;
    }

    case 0x2A:    // Apply horizontal motion
    {
      // Figure out what cycle we're at
      Int32 x = ((clock - myClockWhenFrameStarted) % 228) / 3;

      // See if we need to enable the HMOVE blank bug
      if(myAllowHMOVEBlanks && ourHMOVEBlankEnableCycles[x])
      {
        // TODO: Allow this to be turned off using properties...
        myHMOVEBlankEnabled = true;
      }

      myPOSP0 += ourCompleteMotionTable[x][myHMP0];
      myPOSP1 += ourCompleteMotionTable[x][myHMP1];
      myPOSM0 += ourCompleteMotionTable[x][myHMM0];
      myPOSM1 += ourCompleteMotionTable[x][myHMM1];
      myPOSBL += ourCompleteMotionTable[x][myHMBL];

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

      myCurrentBLMask = &ourBallMaskTable[myPOSBL & 0x03]
          [(myCTRLPF & 0x30) >> 4][160 - (myPOSBL & 0xFC)];

      myCurrentP0Mask = &ourPlayerMaskTable[myPOSP0 & 0x03]
          [0][myNUSIZ0 & 0x07][160 - (myPOSP0 & 0xFC)];
      myCurrentP1Mask = &ourPlayerMaskTable[myPOSP1 & 0x03]
          [0][myNUSIZ1 & 0x07][160 - (myPOSP1 & 0xFC)];

      myCurrentM0Mask = &ourMissleMaskTable[myPOSM0 & 0x03]
          [myNUSIZ0 & 0x07][(myNUSIZ0 & 0x30) >> 4][160 - (myPOSM0 & 0xFC)];
      myCurrentM1Mask = &ourMissleMaskTable[myPOSM1 & 0x03]
          [myNUSIZ1 & 0x07][(myNUSIZ1 & 0x30) >> 4][160 - (myPOSM1 & 0xFC)];

      // Remember what clock HMOVE occured at
      myLastHMOVEClock = clock;

      // Disable TIA M0 "bug" used for stars in Cosmic ark
      myM0CosmicArkMotionEnabled = false;
      break;
    }

    case 0x2b:    // Clear horizontal motion registers
    {
      myHMP0 = 0;
      myHMP1 = 0;
      myHMM0 = 0;
      myHMM1 = 0;
      myHMBL = 0;
      break;
    }

    case 0x2c:    // Clear collision latches
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
uInt8 TIA::ourBallMaskTable[4][4][320];

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt16 TIA::ourCollisionTable[64];

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 TIA::ourDisabledMaskTable[640];

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const Int16 TIA::ourPokeDelayTable[64] = {
   0,  1,  0,  0,  8,  8,  0,  0,  0,  0,  0,  1,  1, -1, -1, -1,
   0,  0,  8,  8,  8,  0,  0,  0,  0,  0,  0,  1,  1,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 TIA::ourMissleMaskTable[4][8][4][320];

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const bool TIA::ourHMOVEBlankEnableCycles[76] = {
  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,   // 00
  true,  true,  true,  true,  true,  true,  true,  true,  true,  true,   // 10
  true,  false, false, false, false, false, false, false, false, false,  // 20
  false, false, false, false, false, false, false, false, false, false,  // 30
  false, false, false, false, false, false, false, false, false, false,  // 40
  false, false, false, false, false, false, false, false, false, false,  // 50
  false, false, false, false, false, false, false, false, false, false,  // 60
  false, false, false, false, false, true                                // 70
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const Int32 TIA::ourCompleteMotionTable[76][16] = {
  { 0, -1, -2, -3, -4, -5, -6, -7,  8,  7,  6,  5,  4,  3,  2,  1}, // HBLANK
  { 0, -1, -2, -3, -4, -5, -6, -7,  8,  7,  6,  5,  4,  3,  2,  1}, // HBLANK
  { 0, -1, -2, -3, -4, -5, -6, -7,  8,  7,  6,  5,  4,  3,  2,  1}, // HBLANK
  { 0, -1, -2, -3, -4, -5, -6, -7,  8,  7,  6,  5,  4,  3,  2,  1}, // HBLANK
  { 0, -1, -2, -3, -4, -5, -6, -6,  8,  7,  6,  5,  4,  3,  2,  1}, // HBLANK
  { 0, -1, -2, -3, -4, -5, -5, -5,  8,  7,  6,  5,  4,  3,  2,  1}, // HBLANK
  { 0, -1, -2, -3, -4, -5, -5, -5,  8,  7,  6,  5,  4,  3,  2,  1}, // HBLANK
  { 0, -1, -2, -3, -4, -4, -4, -4,  8,  7,  6,  5,  4,  3,  2,  1}, // HBLANK
  { 0, -1, -2, -3, -3, -3, -3, -3,  8,  7,  6,  5,  4,  3,  2,  1}, // HBLANK
  { 0, -1, -2, -2, -2, -2, -2, -2,  8,  7,  6,  5,  4,  3,  2,  1}, // HBLANK
  { 0, -1, -2, -2, -2, -2, -2, -2,  8,  7,  6,  5,  4,  3,  2,  1}, // HBLANK
  { 0, -1, -1, -1, -1, -1, -1, -1,  8,  7,  6,  5,  4,  3,  2,  1}, // HBLANK
  { 0,  0,  0,  0,  0,  0,  0,  0,  8,  7,  6,  5,  4,  3,  2,  1}, // HBLANK
  { 1,  1,  1,  1,  1,  1,  1,  1,  8,  7,  6,  5,  4,  3,  2,  1}, // HBLANK
  { 1,  1,  1,  1,  1,  1,  1,  1,  8,  7,  6,  5,  4,  3,  2,  1}, // HBLANK
  { 2,  2,  2,  2,  2,  2,  2,  2,  8,  7,  6,  5,  4,  3,  2,  2}, // HBLANK
  { 3,  3,  3,  3,  3,  3,  3,  3,  8,  7,  6,  5,  4,  3,  3,  3}, // HBLANK
  { 4,  4,  4,  4,  4,  4,  4,  4,  8,  7,  6,  5,  4,  4,  4,  4}, // HBLANK
  { 4,  4,  4,  4,  4,  4,  4,  4,  8,  7,  6,  5,  4,  4,  4,  4}, // HBLANK
  { 5,  5,  5,  5,  5,  5,  5,  5,  8,  7,  6,  5,  5,  5,  5,  5}, // HBLANK
  { 6,  6,  6,  6,  6,  6,  6,  6,  8,  7,  6,  6,  6,  6,  6,  6}, // HBLANK
  { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},    
  { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},    
  { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},    
  { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},    
  { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},    
  { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},    
  { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},    
  { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},    
  { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},    
  { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},    
  { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},    
  { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},    
  { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},    
  { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},    
  { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},    
  { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},    
  { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},    
  { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},    
  { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},    
  { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},    
  { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},    
  { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},    
  { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},    
  { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},    
  { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},    
  { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},    
  { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},    
  { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},    
  { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},    
  { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},    
  { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},    
  { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},    
  { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},    
  { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},    
  { 0,  0,  0,  0,  0,  0,  0, -1,  0,  0,  0,  0,  0,  0,  0,  0},    
  { 0,  0,  0,  0,  0,  0, -1, -2,  0,  0,  0,  0,  0,  0,  0,  0},    
  { 0,  0,  0,  0,  0, -1, -2, -3,  0,  0,  0,  0,  0,  0,  0,  0},    
  { 0,  0,  0,  0,  0, -1, -2, -3,  0,  0,  0,  0,  0,  0,  0,  0},
  { 0,  0,  0,  0, -1, -2, -3, -4,  0,  0,  0,  0,  0,  0,  0,  0}, 
  { 0,  0,  0, -1, -2, -3, -4, -5,  0,  0,  0,  0,  0,  0,  0,  0},
  { 0,  0, -1, -2, -3, -4, -5, -6,  0,  0,  0,  0,  0,  0,  0,  0},
  { 0,  0, -1, -2, -3, -4, -5, -6,  0,  0,  0,  0,  0,  0,  0,  0},
  { 0, -1, -2, -3, -4, -5, -6, -7,  0,  0,  0,  0,  0,  0,  0,  0},
  {-1, -2, -3, -4, -5, -6, -7, -8,  0,  0,  0,  0,  0,  0,  0,  0},
  {-2, -3, -4, -5, -6, -7, -8, -9,  0,  0,  0,  0,  0,  0,  0, -1},
  {-2, -3, -4, -5, -6, -7, -8, -9,  0,  0,  0,  0,  0,  0,  0, -1},
  {-3, -4, -5, -6, -7, -8, -9,-10,  0,  0,  0,  0,  0,  0, -1, -2}, 
  {-4, -5, -6, -7, -8, -9,-10,-11,  0,  0,  0,  0,  0, -1, -2, -3},
  {-5, -6, -7, -8, -9,-10,-11,-12,  0,  0,  0,  0, -1, -2, -3, -4},
  {-5, -6, -7, -8, -9,-10,-11,-12,  0,  0,  0,  0, -1, -2, -3, -4},
  {-6, -7, -8, -9,-10,-11,-12,-13,  0,  0,  0, -1, -2, -3, -4, -5},
  {-7, -8, -9,-10,-11,-12,-13,-14,  0,  0, -1, -2, -3, -4, -5, -6},
  {-8, -9,-10,-11,-12,-13,-14,-15,  0, -1, -2, -3, -4, -5, -6, -7},
  {-8, -9,-10,-11,-12,-13,-14,-15,  0, -1, -2, -3, -4, -5, -6, -7},
  { 0, -1, -2, -3, -4, -5, -6, -7,  8,  7,  6,  5,  4,  3,  2,  1}  // HBLANK
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 TIA::ourPlayerMaskTable[4][2][8][320];

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Int8 TIA::ourPlayerPositionResetWhenTable[8][160][160];

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 TIA::ourPlayerReflectTable[256];

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 TIA::ourPlayfieldTable[2][160];

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const uInt32 TIA::ourNTSCPalette[256] = {
  0x000000, 0x000000, 0x4a4a4a, 0x4a4a4a,
  0x6f6f6f, 0x6f6f6f, 0x8e8e8e, 0x8e8e8e,
  0xaaaaaa, 0xaaaaaa, 0xc0c0c0, 0xc0c0c0,
  0xd6d6d6, 0xd6d6d6, 0xececec, 0xececec,

  0x484800, 0x484800, 0x69690f, 0x69690f,
  0x86861d, 0x86861d, 0xa2a22a, 0xa2a22a,
  0xbbbb35, 0xbbbb35, 0xd2d240, 0xd2d240,
  0xe8e84a, 0xe8e84a, 0xfcfc54, 0xfcfc54,

  0x7c2c00, 0x7c2c00, 0x904811, 0x904811,
  0xa26221, 0xa26221, 0xb47a30, 0xb47a30,
  0xc3903d, 0xc3903d, 0xd2a44a, 0xd2a44a,
  0xdfb755, 0xdfb755, 0xecc860, 0xecc860,

  0x901c00, 0x901c00, 0xa33915, 0xa33915,
  0xb55328, 0xb55328, 0xc66c3a, 0xc66c3a,
  0xd5824a, 0xd5824a, 0xe39759, 0xe39759,
  0xf0aa67, 0xf0aa67, 0xfcbc74, 0xfcbc74,

  0x940000, 0x940000, 0xa71a1a, 0xa71a1a,
  0xb83232, 0xb83232, 0xc84848, 0xc84848,
  0xd65c5c, 0xd65c5c, 0xe46f6f, 0xe46f6f,
  0xf08080, 0xf08080, 0xfc9090, 0xfc9090,

  0x840064, 0x840064, 0x97197a, 0x97197a,
  0xa8308f, 0xa8308f, 0xb846a2, 0xb846a2,
  0xc659b3, 0xc659b3, 0xd46cc3, 0xd46cc3,
  0xe07cd2, 0xe07cd2, 0xec8ce0, 0xec8ce0,

  0x500084, 0x500084, 0x68199a, 0x68199a,
  0x7d30ad, 0x7d30ad, 0x9246c0, 0x9246c0,
  0xa459d0, 0xa459d0, 0xb56ce0, 0xb56ce0,
  0xc57cee, 0xc57cee, 0xd48cfc, 0xd48cfc,

  0x140090, 0x140090, 0x331aa3, 0x331aa3,
  0x4e32b5, 0x4e32b5, 0x6848c6, 0x6848c6,
  0x7f5cd5, 0x7f5cd5, 0x956fe3, 0x956fe3,
  0xa980f0, 0xa980f0, 0xbc90fc, 0xbc90fc,

  0x000094, 0x000094, 0x181aa7, 0x181aa7,
  0x2d32b8, 0x2d32b8, 0x4248c8, 0x4248c8,
  0x545cd6, 0x545cd6, 0x656fe4, 0x656fe4,
  0x7580f0, 0x7580f0, 0x8490fc, 0x8490fc,

  0x001c88, 0x001c88, 0x183b9d, 0x183b9d,
  0x2d57b0, 0x2d57b0, 0x4272c2, 0x4272c2,
  0x548ad2, 0x548ad2, 0x65a0e1, 0x65a0e1,
  0x75b5ef, 0x75b5ef, 0x84c8fc, 0x84c8fc,

  0x003064, 0x003064, 0x185080, 0x185080,
  0x2d6d98, 0x2d6d98, 0x4288b0, 0x4288b0,
  0x54a0c5, 0x54a0c5, 0x65b7d9, 0x65b7d9,
  0x75cceb, 0x75cceb, 0x84e0fc, 0x84e0fc,

  0x004030, 0x004030, 0x18624e, 0x18624e,
  0x2d8169, 0x2d8169, 0x429e82, 0x429e82,
  0x54b899, 0x54b899, 0x65d1ae, 0x65d1ae,
  0x75e7c2, 0x75e7c2, 0x84fcd4, 0x84fcd4,

  0x004400, 0x004400, 0x1a661a, 0x1a661a,
  0x328432, 0x328432, 0x48a048, 0x48a048,
  0x5cba5c, 0x5cba5c, 0x6fd26f, 0x6fd26f,
  0x80e880, 0x80e880, 0x90fc90, 0x90fc90,

  0x143c00, 0x143c00, 0x355f18, 0x355f18,
  0x527e2d, 0x527e2d, 0x6e9c42, 0x6e9c42,
  0x87b754, 0x87b754, 0x9ed065, 0x9ed065,
  0xb4e775, 0xb4e775, 0xc8fc84, 0xc8fc84,

  0x303800, 0x303800, 0x505916, 0x505916,
  0x6d762b, 0x6d762b, 0x88923e, 0x88923e,
  0xa0ab4f, 0xa0ab4f, 0xb7c25f, 0xb7c25f,
  0xccd86e, 0xccd86e, 0xe0ec7c, 0xe0ec7c,

  0x482c00, 0x482c00, 0x694d14, 0x694d14,
  0x866a26, 0x866a26, 0xa28638, 0xa28638,
  0xbb9f47, 0xbb9f47, 0xd2b656, 0xd2b656,
  0xe8cc63, 0xe8cc63, 0xfce070, 0xfce070
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const uInt32 TIA::ourPALPalette[256] = {
  0x000000, 0x000000, 0x2b2b2b, 0x2b2b2b,
  0x525252, 0x525252, 0x767676, 0x767676,
  0x979797, 0x979797, 0xb6b6b6, 0xb6b6b6,
  0xd2d2d2, 0xd2d2d2, 0xececec, 0xececec,

  0x000000, 0x000000, 0x2b2b2b, 0x2b2b2b,
  0x525252, 0x525252, 0x767676, 0x767676,
  0x979797, 0x979797, 0xb6b6b6, 0xb6b6b6,
  0xd2d2d2, 0xd2d2d2, 0xececec, 0xececec,

  0x805800, 0x000000, 0x96711a, 0x2b2b2b,
  0xab8732, 0x525252, 0xbe9c48, 0x767676,
  0xcfaf5c, 0x979797, 0xdfc06f, 0xb6b6b6,
  0xeed180, 0xd2d2d2, 0xfce090, 0xececec,

  0x445c00, 0x000000, 0x5e791a, 0x2b2b2b,
  0x769332, 0x525252, 0x8cac48, 0x767676,
  0xa0c25c, 0x979797, 0xb3d76f, 0xb6b6b6,
  0xc4ea80, 0xd2d2d2, 0xd4fc90, 0xececec,

  0x703400, 0x000000, 0x89511a, 0x2b2b2b,
  0xa06b32, 0x525252, 0xb68448, 0x767676,
  0xc99a5c, 0x979797, 0xdcaf6f, 0xb6b6b6,
  0xecc280, 0xd2d2d2, 0xfcd490, 0xececec,

  0x006414, 0x000000, 0x1a8035, 0x2b2b2b,
  0x329852, 0x525252, 0x48b06e, 0x767676,
  0x5cc587, 0x979797, 0x6fd99e, 0xb6b6b6,
  0x80ebb4, 0xd2d2d2, 0x90fcc8, 0xececec,

  0x700014, 0x000000, 0x891a35, 0x2b2b2b,
  0xa03252, 0x525252, 0xb6486e, 0x767676,
  0xc95c87, 0x979797, 0xdc6f9e, 0xb6b6b6,
  0xec80b4, 0xd2d2d2, 0xfc90c8, 0xececec,

  0x005c5c, 0x000000, 0x1a7676, 0x2b2b2b,
  0x328e8e, 0x525252, 0x48a4a4, 0x767676,
  0x5cb8b8, 0x979797, 0x6fcbcb, 0xb6b6b6,
  0x80dcdc, 0xd2d2d2, 0x90ecec, 0xececec,

  0x70005c, 0x000000, 0x841a74, 0x2b2b2b,
  0x963289, 0x525252, 0xa8489e, 0x767676,
  0xb75cb0, 0x979797, 0xc66fc1, 0xb6b6b6,
  0xd380d1, 0xd2d2d2, 0xe090e0, 0xececec,

  0x003c70, 0x000000, 0x195a89, 0x2b2b2b,
  0x2f75a0, 0x525252, 0x448eb6, 0x767676,
  0x57a5c9, 0x979797, 0x68badc, 0xb6b6b6,
  0x79ceec, 0xd2d2d2, 0x88e0fc, 0xececec,

  0x580070, 0x000000, 0x6e1a89, 0x2b2b2b,
  0x8332a0, 0x525252, 0x9648b6, 0x767676,
  0xa75cc9, 0x979797, 0xb76fdc, 0xb6b6b6,
  0xc680ec, 0xd2d2d2, 0xd490fc, 0xececec,

  0x002070, 0x000000, 0x193f89, 0x2b2b2b,
  0x2f5aa0, 0x525252, 0x4474b6, 0x767676,
  0x578bc9, 0x979797, 0x68a1dc, 0xb6b6b6,
  0x79b5ec, 0xd2d2d2, 0x88c8fc, 0xececec,

  0x340080, 0x000000, 0x4a1a96, 0x2b2b2b,
  0x5f32ab, 0x525252, 0x7248be, 0x767676,
  0x835ccf, 0x979797, 0x936fdf, 0xb6b6b6,
  0xa280ee, 0xd2d2d2, 0xb090fc, 0xececec,

  0x000088, 0x000000, 0x1a1a9d, 0x2b2b2b,
  0x3232b0, 0x525252, 0x4848c2, 0x767676,
  0x5c5cd2, 0x979797, 0x6f6fe1, 0xb6b6b6,
  0x8080ef, 0xd2d2d2, 0x9090fc, 0xececec,

  0x000000, 0x000000, 0x2b2b2b, 0x2b2b2b,
  0x525252, 0x525252, 0x767676, 0x767676,
  0x979797, 0x979797, 0xb6b6b6, 0xb6b6b6,
  0xd2d2d2, 0xd2d2d2, 0xececec, 0xececec,

  0x000000, 0x000000, 0x2b2b2b, 0x2b2b2b,
  0x525252, 0x525252, 0x767676, 0x767676,
  0x979797, 0x979797, 0xb6b6b6, 0xb6b6b6,
  0xd2d2d2, 0xd2d2d2, 0xececec, 0xececec
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const uInt32 TIA::ourNTSCPalette11[256] = {
  0x000000, 0x1c1c1c, 0x393939, 0x595959, 
  0x797979, 0x929292, 0xababab, 0xbcbcbc, 
  0xcdcdcd, 0xd9d9d9, 0xe6e6e6, 0xececec, 
  0xf2f2f2, 0xf8f8f8, 0xffffff, 0xffffff, 
  0x391701, 0x5e2304, 0x833008, 0xa54716, 
  0xc85f24, 0xe37820, 0xff911d, 0xffab1d, 
  0xffc51d, 0xffce34, 0xffd84c, 0xffe651, 
  0xfff456, 0xfff977, 0xffff98, 0xffff98, 
  0x451904, 0x721e11, 0x9f241e, 0xb33a20, 
  0xc85122, 0xe36920, 0xff811e, 0xff8c25, 
  0xff982c, 0xffae38, 0xffc545, 0xffc559, 
  0xffc66d, 0xffd587, 0xffe4a1, 0xffe4a1, 
  0x4a1704, 0x7e1a0d, 0xb21d17, 0xc82119, 
  0xdf251c, 0xec3b38, 0xfa5255, 0xfc6161, 
  0xff706e, 0xff7f7e, 0xff8f8f, 0xff9d9e, 
  0xffabad, 0xffb9bd, 0xffc7ce, 0xffc7ce, 
  0x050568, 0x3b136d, 0x712272, 0x8b2a8c, 
  0xa532a6, 0xb938ba, 0xcd3ecf, 0xdb47dd, 
  0xea51eb, 0xf45ff5, 0xfe6dff, 0xfe7afd, 
  0xff87fb, 0xff95fd, 0xffa4ff, 0xffa4ff, 
  0x280479, 0x400984, 0x590f90, 0x70249d, 
  0x8839aa, 0xa441c3, 0xc04adc, 0xd054ed, 
  0xe05eff, 0xe96dff, 0xf27cff, 0xf88aff, 
  0xff98ff, 0xfea1ff, 0xfeabff, 0xfeabff, 
  0x35088a, 0x420aad, 0x500cd0, 0x6428d0, 
  0x7945d0, 0x8d4bd4, 0xa251d9, 0xb058ec, 
  0xbe60ff, 0xc56bff, 0xcc77ff, 0xd183ff, 
  0xd790ff, 0xdb9dff, 0xdfaaff, 0xdfaaff, 
  0x051e81, 0x0626a5, 0x082fca, 0x263dd4, 
  0x444cde, 0x4f5aee, 0x5a68ff, 0x6575ff, 
  0x7183ff, 0x8091ff, 0x90a0ff, 0x97a9ff, 
  0x9fb2ff, 0xafbeff, 0xc0cbff, 0xc0cbff, 
  0x0c048b, 0x2218a0, 0x382db5, 0x483ec7, 
  0x584fda, 0x6159ec, 0x6b64ff, 0x7a74ff, 
  0x8a84ff, 0x918eff, 0x9998ff, 0xa5a3ff, 
  0xb1aeff, 0xb8b8ff, 0xc0c2ff, 0xc0c2ff, 
  0x1d295a, 0x1d3876, 0x1d4892, 0x1c5cac, 
  0x1c71c6, 0x3286cf, 0x489bd9, 0x4ea8ec, 
  0x55b6ff, 0x70c7ff, 0x8cd8ff, 0x93dbff, 
  0x9bdfff, 0xafe4ff, 0xc3e9ff, 0xc3e9ff, 
  0x2f4302, 0x395202, 0x446103, 0x417a12, 
  0x3e9421, 0x4a9f2e, 0x57ab3b, 0x5cbd55, 
  0x61d070, 0x69e27a, 0x72f584, 0x7cfa8d, 
  0x87ff97, 0x9affa6, 0xadffb6, 0xadffb6, 
  0x0a4108, 0x0d540a, 0x10680d, 0x137d0f, 
  0x169212, 0x19a514, 0x1cb917, 0x1ec919, 
  0x21d91b, 0x47e42d, 0x6ef040, 0x78f74d, 
  0x83ff5b, 0x9aff7a, 0xb2ff9a, 0xb2ff9a, 
  0x04410b, 0x05530e, 0x066611, 0x077714, 
  0x088817, 0x099b1a, 0x0baf1d, 0x48c41f, 
  0x86d922, 0x8fe924, 0x99f927, 0xa8fc41, 
  0xb7ff5b, 0xc9ff6e, 0xdcff81, 0xdcff81, 
  0x02350f, 0x073f15, 0x0c4a1c, 0x2d5f1e, 
  0x4f7420, 0x598324, 0x649228, 0x82a12e, 
  0xa1b034, 0xa9c13a, 0xb2d241, 0xc4d945, 
  0xd6e149, 0xe4f04e, 0xf2ff53, 0xf2ff53, 
  0x263001, 0x243803, 0x234005, 0x51541b, 
  0x806931, 0x978135, 0xaf993a, 0xc2a73e, 
  0xd5b543, 0xdbc03d, 0xe1cb38, 0xe2d836, 
  0xe3e534, 0xeff258, 0xfbff7d, 0xfbff7d, 
  0x401a02, 0x581f05, 0x702408, 0x8d3a13, 
  0xab511f, 0xb56427, 0xbf7730, 0xd0853a, 
  0xe19344, 0xeda04e, 0xf9ad58, 0xfcb75c, 
  0xffc160, 0xffc671, 0xffcb83, 0xffcb83
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const uInt32 TIA::ourPALPalette11[256] = {
  0x000000, 0x000000, 0x242424, 0x242424, 
  0x484848, 0x484848, 0x6d6d6d, 0x6d6d6d, 
  0x919191, 0x919191, 0xb6b6b6, 0xb6b6b6, 
  0xdadada, 0xdadada, 0xffffff, 0xffffff, 
  0x000000, 0x000000, 0x242424, 0x242424, 
  0x484848, 0x484848, 0x6d6d6d, 0x6d6d6d, 
  0x919191, 0x919191, 0xb6b6b6, 0xb6b6b6, 
  0xdadada, 0xdadada, 0xffffff, 0xffffff, 
  0x4a3700, 0x4a3700, 0x705813, 0x705813, 
  0x8c732a, 0x8c732a, 0xa68d46, 0xa68d46, 
  0xbea767, 0xbea767, 0xd4c18b, 0xd4c18b, 
  0xeadcb3, 0xeadcb3, 0xfff6de, 0xfff6de, 
  0x284a00, 0x284a00, 0x44700f, 0x44700f, 
  0x5c8c21, 0x5c8c21, 0x74a638, 0x74a638, 
  0x8cbe51, 0x8cbe51, 0xa6d46e, 0xa6d46e, 
  0xc0ea8e, 0xc0ea8e, 0xdbffb0, 0xdbffb0, 
  0x4a1300, 0x4a1300, 0x70280f, 0x70280f, 
  0x8c3d21, 0x8c3d21, 0xa65438, 0xa65438, 
  0xbe6d51, 0xbe6d51, 0xd4886e, 0xd4886e, 
  0xeaa58e, 0xeaa58e, 0xffc4b0, 0xffc4b0, 
  0x004a22, 0x004a22, 0x0f703b, 0x0f703b, 
  0x218c52, 0x218c52, 0x38a66a, 0x38a66a, 
  0x51be83, 0x51be83, 0x6ed49d, 0x6ed49d, 
  0x8eeab8, 0x8eeab8, 0xb0ffd4, 0xb0ffd4, 
  0x4a0028, 0x4a0028, 0x700f44, 0x700f44, 
  0x8c215c, 0x8c215c, 0xa63874, 0xa63874, 
  0xbe518c, 0xbe518c, 0xd46ea6, 0xd46ea6, 
  0xea8ec0, 0xea8ec0, 0xffb0db, 0xffb0db, 
  0x00404a, 0x00404a, 0x0f6370, 0x0f6370, 
  0x217e8c, 0x217e8c, 0x3897a6, 0x3897a6, 
  0x51afbe, 0x51afbe, 0x6ec7d4, 0x6ec7d4, 
  0x8edeea, 0x8edeea, 0xb0f4ff, 0xb0f4ff, 
  0x43002c, 0x43002c, 0x650f4b, 0x650f4b, 
  0x7e2165, 0x7e2165, 0x953880, 0x953880, 
  0xa6519a, 0xa6519a, 0xbf6eb7, 0xbf6eb7, 
  0xd38ed3, 0xd38ed3, 0xe5b0f1, 0xe5b0f1, 
  0x001d4a, 0x001d4a, 0x0f3870, 0x0f3870, 
  0x21538c, 0x21538c, 0x386ea6, 0x386ea6, 
  0x518dbe, 0x518dbe, 0x6ea8d4, 0x6ea8d4, 
  0x8ec8ea, 0x8ec8ea, 0xb0e9ff, 0xb0e9ff, 
  0x37004a, 0x37004a, 0x570f70, 0x570f70, 
  0x70218c, 0x70218c, 0x8938a6, 0x8938a6, 
  0xa151be, 0xa151be, 0xba6ed4, 0xba6ed4, 
  0xd28eea, 0xd28eea, 0xeab0ff, 0xeab0ff, 
  0x00184a, 0x00184a, 0x0f2e70, 0x0f2e70, 
  0x21448c, 0x21448c, 0x385ba6, 0x385ba6, 
  0x5174be, 0x5174be, 0x6e8fd4, 0x6e8fd4, 
  0x8eabea, 0x8eabea, 0xb0c9ff, 0xb0c9ff, 
  0x13004a, 0x13004a, 0x280f70, 0x280f70, 
  0x3d218c, 0x3d218c, 0x5438a6, 0x5438a6, 
  0x6d51be, 0x6d51be, 0x886ed4, 0x886ed4, 
  0xa58eea, 0xa58eea, 0xc4b0ff, 0xc4b0ff, 
  0x00014a, 0x00014a, 0x0f1170, 0x0f1170, 
  0x21248c, 0x21248c, 0x383aa6, 0x383aa6, 
  0x5153be, 0x5153be, 0x6e70d4, 0x6e70d4, 
  0x8e8fea, 0x8e8fea, 0xb0b2ff, 0xb0b2ff, 
  0x000000, 0x000000, 0x242424, 0x242424, 
  0x484848, 0x484848, 0x6d6d6d, 0x6d6d6d, 
  0x919191, 0x919191, 0xb6b6b6, 0xb6b6b6, 
  0xdadada, 0xdadada, 0xffffff, 0xffffff, 
  0x000000, 0x000000, 0x242424, 0x242424, 
  0x484848, 0x484848, 0x6d6d6d, 0x6d6d6d, 
  0x919191, 0x919191, 0xb6b6b6, 0xb6b6b6, 
  0xdadada, 0xdadada, 0xffffff, 0xff4ffff  // FIXME - check this out
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const uInt32 TIA::ourNTSCPaletteZ26[256] = {
  0x000000, 0x000000, 0x505050, 0x505050, 
  0x646464, 0x646464, 0x787878, 0x787878, 
  0x8c8c8c, 0x8c8c8c, 0xa0a0a0, 0xa0a0a0, 
  0xb4b4b4, 0xb4b4b4, 0xc8c8c8, 0xc8c8c8, 
  0x445400, 0x445400, 0x586800, 0x586800, 
  0x6c7c00, 0x6c7c00, 0x809000, 0x809000, 
  0x94a414, 0x94a414, 0xa8b828, 0xa8b828, 
  0xbccc3c, 0xbccc3c, 0xd0e050, 0xd0e050, 
  0x673900, 0x673900, 0x7b4d00, 0x7b4d00, 
  0x8f6100, 0x8f6100, 0xa37513, 0xa37513, 
  0xb78927, 0xb78927, 0xcb9d3b, 0xcb9d3b, 
  0xdfb14f, 0xdfb14f, 0xf3c563, 0xf3c563, 
  0x7b2504, 0x7b2504, 0x8f3918, 0x8f3918, 
  0xa34d2c, 0xa34d2c, 0xb76140, 0xb76140, 
  0xcb7554, 0xcb7554, 0xdf8968, 0xdf8968, 
  0xf39d7c, 0xf39d7c, 0xffb190, 0xffb190, 
  0x7d122c, 0x7d122c, 0x912640, 0x912640, 
  0xa53a54, 0xa53a54, 0xb94e68, 0xb94e68, 
  0xcd627c, 0xcd627c, 0xe17690, 0xe17690, 
  0xf58aa4, 0xf58aa4, 0xff9eb8, 0xff9eb8, 
  0x730871, 0x730871, 0x871c85, 0x871c85, 
  0x9b3099, 0x9b3099, 0xaf44ad, 0xaf44ad, 
  0xc358c1, 0xc358c1, 0xd76cd5, 0xd76cd5, 
  0xeb80e9, 0xeb80e9, 0xff94fd, 0xff94fd, 
  0x5d0b92, 0x5d0b92, 0x711fa6, 0x711fa6, 
  0x8533ba, 0x8533ba, 0x9947ce, 0x9947ce, 
  0xad5be2, 0xad5be2, 0xc16ff6, 0xc16ff6, 
  0xd583ff, 0xd583ff, 0xe997ff, 0xe997ff, 
  0x401599, 0x401599, 0x5429ad, 0x5429ad, 
  0x683dc1, 0x683dc1, 0x7c51d5, 0x7c51d5, 
  0x9065e9, 0x9065e9, 0xa479fd, 0xa479fd, 
  0xb88dff, 0xb88dff, 0xcca1ff, 0xcca1ff, 
  0x252593, 0x252593, 0x3939a7, 0x3939a7, 
  0x4d4dbb, 0x4d4dbb, 0x6161cf, 0x6161cf, 
  0x7575e3, 0x7575e3, 0x8989f7, 0x8989f7, 
  0x9d9dff, 0x9d9dff, 0xb1b1ff, 0xb1b1ff, 
  0x0f3480, 0x0f3480, 0x234894, 0x234894, 
  0x375ca8, 0x375ca8, 0x4b70bc, 0x4b70bc, 
  0x5f84d0, 0x5f84d0, 0x7398e4, 0x7398e4, 
  0x87acf8, 0x87acf8, 0x9bc0ff, 0x9bc0ff, 
  0x04425a, 0x04425a, 0x18566e, 0x18566e, 
  0x2c6a82, 0x2c6a82, 0x407e96, 0x407e96, 
  0x5492aa, 0x5492aa, 0x68a6be, 0x68a6be, 
  0x7cbad2, 0x7cbad2, 0x90cee6, 0x90cee6, 
  0x044f30, 0x044f30, 0x186344, 0x186344, 
  0x2c7758, 0x2c7758, 0x408b6c, 0x408b6c, 
  0x549f80, 0x549f80, 0x68b394, 0x68b394, 
  0x7cc7a8, 0x7cc7a8, 0x90dbbc, 0x90dbbc, 
  0x0f550a, 0x0f550a, 0x23691e, 0x23691e, 
  0x377d32, 0x377d32, 0x4b9146, 0x4b9146, 
  0x5fa55a, 0x5fa55a, 0x73b96e, 0x73b96e, 
  0x87cd82, 0x87cd82, 0x9be196, 0x9be196, 
  0x1f5100, 0x1f5100, 0x336505, 0x336505, 
  0x477919, 0x477919, 0x5b8d2d, 0x5b8d2d, 
  0x6fa141, 0x6fa141, 0x83b555, 0x83b555, 
  0x97c969, 0x97c969, 0xabdd7d, 0xabdd7d, 
  0x344600, 0x344600, 0x485a00, 0x485a00, 
  0x5c6e14, 0x5c6e14, 0x708228, 0x708228, 
  0x84963c, 0x84963c, 0x98aa50, 0x98aa50, 
  0xacbe64, 0xacbe64, 0xc0d278, 0xc0d278, 
  0x463e00, 0x463e00, 0x5a5205, 0x5a5205, 
  0x6e6619, 0x6e6619, 0x827a2d, 0x827a2d, 
  0x968e41, 0x968e41, 0xaaa255, 0xaaa255, 
  0xbeb669, 0xbeb669, 0xd2ca7d, 0xd2ca7d
}; 
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const uInt32 TIA::ourPALPaletteZ26[256] = {
  0x000000, 0x000000, 0x4c4c4c, 0x4c4c4c, 
  0x606060, 0x606060, 0x747474, 0x747474, 
  0x888888, 0x888888, 0x9c9c9c, 0x9c9c9c, 
  0xb0b0b0, 0xb0b0b0, 0xc4c4c4, 0xc4c4c4, 
  0x000000, 0x000000, 0x4c4c4c, 0x4c4c4c, 
  0x606060, 0x606060, 0x747474, 0x747474, 
  0x888888, 0x888888, 0x9c9c9c, 0x9c9c9c, 
  0xb0b0b0, 0xb0b0b0, 0xc4c4c4, 0xc4c4c4, 
  0x533a00, 0x533a00, 0x674e00, 0x674e00, 
  0x7b6203, 0x7b6203, 0x8f7617, 0x8f7617, 
  0xa38a2b, 0xa38a2b, 0xb79e3f, 0xb79e3f, 
  0xcbb253, 0xcbb253, 0xdfc667, 0xdfc667, 
  0x1b5800, 0x1b5800, 0x2f6c00, 0x2f6c00, 
  0x438001, 0x438001, 0x579415, 0x579415, 
  0x6ba829, 0x6ba829, 0x7fbc3d, 0x7fbc3d, 
  0x93d051, 0x93d051, 0xa7e465, 0xa7e465, 
  0x6a2900, 0x6a2900, 0x7e3d12, 0x7e3d12, 
  0x925126, 0x925126, 0xa6653a, 0xa6653a, 
  0xba794e, 0xba794e, 0xce8d62, 0xce8d62, 
  0xe2a176, 0xe2a176, 0xf6b58a, 0xf6b58a, 
  0x075b00, 0x075b00, 0x1b6f11, 0x1b6f11, 
  0x2f8325, 0x2f8325, 0x439739, 0x439739, 
  0x57ab4d, 0x57ab4d, 0x6bbf61, 0x6bbf61, 
  0x7fd375, 0x7fd375, 0x93e789, 0x93e789, 
  0x741b2f, 0x741b2f, 0x882f43, 0x882f43, 
  0x9c4357, 0x9c4357, 0xb0576b, 0xb0576b, 
  0xc46b7f, 0xc46b7f, 0xd87f93, 0xd87f93, 
  0xec93a7, 0xec93a7, 0xffa7bb, 0xffa7bb, 
  0x00572e, 0x00572e, 0x106b42, 0x106b42, 
  0x247f56, 0x247f56, 0x38936a, 0x38936a, 
  0x4ca77e, 0x4ca77e, 0x60bb92, 0x60bb92, 
  0x74cfa6, 0x74cfa6, 0x88e3ba, 0x88e3ba, 
  0x6d165f, 0x6d165f, 0x812a73, 0x812a73, 
  0x953e87, 0x953e87, 0xa9529b, 0xa9529b, 
  0xbd66af, 0xbd66af, 0xd17ac3, 0xd17ac3, 
  0xe58ed7, 0xe58ed7, 0xf9a2eb, 0xf9a2eb, 
  0x014c5e, 0x014c5e, 0x156072, 0x156072, 
  0x297486, 0x297486, 0x3d889a, 0x3d889a, 
  0x519cae, 0x519cae, 0x65b0c2, 0x65b0c2, 
  0x79c4d6, 0x79c4d6, 0x8dd8ea, 0x8dd8ea, 
  0x5f1588, 0x5f1588, 0x73299c, 0x73299c, 
  0x873db0, 0x873db0, 0x9b51c4, 0x9b51c4, 
  0xaf65d8, 0xaf65d8, 0xc379ec, 0xc379ec, 
  0xd78dff, 0xd78dff, 0xeba1ff, 0xeba1ff, 
  0x123b87, 0x123b87, 0x264f9b, 0x264f9b, 
  0x3a63af, 0x3a63af, 0x4e77c3, 0x4e77c3, 
  0x628bd7, 0x628bd7, 0x769feb, 0x769feb, 
  0x8ab3ff, 0x8ab3ff, 0x9ec7ff, 0x9ec7ff, 
  0x451e9d, 0x451e9d, 0x5932b1, 0x5932b1, 
  0x6d46c5, 0x6d46c5, 0x815ad9, 0x815ad9, 
  0x956eed, 0x956eed, 0xa982ff, 0xa982ff, 
  0xbd96ff, 0xbd96ff, 0xd1aaff, 0xd1aaff, 
  0x2a2b9e, 0x2a2b9e, 0x3e3fb2, 0x3e3fb2, 
  0x5253c6, 0x5253c6, 0x6667da, 0x6667da, 
  0x7a7bee, 0x7a7bee, 0x8e8fff, 0x8e8fff, 
  0xa2a3ff, 0xa2a3ff, 0xb6b7ff, 0xb6b7ff, 
  0x000000, 0x000000, 0x4c4c4c, 0x4c4c4c, 
  0x606060, 0x606060, 0x747474, 0x747474, 
  0x888888, 0x888888, 0x9c9c9c, 0x9c9c9c, 
  0xb0b0b0, 0xb0b0b0, 0xc4c4c4, 0xc4c4c4, 
  0x000000, 0x000000, 0x4c4c4c, 0x4c4c4c, 
  0x606060, 0x606060, 0x747474, 0x747474, 
  0x888888, 0x888888, 0x9c9c9c, 0x9c9c9c, 
  0xb0b0b0, 0xb0b0b0, 0xc4c4c4, 0xc4c4c4
}; 

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TIA::TIA(const TIA& c)
    : myConsole(c.myConsole),
      mySettings(c.mySettings),
      mySound(c.mySound),
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
