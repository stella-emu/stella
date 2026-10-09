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

#include "FBSurface.hxx"
#include "MemViewDataLayer.hxx"
#include "MemViewParams.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
MemViewDataLayer::MemViewDataLayer(Dialog& dialog, MemViewParams &params,
  uInt32 defaultColor, uInt32 fadeColor
)
  : MemViewLayer(dialog, params, true),
    myDefaultColor{defaultColor},
    myFadedColor{fadeColor},
    myEmptyFields(SZT(myParams.myDataSize) * 8, 0xFF000000)
{
  // Allocate some memory for the data
  myData.assign(myParams.myDataSize, 0);
  myFields = myEmptyFields;

  // Calculate a first table with the bytes
  calcBitDataTab();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
MemViewDataLayer::~MemViewDataLayer() = default;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MemViewDataLayer::draw()
{
  // Begin drawing on output layer data
  uInt32* destAddr = nullptr;
  uInt32 pitchWords = 0;
  if (!beginDraw(destAddr, pitchWords))
    return;

  drawImpl(myParams, myVisibility ? myFields : myEmptyFields,
    destAddr, pitchWords, SEPARATOR_COLOR, BACKGROUND_COLOR);

  // Close drawing for this layer
  endDraw();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MemViewDataLayer::drawDirect(MemViewParams& params, uInt32* destAddr)
{
  drawImpl(params, myFields, destAddr, params.myTotalWidth, SEPARATOR_COLOR, BACKGROUND_COLOR);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MemViewDataLayer::render()
{
  renderImpl();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MemViewDataLayer::setVisualParameters(bool inverted, bool byteFade)
{
  myInverted = inverted;
  myByteFade = byteFade;
  // Need to recalculate the bit data tab (because both parameters ar
  // used for that)
  calcBitDataTab();
  // Need to rebuild the data fields with the new optical bytes
  dataToFields();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MemViewDataLayer::updateData(const ByteArray& data)
{
  if (data.size() != myParams.myDataSize)
    return;

  // Copy and rearrange the data to our layout
  if (myParams.myDataSize <= myParams.myBankHeight)
  {
    // Special case for RAM - no rearrangement necessary because single column
    std::copy_n(data.data(), myParams.myDataSize, myData.data());
  }
  else
  {
    const uInt8* src = data.data();
    const uInt8* lineStop = src + myParams.myBankRowSize;
    const uInt8* bankRowStop = src + myParams.myBankRowSize + myParams.myBankHeight - 1;
    const uInt8* fullStop = src + myParams.myDataSize + myParams.myBankHeight - 1;
    uInt8* dest = myData.data(); // NOLINT(misc-const-correctness)

    // One total line at the time
    while (true)
    {
      do
      {
        *(dest++) = *src;
        src += myParams.myBankHeight;
      } while (src != lineStop);

      if (src == fullStop)
      {
        // Done
        break;
      }

      if (src == bankRowStop)
      {
        // Next bank row
        src -= myParams.myBankHeight - 1;
        lineStop += myParams.myBankRowSize - myParams.myBankHeight + 1;
        bankRowStop += myParams.myBankRowSize;
      }
      else
      {
        // Next line
        src -= myParams.myBankRowSize - 1;
        lineStop++;
      }
    }
  }

  dataToFields();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MemViewDataLayer::calcBitDataTab()
{
  // Precalc bit data table
  uInt32 *bitTabPos = myBitDataTab.data(); // NOLINT(misc-const-correctness)
  for (uInt32 value = 0x00; value <= 0xFF; value++)
  {
    const uInt32 v = myInverted ? value ^ 0xFFU : value;
    uInt32 mask = 0x80;
    uInt32 color = myByteFade ? myFadedColor : myDefaultColor;
    const uInt32 colorSub = myByteFade ? DATA_COLOR_FADE_VALUE : 0;
    do
    {
      *(bitTabPos++) = (v & mask) ? color : 0xFF000000;
      color -= colorSub;
      mask >>= 1U;
    } while (mask);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Creates the data fields to be shown by using the precalculated myBitDataTab
// with the bit data for each possible byte.
void MemViewDataLayer::dataToFields()
{
  const uInt8* src = myData.data();
  const uInt8* dataStop = src + myParams.myDataSize;
  uInt32* bitData = myFields.data();
  do
  {
    uInt32* bitsStart = &myBitDataTab[SZT(*(src++)) << 3U];
    std::copy(bitsStart, bitsStart + 8, bitData);
    bitData += 8;
  } while (src != dataStop);
}
