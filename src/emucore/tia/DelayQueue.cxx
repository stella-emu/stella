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

#include "DelayQueue.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DelayQueue::DelayQueue(uInt8 length, uInt8 size)
  : myIndex(0)
{
  myMembers.reserve(length);

  for (uInt16 i = 0; i < length; i++)
    myMembers.emplace_back(size);

  memset(myIndices, 0xFF, 0xFF);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DelayQueue::push(uInt8 address, uInt8 value, uInt8 delay)
{
  uInt8 length = myMembers.size();

  if (delay >= length)
    throw runtime_error("delay exceeds queue length");

  uInt8 currentIndex = myIndices[address];

  if (currentIndex < 0xFF)
    myMembers.at(currentIndex).remove(address);

  uInt8 index = (myIndex + delay) % length;
  myMembers.at(index).push(address, value);

  myIndices[address] = index;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DelayQueue::reset()
{
  for (DelayQueueMember& member : myMembers)
    member.clear();

  memset(myIndices, 0xFF, 0xFF);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool DelayQueue::save(Serializer& out) const
{
  try
  {
    out.putInt(uInt32(myMembers.size()));
    for(const DelayQueueMember& m: myMembers)
      m.save(out);

    out.putByte(myIndex);
    out.putByteArray(myIndices, 0xFF);
  }
  catch(...)
  {
    cerr << "ERROR: TIA_DelayQueue::save" << endl;
    return false;
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool DelayQueue::load(Serializer& in)
{
  try
  {
    myMembers.resize(in.getInt());
    for(DelayQueueMember& m: myMembers)
      m.load(in);

    myIndex = in.getByte();
    in.getByteArray(myIndices, 0xFF);
  }
  catch(...)
  {
    cerr << "ERROR: TIA_DelayQueue::load" << endl;
    return false;
  }

  return true;
}
