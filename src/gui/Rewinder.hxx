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

#ifndef REWINDER_HXX
#define REWINDER_HXX

class OSystem;

#include "DialogContainer.hxx"

/**
  The base dialog for all rewind-related UI items in Stella.

  @author  Stephen Anthony
*/
class Rewinder : public DialogContainer
{
  public:
    Rewinder(OSystem& osystem);
    virtual ~Rewinder() = default;

  private:
    // Following constructors and assignment operators not supported
    Rewinder() = delete;
    Rewinder(const Rewinder&) = delete;
    Rewinder(Rewinder&&) = delete;
    Rewinder& operator=(const Rewinder&) = delete;
    Rewinder& operator=(Rewinder&&) = delete;
};

#endif
