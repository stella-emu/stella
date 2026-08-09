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

#ifndef CHEAT_HXX
#define CHEAT_HXX

class OSystem;

#include "bspf.hxx"

/**
  Abstract base class for a single cheat: holds its name/code and
  enabled state, decoded and applied by each derived class.

  @author  Stephen Anthony
*/
class Cheat
{
  public:
    // Creates a cheat; name defaults to code when empty
    Cheat(OSystem& osystem, string_view name, string_view code)
      : myOSystem{osystem},
        myName{name.empty() ? code : name},
        myCode{code} { }
    virtual ~Cheat() = default;

    // True while the cheat is active
    bool enabled() const { return myEnabled; }
    // Display name of the cheat
    string_view name() const { return myName; }
    // Hex-encoded cheat code
    string_view code() const { return myCode; }

    // Activates the cheat; returns the new enabled state
    virtual bool enable() = 0;
    // Deactivates the cheat, restoring any patched state
    virtual bool disable() = 0;

    // Applies the cheat's effect
    virtual void evaluate() = 0;

  protected:
    // Parent system, used to reach console/cartridge/RAM
    OSystem& myOSystem;

    // Display name of the cheat
    string myName;
    // Hex-encoded cheat code
    string myCode;

    // True while the cheat is active
    bool myEnabled{false};

  private:
    // Following constructors and assignment operators not supported
    Cheat() = delete;
    Cheat(const Cheat&) = delete;
    Cheat(Cheat&&) = delete;
    Cheat& operator=(const Cheat&) = delete;
    Cheat& operator=(Cheat&&) = delete;
};

#endif  // CHEAT_HXX
