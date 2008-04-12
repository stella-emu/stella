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
// $Id: SerialPortWin32.cxx,v 1.3 2008-04-12 23:20:05 stephena Exp $
//============================================================================

#include <windows.h>

#include "SerialPortWin32.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SerialPortWin32::SerialPortWin32()
  : SerialPort(),
    myHandle(NULL)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SerialPortWin32::~SerialPortWin32()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SerialPortWin32::openPort(const string& device)
{
  if(!myHandle)
  {
    myHandle = CreateFile(device.c_str(), GENERIC_READ|GENERIC_WRITE, 0,
                          NULL, OPEN_EXISTING, 0, NULL);

    if(myHandle)
    {
      DCB dcb;
      COMMTIMEOUTS cto;

      FillMemory(&dcb, sizeof(dcb), 0);
      dcb.DCBlength = sizeof(dcb);
      if(!BuildCommDCB("19200,n,8,1", &dcb))
        return false;

      memset(&dcb, 0, sizeof(DCB));
      dcb.BaudRate = CBR_19200;
      dcb.ByteSize = 8;
      dcb.Parity = NOPARITY;
      dcb.StopBits = ONESTOPBIT;
      SetCommState(myHandle, &dcb);
    }
    else 
      return false;
  }
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SerialPortWin32::closePort()
{
  if(myHandle)
  {
cerr << "port closed\n";
    CloseHandle(myHandle);
    myHandle = NULL;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SerialPortWin32::writeByte(const uInt8* data)
{
  if(myHandle)
  {
    cerr << "SerialPortWin32::write " << (int)(*data) << endl;

    DWORD written;
    return WriteFile(myHandle, data, 1, &written, 0) == TRUE;
  }
  return false;
}
