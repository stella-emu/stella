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

#include "TimerMap.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 TimerMap::add(uInt16 fromAddr, uInt16 toAddr,
                     uInt8 fromBank, uInt8 toBank,
                     bool mirrors, bool anyBank)
{
  myList.emplace_back(TimerPoint(fromAddr, fromBank),
                      TimerPoint(toAddr,   toBank), mirrors, anyBank);
  return size() - 1;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 TimerMap::add(uInt16 addr, uInt8 bank, bool mirrors, bool anyBank)
{
  const TimerPoint tp(addr, bank);
  const auto it = std::ranges::find_if(myList, &Timer::isPartial);

  if(it == myList.end())
  {
    // create a new partial timer:
    myList.emplace_back(tp, mirrors, anyBank);
    return size() - 1;
  }
  else
  {
    // complete a partial timer:
    it->setTo(tp, mirrors, anyBank);
    return static_cast<uInt32>(it - myList.begin());
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool TimerMap::erase(uInt32 idx)
{
  if(idx < size())
  {
    myList.erase(myList.begin() + idx);
    return true;
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TimerMap::reset()
{
  for(auto& t: myList)
    t.reset();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TimerMap::update(uInt16 addr, uInt8 bank, uInt64 cycles)
{
  for(auto& t: myList)
  {
    if(t.isPartial) continue;

    const uInt16 a = t.mirrors ? (addr & ADDRESS_MASK) : addr;
    if(a == t.from.addr && (t.anyBank || bank == t.from.bank))
      t.start(cycles);
    if(a == t.to.addr && (t.anyBank || bank == t.to.bank))
      t.stop(cycles);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool TimerMap::save(Serializer& out) const
{
  try
  {
    out.putInt(static_cast<uInt32>(myList.size()));
    for(const auto& t: myList)
      t.save(out);
  }
  catch(...)
  {
    cerr << "ERROR: TimerMap::save\n";
    return false;
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool TimerMap::load(Serializer& in)
{
  try
  {
    clear();
    const uInt32 count = in.getInt();
    // Sanity cap: timers are hand-created in the debugger, so a huge count is
    // a corrupt save file.  Reject rather than attempt a giant allocation
    if(count > 0x10000)
      return false;
    myList.resize(count);
    for(auto& t: myList)
      t.load(in);
  }
  catch(...)
  {
    cerr << "ERROR: TimerMap::load\n";
    return false;
  }

  return true;
}
