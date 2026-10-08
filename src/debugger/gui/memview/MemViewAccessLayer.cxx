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
#include "MemViewAccessLayer.hxx"
#include "MemViewParams.hxx"
#include <cmath>

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
MemViewAccessLayer::MemViewAccessLayer(Dialog& dialog, MemViewParams &params,
  const ColorTab& colorTab
) : MemViewLayer(dialog, params, false),
    myColorTab{colorTab}
{
  myLastAccessData.assign(myParams.myDataSize, 0);
  myStartAccessData.assign(myParams.myDataSize, 0);
  myHeatmap.assign(myParams.myDataSize, 0.0);
  myFields.assign(myParams.myDataSize, 0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
MemViewAccessLayer::~MemViewAccessLayer() = default;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MemViewAccessLayer::draw()
{
  // Draw if visible
  if (myVisibility)
  {
    // Begin drawing on output layer data
    uInt32* destAddr = nullptr;
    uInt32 pitchWords = 0;
    if (!beginDraw(destAddr, pitchWords))
      return;

    drawImpl(myParams, myFields, destAddr, pitchWords);

    // Close drawing for this layer
    endDraw();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MemViewAccessLayer::render()
{
  // Draw if visible
  if (myVisibility)
    renderImpl();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MemViewAccessLayer::updateAccessData(Device::AccessCounter* accessData,
  const MemViewAccessLayer::HeatmapValue& currentDecrement,
  const int elapsedFrames
)
{
  if (accessData == nullptr)
    return;

  // Compare new data with last one and update our heatmap accordingly
  if (myFirstRun || mySkipNextUpdate)
  {
    // First call - take over the data
    std::copy_n(
      accessData,
      myParams.myAccessDataSize,
      myLastAccessData.begin() + myParams.myAccessDataOffset
    );
    if (!mySkipNextUpdate)
      myStartAccessData.assign(myParams.myDataSize, 0);
    mySkipNextUpdate = false;
    myFirstRun = false;
  }
  else
    compareAccessData(accessData, currentDecrement, elapsedFrames);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MemViewAccessLayer::clearHeatmap()
{
  std::ranges::fill(myHeatmap, 0.0);
  myStartAccessData = myLastAccessData;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MemViewAccessLayer::loadTotals(bool skipNextUpdate)
{
  mySkipNextUpdate = skipNextUpdate;

  uInt64 count = 0;
  uInt64 sum = 0;
  for (const auto& value : myLastAccessData)
  {
    if (value != 0)
    {
      sum += value;
      count++;
    }
  }

  if (count == 0)
    return;

  const double average = DBL(sum) / DBL(count);
  const double gain = DBL((255.0 - BASE_ACCESS_VALUE + 1.0) / 2.0) / average;
  for (unsigned int i = 0; i < myParams.myDataSize; i++)
  {
    const Device::AccessCounter& accessValue = myLastAccessData[i];
    if (accessValue != 0) {
      const HeatmapValue value = BASE_ACCESS_VALUE + static_cast<HeatmapValue>(round(DBL(accessValue - 1) * gain));
      myHeatmap[i] = std::min<HeatmapValue>(value, 255.0);
    }
    else
      myHeatmap[i] = 0;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Device::AccessCounter MemViewAccessLayer::getTotalValue(const unsigned int offset) const
{
  return (offset >= myLastAccessData.size()) ? 0 : myLastAccessData[offset];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Device::AccessCounter MemViewAccessLayer::getDeltaValue(const unsigned int offset) const
{
  return (offset >= myLastAccessData.size()) ? 0 :
    (myLastAccessData[offset] - myStartAccessData[offset]);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MemViewAccessLayer::compareAccessData(const Device::AccessCounter* newData,
  const MemViewAccessLayer::HeatmapValue& currentDecrement,
  const int elapsedFrames)
{
  std::vector<Device::AccessCounter>& oldData = myLastAccessData;
  std::vector<HeatmapValue>& heatMap = myHeatmap;
  double gain = myHeatmapGain;

  // If more than one frame has passed since last update, scale down the gain
  // for this round
  if (elapsedFrames > 1)
    gain /= elapsedFrames;

  uInt32 sum = 0;
  uInt32 count = 0;

  for (
    unsigned int src = 0,
    dst = myParams.myAccessDataOffset;
    src < myParams.myAccessDataSize;
    src++, dst++
  )
  {
    if (newData[src] != oldData[dst])
    {
      // Calc diff and take over
      const uInt32 diff = newData[src] - oldData[dst];
      oldData[dst] = newData[src];

      // Do the stats for gain control
      count++;
      sum += diff;

      // Apply gain and calc heatmap value
      const HeatmapValue value = std::min<HeatmapValue>(
        BASE_ACCESS_VALUE + static_cast<HeatmapValue>(diff - 1) * gain, 255.0);

      if (value > heatMap[dst])
      {
        heatMap[dst] = value;
      }
      else
      {
        // At least decrement
        if ((heatMap[dst] - currentDecrement) < value)
          heatMap[dst] = value;
        else
          heatMap[dst] -= currentDecrement;
      }
    }
    else
    {
      // Decrement because nothing new happened at this address
      if (heatMap[dst] <= currentDecrement)
        heatMap[dst] = 0;
      else
        heatMap[dst] -= currentDecrement;
    }
  }

  // Calculate new gain
  if ((elapsedFrames == 1) && count)
  {
    const double average = DBL(sum) / DBL(count);
    const double thisGain = DBL((255.0 - BASE_ACCESS_VALUE + 1.0) / 2.0) / average;
    // 1/3 from the old, 2/3 from the new
    myHeatmapGain = myHeatmapGain / 3.0 + thisGain * 2.0 / 3.0;
    // Limit to some value
    myHeatmapGain = std::min(myHeatmapGain, 64.0);
  }
}
