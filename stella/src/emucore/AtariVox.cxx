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
// Copyright (c) 1995-2008 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: AtariVox.cxx,v 1.10 2008-04-09 17:19:15 stephena Exp $
//============================================================================

#ifdef SPEAKJET_EMULATION
  #include "SpeakJet.hxx"
#endif

#include "SerialPort.hxx"
#include "System.hxx"
#include "AtariVox.hxx"

#define DEBUG_ATARIVOX 0

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AtariVox::AtariVox(Jack jack, const Event& event, const SerialPort& port)
  : Controller(jack, event, Controller::AtariVox),
    mySerialPort((SerialPort*)&port),
    myPinState(0),
    myShiftCount(0),
    myShiftRegister(0),
    myLastDataWriteCycle(0)
{
#ifdef SPEAKJET_EMULATION
  mySpeakJet = new SpeakJet();
#endif

  myDigitalPinState[One] = myDigitalPinState[Two] =
  myDigitalPinState[Three] = myDigitalPinState[Four] = true;

  myAnalogPinValue[Five] = myAnalogPinValue[Nine] = maximumResistance;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AtariVox::~AtariVox()
{
#ifdef SPEAKJET_EMULATION
  delete mySpeakJet;
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AtariVox::write(DigitalPin pin, bool value)
{
  if(DEBUG_ATARIVOX)
    cerr << "AtariVox: write to SWCHA" << endl;

  // Change the pin state based on value
  switch(pin)
  {
    // Pin 1: SpeakJet DATA
    //        output serial data to the speakjet
    case One:
      clockDataIn(value);
      break;
  
    // Pin 2: SpeakJet READY
    case Two:
      // TODO - see how this is used
      break;

    // Pin 3: EEPROM SDA
    //        output data to the 24LC256 EEPROM using the I2C protocol
    case Three:
      // TODO - implement this
      if(DEBUG_ATARIVOX)
        cerr << "AtariVox: value "
             << value
             << " written to SDA line at cycle "
             << mySystem->cycles()
             << endl;
      break;

    // Pin 4: EEPROM SCL
    //        output clock data to the 24LC256 EEPROM using the I2C protocol
    case Four:
      // TODO - implement this
      if(DEBUG_ATARIVOX)
        cerr << "AtariVox: value "
             << value
             << " written to SCLK line at cycle "
             << mySystem->cycles()
             << endl;
      break;

    case Six:
      // Not connected
      break;
  } 
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AtariVox::update()
{
  // Nothing to do, this seems to be an output-only device for now
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AtariVox::clockDataIn(bool value)
{
  // bool oldValue = myPinState & 0x01;
  myPinState = (myPinState & 0xfe) | (int)value;

  uInt32 cycle = mySystem->cycles();
  if(DEBUG_ATARIVOX)
    cerr << "AtariVox: value "
         << value
         << " written to DATA line at "
         << mySystem->cycles()
         << " (-"
         << myLastDataWriteCycle
         << "=="
         << (mySystem->cycles() - myLastDataWriteCycle)
         << ")"
         << endl;

  if(value && (myShiftCount == 0))
  {
    if(DEBUG_ATARIVOX)
      cerr << "value && (myShiftCount == 0), returning" << endl;
    return;
  }

  // If this is the first write this frame, or if it's been a long time
  // since the last write, start a new data byte.
  if(cycle < myLastDataWriteCycle || cycle > myLastDataWriteCycle + 1000)
  {
    myShiftRegister = 0;
    myShiftCount = 0;
  }

  // If this is the first write this frame, or if it's been 62 cycles
  // since the last write, shift this bit into the current byte.
  if(cycle < myLastDataWriteCycle || cycle >= myLastDataWriteCycle + 62)
  {
    if(DEBUG_ATARIVOX)
      cerr << "cycle >= myLastDataWriteCycle + 62, shiftIn("
           << value << ")" << endl;
    shiftIn(value);
  }

  myLastDataWriteCycle = cycle;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AtariVox::shiftIn(bool value)
{
  myShiftRegister >>= 1;
  myShiftRegister |= (value << 15);
  if(++myShiftCount == 10)
  {
    myShiftCount = 0;
    myShiftRegister >>= 6;
    if(!(myShiftRegister & (1<<9)))
      cerr << "AtariVox: bad start bit" << endl;
    else if((myShiftRegister & 1))
      cerr << "AtariVox: bad stop bit" << endl;
    else
    {
      uInt8 data = ((myShiftRegister >> 1) & 0xff);
#ifdef SPEAKJET_EMULATION
      mySpeakJet->write(data);
#endif
      mySerialPort->writeByte(data);
    }
    myShiftRegister = 0;
  }
}
