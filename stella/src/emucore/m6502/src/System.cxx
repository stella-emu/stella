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
// Copyright (c) 1995-2002 by Bradford W. Mott
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: System.cxx,v 1.2 2001-12-30 18:36:02 bwmott Exp $
//============================================================================

#include <assert.h>
#include "Device.hxx"
#include "M6502.hxx"
#include "System.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
System::System(uInt16 n, uInt16 m)
    : myAddressMask((1 << n) - 1),
      myPageShift(m),
      myPageMask((1 << m) - 1),
      myNumberOfPages(1 << (n - m)),
      myNumberOfDevices(0),
      myM6502(0),
      myCycles(0),
      myDataBusState(0)
{
  // Make sure the arguments are reasonable
  assert((1 <= m) && (m <= n) && (n <= 16));

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
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void System::reset()
{
  // Reset system cycle counter
  resetCycles();

  // Frist we reset the devices attached to myself
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
void System::resetCycles()
{
  // Frist we let all of the device attached to me know about the reset
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


