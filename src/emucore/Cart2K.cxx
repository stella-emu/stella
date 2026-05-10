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

#include "System.hxx"
#include "Cart2K.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Cartridge2K::Cartridge2K(ByteSpan image, string_view md5,
                         const Settings& settings, size_t bsSize)
  : CartridgeEnhanced(image, md5, settings, bsSize)
{
  // When creating a 2K cart, we always initially create a buffer of size 2_KB
  // Sometimes we only use a portion of that buffer; we check for that now

  // Size can be a maximum of 2K
  const size_t size = std::min(image.size(), bsSize);

  // Set image size to closest power-of-two for the given size
  size_t newSize = 1; myBankShift = 0;
  while(newSize < size)
  {
    newSize <<= 1;
    myBankShift++;
  }

  // Handle cases where ROM is smaller than the page size
  // It's much easier to do it this way rather than changing the page size
  if(newSize < System::PAGE_SIZE)
  {
    // Manually 'mirror' the ROM image into the buffer
    myImage.assign(System::PAGE_SIZE, 0);
    for(size_t i = 0; i < System::PAGE_SIZE; i += newSize)
      std::copy_n(image.data(), newSize, myImage.data() + i);
    myBankShift = System::PAGE_SHIFT;
  }
  else if(newSize < myImage.size())
    myImage.resize(newSize);
}
