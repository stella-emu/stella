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

#include "DelayQueueIterator.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DelayQueueIterator::DelayQueueIterator(const DelayQueue& delayQueue)
  : myDelayQueue(delayQueue),
    myDelayCycle(0)
{
  while (isValid()) {
    const DelayQueueMember& currentMember = myDelayQueue.myMembers.at(currentIndex());
    myCurrentIterator = currentMember.begin();

    if (myCurrentIterator == currentMember.end())
      myDelayCycle++;
    else
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool DelayQueueIterator::isValid() const
{
  return myDelayCycle < myDelayQueue.myMembers.size();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 DelayQueueIterator::delay() const
{
  if (!isValid()) {
    throw runtime_error("delay called on invalid DelayQueueInterator");
  }

  return myDelayCycle;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 DelayQueueIterator::address() const
{
  if (!isValid()) {
    throw runtime_error("address called on invalid DelayQueueInterator");
  }

  return myCurrentIterator->address;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 DelayQueueIterator::value() const
{
  if (!isValid()) {
    throw runtime_error("value called on invalid DelayQueueIterator");
  }

  return myCurrentIterator->value;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool DelayQueueIterator::next()
{
  if (!isValid()) {
    return false;
  }

  if (++myCurrentIterator ==  myDelayQueue.myMembers.at(currentIndex()).end()) {
    myDelayCycle++;

    while (isValid()) {
      const DelayQueueMember& currentMember = myDelayQueue.myMembers.at(currentIndex());
      myCurrentIterator = currentMember.begin();

      if (myCurrentIterator == currentMember.end())
        myDelayCycle++;
      else
        break;
    }
  }

  return isValid();
}
