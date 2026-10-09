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

#include "MediaFactory.hxx"
#include "SerialPort.hxx"
#include "Serializer.hxx"
#include "SpeakJetSerial.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SpeakJetSerial::SpeakJetSerial(string_view portname)
  : mySerialPort{MediaFactory::createSerialPort()}
{
  const string port{portname};

  myIsOpen = mySerialPort->openPort(port);

  if(myIsOpen)
  {
    myCTSFlip = !mySerialPort->isCTS();
    if(myCTSFlip)
      myAboutString = " (serial port \'" + port + "\', inverted CTS)";
    else
      myAboutString = " (serial port \'" + port + "\')";
  }
  else
    myAboutString = " (invalid serial port \'" + port + "\')";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SpeakJetSerial::~SpeakJetSerial() = default;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SpeakJetSerial::write(uInt8 code)
{
  mySerialPort->writeByte(code);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SpeakJetSerial::ready()
{
  // Some USB-serial adaptors support only CTS, others support only
  // software flow control
  // So we check the state of both then AND the results, on the
  // assumption that if a mode isn't supported, then it reads as TRUE
  // and doesn't change the boolean result
  // Thus the logic is:
  //   READY_SIGNAL = READY_STATE_CTS && READY_STATE_FLOW
  // Note that we also have to take inverted CTS into account

  // When using software flow control, only update on a state change
  uInt8 flowCtrl = 0;
  if(mySerialPort->readByte(flowCtrl))
    // 0x11 is XON
    myReadyStateSoftFlow = flowCtrl == 0x11;

  // Now combine the results of CTS and'ed with flow control
  return (mySerialPort->isCTS() ^ myCTSFlip) && myReadyStateSoftFlow;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SpeakJetSerial::save(Serializer& out) const
{
  try
  {
    out.putBool(myReadyStateSoftFlow);
    out.putBool(myCTSFlip);
  }
  catch(...)
  {
    cerr << "ERROR: SpeakJetSerial::save\n";
    return false;
  }
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SpeakJetSerial::load(Serializer& in)
{
  // Both describe the device attached now, which a saved state can only get
  // wrong: a stale XOFF would hold READY low, since an idle device never sends
  // the XON to clear it.  They are read past, keeping the layout.
  try
  {
    in.getBool();
    in.getBool();
  }
  catch(...)
  {
    cerr << "ERROR: SpeakJetSerial::load\n";
    return false;
  }
  return true;
}
