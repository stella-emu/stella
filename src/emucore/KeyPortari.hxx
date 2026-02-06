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

#ifndef KEYPORTARI_HXX
#define KEYPORTARI_HXX

class Console;
class Event;
class System;

#include "bspf.hxx"
#include "Control.hxx"
#include "Logger.hxx"


/**
  Handler for  KeyPortari.
  @author  Dave Christianson
*/
class KeyPortari
{
  public:
  
    /**
       KeyPortari protocol setting
     */
    enum Protocol {
      AlphaNumeric, // AlphaNumeric characters with port forwarding
      Ascii         // Full ASCII range, no port forwarding
    };
  
    /**
      Create a new KeyPortari handler for both left and right ports.
      Note that this class creates KeyPortari controllers for both ports,
      but does not take responsibility for their deletion.

      @param console  The console that owns the controller
      @param event    The event object to use for events
      @param system   The system using this controller
    */
    KeyPortari(const Protocol protocol, const Event& event);
    ~KeyPortari() = default;
    /**
      Bind to a controller jack
    */
    void bindController(const Controller::Jack jack, const Event& event, const System& system, unique_ptr<Controller>& bindingController, unique_ptr<Controller>& passthroughController);
  
  private:

    struct KeyCodeMapping {
      Event::Type event;
      uint8_t code;
    };

    using KeyCodeMappingArray = std::vector<KeyCodeMapping>;

    uint8_t mapKeyCode();
  
    static KeyCodeMappingArray AlphaNumericKeyCodeMappingArray;
    static KeyCodeMappingArray AsciiKeyCodeMappingArray;
  
    // The actual KeyPortari controller
    class KPControl : public Controller
    {
      public:
        /**
          Create a new KPControl controller plugged into the specified jack

          @param handler  Class which coordinates between left & right controllers
          @param jack     The jack the controller is plugged into
          @param event    The event object to use for events
          @param system   The system using this controller
        */
        KPControl(class KeyPortari& handler, Controller::Jack jack, const Event& event, const System& system);
        ~KPControl() override = default;

    public:

        void setPassthroughController(unique_ptr<Controller> &passthroughController) {
          myPassthroughController = std::move(passthroughController);
        }
      
        void update() override;
      
        /**
          Returns the name of this controller.
        */
        string name() const override { return "KeyPortari"; }

      private:
        class KeyPortari& myHandler;
        unique_ptr<Controller> myPassthroughController;
        
        // Following constructors and assignment operators not supported
        KPControl() = delete;
        KPControl(const KPControl&) = delete;
        KPControl(KPControl&&) = delete;
        KPControl& operator=(const KPControl&) = delete;
        KPControl& operator=(KPControl&&) = delete;
    };

  private:

    // KeyCode mapping protocol
    Protocol myProtocol;
  
    const Event &myEvent;

  private:
    // Following constructors and assignment operators not supported
    KeyPortari(const KeyPortari&) = delete;
    KeyPortari(KeyPortari&&) = delete;
    KeyPortari& operator=(const KeyPortari&) = delete;
    KeyPortari& operator=(KeyPortari&&) = delete;
};

#endif
