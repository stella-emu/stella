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
// Copyright (c) 1995-2020 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#ifndef COMPOSITE_KEY_VALUE_REPOSITORY_HXX
#define COMPOSITE_KEY_VALUE_REPOSITORY_HXX

#include "KeyValueRepository.hxx"
#include "bspf.hxx"

class CompositeKeyValueRepository
{
  public:

    virtual ~CompositeKeyValueRepository() = default;

    virtual shared_ptr<KeyValueRepository> get(const string& key) = 0;

    virtual bool has(const string& key) = 0;
};

#endif // COMPOSITE_KEY_VALUE_REPOSITORY_HXX
