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

#ifndef ANALOG_READOUT_HXX
#define ANALOG_READOUT_HXX

#include "bspf.hxx"
#include "Serializable.hxx"
#include "ConsoleTiming.hxx"

/**
  Models the TIA analog input RC circuit for paddle controllers (INPT0–3).
  Simulates capacitor charge toward U_SUPP through the pot resistance, or
  discharge through R_DUMP when the VBLANK dump bit is set, and reports
  when the capacitor voltage crosses the comparator threshold.

  @author  Christian Speckner (DirtyHairy)
*/
class AnalogReadout : public Serializable
{
  public:
    // How the analog input pin is connected to the external circuit
    enum class ConnectionType : uInt8 {
      // Capacitor is discharged through R_DUMP (dump active)
      ground = 0,
      // Capacitor charges through R0 + pot resistance toward U_SUPP
      vcc = 1,
      // Capacitor holds its current charge
      disconnected = 2
    };

    struct Connection {
      // How the pin is connected
      ConnectionType type{ConnectionType::ground};
      // Pot resistance in ohms
      uInt32 resistance{0};

      bool save(Serializer& out) const;
      bool load(Serializer& in);

      friend bool operator==(const AnalogReadout::Connection& c1, const AnalogReadout::Connection& c2);
    };

  public:
    AnalogReadout();
    ~AnalogReadout() override = default;

    /**
      Reset to initial state, using the given system timestamp.
     */
    void reset(uInt64 timestamp);

    /**
      Process a VBLANK write. Bit 7 controls ground dump: when set the
      capacitor is discharged through R_DUMP, resetting the timing circuit.
     */
    void vblank(uInt8 value, uInt64 timestamp);

    /**
      Is the dump (ground discharge) currently active?
     */
    bool vblankDumped() const { return myIsDumped; }

    /**
      Read INPT0–3. Returns 0x80 while the capacitor voltage is below the
      comparator threshold (charging), then 0x00 once it crosses.
     */
    uInt8 inpt(uInt64 timestamp);

    /**
      Update the connection type; called when a controller event changes the
      paddle resistance or connection state.
     */
    void update(Connection connection, uInt64 timestamp, ConsoleTiming consoleTiming);

    /**
      Serializable methods (see that class for more information).
    */
    bool save(Serializer& out) const override;
    bool load(Serializer& in) override;

  public:
    /**
      Create a grounded connection, representing an active paddle with the
      given pot resistance.
     */
    static constexpr Connection connectToGround(uInt32 resistance = 0) {
      return Connection{ConnectionType::ground, resistance};
    }

    /**
      Create a VCC connection (capacitor charges with no pot resistance).
     */
    static constexpr Connection connectToVcc(uInt32 resistance = 0) {
      return Connection{ConnectionType::vcc, resistance};
    }

    /**
      Create a disconnected connection (capacitor holds its charge).
     */
    static constexpr Connection disconnect() {
      return Connection{ConnectionType::disconnected, 0};
    }

  private:
    /**
      Update the comparator threshold voltage for the current console timing.
     */
    void setConsoleTiming(ConsoleTiming timing);

    /**
      Advance the RC charge simulation to the given timestamp.
     */
    void updateCharge(uInt64 timestamp);

  private:
    // Comparator threshold voltage (derived from TRIPPOINT_LINES)
    double myUThresh{0.0};
    // Current capacitor voltage
    double myU{0.0};

    // Current connection state
    Connection myConnection{ConnectionType::disconnected, 0};
    // Timestamp of the last charge update
    uInt64 myTimestamp{0};

    // Timing mode (affects clock frequency)
    ConsoleTiming myConsoleTiming{ConsoleTiming::ntsc};
    // Color clock frequency derived from console timing
    double myClockFreq{0.0};

    // Whether the dump (ground discharge) is currently active
    bool myIsDumped{false};

    // RC circuit parameters
    static constexpr double
      R0 = 1.8e3,   // 1.8kΩ series resistor in the TIA analog input circuit
      C = 68e-9,    // 68nF timing capacitor
      R_POT = 1e6,  // 1MΩ maximum pot resistance
      R_DUMP = 50,  // 50Ω dump resistor
      U_SUPP = 5;   // 5V supply voltage

    // Comparator trip point in scanline units
    static constexpr double TRIPPOINT_LINES = 379;

  private:
    // Following constructors and assignment operators not supported
    AnalogReadout(const AnalogReadout&) = delete;
    AnalogReadout(AnalogReadout&&) = delete;
    AnalogReadout& operator=(const AnalogReadout&) = delete;
    AnalogReadout& operator=(AnalogReadout&&) = delete;
};

bool operator==(const AnalogReadout::Connection& c1, const AnalogReadout::Connection& c2);
bool operator!=(const AnalogReadout::Connection& c1, const AnalogReadout::Connection& c2);

#endif  // ANALOG_READOUT_HXX
