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

#ifndef LATCHED_INPUT_HXX
#define LATCHED_INPUT_HXX

#include "bspf.hxx"
#include "Serializable.hxx"

/**
  Models the TIA latched digital input circuit for INPT4 and INPT5
  (joystick fire buttons). When latch mode is enabled via VBLANK bit 6,
  a low signal is captured and held until the latch is explicitly cleared
  by a subsequent VBLANK write with bit 6 cleared.

  @author  Christian Speckner (DirtyHairy)
*/
class LatchedInput : public Serializable
{
  public:
    LatchedInput() = default;
    ~LatchedInput() override = default;

    /**
      Reset to initial state.
     */
    void reset();

    /**
      Process a VBLANK write. Bit 6 controls latching: when set, a low signal
      is captured and held until the latch is cleared by a subsequent VBLANK
      write with bit 6 cleared.
     */
    void vblank(uInt8 value);

    /**
      Is the input currently in latched mode?
     */
    bool vblankLatched() const { return myModeLatched; }

    /**
      Read INPT4 or INPT5. Returns 0x80 while the pin is high (button not
      pressed), or 0x00 if low (or latched low). Result is in bit 7.
     */
    uInt8 inpt(bool pinState);

    /**
      Serializable methods (see that class for more information).
    */
    bool save(Serializer& out) const override;
    bool load(Serializer& in) override;

  private:
    // Whether latched mode is active (VBLANK bit 6)
    bool myModeLatched{false};
    // Held at 0x00 once a low signal is captured
    uInt8 myLatchedValue{0};

  private:
    // Following constructors and assignment operators not supported
    LatchedInput(const LatchedInput&) = delete;
    LatchedInput(LatchedInput&&) = delete;
    LatchedInput& operator=(const LatchedInput&) = delete;
    LatchedInput& operator=(LatchedInput&&) = delete;
};

#endif  // LATCHED_INPUT_HXX
