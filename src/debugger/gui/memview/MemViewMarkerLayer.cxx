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
#include "FrameBuffer.hxx"
#include "Dialog.hxx"
#include "FBSurface.hxx"
#include "Rect.hxx"
#include "MemViewParams.hxx"
#include "MemViewMarkerLayer.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
MemViewMarkerLayer::MemViewMarkerLayer(Dialog& dialog, MemViewParams &params,
  uInt32 color
)
  : myDialog{dialog}, myParams{params}, myColor{color}
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
MemViewMarkerLayer::~MemViewMarkerLayer()
{
  deallocateSurface();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MemViewMarkerLayer::allocateSurface()
{
  if (!mySurface)
  {
    mySurface = FrameBuffer::allocateSurface(
      myDialog.window(),
      MemViewParams::myMaxZoom * 8 + 2 * MARKER_THICKNESS,
      MemViewParams::myMaxZoom + 2 * MARKER_THICKNESS,
      ScalingInterpolation::none,
      myDummyBuffer.data()  // just to go into static mode
    );
    mySurface->enableBlend(true);
    // Prepare (clear and draw top and left lines)
    mySurface->fillRectRgb(0, 0, BUFFER_WIDTH, BUFFER_HEIGHT, 0x00000000);
    mySurface->fillRectRgb(0, 0, BUFFER_WIDTH, MARKER_THICKNESS, myColor);
    mySurface->fillRectRgb(0, 0, MARKER_THICKNESS, BUFFER_HEIGHT, myColor);
    // Take over new data (and create a big texture now from the start)
    mySurface->updateStaticData();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MemViewMarkerLayer::deallocateSurface()
{
  if (mySurface)
    FrameBuffer::deallocateSurface(myDialog.window(), mySurface);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool MemViewMarkerLayer::set(bool enable, int byteOffset)
{
  bool changed = (enable != myEnabled);
  myEnabled = enable;
  myLastOffset = byteOffset;
  if (!myEnabled)
    return changed;
  
  allocateSurface();

  if (myParams.myZoomLevel != myDrawnZoomLevel)
  {
    // Need to redraw; clear and draw right and bottom lines
    mySurface->fillRectRgb(MARKER_THICKNESS, MARKER_THICKNESS, myParams.myZoomLevel * 8,
      myParams.myZoomLevel, 0x00000000);

    mySurface->fillRectRgb(MARKER_THICKNESS + myParams.myZoomLevel * 8, MARKER_THICKNESS,
      MARKER_THICKNESS, myParams.myZoomLevel, myColor);

    mySurface->fillRectRgb(MARKER_THICKNESS, MARKER_THICKNESS + myParams.myZoomLevel,
      myParams.myZoomLevel * 8 + MARKER_THICKNESS, MARKER_THICKNESS, myColor);

    myDrawnZoomLevel = myParams.myZoomLevel;

    mySurface->updateStaticData();

    changed = true;
  }

  const Common::Point lastSourcePos = mySourcePos;
  const Common::Point lastDestPos = myDestPos;
  const Common::Size lastSize = mySize;

  // Calculate destination position and size
  mySourcePos.x = 0;
  mySourcePos.y = 0;
  mySize.w = myDrawnZoomLevel * 8 + 2 * MARKER_THICKNESS;
  mySize.h = myDrawnZoomLevel + 2 * MARKER_THICKNESS;

  // X
  const int byteInLine = byteOffset % myParams.myTotalBytesX;
  const int hBank = byteInLine / myParams.myBankWidth;

  myDestPos.x = byteInLine * myDrawnZoomLevel * 8 +
    (myParams.mySeparators ? (hBank * MemViewParams::SEPARATOR_WIDTH) : 0) -
    myParams.myOffsetX + myParams.myLeftBorderWidth - MARKER_THICKNESS;

  const unsigned int cropLeft = (myDestPos.x < -MARKER_THICKNESS) ? -myDestPos.x : 0;
  if (cropLeft >= mySize.w)
  {
    changed = changed || myVisibility;
    myVisibility = false;
    return changed;
  }
  mySourcePos.x += cropLeft;
  myDestPos.x += cropLeft;
  mySize.w -= cropLeft;

  const int posRight = myDestPos.x + mySize.w;
  const unsigned int cropRight = (posRight > (myParams.mySurfaceWidth + MARKER_THICKNESS)) ?
    (posRight - myParams.mySurfaceWidth) : 0;
  if (cropRight >= mySize.w)
  {
    changed = changed || myVisibility;
    myVisibility = false;
    return changed;
  }
  mySize.w -= cropRight;

  // Y
  const int byteInColumn = byteOffset / myParams.myTotalBytesX;
  const int vBank = byteInColumn / myParams.myBankHeight;

  myDestPos.y = byteInColumn * myParams.myZoomLevel +
    (myParams.mySeparators ? (vBank * MemViewParams::SEPARATOR_HEIGHT) : 0) -
    myParams.myOffsetY + myParams.myTopBorderHeight - MARKER_THICKNESS;

  const unsigned int cropTop = (myDestPos.y < -MARKER_THICKNESS) ? -myDestPos.y : 0;
  if (cropTop >= mySize.h)
  {
    changed = changed || myVisibility;
    myVisibility = false;
    return changed;
  }
  mySourcePos.y += cropTop;
  myDestPos.y += cropTop;
  mySize.h -= cropTop;

  const int posBottom = myDestPos.y + mySize.h;
  const unsigned int cropBottom = (posBottom > (myParams.mySurfaceHeight + MARKER_THICKNESS)) ?
    (posBottom - myParams.mySurfaceHeight) : 0;

  if (cropBottom >= mySize.h)
  {
    changed = changed || myVisibility;
    myVisibility = false;
    return changed;
  }
  mySize.h -= cropBottom;

  myVisibility = true;

  return
    changed
    ||
    (mySourcePos != lastSourcePos)
    ||
    (myDestPos != lastDestPos)
    ||
    (mySize != lastSize);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MemViewMarkerLayer::render()
{
  if (mySurface && myEnabled && myVisibility)
  {
    // The view moves on a resize, so its origin is read at render time
    const Common::Point originPos(myParams.mySurfacePosX, myParams.mySurfacePosY);
    const Common::Rect sourceRect(mySourcePos, mySize.w, mySize.h);
    // myParams is in logical pixels; scale up to the window's HiDPI factor
    const int dpi = myDialog.instance().frameBuffer().hidpiScaleFactor(myDialog.window());
    const Common::Point scaledDestPos(myDestPos.x * dpi, myDestPos.y * dpi);
    const Common::Rect destRect(scaledDestPos + originPos, mySize.w * dpi, mySize.h * dpi);
    mySurface->setSrcRect(sourceRect);
    mySurface->setDstRect(destRect);
    mySurface->render();
  }
}
