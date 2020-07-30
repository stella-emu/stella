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
// Copyright (c) 1995-2020 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include <windows.h>

#include "SerialPortWINDOWS.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SerialPortWINDOWS::SerialPortWINDOWS()
  : SerialPort()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SerialPortWINDOWS::~SerialPortWINDOWS()
{
  if(myHandle)
  {
    CloseHandle(myHandle);
    myHandle = 0;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SerialPortWINDOWS::openPort(const string& device)
{
  if(!myHandle)
  {
    myHandle = CreateFile(device.c_str(), GENERIC_READ|GENERIC_WRITE, 0,
                          NULL, OPEN_EXISTING, 0, NULL);

    if(myHandle)
    {
      DCB dcb;

      FillMemory(&dcb, sizeof(dcb), 0);
      dcb.DCBlength = sizeof(dcb);
      if(!BuildCommDCB("19200,n,8,1", &dcb))
      {
        CloseHandle(myHandle);
        myHandle = 0;
        return false;
      }

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
bool SerialPortWINDOWS::writeByte(uInt8 data)
{
  if(myHandle)
  {
    DWORD written;
    return WriteFile(myHandle, &data, 1, &written, 0) == TRUE;
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SerialPortWINDOWS::isCTS()
{
  if(myHandle)
  {
    DWORD modemStat;
    GetCommModemStatus(myHandle, &modemStat);
    return modemStat & MS_CTS_ON;
  }
  return false;
}
