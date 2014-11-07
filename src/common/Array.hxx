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
// Copyright (c) 1995-2014 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

#ifndef ARRAY_HXX
#define ARRAY_HXX

#include <cassert>

#include "bspf.hxx"

namespace Common {

template <class T>
class Array : public vector<T>
{
  public:
    void append(const Array<T>& array)
    {
      this->insert(this->end(), array.begin(), array.end());
    }

    void insertAt(uInt32 idx, const T& element)
    {
      this->insert(this->cbegin()+idx, element);
    }

    void removeAt(uInt32 idx)
    {
      this->erase(this->cbegin()+idx);
    }
};

}  // Namespace Common

// Common array types
class IntArray  : public Common::Array<Int32> { };
class BoolArray : public Common::Array<bool>  { };
class ByteArray : public Common::Array<uInt8> { };

#endif
