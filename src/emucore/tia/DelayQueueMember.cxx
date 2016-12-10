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

  for (index = 0; index < mySize; index++) {
    if (myEntries.at(index).address == address) break;
  }

  if (index < mySize) {
    myEntries.at(index) = myEntries.at(mySize - 1);
    mySize--;
  }
}
