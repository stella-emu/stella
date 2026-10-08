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

#include "bspf.hxx"
#include "Cart.hxx"
#include "MemViewWidget.hxx"
#include "MemViewParams.hxx"
#include <cmath>

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
MemViewParams::MemViewParams(uInt16 bankSize, uInt16 bankCount, uInt32 baseAddress,
  Cartridge& cartridge, int posX, int posY
)
  : myBankSize{bankSize},
    myBankCount{bankCount},
    myDataSize{U32(bankSize * bankCount)},
    myBaseAddress{baseAddress},
    myCartridge{cartridge},
    mySurfacePosX{posX},
    mySurfacePosY{posY}
{

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool MemViewParams::setAccessDataParams(uInt32 size, uInt32 offset)
{
  if (offset > myDataSize)
  {
    cerr << "MemView access data size/offset error (" << offset << " + " << size <<
      " > " << myDataSize << ")\n";
    myAccessDataSize = myAccessDataOffset = 0;
    return false;
  }
  else if ((offset + size) > myDataSize)
  {
    cerr << "MemView access data size/offset warning (" << offset << " + " << size <<
      " > " << myDataSize << ")\n";
    myAccessDataSize = myDataSize - offset;
  }
  else
  {
    myAccessDataSize = size;
  }

  myAccessDataOffset = offset;

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MemViewParams::setLayoutParameters(LayoutParams& params,
  bool singleRow, bool separators)
{
  // Take over
  myBankWidth = params.bankWidth;
  myBankHeight = params.bankHeight;
  myHBanks = params.hBanks;
  myVBanks = params.vBanks;
  myMinZoom = myZoomLevel = params.minZoomLevel;
  mySingleRow = singleRow;
  mySeparators = separators;

  // Precalculate often used values
  myTotalBytesX = myBankWidth * myHBanks;
  myTotalBitsX = myTotalBytesX * 8;
  myTotalBitsY = myBankHeight * myVBanks;
  myTotalSepsWidth = (mySeparators && (myHBanks >= 2)) ? (myHBanks - 1) * SEPARATOR_WIDTH : 0;
  myTotalSepsHeight = (mySeparators && (myVBanks >= 2)) ? (myVBanks - 1) * SEPARATOR_HEIGHT : 0;
  myBankRowSize = myBankSize * myHBanks;

  layoutPreCalc();

  // Center if zoom is necessary
  myOffsetX = (myTotalWidth > mySurfaceWidth) ? (myTotalWidth - mySurfaceWidth) / 2 : 0;
  myOffsetY = (myTotalHeight > mySurfaceHeight) ? (myTotalHeight - mySurfaceHeight) / 2 : 0;

  layoutPostCalc();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MemViewParams::layoutPreCalc()
{
  // Calc current dimensions

  // The total dimentions zoomed in (which may be too big to display at once)
  myTotalWidth = myTotalBitsX * myZoomLevel + myTotalSepsWidth;
  myTotalHeight = myTotalBitsY * myZoomLevel + myTotalSepsHeight;

  // The currently visible dimensions
  myCurrentWidth = (myTotalWidth >= mySurfaceWidth) ? mySurfaceWidth : myTotalWidth;
  myCurrentHeight = (myTotalHeight >= mySurfaceHeight) ? mySurfaceHeight : myTotalHeight;
  myLeftBorderWidth = (mySurfaceWidth - myCurrentWidth) / 2;
  myTopBorderHeight = (mySurfaceHeight - myCurrentHeight) / 2;
  myRightBorderWidth = mySurfaceWidth - myLeftBorderWidth - myCurrentWidth;
  myBottomBorderHeight = mySurfaceHeight - myCurrentHeight - myTopBorderHeight;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MemViewParams::layoutPostCalc()
{
  // Offset is 0 again when completely zoomed out
  myOffsetX = (myTotalWidth > myCurrentWidth)
    ? BSPF::clamp(myOffsetX, 0, myTotalWidth - myCurrentWidth) : 0;
  myOffsetY = (myTotalHeight > myCurrentHeight)
    ? BSPF::clamp(myOffsetY, 0, myTotalHeight - myCurrentHeight) : 0;

  // Build up separator positions
  myVSepPos.clear();
  myHSepPos.clear();
  myAbsVSepPos.clear();
  myAbsHSepPos.clear();
  if (mySeparators)
  {
    // The | ones
    {
      const int interval = myBankWidth * 8 * myZoomLevel + SEPARATOR_WIDTH;
      int x = interval - SEPARATOR_WIDTH;
      while (x < myTotalWidth)
      {
        // Position even if out of current view
        myVSepPos.push_back(x);
        // Build new absolute positions for easy rendering
        if (x < myOffsetX)
        {
          if ((x + SEPARATOR_WIDTH) > myOffsetX)
          {
            // Right part of separator is visible
            myAbsVSepPos.emplace_back(
              myLeftBorderWidth, x + SEPARATOR_WIDTH - myOffsetX
            );
          }
        }
        else if (x < (myOffsetX + myCurrentWidth))
        {
          if ((x + SEPARATOR_WIDTH) > (myOffsetX + myCurrentWidth))
          {
            // Left part of separator is visible
            myAbsVSepPos.emplace_back(
              x - myOffsetX + myLeftBorderWidth,
              myCurrentWidth + myOffsetX - x
            );
          }
          else
          {
            // Fully visible
            myAbsVSepPos.emplace_back(
              x - myOffsetX + myLeftBorderWidth,
              SEPARATOR_WIDTH
            );
          }
        }
        // Advance to next
        x += interval;
      }
    }
    // The --- ones
    {
      const int interval = myBankHeight * myZoomLevel + SEPARATOR_HEIGHT;
      int y = interval - SEPARATOR_HEIGHT;
      while (y < myTotalHeight)
      {
        // Position even if out of current view
        myHSepPos.push_back(y);
        // Build new absolute positions for easy rendering
        if (y < myOffsetY)
        {
          if ((y + SEPARATOR_HEIGHT) > myOffsetY)
          {
            // Lower part of separator is visible
            myAbsHSepPos.emplace_back(
              myTopBorderHeight,
              y + SEPARATOR_HEIGHT - myOffsetY
            );
          }
        }
        else if (y < (myOffsetY + myCurrentHeight))
        {
          if ((y + SEPARATOR_HEIGHT) > (myOffsetY + myCurrentHeight))
          {
            // Upper part of separator is visible
            myAbsHSepPos.emplace_back(
              y - myOffsetY + myTopBorderHeight,
              myCurrentHeight + myOffsetY - y
            );
          }
          else
          {
            // Fully visible
            myAbsHSepPos.emplace_back(
              y - myOffsetY + myTopBorderHeight,
              SEPARATOR_HEIGHT
            );
          }
        }
        // Advance to next
        y += interval;
      }
    }
  }

  // Calculate parameters needed for drawing
  myYStart = myTopBorderHeight;
  myXFirstPixelReps = myZoomLevel;
  myYFirstPixelReps = myZoomLevel;
  mySkipXPixels = 0;  // Pixels to skip on left side because of separator
  int bitOffsetInByte = 0;
  myFirstBitCount = myBankWidth * 8;

  // Find out how to start drawing (point 0, 0)
  unsigned int byteOffset = 0;
  int xFracPixels = 0;
  int yFracPixels = 0;
  const PositionFlags posFlags = getPosition(0, 0, &byteOffset, &xFracPixels, &yFracPixels);

  if (posFlags & WITHIN_VERTICAL_SEPARATOR)
  {
    mySkipXPixels = xFracPixels;
  }
  else
  {
    myXFirstPixelReps -= xFracPixels % myZoomLevel;
    bitOffsetInByte = xFracPixels / myZoomLevel;
    myFirstBitCount = (myBankWidth - (byteOffset % myBankWidth)) * 8 - bitOffsetInByte;
  }

  if (posFlags & WITHIN_HORIZONTAL_SEPARATOR)
  {
    myYStart += yFracPixels;
  }
  else
  {
    myYFirstPixelReps -= yFracPixels;
  }

  myBitOffset = byteOffset * 8 + bitOffsetInByte;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MemViewParams::setDecayRate(int percantage)
{
  if (percantage == 0)
  {
    myAccessDataDecrement = 0.0;
    return;
  }
  // Scaling calculation
#if 0
  // Not all compilers do this:
  static constexpr double base = 0.481; // magic value to compress the log function towards its slope
  static constexpr double max = std::pow(base, std::log(101.0));
  static constexpr double scale = 255.0 / (1.0 - max);
#else
  static constexpr double base = 0.481; // magic value to compress the log function towards its slope
  static constexpr double max = 0.03412410114847106;
  static constexpr double scale = 264.0090722868298;
#endif
  myAccessDataDecrement = (
    std::pow(base, std::log(101.0 - DBL(percantage))) - max) * scale;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int MemViewParams::limitPosX(int x) const
{
  if (x < myLeftBorderWidth)
    x = 0;
  else if (x >= myLeftBorderWidth + myCurrentWidth)
    x = myCurrentWidth - 1;
  else
    x -= myLeftBorderWidth;

  return x;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int MemViewParams::limitPosY(int y) const
{
  if (y < myTopBorderHeight)
    y = 0;
  else if (y >= myTopBorderHeight + myCurrentHeight)
    y = myCurrentHeight - 1;
  else
    y -= myTopBorderHeight;

  return y;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
MemViewParams::PositionFlags MemViewParams::getPosition(
  int x, int y, unsigned int* byteOffset, int* xFracPixels, int* yFracPixels
) const
{
  PositionFlags result = WITHIN_DATA;
  *byteOffset = 0;
  int xSub = 0;
  int ySub = 0;
  x += myOffsetX;
  y += myOffsetY;

  // Find y in horizontal separator list (the ones going left to right ---)
  for (const auto pos : myHSepPos)
  {
    if (y < pos)
      break;
    // Add one bank row height for horizontal offset
    *byteOffset += myBankRowSize;
    if (y < (pos + MemViewParams::SEPARATOR_HEIGHT))
    {
      // Within horizontal separator
      result |= WITHIN_HORIZONTAL_SEPARATOR;
      *yFracPixels = pos + MemViewParams::SEPARATOR_HEIGHT - y;
      break;
    }
    ySub += myBankHeight * myZoomLevel + MemViewParams::SEPARATOR_HEIGHT;
  }

  // Find x in vertical separator list (the ones goint top to bottom |)
  for (const auto pos : myVSepPos)
  {
    if (x < pos)
      break;
    // Add one bank width for offset
    *byteOffset += myBankWidth;
    if (x < (pos + MemViewParams::SEPARATOR_WIDTH))
    {
      // Within vertical separator
      result |= WITHIN_VERTICAL_SEPARATOR;
      *xFracPixels = pos + MemViewParams::SEPARATOR_WIDTH - x;
      break;
    }
    xSub += myBankWidth * myZoomLevel * 8 + MemViewParams::SEPARATOR_WIDTH;
  }

  // Only calculate the pixel positions within data when the appropriate flags are not set
  if (!(result & WITHIN_VERTICAL_SEPARATOR))
  {
    x -= xSub;
    *xFracPixels = x % (myZoomLevel * 8);
    *byteOffset += (x / 8) / myZoomLevel;
  }

  if (!(result & WITHIN_HORIZONTAL_SEPARATOR))
  {
    y -= ySub;
    *yFracPixels = y % myZoomLevel;
    *byteOffset += (y / myZoomLevel) * myTotalBytesX;
  }

  return result;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
unsigned int MemViewParams::getLinearOffset(unsigned int byteOffset) const
{
  return
    // Bank row offset
    (byteOffset / myBankRowSize) * myBankRowSize
    +
    // Bank column offset
    (byteOffset % myTotalBytesX) * myBankHeight
    +
    // Single byte offset
    ((byteOffset % myBankRowSize) / myTotalBytesX);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
unsigned int MemViewParams::getRearrangedOffset(unsigned int offset) const
{
  const int rowOffset = offset % myBankRowSize;
  return
    // Bank row offset
    (offset / myBankRowSize) * myBankRowSize
    +
    // Column offset
    rowOffset / myBankHeight
    +
    // Single byte offset
    (rowOffset % myBankHeight) * myTotalBytesX;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Common::RwAddress MemViewParams::getAddress(int x, int y, int* bank, unsigned int* offset) const
{
  // Limit to positions within the shown data (if mouse is over border)
  x = limitPosX(x);
  y = limitPosY(y);
  unsigned int byteOffset = 0;
  int xFracPixels = 0;
  int yFracPixels = 0;
  getPosition(x, y, &byteOffset, &xFracPixels, &yFracPixels);
  const int linOffset = getLinearOffset(byteOffset);
  if (offset != nullptr)
    *offset = linOffset;

  const int b = linOffset / myBankSize;
  if (bank != nullptr)
    *bank = b;

  const uInt32 bankOffs = linOffset % myBankSize;

  Common::RwAddress address = getBankOrigin(b);
  address.read += bankOffs;
  address.write += bankOffs;

  return address;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Common::RwAddress MemViewParams::getBankOrigin(int bank, uInt16 PC) const
{
  if (myBaseAddress & MemViewWidget::QUERY_ROM_BANK_ORIGIN)
  {
    return Common::RwAddress(true, (myBaseAddress & 0xFFF) + myCartridge.bankOrigin(bank, PC));
  }
  else if (myBaseAddress & MemViewWidget::QUERY_RAM_BANK_ORIGIN)
  {
    Common::RwAddress ramBankOrigin = myCartridge.ramBankOrigin(bank, PC);
    return Common::RwAddress(
      ramBankOrigin.valid,
      (myBaseAddress & 0xFFF) + ramBankOrigin.read,
      (myBaseAddress & 0xFFF) + ramBankOrigin.write
    );
  }
  else
  {
    return Common::RwAddress(true, myBaseAddress);
  }
}
