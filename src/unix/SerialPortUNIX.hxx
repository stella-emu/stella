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
// Copyright (c) 1995-2015 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

#ifndef SERIALPORT_UNIX_HXX
#define SERIALPORT_UNIX_HXX

#include "SerialPort.hxx"

/**
  Implement reading and writing from a serial port under UNIX.  For now,
  it seems to be Linux-only, and reading isn't actually supported at all.

  @author  Stephen Anthony
  @version $Id$
*/
class SerialPortUNIX : public SerialPort
{
  public:
    SerialPortUNIX();
    virtual ~SerialPortUNIX();

    /**
      Open the given serial port with the specified attributes.

      @param device  The name of the port
      @return  False on any errors, else true
    */
    bool openPort(const string& device) override;

    /**
      Close a previously opened serial port.
    */
    void closePort() override;

    /**
      Write a byte to the serial port.

      @param data  The byte to write to the port
      @return  True if a byte was written, else false
    */
    bool writeByte(const uInt8* data) override;

  private:
    // File descriptor for serial connection
    int myHandle;

  private:
    // Following constructors and assignment operators not supported
    SerialPortUNIX(const SerialPortUNIX&) = delete;
    SerialPortUNIX(SerialPortUNIX&&) = delete;
    SerialPortUNIX& operator=(const SerialPortUNIX&) = delete;
    SerialPortUNIX& operator=(SerialPortUNIX&&) = delete;
};

#endif
