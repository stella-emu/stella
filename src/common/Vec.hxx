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

#ifndef VECTOR_OPS_HXX
#define VECTOR_OPS_HXX

#include "bspf.hxx"

namespace Vec {

template<typename T>
void append(vector<T>& dst, const vector<T>& src)
{
  dst.reserve(dst.size() + src.size());
  dst.insert(dst.end(), src.begin(), src.end());
}
template<typename T>
void append(vector<T>& dst, vector<T>&& src)
{
  if(dst.empty())
    dst = std::move(src);
  else
  {
    dst.reserve(dst.size() + src.size());
    dst.insert(dst.end(),
               std::make_move_iterator(src.begin()),
               std::make_move_iterator(src.end()));
  }
  src.clear();
}

template<typename T, typename U>
void insertAt(vector<T>& dst, size_t idx, U&& element)
{
  assert(idx <= dst.size());
  dst.insert(dst.begin() + idx, std::forward<U>(element));
}

template<typename T>
void removeAt(vector<T>& dst, size_t idx)
{
  assert(idx < dst.size());
  dst.erase(dst.begin() + idx);
}

} // namespace Vec

#endif  // VECTOR_OPS_HXX
