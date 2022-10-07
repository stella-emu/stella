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
// Copyright (c) 1995-2022 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include "TimerMap.hxx"

/*
  TODOs:
    x unordered_multimap (not required for just a few timers)
    o 13 vs 16 bit, use ADDRESS_MASK & ANY_BANK, when???
    ? timer line display in disassembly? (color, symbol,...?)
*/

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 TimerMap::add(const uInt16 fromAddr, const uInt16 toAddr,
                     const uInt8 fromBank, const uInt8 toBank)
{
  const TimerPoint tpFrom(fromAddr, fromBank);
  const TimerPoint tpTo(toAddr, toBank);
  const Timer complete(tpFrom, tpTo);

  myList.push_back(complete);
  myFromMap.insert(TimerPair(tpFrom, &myList.back()));
  myToMap.insert(TimerPair(tpTo, &myList.back()));

  return size() - 1;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 TimerMap::add(const uInt16 addr, const uInt8 bank)
{
  const uInt32 idx = size() - 1;
  const bool isPartialTimer = size() && get(idx).isPartial;
  const TimerPoint tp(addr, bank);

  if(!isPartialTimer)
  {
    const Timer partial(tp);

    myList.push_back(partial);
    myFromMap.insert(TimerPair(tp, &myList.back()));
  }
  else
  {
    Timer& partial = myList[idx];

    partial.setTo(tp);
    myToMap.insert(TimerPair(tp, &partial));
  }
  return size() - 1;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool TimerMap::erase(const uInt32 idx)
{
  if(size() > idx)
  {
    const Timer* timer = &myList[idx];
    const TimerPoint tpFrom(timer->from);
    const TimerPoint tpTo(timer->to);

    // Find address in from and to maps, TODO what happens if not found???
    const auto from = myFromMap.equal_range(tpFrom);
    for(auto it = from.first; it != from.second; ++it)
      if(it->second == timer)
      {
        myFromMap.erase(it);
        break;
      }

    const auto to = myToMap.equal_range(tpTo);
    for(auto it = to.first; it != to.second; ++it)
      if(it->second == timer)
      {
        myToMap.erase(it);
        break;
      }

    // Finally remove from list
    myList.erase(myList.begin() + idx);
    return true;
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TimerMap::clear()
{
  myList.clear();
  myFromMap.clear();
  myToMap.clear();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TimerMap::reset()
{
  for(auto it = myList.begin(); it != myList.end(); ++it)
    it->reset();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TimerMap::update(const uInt16 addr, const uInt8 bank,
                      const uInt64 cycles)
{
  // 13 bit timerpoint
  if((addr & ADDRESS_MASK) != addr)
  {
    TimerPoint tp(addr, bank); // -> addr & ADDRESS_MASK

    // Find address in from and to maps
    const auto from = myFromMap.equal_range(tp);
    for(auto it = from.first; it != from.second; ++it)
      if(!it->second->isPartial)
        it->second->start(cycles);

    const auto to = myToMap.equal_range(tp);
    for(auto it = to.first; it != to.second; ++it)
      if(!it->second->isPartial)
        it->second->stop(cycles);
  }

  // 16 bit timerpoint
  TimerPoint tp(addr, bank, false); // -> addr

  // Find address in from and to maps
  const auto from = myFromMap.equal_range(tp);
  for(auto it = from.first; it != from.second; ++it)
    if(!it->second->isPartial)
      it->second->start(cycles);

  const auto to = myToMap.equal_range(tp);
  for(auto it = to.first; it != to.second; ++it)
    if(!it->second->isPartial)
      it->second->stop(cycles);
}
