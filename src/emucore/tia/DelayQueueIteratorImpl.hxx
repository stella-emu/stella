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
// Copyright (c) 1995-2026 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#ifndef DELAY_QUEUE_ITERATOR_IMPL_HXX
#define DELAY_QUEUE_ITERATOR_IMPL_HXX

#include "bspf.hxx"
#include "DelayQueue.hxx"
#include "DelayQueueIterator.hxx"

/**
  Concrete iterator over a DelayQueue<length, capacity>. Traverses all
  pending register writes in chronological order, exposing the remaining
  delay, TIA register address, and write value for each entry.

  @author  Christian Speckner (DirtyHairy)
*/
template<unsigned length, unsigned capacity>
class DelayQueueIteratorImpl : public DelayQueueIterator
{
  public:
    explicit DelayQueueIteratorImpl(const DelayQueue<length, capacity>& delayQueue);

  public:
    bool isValid() const override;
    uInt8 delay() const override;
    uInt8 address() const override;
    uInt8 value() const override;
    bool next() override;

  private:
    /**
      Compute the actual circular-buffer index for the current delay cycle.
     */
    uInt8 currentIndex() const;

  private:
    // The queue being iterated
    const DelayQueue<length, capacity>& myDelayQueue;
    // How many clocks ahead of myDelayQueue.myIndex we currently are
    uInt8 myDelayCycle{0};
    // Entry index within the current delay slot
    uInt8 myIndex{0};
};

// ############################################################################
// Implementation
// ############################################################################

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<unsigned length, unsigned capacity>
DelayQueueIteratorImpl<length, capacity>::DelayQueueIteratorImpl(
  const DelayQueue<length, capacity>& delayQueue
)
  : myDelayQueue(delayQueue)
{
  while (myDelayCycle < length && myDelayQueue.myMembers[currentIndex()].mySize == 0)
    myDelayCycle++;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<unsigned length, unsigned capacity>
bool DelayQueueIteratorImpl<length, capacity>::isValid() const
{
  return myDelayCycle < length;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<unsigned length, unsigned capacity>
uInt8 DelayQueueIteratorImpl<length, capacity>::delay() const
{
  if (!isValid()) {
    throw std::runtime_error("delay called on invalid DelayQueueInterator");
  }

  return myDelayCycle;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<unsigned length, unsigned capacity>
uInt8 DelayQueueIteratorImpl<length, capacity>::address() const
{
  if (!isValid()) {
    throw std::runtime_error("address called on invalid DelayQueueInterator");
  }

  return myDelayQueue.myMembers[currentIndex()].myEntries[myIndex].address;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<unsigned length, unsigned capacity>
uInt8 DelayQueueIteratorImpl<length, capacity>::value() const
{
  if (!isValid()) {
    throw std::runtime_error("value called on invalid DelayQueueInterator");
  }

  return myDelayQueue.myMembers[currentIndex()].myEntries[myIndex].value;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<unsigned length, unsigned capacity>
bool DelayQueueIteratorImpl<length, capacity>::next()
{
  if (!isValid()) return false;

  if (++myIndex < myDelayQueue.myMembers[currentIndex()].mySize)
    return true;

  myIndex = 0;

  do {
    ++myDelayCycle;
  } while (myDelayQueue.myMembers[currentIndex()].mySize == 0 && isValid());

  return isValid();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<unsigned length, unsigned capacity>
uInt8 DelayQueueIteratorImpl<length, capacity>::currentIndex() const
{
  return (myDelayQueue.myIndex + myDelayCycle) % length;
}

#endif  // DELAY_QUEUE_ITERATOR_IMPL_HXX
