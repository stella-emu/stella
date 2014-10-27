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
// Copyright (c) 1995-2014 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

#include <cassert>
#include <iostream>

#include "Device.hxx"
#include "M6502.hxx"
#include "M6532.hxx"
#include "TIA.hxx"
#include "System.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
System::System(const OSystem& osystem)
  : myOSystem(osystem),
    myNumberOfDevices(0),
    myM6502(0),
    myM6532(0),
    myTIA(0),
    myCycles(0),
    myDataBusState(0),
    myDataBusLocked(false),
    mySystemInAutodetect(false)
{
  // Re-initialize random generator
  randGenerator().initSeed();

  // Allocate page table and dirty list
  myPageAccessTable = new PageAccess[NUM_PAGES];
  myPageIsDirtyTable = new bool[NUM_PAGES];

  // Initialize page access table
  PageAccess access(&myNullDevice, System::PA_READ);
  for(int page = 0; page < NUM_PAGES; ++page)
  {
    setPageAccess(page, access);
    myPageIsDirtyTable[page] = false;
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

  // Free my page access table and dirty list
  delete[] myPageAccessTable;
  delete[] myPageIsDirtyTable;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void System::reset(bool autodetect)
{
  // Provide hint to devices that autodetection is active (or not)
  mySystemInAutodetect = autodetect;

  // Reset system cycle counter
  resetCycles();

  // First we reset the devices attached to myself
  for(uInt32 i = 0; i < myNumberOfDevices; ++i)
    myDevices[i]->reset();

  // Now we reset the processor if it exists
  if(myM6502 != 0)
    myM6502->reset();

  // There are no dirty pages upon startup
  clearDirtyPages();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void System::attach(Device* device)
{
  assert(myNumberOfDevices < 5);

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
  attach(static_cast<Device*>(m6532));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void System::attach(TIA* tia)
{
  myTIA = tia;
  attach(static_cast<Device*>(tia));
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
  assert(page < NUM_PAGES);

  // Make sure the access methods make sense
  assert(access.device != 0);

  myPageAccessTable[page] = access;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const System::PageAccess& System::getPageAccess(uInt16 page) const
{
  // Make sure the page is within range
  assert(page < NUM_PAGES);

  return myPageAccessTable[page];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
System::PageAccessType System::getPageAccessType(uInt16 addr) const
{
  return myPageAccessTable[(addr & ADDRESS_MASK) >> PAGE_SHIFT].type;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void System::setDirtyPage(uInt16 addr)
{
  myPageIsDirtyTable[(addr & ADDRESS_MASK) >> PAGE_SHIFT] = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool System::isPageDirty(uInt16 start_addr, uInt16 end_addr) const
{
  uInt16 start_page = (start_addr & ADDRESS_MASK) >> PAGE_SHIFT;
  uInt16 end_page = (end_addr & ADDRESS_MASK) >> PAGE_SHIFT;

  for(uInt16 page = start_page; page <= end_page; ++page)
    if(myPageIsDirtyTable[page])
      return true;

  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void System::clearDirtyPages()
{
  for(uInt32 i = 0; i < NUM_PAGES; ++i)
    myPageIsDirtyTable[i] = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 System::peek(uInt16 addr, uInt8 flags)
{
  PageAccess& access = myPageAccessTable[(addr & ADDRESS_MASK) >> PAGE_SHIFT];

#ifdef DEBUGGER_SUPPORT
  // Set access type
  if(access.codeAccessBase)
    *(access.codeAccessBase + (addr & PAGE_MASK)) |= flags;
  else
    access.device->setAccessFlags(addr, flags);
#endif

  // See if this page uses direct accessing or not 
  uInt8 result;
  if(access.directPeekBase)
    result = *(access.directPeekBase + (addr & PAGE_MASK));
  else
    result = access.device->peek(addr);

#ifdef DEBUGGER_SUPPORT
  if(!myDataBusLocked)
#endif
    myDataBusState = result;

  return result;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void System::poke(uInt16 addr, uInt8 value)
{
  uInt16 page = (addr & ADDRESS_MASK) >> PAGE_SHIFT;
  PageAccess& access = myPageAccessTable[page];

  // See if this page uses direct accessing or not 
  if(access.directPokeBase)
  {
    // Since we have direct access to this poke, we can dirty its page
    *(access.directPokeBase + (addr & PAGE_MASK)) = value;
    myPageIsDirtyTable[page] = true;
  }
  else
  {
    // The specific device informs us if the poke succeeded
    myPageIsDirtyTable[page] = access.device->poke(addr, value);
  }

#ifdef DEBUGGER_SUPPORT
  if(!myDataBusLocked)
#endif
    myDataBusState = value;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 System::getAccessFlags(uInt16 addr) const
{
#ifdef DEBUGGER_SUPPORT
  PageAccess& access = myPageAccessTable[(addr & ADDRESS_MASK) >> PAGE_SHIFT];

  if(access.codeAccessBase)
    return *(access.codeAccessBase + (addr & PAGE_MASK));
  else
    return access.device->getAccessFlags(addr);
#else
  return 0;
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void System::setAccessFlags(uInt16 addr, uInt8 flags)
{
#ifdef DEBUGGER_SUPPORT
  PageAccess& access = myPageAccessTable[(addr & ADDRESS_MASK) >> PAGE_SHIFT];

  if(access.codeAccessBase)
    *(access.codeAccessBase + (addr & PAGE_MASK)) |= flags;
  else
    access.device->setAccessFlags(addr, flags);
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool System::save(Serializer& out) const
{
  try
  {
    out.putString(name());
    out.putInt(myCycles);
    out.putByte(myDataBusState);

    if(!myM6502->save(out))
      return false;

    // Now save the state of each device
    for(uInt32 i = 0; i < myNumberOfDevices; ++i)
      if(!myDevices[i]->save(out))
        return false;
  }
  catch(...)
  {
    cerr << "ERROR: System::save" << endl;
    return false;
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool System::load(Serializer& in)
{
  try
  {
    if(in.getString() != name())
      return false;

    myCycles = in.getInt();
    myDataBusState = in.getByte();

    // Next, load state for the CPU
    if(!myM6502->load(in))
      return false;

    // Now load the state of each device
    for(uInt32 i = 0; i < myNumberOfDevices; ++i)
      if(!myDevices[i]->load(in))
        return false;
  }
  catch(...)
  {
    cerr << "ERROR: System::load" << endl;
    return false;
  }

  return true;
}
