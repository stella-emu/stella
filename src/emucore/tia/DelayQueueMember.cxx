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
// Copyright (c) 1995-2017 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include "DelayQueueMember.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DelayQueueMember::DelayQueueMember(uInt8 size)
  : myEntries(size),
    mySize(0)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DelayQueueMember::push(uInt8 address, uInt8 value)
{
  Entry& entry = myEntries.at(mySize++);

  entry.address = address;
  entry.value = value;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DelayQueueMember::remove(uInt8 address)
{
  size_t index;

  for (index = 0; index < mySize; index++)
    if (myEntries.at(index).address == address)
      break;

  if (index < mySize) {
    myEntries.at(index) = myEntries.at(mySize - 1);
    mySize--;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool DelayQueueMember::save(Serializer& out) const
{
  try
  {
    out.putInt(mySize);
    for(uInt32 i = 0; i < mySize; ++i)
    {
      const Entry& e = myEntries[i];
      out.putByte(e.address);
      out.putByte(e.value);
    }
  }
  catch(...)
  {
    cerr << "ERROR: TIA_DelayQueueMember::save" << endl;
    return false;
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool DelayQueueMember::load(Serializer& in)
{
  try
  {
    mySize = in.getInt();
    for(uInt32 i = 0; i < mySize; ++i)
    {
      Entry& e = myEntries[i];
      e.address = in.getByte();
      e.value = in.getByte();
    }
  }
  catch(...)
  {
    cerr << "ERROR: TIA_DelayQueueMember::load" << endl;
    return false;
  }

  return true;
}
