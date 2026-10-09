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

#include "OSystem.hxx"
#include "FBSurface.hxx"
#include "Dialog.hxx"
#include "MemViewLayer.hxx"
#include "MemViewParams.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
MemViewLayer::MemViewLayer(Dialog& dialog, MemViewParams &params, bool isDataLayer)
  : myDialog(dialog),
    myParams{params},
    myIsDataLayer{isDataLayer}
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
MemViewLayer::~MemViewLayer()
{
  deallocateSurface();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MemViewLayer::allocateSurface()
{
  if (!mySurface)
  {
    mySurface = FrameBuffer::allocateSurface(
      myDialog.window(),
      myParams.mySurfaceWidth,
      myParams.mySurfaceHeight,
      ScalingInterpolation::none
    );
    // myParams is in logical pixels; scale up to the window's HiDPI factor
    const uInt32 dpi = myDialog.instance().frameBuffer().hidpiScaleFactor(myDialog.window());
    mySurface->setDstSize(myParams.mySurfaceWidth * dpi, myParams.mySurfaceHeight * dpi);
    mySurface->enableBlend(!myIsDataLayer);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MemViewLayer::deallocateSurface()
{
  if (mySurface)
    FrameBuffer::deallocateSurface(myDialog.window(), mySurface);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool MemViewLayer::beginDraw(uInt32*& destAddr, uInt32& pitchWords)
{
  allocateSurface();

  const auto [pixels, pitch] = mySurface->basePtr();
  destAddr = pixels;
  pitchWords = pitch;

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MemViewLayer::endDraw()
{
  // FBSurface::render() reads mySurface's pixel buffer directly each call,
  // so there's nothing to do here
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32* MemViewLayer::drawSeparatorLine(const MemViewParams& params, uInt32*& lineStart,
  const uInt32& backgroundColor, const uInt32& separatorColor
)
{
  // Draw left border
  std::fill_n(lineStart, params.myLeftBorderWidth, backgroundColor);
  // Draw separator
  std::fill_n(lineStart + params.myLeftBorderWidth, params.myCurrentWidth, separatorColor);
  // Draw right border
  std::fill_n(lineStart + params.myLeftBorderWidth + params.myCurrentWidth,
    params.myRightBorderWidth, backgroundColor);
  return lineStart;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool MemViewLayer::appendLine(const uInt32* src, uInt32*& dest, const uInt32*& destStop,
  const int& firstPixelReps, const int& defaultPixelReps, int unitCount
)
{
  int pixelReps = firstPixelReps;

  while (dest < destStop)
  {
    const int fillCount = std::min(pixelReps, I32(destStop - dest));
    std::fill_n(dest, fillCount, *src);
    dest += fillCount;
    src++;
    if (!--unitCount) {
      // More available
      return false;
    }
    pixelReps = defaultPixelReps;
  }

  // Line finished
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MemViewLayer::drawImpl(
  const MemViewParams& params,
  const uIntArray& srcData,
  uInt32 *destAddr,
  const int pitchWords,
  uInt32 separatorColor, uInt32 backgroundColor
) const
{
  // Setup some running vars
  const uInt32* srcAddr = srcData.data();
  int y = params.myYStart;
  int yPixelReps = params.myYFirstPixelReps;
  const int& skipXPixels = params.mySkipXPixels;
  const int& firstBitCount = params.myFirstBitCount;
  const int bitOffset = params.myBitOffset;
  const bool isDataLayer = myIsDataLayer;

  // Only the data layer does single bits - the other layers whole bytes at once
  const int unitsPerField = isDataLayer ? 8 : 1;
  const int unitDivisor = isDataLayer ? 1 : 8;
  const int pixelPerUnit = params.myZoomLevel * unitDivisor;
  const int xFirstPixelRepsMod = params.myXFirstPixelReps
    + (isDataLayer ? 0 : ((7 - I32(U32(bitOffset) & 0x7U)) * params.myZoomLevel));
  const int firstUnitCount = (firstBitCount / unitDivisor)
    + ((firstBitCount % unitDivisor) ? 1 : 0);
  int unitOffset = bitOffset / unitDivisor;
  const int unitsPerLine = params.myTotalBitsX / unitDivisor;

  // Start with no already drawn separator line
  uInt32* drawnSeparatorLine = nullptr;

  // Fill top with background
  if (params.myTopBorderHeight)
    std::fill_n(destAddr, params.myTopBorderHeight * pitchWords, backgroundColor);

  // Get the first vertical separator X position (if any)
  auto nextSeparatorPosIt = params.myAbsHSepPos.begin();

  // Check if we start in a separator on top
  int topSepLines = y - params.myTopBorderHeight;
  if (topSepLines)
  {
    uInt32* sepLineStart = destAddr + (static_cast<ptrdiff_t>(y - topSepLines) * pitchWords);
    do
    {
      if (drawnSeparatorLine)
        // Separator line has already been drawn - just copy
        std::copy_n(drawnSeparatorLine, params.mySurfaceWidth, sepLineStart);
      else
        // Build new separator line
        drawnSeparatorLine = drawSeparatorLine(params, sepLineStart, backgroundColor, separatorColor);
      sepLineStart += pitchWords;
    } while (--topSepLines);
    // Go to next separator
    nextSeparatorPosIt++;
  }

  // Build and draw data area

  uInt32* lineStart = destAddr + (static_cast<ptrdiff_t>(y) * pitchWords);
  const int yEnd = params.myTopBorderHeight + params.myCurrentHeight;
  do
  {
    // Build line

    uInt32* dest = lineStart + skipXPixels + params.myLeftBorderWidth;
    const uInt32* destStop = lineStart + params.myCurrentWidth + params.myLeftBorderWidth;

#if 0
    // Clear
    std::fill_n(lineStart, params.mySurfaceWidth, 0);
#endif

    // Draw left border
    std::fill_n(lineStart, params.myLeftBorderWidth, backgroundColor);

    int xPixelReps = xFirstPixelRepsMod;
    int unitCount = firstUnitCount;
    int runningDataOffset = unitOffset;
    // Append while line not finished
    while (!appendLine(&srcAddr[runningDataOffset], dest, destStop, xPixelReps, pixelPerUnit, unitCount))
    {
      // Reached a separator (end of bank width)
      runningDataOffset += unitCount;
      xPixelReps = pixelPerUnit;
      unitCount = params.myBankWidth * unitsPerField;
      dest += (params.mySeparators ? MemViewParams::SEPARATOR_WIDTH : 0);
    }

    // Draw the | separators at the precalculated spots
    for (const auto& sepPos : params.myAbsVSepPos)
      std::fill_n(lineStart + sepPos.first, sepPos.second, separatorColor);

    // Draw right border
    std::fill_n(lineStart + params.myLeftBorderWidth + params.myCurrentWidth,
      params.myRightBorderWidth, backgroundColor);

    // Replicate drawn line for zoomed bits
    const uInt32* originalLine = lineStart;
    y++;
    lineStart += pitchWords;
    while (--yPixelReps && (y < yEnd))
    {
      std::copy_n(originalLine, params.mySurfaceWidth, lineStart);
      y++;
      lineStart += pitchWords;
    }

    // Advance offset a whole line
    unitOffset += unitsPerLine;
    yPixelReps = params.myZoomLevel;

    // Time for separator?
    if (
      (nextSeparatorPosIt != params.myAbsHSepPos.end())
      &&
      ((*nextSeparatorPosIt).first == y)
    )
    {
      const int sepEnd = y + (*nextSeparatorPosIt).second;
      do
      {
        if (y >= sepEnd)
          break;

        if (drawnSeparatorLine)
          // Separator line has already been drawn - just copy
          std::copy_n(drawnSeparatorLine, params.mySurfaceWidth, lineStart);
        else
          // Build new separator line
          drawnSeparatorLine = drawSeparatorLine(params, lineStart, backgroundColor, separatorColor);

        lineStart += pitchWords;

      } while (++y < sepEnd);

      nextSeparatorPosIt++;
    }

  } while (y < yEnd);

  // Fill bottom with background color
  if (params.myBottomBorderHeight)
  {
    std::fill_n(
      lineStart,
      params.myBottomBorderHeight * pitchWords,
      backgroundColor
    );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MemViewLayer::renderImpl()
{
  // Nothing drawn yet
  if(!mySurface)
    return;

  // The view moves on a resize, so its origin is read at render time
  mySurface->setDstPos(myParams.mySurfacePosX, myParams.mySurfacePosY);
  mySurface->render();
}
