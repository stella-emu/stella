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

#include "MT24LC256.hxx"
#include "OSystem.hxx"
#include "Settings.hxx"
#include "Serializer.hxx"
#include "SpeakJetSerial.hxx"
#include "SpeakJetSoftware.hxx"
#include "System.hxx"
#include "AtariVox.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AtariVox::AtariVox(Jack jack, const Event& event, const OSystem& osystem,
                   const System& system, const string& portname,
                   const FSNode& eepromfile, const onMessageCallback& callback)
  : Controller(jack, event, system, Controller::Type::AtariVox),
    myEEPROM{std::make_unique<MT24LC256>(eepromfile, system, callback)}
{
  // Talk to a real AtariVox when one is reachable on the configured port,
  // unless speech has been forced to software
  string deadPort;

  if(!portname.empty() &&
     osystem.settings().getString("avoxmode") != "software")
  {
    auto serial = std::make_unique<SpeakJetSerial>(portname);

    if(serial->isOpen())
      myBackend = std::move(serial);
    else
      // Tried and failed, so say which port in about(); choosing software
      // outright is not a failure and gets no such note
      deadPort = portname;
  }

  if(myBackend == nullptr)
    myBackend = std::make_unique<SpeakJetSoftware>(osystem, deadPort);

  setPin(DigitalPin::One, true);
  setPin(DigitalPin::Two, true);
  setPin(DigitalPin::Three, true);
  setPin(DigitalPin::Four, true);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AtariVox::~AtariVox() = default;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AtariVox::read(DigitalPin pin)
{
  // We need to override the Controller::read() method, since the timing
  // of the actual read is important for the EEPROM (we can't just read
  // 60 times per second in the ::update() method)
  switch(pin)
  {
    // Pin 2: SpeakJet READY
    //        READY signal is sent directly to pin 2
    case DigitalPin::Two:
      return setPin(pin, myBackend->ready());

    // Pin 3: EEPROM SDA
    //        input data from the 24LC256 EEPROM using the I2C protocol
    case DigitalPin::Three:
      return setPin(pin, myEEPROM->readSDA());

    default:
      return Controller::read(pin);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AtariVox::write(DigitalPin pin, bool value)
{
  // Change the pin state based on value
  switch(pin)
  {
    // Pin 1: SpeakJet DATA
    //        output serial data to the speakjet
    case DigitalPin::One:
      setPin(pin, value);
      clockDataIn(value);
      break;

    // Pin 3: EEPROM SDA
    //        output data to the 24LC256 EEPROM using the I2C protocol
    case DigitalPin::Three:
      setPin(pin, value);
      myEEPROM->writeSDA(value);
      break;

    // Pin 4: EEPROM SCL
    //        output clock data to the 24LC256 EEPROM using the I2C protocol
    case DigitalPin::Four:
      setPin(pin, value);
      myEEPROM->writeSCL(value);
      break;

    default:
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AtariVox::clockDataIn(bool value)
{
  if(value && (myShiftCount == 0))
    return;

  // If this is the first write this frame, or if it's been a long time
  // since the last write, start a new data byte.
  const uInt64 cycle = mySystem.cycles();
  if((cycle < myLastDataWriteCycle) || (cycle > myLastDataWriteCycle + 1000))
  {
    myShiftRegister = 0;
    myShiftCount = 0;
  }

  // If this is the first write this frame, or if it's been 62 cycles
  // since the last write, shift this bit into the current byte.
  if((cycle < myLastDataWriteCycle) || (cycle >= myLastDataWriteCycle + 62))
  {
    myShiftRegister >>= 1U;
    myShiftRegister |= (U32(value) << 15U);
    if(++myShiftCount == 10)
    {
      myShiftCount = 0;
      myShiftRegister >>= 6U;
      if(!(myShiftRegister & (1U<<9U)))
        cerr << "AtariVox: bad start bit\n";
      else if((myShiftRegister & 1U))
        cerr << "AtariVox: bad stop bit\n";
      else
      {
        const uInt8 data = ((U32(myShiftRegister) >> 1U) & 0xffU);
        myBackend->write(data);
      }
      myShiftRegister = 0;
    }
  }

  myLastDataWriteCycle = cycle;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AtariVox::update()
{
  myBackend->update();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AtariVox::reset()
{
  myLastDataWriteCycle = 0;
  // SpeakJetBackend::reset(), not unique_ptr::reset()
  (*myBackend).reset();
  myEEPROM->systemReset();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AtariVox::close()
{
  myEEPROM.reset();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AtariVox::eraseCurrent()
{
  myEEPROM->eraseCurrent();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AtariVox::isPageUsed(uInt32 page) const
{
  return myEEPROM->isPageUsed(page);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string AtariVox::about(bool swappedPorts) const
{
  return Controller::about(swappedPorts) + myBackend->about();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AtariVox::save(Serializer& out) const
{
  if(!(Controller::save(out) && myEEPROM->save(out))) return false;
  try
  {
    out.putByte(myShiftCount);
    out.putShort(myShiftRegister);
    out.putLong(myLastDataWriteCycle);
  }
  catch(...)
  {
    cerr << "ERROR: AtariVox::save\n";
    return false;
  }
  return myBackend->save(out);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AtariVox::load(Serializer& in)
{
  if(!(Controller::load(in) && myEEPROM->load(in))) return false;
  try
  {
    myShiftCount = in.getByte();
    myShiftRegister = in.getShort();
    myLastDataWriteCycle = in.getLong();
  }
  catch(...)
  {
    cerr << "ERROR: AtariVox::load\n";
    return false;
  }
  return myBackend->load(in);
}
