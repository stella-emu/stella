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
// $Id: SerialPortUNIX.cxx,v 1.4 2008-04-11 17:56:34 stephena Exp $
//============================================================================

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/termios.h>
#include <sys/types.h>
#include <sys/ioctl.h>

#include "SerialPortUNIX.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SerialPortUNIX::SerialPortUNIX()
  : SerialPort(),
    myHandle(0)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SerialPortUNIX::~SerialPortUNIX()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SerialPortUNIX::openPort(const string& device)
{
  myHandle = open(device.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
  if(myHandle <= 0)
    return false;

  struct termios termios;
  memset(&termios, 0, sizeof(struct termios));

  termios.c_cflag = CREAD | CLOCAL;
  termios.c_cflag |= B19200;
  termios.c_cflag |= CS8;
  tcflush(myHandle, TCIFLUSH);
  tcsetattr(myHandle, TCSANOW, &termios);

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SerialPortUNIX::closePort()
{
  if(myHandle)
    close(myHandle);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SerialPortUNIX::writeByte(const uInt8* data)
{
  if(myHandle)
  {
//    cerr << "SerialPortUNIX::writeByte " << (int)(*data) << endl;
    return write(myHandle, data, 1) == 1;
  }
  return false;
}
