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

#include "PlusROM.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool PlusROM::initialize(const ByteBuffer& image, size_t size)
{
  // Host and path are stored at the NMI vector
  size_t i = ((image[size - 5] - 16) << 8) | image[size - 6];  // NMI @ $FFFA
  if(i >= size)
    return myIsPlusROM = false;  // Invalid NMI

  // Convenience functions to detect valid path and host characters
  auto isValidPathChar = [](uInt8 c) {
    return ((c > 44 && c < 58) || (c > 64 && c < 91) || (c > 96 && c < 122));
  };
  auto isValidHostChar = [](uInt8 c) {
    return (c == 45 || c == 46 || (c > 47 && c < 58) ||
           (c > 64 && c < 91) || (c > 96 && c < 122));
  };

  // Path stored first, 0-terminated
  while(i < size && isValidPathChar(image[i]))
    myPath += static_cast<char>(image[i++]);

  // Did we get a 0-terminated path?
  if(i >= size || image[i] != 0)
    return myIsPlusROM = false;  // Wrong delimiter

  i++;  // advance past 0 terminator

  // Host stored next, 0-terminated
  while(i < size && isValidHostChar(image[i]))
    myHost += static_cast<char>(image[i++]);

  // Did we get a valid, 0-terminated host?
  if(i >= size || image[i] != 0 || myHost.size() < 3 || myHost.find(".") == string::npos)
    return myIsPlusROM = false;  // Wrong delimiter or dotless IP

  cerr << "Path: " << myPath << endl;
  cerr << "Host: " << myHost << endl;

  return myIsPlusROM = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool PlusROM::peekHotspot(uInt16 address, uInt8& value)
{
  switch(address & 0x0FFF)
  {
    case 0x0FF2:  // Read next byte from Rx buffer
      return false;

    case 0x0FF3:  // Get number of unread bytes in Rx buffer
      return false;
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool PlusROM::pokeHotspot(uInt16 address, uInt8 value)
{
  switch(address & 0x0FFF)
  {
    case 0x0FF0:  // Write byte to Tx buffer
      return false;

    case 0x0FF1:  // Write byte to Tx buffer and send to backend
                  // (and receive into Rx buffer)
      return false;
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool PlusROM::save(Serializer& out) const
{
  try
  {
    out.putByteArray(myRxBuffer.data(), myRxBuffer.size());
    out.putByteArray(myTxBuffer.data(), myTxBuffer.size());
  }
  catch(...)
  {
    cerr << "ERROR: PlusROM::save" << endl;
    return false;
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool PlusROM::load(Serializer& in)
{
  try
  {
    in.getByteArray(myRxBuffer.data(), myRxBuffer.size());
    in.getByteArray(myTxBuffer.data(), myTxBuffer.size());
  }
  catch(...)
  {
    cerr << "ERROR: PlusROM::load" << endl;
    return false;
  }

  return true;
}
