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
// Copyright (c) 1995-1998 by Bradford W. Mott
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: M6532.cxx,v 1.1.1.1 2001-12-27 19:54:22 bwmott Exp $
//============================================================================

#include <assert.h>
#include "Console.hxx"
#include "M6532.hxx"
#include "Random.hxx"
#include "Switches.hxx"
#include "System.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
M6532::M6532(const Console& console)
    : myConsole(console)
{
  // Randomize the 128 bytes of memory
  Random random;

  for(uInt32 t = 0; t < 128; ++t)
  {
    myRAM[t] = random.next();
  }

  // Initialize other data members
  reset();
}
 
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
M6532::~M6532()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const char* M6532::name() const
{
  return "M6532";
}
 
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void M6532::reset()
{
  myTimer = 100;
  myIntervalShift = 6;
  myCyclesWhenTimerSet = 0;
  myCyclesWhenInterruptReset = 0;
  myTimerReadAfterInterrupt = false;

  // Zero the I/O registers
  myDDRA = 0x00;
  myDDRB = 0x00;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void M6532::systemCyclesReset()
{
  // System cycles are being reset to zero so we need to adjust
  // the cycle count we remembered when the timer was last set
  myCyclesWhenTimerSet -= mySystem->cycles();
  myCyclesWhenInterruptReset -= mySystem->cycles();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void M6532::install(System& system)
{
  // Remember which system I'm installed in
  mySystem = &system;

  uInt16 shift = mySystem->pageShift();
  uInt16 mask = mySystem->pageMask();

  // Make sure the system we're being installed in has a page size that'll work
  assert((0x1080 & mask) == 0);
  
  // All accesses are to this device
  System::PageAccess access;
  access.device = this;

  // We're installing in a 2600 system
  for(int address = 0; address < 8192; address += (1 << shift))
  {
    if((address & 0x1080) == 0x0080)
    {
      if((address & 0x0200) == 0x0000)
      {
        access.directPeekBase = &myRAM[address & 0x007f];
        access.directPokeBase = &myRAM[address & 0x007f];
        mySystem->setPageAccess(address >> shift, access);
      }
      else
      {
        access.directPeekBase = 0; 
        access.directPokeBase = 0;
        mySystem->setPageAccess(address >> shift, access);
      }
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 M6532::peek(uInt16 addr)
{
  switch(addr & 0x07)
  {
    case 0x00:    // Port A I/O Register (Joystick)
    {
      uInt8 value = 0x00;

      if(myConsole.controller(Controller::Left).read(Controller::One))
        value |= 0x10;
      if(myConsole.controller(Controller::Left).read(Controller::Two))
        value |= 0x20;
      if(myConsole.controller(Controller::Left).read(Controller::Three))
        value |= 0x40;
      if(myConsole.controller(Controller::Left).read(Controller::Four))
        value |= 0x80;

      if(myConsole.controller(Controller::Right).read(Controller::One))
        value |= 0x01;
      if(myConsole.controller(Controller::Right).read(Controller::Two))
        value |= 0x02;
      if(myConsole.controller(Controller::Right).read(Controller::Three))
        value |= 0x04;
      if(myConsole.controller(Controller::Right).read(Controller::Four))
        value |= 0x08;

      return value;
    }

    case 0x01:    // Port A Data Direction Register 
    {
      return myDDRA;
    }

    case 0x02:    // Port B I/O Register (Console switches)
    {
      return myConsole.switches().read();
    }

    case 0x03:    // Port B Data Direction Register
    {
      return myDDRB;
    }

    case 0x04:    // Timer Output
    case 0x06:
    {
      uInt32 cycles = mySystem->cycles() - 1;
      uInt32 delta = cycles - myCyclesWhenTimerSet;
      Int32 timer = (Int32)myTimer - (Int32)(delta >> myIntervalShift) - 1;

      // See if the timer has expired yet?
      if(timer >= 0)
      {
        return (uInt8)timer; 
      }
      else
      {
        timer = (Int32)(myTimer << myIntervalShift) - (Int32)delta - 1;

        if((timer <= -2) && !myTimerReadAfterInterrupt)
        {
          // Indicate that timer has been read after interrupt occured
          myTimerReadAfterInterrupt = true;
          myCyclesWhenInterruptReset = mySystem->cycles();
        }

        if(myTimerReadAfterInterrupt)
        {
          Int32 offset = myCyclesWhenInterruptReset - 
              (myCyclesWhenTimerSet + (myTimer << myIntervalShift));

          timer = (Int32)myTimer - (Int32)(delta >> myIntervalShift) - offset;
        }

        return (uInt8)timer;
      }
    }

    case 0x05:    // Interrupt Flag
    case 0x07:
    {
      uInt32 cycles = mySystem->cycles() - 1;
      uInt32 delta = cycles - myCyclesWhenTimerSet;
      Int32 timer = (Int32)myTimer - (Int32)(delta >> myIntervalShift) - 1;

      if((timer >= 0) || myTimerReadAfterInterrupt)
        return 0x00;
      else
        return 0x80;
    }

    default:
    {    
#ifdef DEBUG_ACCESSES
      cerr << "BAD M6532 Peek: " << hex << addr << endl;
#endif
      return 0;
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void M6532::poke(uInt16 addr, uInt8 value)
{
  if((addr & 0x07) == 0x00)         // Port A I/O Register (Joystick)
  {
    uInt8 a = value & myDDRA;

    myConsole.controller(Controller::Left).write(Controller::One, a & 0x10);
    myConsole.controller(Controller::Left).write(Controller::Two, a & 0x20);
    myConsole.controller(Controller::Left).write(Controller::Three, a & 0x40);
    myConsole.controller(Controller::Left).write(Controller::Four, a & 0x80);
    
    myConsole.controller(Controller::Right).write(Controller::One, a & 0x01);
    myConsole.controller(Controller::Right).write(Controller::Two, a & 0x02);
    myConsole.controller(Controller::Right).write(Controller::Three, a & 0x04);
    myConsole.controller(Controller::Right).write(Controller::Four, a & 0x08);
  }
  else if((addr & 0x07) == 0x01)    // Port A Data Direction Register 
  {
    myDDRA = value;
  }
  else if((addr & 0x07) == 0x02)    // Port B I/O Register (Console switches)
  {
    return;
  }
  else if((addr & 0x07) == 0x03)    // Port B Data Direction Register
  {
//        myDDRB = value;
    return;
  }
  else if((addr & 0x17) == 0x14)    // Write timer divide by 1 
  {
    myTimer = value;
    myIntervalShift = 0;
    myCyclesWhenTimerSet = mySystem->cycles();
    myTimerReadAfterInterrupt = false;
  }
  else if((addr & 0x17) == 0x15)    // Write timer divide by 8
  {
    myTimer = value;
    myIntervalShift = 3;
    myCyclesWhenTimerSet = mySystem->cycles();
    myTimerReadAfterInterrupt = false;
  }
  else if((addr & 0x17) == 0x16)    // Write timer divide by 64
  {
    myTimer = value;
    myIntervalShift = 6;
    myCyclesWhenTimerSet = mySystem->cycles();
    myTimerReadAfterInterrupt = false;
  }
  else if((addr & 0x17) == 0x17)    // Write timer divide by 1024
  {
    myTimer = value;
    myIntervalShift = 10;
    myCyclesWhenTimerSet = mySystem->cycles();
    myTimerReadAfterInterrupt = false;
  }
  else if((addr & 0x14) == 0x04)    // Write Edge Detect Control
  {
#ifdef DEBUG_ACCESSES
    cerr << "M6532 Poke (Write Edge Detect): "
        << ((addr & 0x02) ? "PA7 enabled" : "PA7 disabled")
        << ", "
        << ((addr & 0x01) ? "Positive edge" : "Negative edge")
        << endl;
#endif
  }
  else
  {
#ifdef DEBUG_ACCESSES
    cerr << "BAD M6532 Poke: " << hex << addr << endl;
#endif
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
M6532::M6532(const M6532& c)
    : myConsole(c.myConsole)
{
  assert(false);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
M6532& M6532::operator = (const M6532&)
{
  assert(false);

  return *this;
}
 
