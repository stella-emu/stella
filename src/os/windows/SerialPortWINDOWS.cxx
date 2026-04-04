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

#include "Windows.hxx"
#include "SerialPortWINDOWS.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SerialPortWINDOWS::~SerialPortWINDOWS()
{
  if(isOpen())
  {
    CloseHandle(myHandle);
    myHandle = INVALID_HANDLE_VALUE;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SerialPortWINDOWS::openPort(const string& device)
{
  if(!isOpen())
  {
    myHandle = CreateFile(device.c_str(), GENERIC_READ|GENERIC_WRITE, 0,
                          NULL, OPEN_EXISTING, 0, NULL);
    if(!isOpen())
      return false;

    const auto closeAndFail = [this]() -> bool {
      CloseHandle(myHandle);
      myHandle = INVALID_HANDLE_VALUE;
      return false;
    };

    DCB dcb{};
    dcb.DCBlength = sizeof(dcb);
    if(!GetCommState(myHandle, &dcb))
      return closeAndFail();

    dcb.BaudRate = CBR_19200;
    dcb.ByteSize = 8;
    dcb.Parity   = NOPARITY;
    dcb.StopBits = ONESTOPBIT;
    if(!SetCommState(myHandle, &dcb))
      return closeAndFail();

    COMMTIMEOUTS commtimeouts{};
    commtimeouts.ReadIntervalTimeout      = MAXDWORD;
    commtimeouts.ReadTotalTimeoutConstant = 1;
    if(!SetCommTimeouts(myHandle, &commtimeouts))
      return closeAndFail();
  }
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SerialPortWINDOWS::readByte(uInt8& data)
{
  if(isOpen())
  {
    DWORD read;
    std::ignore = ReadFile(myHandle, &data, 1, &read, NULL);
    return read == 1;
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SerialPortWINDOWS::writeByte(uInt8 data)
{
  if(isOpen())
  {
    DWORD written;
    std::ignore = WriteFile(myHandle, &data, 1, &written, NULL);
    return written == 1;
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SerialPortWINDOWS::isCTS()
{
  if(isOpen())
  {
    DWORD modemStat;
    std::ignore = GetCommModemStatus(myHandle, &modemStat);
    return (modemStat & MS_CTS_ON) != 0;
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
StringList SerialPortWINDOWS::portNames()
{
  StringList ports;

  HKEY hKey{nullptr};
  if(RegOpenKeyExW(HKEY_LOCAL_MACHINE,
      L"HARDWARE\\DEVICEMAP\\SERIALCOMM", 0, KEY_READ, &hKey) != ERROR_SUCCESS)
    return ports;

  ScopeExit guard{[&hKey]() noexcept {
    RegCloseKey(hKey);
  }};

  DWORD numValues{0};
  DWORD maxValueNameLen{0};
  DWORD maxDataLen{0};
  if(RegQueryInfoKeyW(hKey, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
      &numValues, &maxValueNameLen, &maxDataLen, nullptr, nullptr) != ERROR_SUCCESS)
    return ports;

  if(numValues == 0)
    return ports;

  const DWORD nameBufferLen = maxValueNameLen + 1;
  const DWORD dataBufferLen = maxDataLen / sizeof(wchar_t) + 1;

  for(DWORD i = 0; i < numValues; ++i)
  {
    std::wstring valueName(nameBufferLen, L'\0');
    DWORD valueNameLen = nameBufferLen;
    DWORD type{0};
    DWORD dataLen = maxDataLen;

    std::wstring data(dataBufferLen, L'\0');
    if(RegEnumValueW(hKey, i, valueName.data(), &valueNameLen,
        nullptr, &type, reinterpret_cast<LPBYTE>(data.data()), &dataLen) != ERROR_SUCCESS)
      continue;

    if(type != REG_SZ || dataLen == 0)
      continue;

    // Strip null terminators the registry included in dataLen
    while(!data.empty() && data.back() == L'\0')
      data.pop_back();

    const int len = WideCharToMultiByte(CP_UTF8, 0, data.c_str(), -1,
                                        nullptr, 0, nullptr, nullptr);
    if(len > 0)
    {
      // Allocate len chars (includes the null terminator WideCharToMultiByte writes),
      // then shrink by 1 so the std::string length excludes it.
      std::string narrow(len, '\0');
      WideCharToMultiByte(CP_UTF8, 0, data.c_str(), -1,
                          narrow.data(), len, nullptr, nullptr);
      narrow.resize(len - 1);
      ports.emplace_back(std::move(narrow));
    }
  }

  std::ranges::sort(ports);
  return ports;
}
