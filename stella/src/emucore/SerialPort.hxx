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
// $Id: SerialPort.hxx,v 1.1 2008-03-31 00:59:30 stephena Exp $
//============================================================================

#ifndef SERIALPORT_HXX
#define SERIALPORT_HXX

#include "bspf.hxx"

/**
  This class provides an interface for a standard serial port.
  For now, this used when connecting a real AtariVox device,
  but it may be used for other devices in the future.

  @author  Stephen Anthony
  @version $Id: SerialPort.hxx,v 1.1 2008-03-31 00:59:30 stephena Exp $
*/
class SerialPort
{
  public:
    SerialPort() { }
    virtual ~SerialPort() { }

    /**
      Open the given serial port with the specified attributes.

      @param device  The name of the port
      @param baud    Baud rate
      @param data    Number of data bits
      @param stop    Number of stop bits
      @param parity  Type of parity bit (0=none, 1=odd, 2=even)

      @return  False on any errors, else true
    */
    virtual bool open(const string& device, int baud, int data,
                      int stop, int parity) { return false; }

    /**
      Close a previously opened serial port.
    */
    virtual void close() { }

    /**
      Read a byte from the serial port.

      @param data  Destination for the byte read from the port
      @return  True if a byte was read, else false
    */
    virtual bool read(uInt8& data) { return false; }

    /**
      Write a byte to the serial port.

      @param data  The byte to write to the port
      @return  True if a byte was written, else false
    */
    virtual bool write(const uInt8 data) { return false; }
};

#endif
