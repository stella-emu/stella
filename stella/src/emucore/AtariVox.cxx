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
// Copyright (c) 1995-2005 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: AtariVox.cxx,v 1.1 2006-06-09 02:45:11 urchlay Exp $
//============================================================================

#include "Event.hxx"
#include "AtariVox.hxx"

#define DEBUG_ATARIVOX 1

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AtariVox::AtariVox(Jack jack, const Event& event)
    : Controller(jack, event),
      myPinState(0),
      myShiftRegister(0),
      myShiftCount(0)
{
  myType = Controller::AtariVox;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AtariVox::~AtariVox()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AtariVox::read(DigitalPin pin)
{
  // For now, always return true, meaning the device is ready
/*
  if(DEBUG_ATARIVOX)
    cerr << "AtariVox: read from SWCHA" << endl;
*/
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Int32 AtariVox::read(AnalogPin)
{
  // Analog pins are not connected in AtariVox, so we have infinite resistance 
  return maximumResistance;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AtariVox::write(DigitalPin pin, bool value)
{
  if(DEBUG_ATARIVOX)
    cerr << "AtariVox: write to SWCHA" << endl;

  // Change the pin state based on value
  switch(pin)
  {
    // Pin 1 is the DATA line, used to output serial data to the
    // speakjet
    case One:
      if(DEBUG_ATARIVOX)
        cerr << "AtariVox: value " << value << " written to DATA line" << endl;
      myShiftRegister >>= 1;
      myShiftRegister |= (value << 15);
		// cerr << myShiftRegister << endl;
      if(++myShiftCount == 10) {
        myShiftCount = 0;
        myShiftRegister >>= 6;
		  // cerr << "(<<6) == " << myShiftRegister << endl;
        if(!(myShiftRegister & (1<<9)))
          cerr << "AtariVox: bad start bit" << endl;
        else if((myShiftRegister & 1))
          cerr << "AtariVox: bad stop bit" << endl;
        else
          cerr << "AtariVox: output byte "
               << ((myShiftRegister >> 1) & 0xff)
               << endl;
		  myShiftRegister = 0;
      }
      break;
  
    // Pin 2 is the SDA line, used to output data to the 24LC256
    // serial EEPROM, using the I2C protocol.
    // I'm not even trying to emulate this right now :(
    case Two:
      if(DEBUG_ATARIVOX)
        cerr << "AtariVox: value " << value << " written to SDA line" << endl;
      break;

    case Three:
    case Four:
    default:
      break;
  } 
}
