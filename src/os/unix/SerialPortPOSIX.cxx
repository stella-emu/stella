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

#ifdef __OpenBSD__
  #include <errno.h>
#else
  #include <sys/errno.h>
#endif

#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/termios.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>

#include "FSNode.hxx"
#include "SerialPortPOSIX.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SerialPortPOSIX::~SerialPortPOSIX()
{
  if(isOpen())
  {
    tcflush(myHandle, TCOFLUSH);
    tcflush(myHandle, TCIFLUSH);
    tcsetattr(myHandle, TCSANOW, &myOldtio);

    close(myHandle);
    myHandle = INVALID_HANDLE_VALUE;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SerialPortPOSIX::openPort(const string& device)
{
  myHandle = open(device.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
  if(!isOpen())
    return false;

  // Clear buffers, then open the device in nonblocking mode
  tcflush(myHandle, TCOFLUSH);
  tcflush(myHandle, TCIFLUSH);

  tcgetattr(myHandle, &myOldtio); // Save current port settings

  myNewtio = myOldtio;
  myNewtio.c_cflag = CS8 | CLOCAL | CREAD;

#if defined(__FreeBSD__) || defined(__OpenBSD__)
  if(cfsetspeed(&myNewtio, static_cast<speed_t>(DEFAULT_BAUD_RATE)) != 0)
  {
    cerr << "ERROR: baudrate " << DEFAULT_BAUD_RATE << " not supported\n";
    close(myHandle);
    myHandle = INVALID_HANDLE_VALUE;
    return false;
  }
#else
  switch(DEFAULT_BAUD_RATE)
  {
#ifdef B1152000
    case 1152000: myNewtio.c_cflag |= B1152000; break;
#endif
#ifdef B576000
    case  576000: myNewtio.c_cflag |= B576000;  break;
#endif
#ifdef B230400
    case  230400: myNewtio.c_cflag |= B230400;  break;
#endif
    case  115200: myNewtio.c_cflag |= B115200;  break;
    case   57600: myNewtio.c_cflag |= B57600;   break;
    case   38400: myNewtio.c_cflag |= B38400;   break;
    case   19200: myNewtio.c_cflag |= B19200;   break;
    case    9600: myNewtio.c_cflag |= B9600;    break;
    default:
    {
      cerr << "ERROR: unknown baudrate " << DEFAULT_BAUD_RATE << '\n';
      close(myHandle);
      myHandle = INVALID_HANDLE_VALUE;
      return false;
    }
  }
#endif

  tcflush(myHandle, TCIFLUSH);
  if(tcsetattr(myHandle, TCSANOW, &myNewtio) != 0)
  {
    cerr << "Could not change serial port behaviour for "
         << device << " (tcsetattr failed)\n";
    close(myHandle);
    myHandle = INVALID_HANDLE_VALUE;
    return false;
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SerialPortPOSIX::readByte(uInt8& data)
{
  return isOpen() ? (read(myHandle, &data, 1) == 1) : false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SerialPortPOSIX::writeByte(uInt8 data)
{
  return isOpen() ? (write(myHandle, &data, 1) == 1) : false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SerialPortPOSIX::isCTS()
{
  if(isOpen())
  {
    int status{};
    // status stays 0 if ioctl fails; isCTS() will correctly return false
    ioctl(myHandle, TIOCMGET, &status);
    return status & TIOCM_CTS;
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
StringList SerialPortPOSIX::portNames()
{
  StringList ports;

  // Check if port is valid; for now that means if it can be opened
  // Eventually we may extend this to do more intensive checks
  const auto isPortValid = [](const string& port) {
    const int handle = open(port.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
    const bool valid = handle != INVALID_HANDLE_VALUE;
    if(valid)  close(handle);
    return valid;
  };

  // Get all possible devices in the '/dev' directory
  const FSNode::NameFilter filter = [](const FSNode& node) {
#ifdef BSPF_MACOS
    return BSPF::startsWithIgnoreCase(node.getPath(), "/dev/cu.usb") ||
           BSPF::startsWithIgnoreCase(node.getPath(), "/dev/tty.usb");
#else
    return BSPF::startsWithIgnoreCase(node.getPath(), "/dev/ttyUSB") ||
           BSPF::startsWithIgnoreCase(node.getPath(), "/dev/ttyACM") ||
           BSPF::startsWithIgnoreCase(node.getPath(), "/dev/ttyS");
#endif
  };
  FSList portList;
  portList.reserve(5);

  const FSNode dev("/dev/");
  dev.getChildren(portList, FSNode::ListMode::All, filter, false);

  // Add only those that can be opened
  for(const auto& port: portList)
    if(isPortValid(port.getPath()))
      ports.emplace_back(port.getPath());

  return ports;
}
