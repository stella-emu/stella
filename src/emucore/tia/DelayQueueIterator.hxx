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

#ifndef TIA_DELAY_QUEUE_ITERATOR
#define TIA_DELAY_QUEUE_ITERATOR

#include "bspf.hxx"
#include "DelayQueue.hxx"
#include "DelayQueueMember.hxx"

class DelayQueueIterator
{
  public:
    DelayQueueIterator(const DelayQueue&);

    bool isValid() const;

    uInt8 delay() const;

    uInt8 address() const;

    uInt8 value() const;

    bool next();

  private:
    uInt8 currentIndex() const {
      return (myDelayQueue.myIndex +  myDelayCycle) % myDelayQueue.myMembers.size();
    }

  private:
    const DelayQueue& myDelayQueue;

    uInt8 myDelayCycle;
    DelayQueueMember::iterator myCurrentIterator;
};

#endif // TIA_DELAY_QUEUE_ITERATOR
