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

#ifndef DELAY_QUEUE_ITERATOR_HXX
#define DELAY_QUEUE_ITERATOR_HXX

#include "bspf.hxx"

/**
  Abstract iterator for inspecting pending entries in a DelayQueue without
  modifying it. Exposes the delay, address, and value for each pending
  write; concrete implementations are provided by DelayQueueIteratorImpl.

  @author  Christian Speckner (DirtyHairy)
*/
class DelayQueueIterator
{
  public:
    DelayQueueIterator() = default;
    virtual ~DelayQueueIterator() = default;

    /**
      Is the iterator positioned on a valid entry?
     */
    virtual bool isValid() const = 0;

    /**
      Number of clocks until the current entry's write executes.
     */
    virtual uInt8 delay() const = 0;

    /**
      TIA register address of the current entry.
     */
    virtual uInt8 address() const = 0;

    /**
      Value to be written for the current entry.
     */
    virtual uInt8 value() const = 0;

    /**
      Advance to the next entry. Returns false when there are no more entries.
     */
    virtual bool next() = 0;

  private:
    // Following constructors and assignment operators not supported
    DelayQueueIterator(const DelayQueueIterator&) = delete;
    DelayQueueIterator(DelayQueueIterator&&) = delete;
    DelayQueueIterator& operator=(const DelayQueueIterator&) = delete;
    DelayQueueIterator& operator=(DelayQueueIterator&&) = delete;
};

#endif  // DELAY_QUEUE_ITERATOR_HXX
