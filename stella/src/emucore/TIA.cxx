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
// $Id: TIA.cxx,v 1.2 2001-12-30 18:38:54 bwmott Exp $
//============================================================================

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "Console.hxx"
#include "Control.hxx"
#include "M6502.hxx"
#include "Sound.hxx"
#include "System.hxx"
#include "TIA.hxx"

#define HBLANK 68

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TIA::TIA(const Console& console, Sound& sound)
    : myConsole(console),
      mySound(sound),
      myCOLUBK(myColor[0]),
      myCOLUPF(myColor[1]),
      myCOLUP0(myColor[2]),
      myCOLUP1(myColor[3])
{
  // Allocate buffers for two frame buffers
  myCurrentFrameBuffer = new uInt8[160 * 300];
  myPreviousFrameBuffer = new uInt8[160 * 300];

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
          color = (enabled & ScoreBit) ? ((x == 0) ? 2 : 3) : 1;

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
  // Clear frame buffers
  for(uInt32 i = 0; i < 160 * 300; ++i)
  {
    myCurrentFrameBuffer[i] = myPreviousFrameBuffer[i] = 0;
  }

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
  myScanlineCountForFrame = 0;

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
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIA::systemCyclesReset()
{
  // Get the current system cycle
  uInt32 cycles = mySystem->cycles();

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
void TIA::update()
{
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
  myClockWhenFrameStarted = -clocks;
  myClockStartDisplay = myClockWhenFrameStarted + myStartDisplayOffset;
  myClockStopDisplay = myClockWhenFrameStarted + myStopDisplayOffset;
  myClockAtLastUpdate = myClockStartDisplay;
  myClocksToEndOfScanLine = 228;

  // Reset frame buffer pointer
  myFramePointer = myCurrentFrameBuffer;

  // Execute instructions until frame is finished
  mySystem->m6502().execute(25000);

  // TODO: have code here that handles errors....

  // Compute the number of scanlines in the frame
  uInt32 totalClocks = (mySystem->cycles() * 3) - myClockWhenFrameStarted;
  myScanlineCountForFrame = totalClocks / 228;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const uInt32* TIA::palette() const
{
  // See which palette we should be using based on properties
  if(myConsole.properties().get("Display.Format") == "PAL")
  {
    return ourPALPalette;
  }
  else
  {
    return ourNTSCPalette;
  }
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
  return (uInt32)myScanlineCountForFrame;
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
        for(; ((int)myFramePointer & 0x03) && (myFramePointer < ending);
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
        for(; ((int)myFramePointer & 0x03) && (myFramePointer < ending); 
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
          if(!((int)myFramePointer & 0x03) && !*(uInt32*)mP0)
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
          if(!((int)myFramePointer & 0x03) && !*(uInt32*)mP1)
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
          if(!((int)myFramePointer & 0x03) && !*(uInt32*)mP0 &&
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
          if(!((int)myFramePointer & 0x03) && !*(uInt32*)mM0)
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
          if(!((int)myFramePointer & 0x03) && !*(uInt32*)mM1)
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
          if(!((int)myFramePointer & 0x03) && !*(uInt32*)mBL)
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
          if(!((int)myFramePointer & 0x03) && !*(uInt32*)mM0 && !*(uInt32*)mM1)
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
          if(!((int)myFramePointer & 0x03) && !*(uInt32*)mBL && !*(uInt32*)mM0)
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
          if(!((int)myFramePointer & 0x03) && !*(uInt32*)mBL && !*(uInt32*)mM0)
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
          if(!((int)myFramePointer & 0x03) && !*(uInt32*)mBL && 
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
          if(!((int)myFramePointer & 0x03) && !*(uInt32*)mBL && 
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
          if(!((int)myFramePointer & 0x03) && !*(uInt32*)mP1 && !*(uInt32*)mBL)
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
          if(!((int)myFramePointer & 0x03) && !*(uInt32*)mP1 && !*(uInt32*)mBL)
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
          if(!((int)myFramePointer & 0x03) && !*(uInt32*)mP0)
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
          if(!((int)myFramePointer & 0x03) && !*(uInt32*)mP0)
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
          if(!((int)myFramePointer & 0x03) && !*(uInt32*)mP1)
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
          if(!((int)myFramePointer & 0x03) && !*(uInt32*)mP1)
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
          if(!((int)myFramePointer & 0x03) && !*(uInt32*)mBL)
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

      // Handle HMOVE blanks
      if(myHMOVEBlankEnabled && (startOfScanLine < HBLANK + 8) &&
          (clocksFromStartOfScanLine == (Int32)(HBLANK + myFrameXStart)))
      {
        Int32 blanks = 8 - myFrameXStart;
        myHMOVEBlankEnabled = false;
        memset(myFramePointer, 0, blanks);
        myFramePointer += blanks;
        clocksFromStartOfScanLine += blanks;

        if(clocksToUpdate >= blanks)
        {
          clocksToUpdate -= blanks;
        }
        else
        {
          // Updating more that we were supposed to so adjust the clocks
          myClocksToEndOfScanLine -= (blanks - clocksToUpdate);
          myClockAtLastUpdate += (blanks - clocksToUpdate);
          clocksToUpdate = 0;
        }
      }
    }

    // Update as much of the scanline as we can
    if(clocksToUpdate != 0)
    {
      updateFrameScanline(clocksToUpdate, clocksFromStartOfScanLine - HBLANK);
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
uInt8 TIA::peek(uInt16 addr)
{
  // Update frame to current color clock before we look at anything!
  updateFrame(mySystem->cycles() * 3);

  uInt8 noise = mySystem->getDataBusState();
  uInt8 noise1 = noise & 0x7F;
  uInt8 noise2 = noise & 0x3F;

  switch(addr & 0x000f)
  {
    case 0x00:    // CXM0P
      return ((myCollision & 0x0001) ? 0x80 : 0x00) | 
          ((myCollision & 0x0002) ? 0x40 : 0x00) | noise2;

    case 0x01:    // CXM1P
      return ((myCollision & 0x0004) ? 0x80 : 0x00) | 
          ((myCollision & 0x0008) ? 0x40 : 0x00) | noise2;

    case 0x02:    // CXP0FB
      return ((myCollision & 0x0010) ? 0x80 : 0x00) | 
          ((myCollision & 0x0020) ? 0x40 : 0x00) | noise2;

    case 0x03:    // CXP1FB
      return ((myCollision & 0x0040) ? 0x80 : 0x00) | 
          ((myCollision & 0x0080) ? 0x40 : 0x00) | noise2;

    case 0x04:    // CXM0FB
      return ((myCollision & 0x0100) ? 0x80 : 0x00) | 
          ((myCollision & 0x0200) ? 0x40 : 0x00) | noise2;

    case 0x05:    // CXM1FB
      return ((myCollision & 0x0400) ? 0x80 : 0x00) | 
          ((myCollision & 0x0800) ? 0x40 : 0x00) | noise2;

    case 0x06:    // CXBLPF
      return ((myCollision & 0x1000) ? 0x80 : 0x00) | noise1;

    case 0x07:    // CXPPMM
      return ((myCollision & 0x2000) ? 0x80 : 0x00) | 
          ((myCollision & 0x4000) ? 0x40 : 0x00) | noise2;

    case 0x08:    // INPT0
    {
      Int32 r = myConsole.controller(Controller::Left).read(Controller::Nine);
      if(r == Controller::minimumResistance)
      {
        return 0x80 | noise1;
      }
      else if((r == Controller::maximumResistance) || myDumpEnabled)
      {
        return noise1;
      }
      else
      {
        double t = (1.6 * r * 0.01E-6);
        uInt32 needed = (uInt32)(t * 1.19E6);
        if(mySystem->cycles() > (myDumpDisabledCycle + needed))
        {
          return 0x80 | noise1;
        }
        else
        {
          return noise1;
        }
      }
    }

    case 0x09:    // INPT1
    {
      Int32 r = myConsole.controller(Controller::Left).read(Controller::Five);
      if(r == Controller::minimumResistance)
      {
        return 0x80 | noise1;
      }
      else if((r == Controller::maximumResistance) || myDumpEnabled)
      {
        return noise1;
      }
      else
      {
        double t = (1.6 * r * 0.01E-6);
        uInt32 needed = (uInt32)(t * 1.19E6);
        if(mySystem->cycles() > (myDumpDisabledCycle + needed))
        {
          return 0x80 | noise1;
        }
        else
        {
          return noise1;
        }
      }
    }

    case 0x0A:    // INPT2
    {
      Int32 r = myConsole.controller(Controller::Right).read(Controller::Nine);
      if(r == Controller::minimumResistance)
      {
        return 0x80 | noise1;
      }
      else if((r == Controller::maximumResistance) || myDumpEnabled)
      {
        return noise1;
      }
      else
      {
        double t = (1.6 * r * 0.01E-6);
        uInt32 needed = (uInt32)(t * 1.19E6);
        if(mySystem->cycles() > (myDumpDisabledCycle + needed))
        {
          return 0x80 | noise1;
        }
        else
        {
          return noise1;
        }
      }
    }

    case 0x0B:    // INPT3
    {
      Int32 r = myConsole.controller(Controller::Right).read(Controller::Five);
      if(r == Controller::minimumResistance)
      {
        return 0x80 | noise1;
      }
      else if((r == Controller::maximumResistance) || myDumpEnabled)
      {
        return noise1;
      }
      else
      {
        double t = (1.6 * r * 0.01E-6);
        uInt32 needed = (uInt32)(t * 1.19E6);
        if(mySystem->cycles() > (myDumpDisabledCycle + needed))
        {
          return 0x80 | noise1;
        }
        else
        {
          return noise1;
        }
      }
    }

    case 0x0C:    // INPT4
      return myConsole.controller(Controller::Left).read(Controller::Six) ?
          (0x80 | noise1) : noise1;

    case 0x0D:    // INPT5
      return myConsole.controller(Controller::Right).read(Controller::Six) ?
          (0x80 | noise1) : noise1;

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
      uInt32 color = (uInt32)value;
      myCOLUP0 = (((((color << 8) | color) << 8) | color) << 8) | color;
      break;
    }

    case 0x07:    // Color-Luminance Player 1
    {
      uInt32 color = (uInt32)value;
      myCOLUP1 = (((((color << 8) | color) << 8) | color) << 8) | color;
      break;
    }

    case 0x08:    // Color-Luminance Playfield
    {
      uInt32 color = (uInt32)value;
      myCOLUPF = (((((color << 8) | color) << 8) | color) << 8) | color;
      break;
    }

    case 0x09:    // Color-Luminance Background
    {
      uInt32 color = (uInt32)value;
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

      if(myPF != 0)
        myEnabledObjects |= myPFBit;
      else
        myEnabledObjects &= ~myPFBit;

      break;
    }

    case 0x0E:    // Playfield register byte 1
    {
      myPF = (myPF & 0x000FF00F) | ((uInt32)value << 4);

      if(myPF != 0)
        myEnabledObjects |= myPFBit;
      else
        myEnabledObjects &= ~myPFBit;

      break;
    }

    case 0x0F:    // Playfield register byte 2
    {
      myPF = (myPF & 0x00000FFF) | ((uInt32)value << 12);

      if(myPF != 0)
        myEnabledObjects |= myPFBit;
      else
        myEnabledObjects &= ~myPFBit;

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

      myCurrentM0Mask = &ourMissleMaskTable[myPOSM0 & 0x03]
          [myNUSIZ0 & 0x07][(myNUSIZ0 & 0x30) >> 4][160 - (myPOSM0 & 0xFC)];
      break;
    }

    case 0x13:    // Reset Missle 1
    {
      int hpos = (clock - myClockWhenFrameStarted) % 228;
      myPOSM1 = hpos < HBLANK ? 2 : (((hpos - HBLANK) + 4) % 160);

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
      
      myCurrentBLMask = &ourBallMaskTable[myPOSBL & 0x03]
          [(myCTRLPF & 0x30) >> 4][160 - (myPOSBL & 0xFC)];
      break;
    }

    case 0x15:    // Audio control 0
    {
      mySound.set(Sound::AUDC0, value);
      break;
    }
  
    case 0x16:    // Audio control 1
    {
      mySound.set(Sound::AUDC1, value);
      break;
    }
  
    case 0x17:    // Audio frequency 0
    {
      mySound.set(Sound::AUDF0, value);
      break;
    }
  
    case 0x18:    // Audio frequency 1
    {
      mySound.set(Sound::AUDF1, value);
      break;
    }
  
    case 0x19:    // Audio volume 0
    {
      mySound.set(Sound::AUDV0, value);
      break;
    }
  
    case 0x1A:    // Audio volume 1
    {
      mySound.set(Sound::AUDV1, value);
      break;
    }

    case 0x1B:    // Graphics Player 0
    {
      // Set player 0 graphics
      myGRP0 = value;

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
      myGRP1 = value;

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

    case 0x1D:    // Enable Missle 0 graphics
    {
      myENAM0 = value & 0x02;

      if(myENAM0 && !myRESMP0)
        myEnabledObjects |= myM0Bit;
      else
        myEnabledObjects &= ~myM0Bit;
      break;
    }

    case 0x1E:    // Enable Missle 1 graphics
    {
      myENAM1 = value & 0x02;

      if(myENAM1 && !myRESMP1)
        myEnabledObjects |= myM1Bit;
      else
        myEnabledObjects &= ~myM1Bit;
      break;
    }

    case 0x1F:    // Enable Ball graphics
    {
      myENABL = value & 0x02;

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
   0,  0,  0,  0, 12, 12,  0,  0,  0,  0,  0,  1,  1, -1, -1, -1,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  0,  0,  0,
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
const uInt32 TIA::ourPALPalette[256] = {
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
  0xdadada, 0xdadada, 0xffffff, 0xffffff
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TIA::TIA(const TIA& c)
    : myConsole(c.myConsole),
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

