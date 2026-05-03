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

#include "BreakpointMap.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BreakpointMap::add(const Breakpoint& breakpoint, uInt32 flags)
{
  myInitialized = true;
  myMap[masked(breakpoint)] = flags;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BreakpointMap::add(uInt16 addr, uInt8 bank, uInt32 flags)
{
  add(Breakpoint(addr, bank), flags);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BreakpointMap::erase(const Breakpoint& breakpoint)
{
  // 16 bit breakpoint
  if(!myMap.erase(breakpoint))
  {
    // 13 bit breakpoint
    myMap.erase(masked(breakpoint));
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BreakpointMap::erase(uInt16 addr, uInt8 bank)
{
  erase(Breakpoint(addr, bank));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 BreakpointMap::get(const Breakpoint& breakpoint) const
{
  const auto it = find(breakpoint);
  return it != myMap.end() ? it->second : 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 BreakpointMap::get(uInt16 addr, uInt8 bank) const
{
  return get(Breakpoint(addr, bank));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BreakpointMap::check(const Breakpoint& breakpoint) const
{
  return find(breakpoint) != myMap.end();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BreakpointMap::check(uInt16 addr, uInt8 bank) const
{
  return check(Breakpoint(addr, bank));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BreakpointMap::BreakpointList BreakpointMap::getBreakpoints() const
{
  BreakpointList list;
  list.reserve(myMap.size());
  for(const auto& [bp, _]: myMap)
    list.push_back(bp);
  std::ranges::sort(list);
  return list;
}
