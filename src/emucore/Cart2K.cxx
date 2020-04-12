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

#include "System.hxx"
#include "Cart2K.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Cartridge2K::Cartridge2K(const ByteBuffer& image, size_t size,
                         const string& md5, const Settings& settings)
  : CartridgeEnhanced(image, size, md5, settings)
{
  // Size can be a maximum of 2K
  if(size > 2_KB)
    size = 2_KB;

  // Set image size to closest power-of-two for the given size
  mySize = 1; myBankShift = 0;
  while(mySize < size)
  {
    mySize <<= 1;
    myBankShift++;
  }

  // Initialize ROM with illegal 6502 opcode that causes a real 6502 to jam
  size_t bufSize = std::max<size_t>(mySize, System::PAGE_SIZE);
  myImage = make_unique<uInt8[]>(bufSize);
  std::fill_n(myImage.get(), bufSize, 0x02);

  // Handle cases where ROM is smaller than the page size
  // It's much easier to do it this way rather than changing the page size
  if(mySize >= System::PAGE_SIZE)
  {
    // Directly copy the ROM image into the buffer
    std::copy_n(image.get(), mySize, myImage.get());
  }
  else
  {
    // Manually 'mirror' the ROM image into the buffer
    for(size_t i = 0; i < System::PAGE_SIZE; i += mySize)
      std::copy_n(image.get(), mySize, myImage.get() + i);
    mySize = System::PAGE_SIZE;
    myBankShift = 6;
  }
}
