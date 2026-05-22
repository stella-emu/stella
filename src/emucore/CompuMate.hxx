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

#ifndef COMPUMATE_HXX
#define COMPUMATE_HXX

class Console;
class Event;
class FSNode;
class System;

#include "bspf.hxx"
#include "Control.hxx"
#include "CompuMateCassette.hxx"

/**
  Handler for SpectraVideo CompuMate bankswitched games.

  The specifics of the CompuMate format can be found in the Cart side
  (CartCM), the Controller side (CMControl) and the cassette handling
  (CompuMateCassette).  The CompuMate device is unique for the 2600 in
  that it requires close co-operation between the cartridge and the
  left and right controllers.

  This class acts as a 'parent' for cartridge, cassette and both the left
  and right CMControl's, taking care of their creation and communication
  between them.  Keyboard trigger detection (Func+J for LOAD, Func+H for SAVE)
  lives here; all cassette I/O is delegated to CompuMateCassette.

  @author  Stephen Anthony
*/
class CompuMate
{
  public:
    /**
      Create a new CompuMate handler for both left and right ports.
      Note that this class creates CMControl controllers for both ports,
      but does not take responsibility for their deletion.

      @param console  The console that owns the controller
      @param event    The event object to use for events
      @param system   The system using this controller
    */
    CompuMate(const Console& console, const Event& event, const System& system);
    ~CompuMate() = default;

    /**
      Return the left and right CompuMate controllers
    */
    unique_ptr<Controller>& leftController()  { return myLeftController;  }
    unique_ptr<Controller>& rightController() { return myRightController; }

    /**
      Load a cassette .bin image from the given ROM's sibling file.
      Called by Console immediately after construction.
      NOTE: eventually we'll use a BrowserWidget to ask the user for the
            file to load.
    */
    void loadCassette(const FSNode& romFile);

    /**
      Called by CartCM whenever SWCHA bit 6 (D6, audio-out/CLK) changes.
      Forwarded to CompuMateCassette's FSK decoder during save.
    */
    void cassetteD6Toggled(uInt64 cycles) { myCassette.cassetteD6Toggled(cycles); }

    bool isRecording() const { return myCassette.isRecording(); }
    void checkTimeout(uInt64 currentCycle) { myCassette.checkTimeout(currentCycle); }

    /**
      Return the current cassette waveform bit for SWCHA D7.
      Returns 1 when no cassette is loaded (pulled high = no signal).
    */
    uInt8 cassetteBit() const { return myCassette.cassetteBit(); }

    /**
      Cancel a pending cassette load (e.g. when a file-browser dialog is
      dismissed).  Resets the Func+J / Enter state machine so the user
      must press Func+J → Enter again to start a new load.
    */
    void cancelCassetteLoad() { myLoadArm.reset(); }

    /**
      Cancel a pending cassette save (e.g. when a file-browser dialog is
      dismissed).  Resets the Func+H / Enter state machine so the user
      must press Func+H → Enter again to start a new save.
    */
    void cancelCassetteSave() { mySaveArm.reset(); }

    /** Needed for communication with CartCM class */
    uInt8& column() { return myColumn; }

  private:
    /**
      Called by the controller(s) when all pins have been written
      This method keeps track of consecutive calls, and only updates once
    */
    void update();

    // The actual CompuMate controller
    // More information about these scheme can be found in CartCM.hxx
    class CMControl : public Controller
    {
      public:
        /**
          Create a new CMControl controller plugged into the specified jack

          @param handler  Class which coordinates between left & right controllers
          @param jack     The jack the controller is plugged into
          @param event    The event object to use for events
          @param system   The system using this controller
        */
        CMControl(class CompuMate& handler, Controller::Jack jack, const Event& event,
                  const System& system)
          : Controller(jack, event, system, Controller::Type::CompuMate),
            myHandler{handler}
        {
          if(jack == Controller::Jack::Left) {
            setPin(AnalogPin::Nine, AnalogReadout::connectToGround());
            setPin(AnalogPin::Five, AnalogReadout::connectToVcc());
          } else {
            setPin(AnalogPin::Nine, AnalogReadout::connectToVcc());
            setPin(AnalogPin::Five, AnalogReadout::connectToGround());
          }
        }
        ~CMControl() override = default;

        using Controller::setPin;

      public:
        /**
          Called after *all* digital pins have been written on Port A.
          Only update on the left controller; the right controller will
          happen at the same cycle and is redundant.
        */
        void controlWrite(uInt8) override {
          if(myJack == Controller::Jack::Left) myHandler.update();
        }

        /**
          Update the entire digital and analog pin state according to the
          events currently set.
        */
        void update() override { }

        /**
          Returns the name of this controller.
        */
        string name() const override { return "CompuMate"; }

      private:
        class CompuMate& myHandler;

        // Following constructors and assignment operators not supported
        CMControl() = delete;
        CMControl(const CMControl&) = delete;
        CMControl(CMControl&&) = delete;
        CMControl& operator=(const CMControl&) = delete;
        CMControl& operator=(CMControl&&) = delete;
    };

  private:
    // Console, Event, and System objects
    const Console& myConsole;
    const Event&   myEvent;
    const System&  mySystem;

    // Left and right controllers
    unique_ptr<Controller> myLeftController, myRightController;

    // Column currently active
    uInt8 myColumn{0};

    // Cassette tape emulation
    CompuMateCassette myCassette;

    // Shared state for the Func+J (LOAD) and Func+H (SAVE) arm sequences
    struct ArmState {
      bool   seen{false};
      uInt8  bufLen{0};
      bool   waitingForRelease{false};
      bool   prevEnter{false};
      bool   prevBackspace{false};
      bool   prevAnyChar{false};

      void reset() { *this = {}; }
    };
    FSNode   myPendingLoadPath;  // set by dialog (or default sibling) at Func+J time
    ArmState myLoadArm;
    ArmState mySaveArm;

  private:
    // Following constructors and assignment operators not supported
    CompuMate() = delete;
    CompuMate(const CompuMate&) = delete;
    CompuMate(CompuMate&&) = delete;
    CompuMate& operator=(const CompuMate&) = delete;
    CompuMate& operator=(CompuMate&&) = delete;
};

#endif  // COMPUMATE_HXX
