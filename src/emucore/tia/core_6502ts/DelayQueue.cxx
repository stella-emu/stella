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
// Copyright (c) 1995-2016 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

#include "DelayQueue.hxx"

namespace TIA6502tsCore {

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
}

} // namespace TIA6502tsCore
