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
// Copyright (c) 1995-2005 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: System.cxx,v 1.10 2005-06-21 05:00:46 urchlay Exp $
//============================================================================

#include <assert.h>
#include <iostream>

#include "Device.hxx"
#include "M6502.hxx"
#include "System.hxx"
#include "Serializer.hxx"
#include "Deserializer.hxx"

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

  // Set up (de)serializer in case we are asked to save/load state
  serializer = new Serializer();
  deserializer = new Deserializer();
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

  // Free the serializer stuff
  if(serializer)
    delete serializer;
  if(deserializer)
    delete deserializer;
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
bool System::save(Serializer& out)
{
  string name = "System";

  try
  {
    out.putString(name);
    out.putLong(myCycles);
  }
  catch(char *msg)
  {
    cerr << msg << endl;
    return false;
  }
  catch(...)
  {
    cerr << "Unknown error in save state for " << name << endl;
    return false;
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool System::load(Deserializer& in)
{
  string name = "System";

  try
  {
    if(in.getString() != name)
      return false;

    myCycles = (uInt32) in.getLong();
  }
  catch(char *msg)
  {
    cerr << msg << endl;
    return false;
  }
  catch(...)
  {
    cerr << "Unknown error in load state for " << name << endl;
    return false;
  }

  return true;
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
int System::saveState(const string& fileName, const string& md5sum)
{
  // Open the file as a new Serializer
  if(!serializer->open(fileName))
  {
    serializer->close();
    return 2;
  }

  // Prepend the state file with the md5sum of this cartridge
  // This is the first defensive check for an invalid state file
  serializer->putString(md5sum);

  // First save state for this system
  if(!save(*serializer))
  {
    serializer->close();
    return 3;
  }

  // Next, save state for the CPU
  if(!myM6502->save(*serializer))
  {
    serializer->close();
    return 3;
  }

  // Now save the state of each device
  for(uInt32 i = 0; i < myNumberOfDevices; ++i)
  {
    if(!myDevices[i]->save(*serializer))
    {
      serializer->close();
      return 3;
    }
  }

  serializer->close();
  return 1;  // success
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int System::loadState(const string &fileName, const string& md5sum)
{
  // Open the file as a new Deserializer
  if(!deserializer->open(fileName))
  {
    deserializer->close();
    return 2;
  }

  // Look at the beginning of the state file.  It should contain the md5sum
  // of the current cartridge.  If it doesn't, this state file is invalid.
  if(deserializer->getString() != md5sum)
  {
    deserializer->close();
    return 3;
  }

  // First load state for this system
  if(!load(*deserializer))
  {
    deserializer->close();
    return 3;
  }

  // Next, load state for the CPU
  if(!myM6502->load(*deserializer))
  {
    deserializer->close();
    return 3;
  }

  // Now load the state of each device
  for(uInt32 i = 0; i < myNumberOfDevices; ++i)
  {
    if(!myDevices[i]->load(*deserializer))
    {
      deserializer->close();
      return 3;
    }
  }

  deserializer->close();
  return 1;  // success
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

  myDataBusState = value;
}
