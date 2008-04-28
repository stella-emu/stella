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
// Copyright (c) 1995-2008 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: M6532.cxx,v 1.23 2008-04-28 21:31:40 stephena Exp $
//============================================================================

#include <assert.h>
#include "Console.hxx"
#include "M6532.hxx"
#include "Random.hxx"
#include "Switches.hxx"
#include "System.hxx"
#include "Serializer.hxx"
#include "Deserializer.hxx"
#include <iostream>

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
M6532::M6532(const Console& console)
  : myConsole(console)
{
  // Randomize the 128 bytes of memory
  class Random random;

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
void M6532::reset()
{
  class Random random;

  myTimer = 25 + (random.next() % 75);
  myIntervalShift = 6;
  myCyclesWhenTimerSet = 0;
  myInterruptEnabled = false;
  myInterruptTriggered = false;

  // Zero the I/O registers
  myDDRA = myDDRB = myOutA = 0x00;

  // Zero the timer registers
  myOutTimer[0] = myOutTimer[1] = myOutTimer[2] = myOutTimer[3] = 0x00;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void M6532::systemCyclesReset()
{
  // System cycles are being reset to zero so we need to adjust
  // the cycle count we remembered when the timer was last set
  myCyclesWhenTimerSet -= mySystem->cycles();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void M6532::install(System& system)
{
  install(system, *this);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void M6532::install(System& system, Device& device)
{
  // Remember which system I'm installed in
  mySystem = &system;

  uInt16 shift = mySystem->pageShift();
  uInt16 mask = mySystem->pageMask();

  // Make sure the system we're being installed in has a page size that'll work
  assert((0x1080 & mask) == 0);
  
  // All accesses are to the given device
  System::PageAccess access;
  access.device = &device;

  // We're installing in a 2600 system
  for(int address = 0; address < 8192; address += (1 << shift))
  {
    if((address & 0x1080) == 0x0080)
    {
      access.directPeekBase = 0; 
      access.directPokeBase = 0;
      mySystem->setPageAccess(address >> shift, access);
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 M6532::peek(uInt16 addr)
{
  // Access RAM directly.  Originally, accesses to RAM could bypass
  // this method and its pages could be installed directly into the
  // system.  However, certain cartridges (notably 4A50) can mirror
  // the RAM address space, making it necessary to chain accesses.
  if((addr & 0x1080) == 0x0080 && (addr & 0x0200) == 0x0000)
  {
    return myRAM[addr & 0x007f];
  }

  switch(addr & 0x07)
  {
    case 0x00:    // Port A I/O Register (Joystick)
    {
      uInt8 value = 0x00;

      Controller& port0 = myConsole.controller(Controller::Left);
      if(port0.read(Controller::One))   value |= 0x10;
      if(port0.read(Controller::Two))   value |= 0x20;
      if(port0.read(Controller::Three)) value |= 0x40;
      if(port0.read(Controller::Four))  value |= 0x80;

      Controller& port1 = myConsole.controller(Controller::Right);
      if(port1.read(Controller::One))   value |= 0x01;
      if(port1.read(Controller::Two))   value |= 0x02;
      if(port1.read(Controller::Three)) value |= 0x04;
      if(port1.read(Controller::Four))  value |= 0x08;

      // Return the input bits set by the controller *and* the
      // output bits set by the last write to SWCHA
      return (value & ~myDDRA) | (myOutA & myDDRA);
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
      myInterruptTriggered = false;
      Int32 timer = timerClocks();

      if(timer >= 0)
      {
        return (uInt8)(timer >> myIntervalShift);
      }
      else
      {
        if(timer != -1)
          myInterruptTriggered = true;

        // According to the M6532 documentation, the timer continues to count
        // down to -255 timer clocks after wraparound.  However, it isn't
        // entirely clear what happens *after* if reaches -255.  If we go
        // to zero at that time, Solaris fails to load correctly.
        // However, if the count goes on forever, HERO fails to load
        // correctly.
        // So we use the approach of z26, and let the counter continue
        // downward (after wraparound) for the maximum number of clocks
        // (256 * 1024) = 0x40000.  I suspect this is a hack that works
        // for all the ROMs we've tested; it would be nice to determine
        // what really happens in hardware.
        return (uInt8)(timer >= -0x40000 ? timer : 0);
      }
    }

    case 0x05:    // Interrupt Flag
    case 0x07:
    {
      if((timerClocks() >= 0) || myInterruptEnabled && myInterruptTriggered)
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
  // Access RAM directly.  Originally, accesses to RAM could bypass
  // this method and its pages could be installed directly into the
  // system.  However, certain cartridges (notably 4A50) can mirror
  // the RAM address space, making it necessary to chain accesses.
  if((addr & 0x1080) == 0x0080 && (addr & 0x0200) == 0x0000)
  {
    myRAM[addr & 0x007f] = value;
    return;
  }

  // A2 distinguishes I/O registers from the timer
  if((addr & 0x04) != 0)
  {
    if((addr & 0x10) != 0)
    {
      myInterruptEnabled = (addr & 0x08);
      setTimerRegister(value, addr & 0x03);
    }
  }
  else
  {
    switch(addr & 0x03)
    {
      case 0:     // Port A I/O Register (Joystick)
      {
        myOutA = value;
        uInt8 a = myOutA & myDDRA;

        Controller& port0 = myConsole.controller(Controller::Left);
        port0.write(Controller::One, a & 0x10);
        port0.write(Controller::Two, a & 0x20);
        port0.write(Controller::Three, a & 0x40);
        port0.write(Controller::Four, a & 0x80);
    
        Controller& port1 = myConsole.controller(Controller::Right);
        port1.write(Controller::One, a & 0x01);
        port1.write(Controller::Two, a & 0x02);
        port1.write(Controller::Three, a & 0x04);
        port1.write(Controller::Four, a & 0x08);
        break;
      }

      case 1:     // Port A Data Direction Register 
      {
        myDDRA = value;

        /*
          Undocumented feature used by the AtariVox and SaveKey.
          When the DDR is changed, the ORA (output register A) cached
          from a previous write to SWCHA is 'applied' to the controller pins.

          When a bit in the DDR is set as input, +5V is placed on its output
          pin.  When it's set as output, either +5V or 0V (depending on the
          contents of SWCHA) will be placed on the output pin.
          The standard macros for the AtariVox use this fact to send data
          to the port.  This is represented by the following algorithm:

            if(DDR bit is input)       set output as 1
            else if(DDR bit is output) set output as bit in ORA
        */
        uInt8 a = myOutA | ~myDDRA;

        Controller& port0 = myConsole.controller(Controller::Left);
        port0.write(Controller::One, a & 0x10);
        port0.write(Controller::Two, a & 0x20);
        port0.write(Controller::Three, a & 0x40);
        port0.write(Controller::Four, a & 0x80);

        Controller& port1 = myConsole.controller(Controller::Right);
        port1.write(Controller::One, a & 0x01);
        port1.write(Controller::Two, a & 0x02);
        port1.write(Controller::Three, a & 0x04);
        port1.write(Controller::Four, a & 0x08);
        break;
      }

      default:    // Port B I/O & DDR Registers (Console switches)
        break;    // hardwired as read-only
     }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void M6532::setTimerRegister(uInt8 value, uInt8 interval)
{
  static const uInt8 shift[] = { 0, 3, 6, 10 };

  myInterruptTriggered = false;
  myIntervalShift = shift[interval];
  myOutTimer[interval] = value;
  myTimer = value << myIntervalShift;
  myCyclesWhenTimerSet = mySystem->cycles();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool M6532::save(Serializer& out) const
{
  string device = name();

  try
  {
    out.putString(device);

    // Output the RAM
    out.putInt(128);
    for(uInt32 t = 0; t < 128; ++t)
      out.putByte((char)myRAM[t]);

    out.putInt(myTimer);
    out.putInt(myIntervalShift);
    out.putInt(myCyclesWhenTimerSet);
    out.putBool(myInterruptEnabled);
    out.putBool(myInterruptTriggered);

    out.putByte((char)myDDRA);
    out.putByte((char)myDDRB);
    out.putByte((char)myOutA);
    out.putByte((char)myOutTimer[0]);
    out.putByte((char)myOutTimer[1]);
    out.putByte((char)myOutTimer[2]);
    out.putByte((char)myOutTimer[3]);
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
bool M6532::load(Deserializer& in)
{
  string device = name();

  try
  {
    if(in.getString() != device)
      return false;

    // Input the RAM
    uInt32 limit = (uInt32) in.getInt();
    for(uInt32 t = 0; t < limit; ++t)
      myRAM[t] = (uInt8) in.getByte();

    myTimer = (uInt32) in.getInt();
    myIntervalShift = (uInt32) in.getInt();
    myCyclesWhenTimerSet = (uInt32) in.getInt();
    myInterruptEnabled = in.getBool();
    myInterruptTriggered = in.getBool();

    myDDRA = (uInt8) in.getByte();
    myDDRB = (uInt8) in.getByte();
    myOutA = (uInt8) in.getByte();
    myOutTimer[0] = (uInt8) in.getByte();
    myOutTimer[1] = (uInt8) in.getByte();
    myOutTimer[2] = (uInt8) in.getByte();
    myOutTimer[3] = (uInt8) in.getByte();
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
