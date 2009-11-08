//============================================================================
//
// MM     MM  6666  555555  0000   2222
// MMMM MMMM 66  66 55     00  00 22  22
// MM MMM MM 66     55     00  00     22
// MM  M  MM 66666  55555  00  00  22222  --  "A 6502 Microprocessor Emulator"
// MM     MM 66  66     55 00  00 22
// MM     MM 66  66 55  55 00  00 22
// MM     MM  6666   5555   0000  222222
//
// Copyright (c) 1995-2009 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

#include <assert.h>
#include <iostream>

#include "Device.hxx"
#include "M6502.hxx"
#include "M6532.hxx"
#include "TIA.hxx"
#include "System.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
System::System(uInt16 n, uInt16 m)
  : myAddressMask((1 << n) - 1),
    myPageShift(m),
    myPageMask((1 << m) - 1),
    myNumberOfPages(1 << (n - m)),
    myNumberOfDevices(0),
    myM6502(0),
    myTIA(0),
    myCycles(0),
    myDataBusState(0),
    myDataBusLocked(false)
{
  // Make sure the arguments are reasonable
  assert((1 <= m) && (m <= n) && (n <= 16));

  // Create a new random number generator
  myRandom = new Random();

  // Allocate page table
  myPageAccessTable = new PageAccess[myNumberOfPages];

  // Initialize page access table
  PageAccess access;
  access.directPeekBase = 0;
  access.directPokeBase = 0;
  access.device = &myNullDevice;
  for(int page = 0; page < myNumberOfPages; ++page)
  {
    setPageAccess(page, access);
  }

  // Bus starts out unlocked (in other words, peek() changes myDataBusState)
  myDataBusLocked = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
System::~System()
{
  // Free the devices attached to me, since I own them
  for(uInt32 i = 0; i < myNumberOfDevices; ++i)
  {
    delete myDevices[i];
  }

  // Free the M6502 that I own
  delete myM6502;

  // Free my page access table
  delete[] myPageAccessTable;

  // Free the random number generator
  delete myRandom;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void System::reset()
{
  // Reset system cycle counter
  resetCycles();

  // First we reset the devices attached to myself
  for(uInt32 i = 0; i < myNumberOfDevices; ++i)
  {
    myDevices[i]->reset();
  }

  // Now we reset the processor if it exists
  if(myM6502 != 0)
  {
    myM6502->reset();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void System::attach(Device* device)
{
  assert(myNumberOfDevices < 100);

  // Add device to my collection of devices
  myDevices[myNumberOfDevices++] = device;

  // Ask the device to install itself
  device->install(*this);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void System::attach(M6502* m6502)
{
  // Remember the processor
  myM6502 = m6502;

  // Ask the processor to install itself
  myM6502->install(*this);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void System::attach(M6532* m6532)
{
  // Remember the processor
  myM6532 = m6532;

  // Attach it as a normal device
  attach((Device*) m6532);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void System::attach(TIA* tia)
{
  myTIA = tia;
  attach((Device*) tia);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void System::resetCycles()
{
  // First we let all of the device attached to me know about the reset
  for(uInt32 i = 0; i < myNumberOfDevices; ++i)
  {
    myDevices[i]->systemCyclesReset();
  }

  // Now, we reset cycle count to zero
  myCycles = 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void System::setPageAccess(uInt16 page, const PageAccess& access)
{
  // Make sure the page is within range
  assert(page <= myNumberOfPages);

  // Make sure the access methods make sense
  assert(access.device != 0);

  myPageAccessTable[page] = access;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const System::PageAccess& System::getPageAccess(uInt16 page)
{
  // Make sure the page is within range
  assert(page <= myNumberOfPages);

  return myPageAccessTable[page];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
System::System(const System& s)
  : myAddressMask(s.myAddressMask),
    myPageShift(s.myPageShift),
    myPageMask(s.myPageMask),
    myNumberOfPages(s.myNumberOfPages)
{
  assert(false);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
System& System::operator = (const System&)
{
  assert(false);

  return *this;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 System::peek(uInt16 addr)
{
  PageAccess& access = myPageAccessTable[(addr & myAddressMask) >> myPageShift];

  uInt8 result;
 
  // See if this page uses direct accessing or not 
  if(access.directPeekBase != 0)
  {
    result = *(access.directPeekBase + (addr & myPageMask));
  }
  else
  {
    result = access.device->peek(addr);
  }

#ifdef DEBUGGER_SUPPORT
  if(!myDataBusLocked)
#endif
    myDataBusState = result;

  return result;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void System::poke(uInt16 addr, uInt8 value)
{
  PageAccess& access = myPageAccessTable[(addr & myAddressMask) >> myPageShift];
  
  // See if this page uses direct accessing or not 
  if(access.directPokeBase != 0)
  {
    *(access.directPokeBase + (addr & myPageMask)) = value;
  }
  else
  {
    access.device->poke(addr, value);
  }

#ifdef DEBUGGER_SUPPORT
  if(!myDataBusLocked)
#endif
    myDataBusState = value;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void System::lockDataBus()
{
  myDataBusLocked = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void System::unlockDataBus()
{
  myDataBusLocked = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool System::save(Serializer& out) const
{
  const string& device = name();
  try
  {
    out.putString(device);
    out.putInt(myCycles);

    if(!myM6502->save(out))
      return false;

    // Now save the state of each device
    for(uInt32 i = 0; i < myNumberOfDevices; ++i)
      if(!myDevices[i]->save(out))
        return false;
  }
  catch(const char* msg)
  {
    cerr << "ERROR: System::save" << endl << "  " << msg << endl;
    return false;
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool System::load(Serializer& in)
{
  const string& device = name();
  try
  {
    if(in.getString() != device)
      return false;

    myCycles = (uInt32) in.getInt();

    // Next, load state for the CPU
    if(!myM6502->load(in))
      return false;

    // Now load the state of each device
    for(uInt32 i = 0; i < myNumberOfDevices; ++i)
      if(!myDevices[i]->load(in))
        return false;
  }
  catch(const char* msg)
  {
    cerr << "ERROR: System::load" << endl << "  " << msg << endl;
    return false;
  }

  return true;
}
