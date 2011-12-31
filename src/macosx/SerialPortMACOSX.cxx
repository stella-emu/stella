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

#include <stdio.h>
#include <fcntl.h>
#include <sys/errno.h>
#include <sys/termios.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/filio.h>
#include <sys/ioctl.h>

#include "SerialPortMACOSX.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SerialPortMACOSX::SerialPortMACOSX()
  : SerialPort(),
    myHandle(0)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SerialPortMACOSX::~SerialPortMACOSX()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SerialPortMACOSX::openPort(const string& device)
{
  myHandle = open(device.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
  if(myHandle <= 0)
    return false;

  struct termios termios;
  tcgetattr(myHandle, &termios);
  memset(&termios, 0, sizeof(struct termios));
  cfmakeraw(&termios);
  cfsetspeed(&termios, 19200);       // change to 19200 baud
  termios.c_cflag = CREAD | CLOCAL;  // turn on READ and ignore modem control lines
  termios.c_cflag |= CS8;            // 8 bit
  termios.c_cflag |= CDTR_IFLOW;     // inbound DTR
  tcsetattr(myHandle, TCSANOW, &termios);

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SerialPortMACOSX::closePort()
{
  if(myHandle)
    close(myHandle);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SerialPortMACOSX::writeByte(const uInt8* data)
{
  if(myHandle)
  {
//    cerr << "SerialPortMACOSX::writeByte " << (int)(*data) << endl;
    return write(myHandle, data, 1) == 1;
  }
  return false;
}
