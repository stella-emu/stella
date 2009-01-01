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
// Copyright (c) 1995-2009 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: SerialPortUNIX.hxx,v 1.5 2009-01-01 18:13:39 stephena Exp $
//============================================================================

#ifndef SERIALPORT_UNIX_HXX
#define SERIALPORT_UNIX_HXX

#include "SerialPort.hxx"

/**
  Implement reading and writing from a serial port under UNIX.  For now,
  it seems to be Linux-only, and reading isn't actually supported at all.

  @author  Stephen Anthony
  @version $Id: SerialPortUNIX.hxx,v 1.5 2009-01-01 18:13:39 stephena Exp $
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
    bool openPort(const string& device);

    /**
      Close a previously opened serial port.
    */
    void closePort();

    /**
      Write a byte to the serial port.

      @param data  The byte to write to the port
      @return  True if a byte was written, else false
    */
    bool writeByte(const uInt8* data);

  private:
    // File descriptor for serial connection
    int myHandle;
};

#endif
