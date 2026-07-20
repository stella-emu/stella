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

#include <array>
#include <cmath>

#include "OSystem.hxx"
#include "Console.hxx"
#include "TIA.hxx"
#include "TIAConstants.hxx"
#include "TIASurface.hxx"
#include "FrameBuffer.hxx"
#include "FBSurface.hxx"
#include "Widget.hxx"
#include "GuiObject.hxx"
#include "Dialog.hxx"
#include "ContextMenu.hxx"
#include "TiaDisplayWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TiaDisplayWidget::TiaDisplayWidget(GuiObject* boss, const GUI::Font& font)
  : Widget(boss, font, 0, 0, 0, 0),
    CommandSender(boss)
{
  _flags = Widget::FLAG_ENABLED | Widget::FLAG_CLEARBG |
           Widget::FLAG_RETAIN_FOCUS | Widget::FLAG_TRACK_MOUSE;
  _bgcolor = _bgcolorhi = kDlgColor;

  addFocusWidget(this);

  // Context menu with zoom presets
  VariantList l;
  VarList::push_back(l, "Fit to window", "fit");
  VarList::push_back(l, "2x zoom", "2");
  VarList::push_back(l, "4x zoom", "4");
  VarList::push_back(l, "8x zoom", "8");
  myMenu = std::make_unique<ContextMenu>(this, font, l);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TiaDisplayWidget::loadConfig()
{
  setDirty();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TiaDisplayWidget::frameSize(uInt32& w, uInt32& h) const
{
  const TIA& tia = instance().console().tia();
  w = tia.width();
  h = std::min<uInt32>(tia.height(), TIAConstants::frameBufferHeight);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TiaDisplayWidget::visibleSize(float& w, float& h) const
{
  uInt32 fw = 0, fh = 0;
  frameSize(fw, fh);
  w = fw / myZoom;
  h = fh / myZoom;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TiaDisplayWidget::clampSource()
{
  uInt32 fw = 0, fh = 0;
  frameSize(fw, fh);
  float vw = 0.F, vh = 0.F;
  visibleSize(vw, vh);

  mySrcX = std::clamp(mySrcX, 0.F, std::max(0.F, fw - vw));
  mySrcY = std::clamp(mySrcY, 0.F, std::max(0.F, fh - vh));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TiaDisplayWidget::visibleRegion(uInt32& sx, uInt32& sy,
                                     uInt32& vw, uInt32& vh) const
{
  float vwf = 0.F, vhf = 0.F;
  visibleSize(vwf, vhf);
  vw = static_cast<uInt32>(std::lround(vwf));
  vh = static_cast<uInt32>(std::lround(vhf));
  sx = static_cast<uInt32>(std::lround(mySrcX));
  sy = static_cast<uInt32>(std::lround(mySrcY));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TiaDisplayWidget::updateSurface()
{
  uInt32 fw = 0, fh = 0;
  frameSize(fw, fh);

  uInt32 vw = 0, vh = 0, sx = 0, sy = 0;
  visibleRegion(sx, sy, vw, vh);

  const TIASurface& tiaSurface = instance().frameBuffer().tiaSurface();
  const uInt8* buffer = instance().console().tia().outputBuffer();

  // Grey out the part of the frame the electron beam hasn't reached yet
  uInt32 scanx = 0, scany = 0;
  instance().console().tia().electronBeamPos(scanx, scany);
  const uInt32 scanoffset = fw * scany + scanx;

  // Copy the visible (possibly magnified and panned) region into the surface's
  // TOP-LEFT corner, so the source rectangle always starts at (0,0).  The
  // blitter sizes its texture to the source rectangle and updates it at the
  // rectangle's origin, so a non-zero origin would update outside the texture
  // and leave the image blank; panning is done here instead, by choosing which
  // region to copy.
  std::array<uInt32, TIAConstants::frameBufferWidth> line{};
  for(uInt32 dy = 0; dy < vh; ++dy)
  {
    const uInt32 rowoffset = (sy + dy) * fw + sx;
    for(uInt32 dx = 0; dx < vw; ++dx)
    {
      const uInt32 i = rowoffset + dx;
      line[dx] = tiaSurface.mapIndexedPixel(buffer[i], i >= scanoffset ? 1 : 0);
    }
    myTiaSurface->drawPixels(line.data(), 0, dy, vw);
  }

  myTiaSurface->setSrcPos(0, 0);
  myTiaSurface->setSrcSize(vw, vh);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TiaDisplayWidget::recalcRects()
{
  float vwf = 0.F, vhf = 0.F;
  visibleSize(vwf, vhf);

  // Fit the image into the widget area (inside a 1px border), preserving the
  // TIA's 2:1 pixel aspect ratio
  const int availW = _w - 2, availH = _h - 2;
  const float contentW = vwf * 2.F, contentH = vhf;
  const float scale = std::min(availW / contentW, availH / contentH);

  myImgW = static_cast<int>(contentW * scale);
  myImgH = static_cast<int>(contentH * scale);
  myImgX = 1 + (availW - myImgW) / 2;
  myImgY = 1 + (availH - myImgH) / 2;

  // Map widget-local logical coordinates to physical surface coordinates,
  // accounting for the dialog's on-screen position and any HiDPI scaling
  const Common::Rect& s_dst = dialog().surface().dstRect();
  const Int32 dpi = instance().frameBuffer().hidpiScaleFactor();
  myTiaSurface->setDstPos(s_dst.x() + (_x + myImgX) * dpi,
                          s_dst.y() + (_y + myImgY) * dpi);
  myTiaSurface->setDstSize(myImgW * dpi, myImgH * dpi);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TiaDisplayWidget::applyZoom(float zoom, int anchorX, int anchorY)
{
  // Source point currently under the anchor (widget-local) point
  float vw = 0.F, vh = 0.F;
  visibleSize(vw, vh);

  const float fracX = (myImgW > 0)
    ? std::clamp((anchorX - myImgX) / static_cast<float>(myImgW), 0.F, 1.F) : 0.5F;
  const float fracY = (myImgH > 0)
    ? std::clamp((anchorY - myImgY) / static_cast<float>(myImgH), 0.F, 1.F) : 0.5F;
  const float srcPtX = mySrcX + fracX * vw;
  const float srcPtY = mySrcY + fracY * vh;

  myZoom = std::clamp(zoom, MIN_ZOOM, MAX_ZOOM);

  // Re-anchor the same source point under the cursor at the new zoom
  visibleSize(vw, vh);
  mySrcX = srcPtX - fracX * vw;
  mySrcY = srcPtY - fracY * vh;
  clampSource();
  setDirty();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TiaDisplayWidget::handleMouseDown(int x, int y, MouseButton b, int clickCount)
{
  if(b == MouseButton::LEFT)
  {
    // Begin drag-to-pan
    myMouseMoving = true;
    myClickX = x;
    myClickY = y;
  }
  else if(b == MouseButton::RIGHT)
    // Context menu at the current mouse location
    myMenu->show(x + getAbsX(), y + getAbsY(), dialog().surface().dstRect());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TiaDisplayWidget::handleMouseUp(int x, int y, MouseButton b, int clickCount)
{
  myMouseMoving = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TiaDisplayWidget::handleMouseLeft()
{
  myMouseMoving = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TiaDisplayWidget::handleMouseWheel(int x, int y, int direction)
{
  // Wheel up (direction < 0) zooms in towards the cursor; wheel down zooms out
  static constexpr float STEP = 1.25F;
  const float factor = (direction < 0) ? STEP : 1.F / STEP;
  applyZoom(myZoom * factor, x, y);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TiaDisplayWidget::handleMouseMoved(int x, int y)
{
  if(!myMouseMoving)
    return;

  const int dx = x - myClickX, dy = y - myClickY;
  myClickX = x;
  myClickY = y;

  if(myImgW > 0 && myImgH > 0)
  {
    // Convert the widget-space drag into source pixels; pan opposite the drag
    float vw = 0.F, vh = 0.F;
    visibleSize(vw, vh);
    mySrcX -= dx * vw / static_cast<float>(myImgW);
    mySrcY -= dy * vh / static_cast<float>(myImgH);
    clampSource();
    setDirty();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TiaDisplayWidget::handleCommand(CommandSender* sender, int cmd, int data, int id)
{
  if(cmd == ContextMenu::kItemSelectedCmd)
  {
    const string& tag = myMenu->getSelectedTag().toString();
    const int cx = _w / 2, cy = _h / 2;  // zoom about the widget centre

    if(tag == "fit")
      applyZoom(MIN_ZOOM, cx, cy);
    else if(tag == "2")
      applyZoom(2.F, cx, cy);
    else if(tag == "4")
      applyZoom(4.F, cx, cy);
    else if(tag == "8")
      applyZoom(8.F, cx, cy);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TiaDisplayWidget::drawWidget(bool hilite)
{
  // Lazily create the display surface the first time we draw.  This runs while
  // the companion (secondary) backend is the active render target, so the
  // surface binds to the companion window's renderer.  A render callback then
  // composites it on top of the dialog's base surface.
  if(myTiaSurface == nullptr)
  {
    // Nearest-neighbour ('none') scaling: crisp pixels (this view is for
    // examining TIA output close up) at ANY scale factor, up or down.  Note
    // 'sharp' is quasi-integer scaling, which truncates the scale to an integer
    // and so collapses to a zero-size texture (black) for any sub-1x fit, and
    // 'blur' is bilinear, which softens the pixels.
    myTiaSurface = instance().frameBuffer().allocateSurface(
      TIAConstants::frameBufferWidth, TIAConstants::frameBufferHeight,
      ScalingInterpolation::none);
    myTiaSurface->setVisible(true);

    // Pixel-locked overlay layer (electron-beam cursor, future TIA-aligned
    // marks).  Same size/scaling as the image so it can share the image's
    // src/dst rectangles; blended so its transparent background lets the image
    // show through and only the drawn marks appear on top.
    myMarkSurface = instance().frameBuffer().allocateSurface(
      TIAConstants::frameBufferWidth, TIAConstants::frameBufferHeight,
      ScalingInterpolation::none);
    myMarkSurface->setVisible(true);
    myMarkSurface->enableBlend(true);
    myMarkSurface->setBlendLevel(100);

    // Composite order on every render: the TIA image first, then the overlay
    // layers on top of it.  The screen-space HUD layer (myHudSurface) is
    // created lazily by its first user and may still be null here.
    dialog().addRenderCallback([this]() {
      if(myTiaSurface)  myTiaSurface->render();
      if(myMarkSurface) myMarkSurface->render();
      if(myHudSurface)  myHudSurface->render();
    });
  }

  // Border around the display area; the scaled TIA image is drawn inside it
  // (on top) by the render callback
  FBSurface& s = dialog().surface();
  s.frameRect(_x, _y, _w, _h, hilite ? kWidColorHi : kColor);

  uInt32 fw = 0, fh = 0;
  frameSize(fw, fh);
  if(fw == 0 || fh == 0)
    return;

  clampSource();
  updateSurface();
  recalcRects();
  drawMarkers();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TiaDisplayWidget::drawMarkers()
{
  // Clear the previous frame's marks (fully transparent background)
  myMarkSurface->invalidate();

  // Electron-beam position cursor.  It is drawn in the image's own (top-left
  // anchored) source space and then handed the image's exact src/dst rectangles
  // below, so it lands on the precise TIA pixel at any zoom/pan.
  uInt32 scanx = 0, scany = 0;
  if(instance().console().tia().electronBeamPos(scanx, scany))
  {
    uInt32 sx = 0, sy = 0, vw = 0, vh = 0;
    visibleRegion(sx, sy, vw, vh);

    // ...but only when the beam lies inside the currently visible (panned) region
    if(scanx >= sx && scanx < sx + vw && scany >= sy && scany < sy + vh)
    {
      // A small box, in TIA pixels (so it inherits the 2:1 pixel aspect),
      // clamped to the visible region's right/bottom edge
      static constexpr uInt32 size = 2;
      const uInt32 bx = scanx - sx, by = scany - sy;
      myMarkSurface->fillRect(bx, by, std::min(size, vw - bx),
                              std::min(size, vh - by), kColorInfo);
    }
  }

  // Mirror the image surface's geometry so the overlay scales and pans with it
  myMarkSurface->setSrcRect(myTiaSurface->srcRect());
  myMarkSurface->setDstRect(myTiaSurface->dstRect());
}
