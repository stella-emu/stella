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
// $Id: TIA.cxx,v 1.21 2003-09-26 00:32:00 stephena Exp $
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
#include "UserInterface.hxx"
#include "TIASound.h"

#define HBLANK 68

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TIA::TIA(const Console& console, uInt32 sampleRate)
    : myConsole(console),
      myPauseState(false),
      myMessageTime(0),
      myMessageText(""),
      myLastSoundUpdateCycle(0),
      myColorLossEnabled(false),
      myCOLUBK(myColor[0]),
      myCOLUPF(myColor[1]),
      myCOLUP0(myColor[2]),
      myCOLUP1(myColor[3]),
      mySampleQueue(sampleRate),
      mySampleRate(sampleRate)
{
  if(mySampleRate != 0)
  {
    Tia_sound_init(31400, mySampleRate);
  }

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
  // Reset sound cycle indicator
  myLastSoundUpdateCycle = 0;

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
  myScanlineCountForLastFrame = 0;

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

  if(myConsole.properties().get("Display.Format") == "PAL")
  {
    myColorLossEnabled = true;
  }
  else
  {
    myColorLossEnabled = false;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIA::systemCyclesReset()
{
  // Get the current system cycle
  uInt32 cycles = mySystem->cycles();

  // Adjust the sound cycle indicator
  myLastSoundUpdateCycle -= cycles;

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

    out.putLong(myLastSoundUpdateCycle);
    out.putLong(myClockWhenFrameStarted);
    out.putLong(myClockStartDisplay);
    out.putLong(myClockStopDisplay);
    out.putLong(myClockAtLastUpdate);
    out.putLong(myClocksToEndOfScanLine);
    out.putLong(myScanlineCountForLastFrame);
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

    // Save the sample stuff ...
    string soundDevice = "TIASound";
    out.putString(soundDevice);

    uInt8 reg1 = 0, reg2 = 0, reg3 = 0, reg4 = 0, reg5 = 0, reg6 = 0;

    // Only get the TIA sound registers if sound is enabled
    if(mySampleRate != 0)
      Tia_get_registers(&reg1, &reg2, &reg3, &reg4, &reg5, &reg6);

    out.putLong(reg1);
    out.putLong(reg2);
    out.putLong(reg3);
    out.putLong(reg4);
    out.putLong(reg5);
    out.putLong(reg6);
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

    myLastSoundUpdateCycle = (Int32) in.getLong();
    myClockWhenFrameStarted = (Int32) in.getLong();
    myClockStartDisplay = (Int32) in.getLong();
    myClockStopDisplay = (Int32) in.getLong();
    myClockAtLastUpdate = (Int32) in.getLong();
    myClocksToEndOfScanLine = (Int32) in.getLong();
    myScanlineCountForLastFrame = (Int32) in.getLong();
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

    // Load the sample stuff ...
    string soundDevice = "TIASound";
    if(in.getString() != soundDevice)
      return false;

    uInt8 reg1 = 0, reg2 = 0, reg3 = 0, reg4 = 0, reg5 = 0, reg6 = 0;
    reg1 = (uInt8) in.getLong();
    reg2 = (uInt8) in.getLong();
    reg3 = (uInt8) in.getLong();
    reg4 = (uInt8) in.getLong();
    reg5 = (uInt8) in.getLong();
    reg6 = (uInt8) in.getLong();

    // Only update the TIA sound registers if sound is enabled
    if(mySampleRate != 0)
      Tia_set_registers(reg1, reg2, reg3, reg4, reg5, reg6);
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
  // Don't do an update if the emulator is paused
  if(myPauseState)
  {
    return;
  }

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

  // Execute instructions until frame is finished
  mySystem->m6502().execute(25000);

  // TODO: have code here that handles errors....

  // Make sure all of the audio samples have been created
  createAudioSamples(0, 0);

  // Compute the number of scanlines in the frame
  uInt32 totalClocks = (mySystem->cycles() * 3) - myClockWhenFrameStarted;
  myScanlineCountForLastFrame = totalClocks / 228;

  // Draw any pending user interface elements to the framebuffer
  if(myConsole.gui().drawPending())
    myConsole.gui().update();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool TIA::pause(bool state)
{
  if(myPauseState == state)
  {
    // Ignore multiple calls to do the same thing
    return false;
  }
  else
  {
    myPauseState = state;
    return true;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIA::drawMessageText()
{
  // Set up the correct coordinates to draw the surrounding box
  uInt32 xBoxOffSet = 2 + myFrameXStart;
  uInt32 yBoxOffSet = myFrameHeight - 18;
  uInt32 boxToTextXOffSet = 2;
  uInt32 boxToTextYOffSet = 4;

  // Set up the correct coordinates to print the message
  uInt32 xTextOffSet = xBoxOffSet + boxToTextXOffSet;
  uInt32 yTextOffSet = yBoxOffSet + boxToTextYOffSet;

  // Used to indicate the current x/y position of a pixel
  uInt32 xPos, yPos;

  // The actual font data for a letter
  uInt32 data;

  // The index into the palette to color the current text and background
  uInt8 fontColor, backColor;

  // Palette index depends on whether we are in NTSC or PAL mode
  if(myConsole.properties().get("Display.Format") == "PAL")
  {
    fontColor = 10;
    backColor = 0;
  }
  else
  {
    fontColor = 10;
    backColor = 0;
  }

  // Clip the length if its wider than the screen
  uInt8 length = myMessageText.length();
  if(((length * 5) + xTextOffSet) >= myFrameWidth)
    length = (myFrameWidth - xTextOffSet) / 5;

  // Reset the offsets to center the message
  uInt32 boxWidth  = (5 * length) + boxToTextXOffSet;
  uInt32 boxHeight = 8 + (2 * (yTextOffSet - yBoxOffSet));
  xBoxOffSet = (myFrameWidth >> 1) - (boxWidth >> 1);
  xTextOffSet = xBoxOffSet + boxToTextXOffSet;

  // First, draw the surrounding box
  for(uInt32 x = 0; x < boxWidth; ++x)
  {
    for(uInt32 y = 0; y < boxHeight; ++y)
    {
      uInt32 position = ((yBoxOffSet + y) * myFrameWidth) + x + xBoxOffSet;

      if((x == 0) || (x == boxWidth - 1) || (y == 0) || (y == boxHeight - 1))
        myCurrentFrameBuffer[position] = fontColor;
      else
        myCurrentFrameBuffer[position] = backColor;
    }
  }

  // Then, draw the text
//FIXME - change back to x
  for(uInt8 x1 = 0; x1 < length; ++x1)
  {
    char letter = myMessageText[x1];

    if((letter >= 'A') && (letter <= 'Z'))
      data = ourFontData[(int)letter - 65];
    else if((letter >= '0') && (letter <= '9'))
      data = ourFontData[(int)letter - 48 + 26];
    else   // unknown character or space
    {
      xTextOffSet += 3;
      continue;
    }

    // start scanning the font data from the bottom up
    yPos = 7;

    for(uInt8 y = 0; y < 32; ++y)
    {
      // determine the correct scanline
      xPos = y % 4;
      if(xPos == 0)
        --yPos;

      if((data >> y) & 1)
      {
        uInt32 position = (yPos + yTextOffSet) * myFrameWidth + (4 - xPos) + xTextOffSet;
        myCurrentFrameBuffer[position] = fontColor;
      }
    }

    // move left to the next character
    xTextOffSet += 5;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIA::showMessage(string& message, Int32 duration)
{
  myMessageText = message;
  myMessageTime = duration;

  // Make message uppercase, since there are no lowercase fonts defined
  uInt32 length = myMessageText.length();
  for(uInt32 i = 0; i < length; ++i)
    myMessageText[i] = toupper(myMessageText[i]);
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
  return (uInt32)myScanlineCountForLastFrame;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIA::clearAudioSamples()
{
  mySampleQueue.clear();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 TIA::dequeueAudioSamples(uInt8* buffer, int size)
{
  return mySampleQueue.dequeue(buffer, size);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 TIA::numberOfAudioSamples() const
{
  return mySampleQueue.size();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
MediaSource::AudioSampleType TIA::typeOfAudioSamples() const
{
  return MediaSource::UNSIGNED_8BIT_MONO_AUDIO;
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
      createAudioSamples(addr, value);
      break;
    }
  
    case 0x16:    // Audio control 1
    {
      createAudioSamples(addr, value);
      break;
    }
  
    case 0x17:    // Audio frequency 0
    {
      createAudioSamples(addr, value);
      break;
    }
  
    case 0x18:    // Audio frequency 1
    {
      createAudioSamples(addr, value);
      break;
    }
  
    case 0x19:    // Audio volume 0
    {
      createAudioSamples(addr, value);
      break;
    }
  
    case 0x1A:    // Audio volume 1
    {
      createAudioSamples(addr, value);
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
void TIA::createAudioSamples(uInt16 addr, uInt8 value)
{
  // If the sample rate is zero then we should not create any audio samples
  if(mySampleRate == 0)
  {
    return;
  }

  // Calculate the number of samples that need to be generated based on the
  // number of CPU cycles which have passed since the last sound update
  uInt32 samplesToGenerate = 
      (mySampleRate * (mySystem->cycles() - myLastSoundUpdateCycle)) / 1190000;

  // Update counters and create samples if there's one sample to generate
  // TODO: This doesn't handle rounding quite right (10/08/2002)
  if(samplesToGenerate >= 1)
  {
    uInt8 buffer[1024];

    for(Int32 sg = (Int32)samplesToGenerate; sg > 0; sg -= 1024)
    {
      Tia_process(buffer, ((sg >= 1024) ? 1024 : sg));
      mySampleQueue.enqueue(buffer, ((sg >= 1024) ? 1024 : sg));
    }

    myLastSoundUpdateCycle = myLastSoundUpdateCycle + 
        ((samplesToGenerate * 1190000) / mySampleRate);
  }

  if(addr != 0)
  {
    Update_tia_sound(addr, value);
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
const uInt32 TIA::ourFontData[36] = { 
  0x699f999, // A
  0xe99e99e, // B
  0x6988896, // C
  0xe99999e, // D
  0xf88e88f, // E
  0xf88e888, // F
  0x698b996, // G
  0x999f999, // H
  0x7222227, // I
  0x72222a4, // J
  0x9accaa9, // K
  0x888888f, // L
  0x9ff9999, // M
  0x9ddbb99, // N
  0x6999996, // O
  0xe99e888, // P
  0x69999b7, // Q
  0xe99ea99, // R
  0x6986196, // S
  0x7222222, // T
  0x9999996, // U
  0x9999966, // V
  0x9999ff9, // W
  0x99fff99, // X
  0x9996244, // Y
  0xf12488f, // Z
  0x69bd996, // 0
  0x2622227, // 1
  0x691248f, // 2
  0x6916196, // 3
  0xaaaf222, // 4
  0xf88e11e, // 5
  0x698e996, // 6
  0xf112244, // 7
  0x6996996, // 8
  0x6997196  // 9
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TIA::TIA(const TIA& c)
    : myConsole(c.myConsole),
      myCOLUBK(myColor[0]),
      myCOLUPF(myColor[1]),
      myCOLUP0(myColor[2]),
      myCOLUP1(myColor[3]),
      mySampleQueue(1024)
{
  assert(false);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TIA& TIA::operator = (const TIA&)
{
  assert(false);

  return *this;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TIA::SampleQueue::SampleQueue(uInt32 capacity)
    : myCapacity(capacity),
      myBuffer(0),
      mySize(0),
      myHead(0),
      myTail(0)
{
  myBuffer = new uInt8[myCapacity];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TIA::SampleQueue::~SampleQueue()
{
  delete[] myBuffer;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIA::SampleQueue::clear()
{
  myHead = myTail = mySize = 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 TIA::SampleQueue::dequeue(uInt8* buffer, uInt32 size)
{
  // We can only dequeue up to the number of items in the queue
  if(size > mySize)
  {
    size = mySize;
  }

  if((myHead + size) < myCapacity)
  {
    memcpy((void*)buffer, (const void*)(myBuffer + myHead), size);
    myHead += size;
  }
  else
  {
    uInt32 s1 = myCapacity - myHead;
    uInt32 s2 = size - s1;
    memcpy((void*)buffer, (const void*)(myBuffer + myHead), s1);
    memcpy((void*)(buffer + s1), (const void*)myBuffer, s2);
    myHead = (myHead + size) % myCapacity;
  }

  mySize -= size;

  return size;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TIA::SampleQueue::enqueue(uInt8* buffer, uInt32 size)
{
  // If an attempt is made to enqueue more than the queue can hold then
  // we'll only enqueue the last myCapacity elements.
  if(size > myCapacity)
  {
    buffer += (size - myCapacity);
    size = myCapacity;
  }

  if((myTail + size) < myCapacity)
  {
    memcpy((void*)(myBuffer + myTail), (const void*)buffer, size);
    myTail += size;
  }
  else
  {
    uInt32 s1 = myCapacity - myTail;
    uInt32 s2 = size - s1;
    memcpy((void*)(myBuffer + myTail), (const void*)buffer, s1);
    memcpy((void*)myBuffer, (const void*)(buffer + s1), s2);
    myTail = (myTail + size) % myCapacity;
  }

  if((mySize + size) > myCapacity)
  {
    myHead = (myHead + ((mySize + size) - myCapacity)) % myCapacity;
    mySize = myCapacity;
  }
  else
  {
    mySize += size;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 TIA::SampleQueue::size() const
{
  return mySize;
}

