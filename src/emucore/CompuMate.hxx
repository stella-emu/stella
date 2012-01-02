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
// Copyright (c) 1995-2012 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

#ifndef COMPUMATE_HXX
#define COMPUMATE_HXX

#include "bspf.hxx"
#include "Control.hxx"
#include "Event.hxx"

/**
  The Spectravideo CompuMate SV010 was a home computer expansion for the
  Atari VCS / 2600 video game system.

  It consists of a membrane keyboard unit with interface connectors. These
  connectors were placed in the module slot and both controller ports of the
  Atari console.  As the user could place the keyboard on the old style VCS
  consoles, the two devices resulted in one compact unit. When using with the
  2600jr console, resulted in a desktop computer look with separated keyboard.

  The CompuMate was equipped with an audio jack for use with a standard tape
  connector as a possibility of permanent data storage.

  Bankswitching is done though the controller ports
    INPT0: D7 = CTRL key input (0 on startup / 1 = key pressed)
    INPT1: D7 = always HIGH input (tested at startup)
    INPT2: D7 = always HIGH input (tested at startup)
    INPT3: D7 = SHIFT key input (0 on startup / 1 = key pressed)
    INPT4: D7 = keyboard row 1 input (0 = key pressed)
    INPT5: D7 = keyboard row 3 input (0 = key pressed)
    SWCHA: D7 = tape recorder I/O ?
           D6 = 1 -> increase key collumn (0 to 9)
           D5 = 1 -> reset key collumn to 0 (if D4 = 0)
           D5 = 0 -> enable RAM writing (if D4 = 1)
           D4 = 1 -> map 2K of RAM at $1800 - $1fff
           D3 = keyboard row 4 input (0 = key pressed)
           D2 = keyboard row 2 input (0 = key pressed)
           D1 = bank select high bit
           D0 = bank select low bit

  Keyboard column numbering:
    column 0 = 7 U J M
    column 1 = 6 Y H N
    column 2 = 8 I K ,
    column 3 = 2 W S X
    column 4 = 3 E D C
    column 5 = 0 P ENTER SPACE
    column 6 = 9 O L .
    column 7 = 5 T G B
    column 8 = 1 Q A Z
    column 9 = 4 R F V

  This code was heavily borrowed from z26, and uses conventions defined
  there.  Specifically, IOPortA is treated as a complete uInt8, whereas
  the Stella core actually stores this information in boolean arrays
  addressable by DigitalPin number.

  @author  Stephen Anthony & z26 team
  @version $Id$
*/
class CompuMate : public Controller
{
  public:
    /**
      Create a new MindLink controller plugged into the specified jack

      @param jack   The jack the controller is plugged into
      @param event  The event object to use for events
      @param system The system using this controller
    */
    CompuMate(Jack jack, const Event& event, const System& system);

    /**
      Destructor
    */
    virtual ~CompuMate();

  public:
    /**
      Called after *all* digital pins have been written on Port A.
    */
    void controlWrite();

    /**
      Update the entire digital and analog pin state according to the
      events currently set.
    */
    void update();

  private:
    // Internal state of the port pins
    uInt8 myIOPort;
};

#endif
