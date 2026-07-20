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
#include "FBSurface.hxx"
#include "Widget.hxx"
#include "GuiObject.hxx"
#include "Dialog.hxx"
#include "ContextMenu.hxx"
#include "TiaZoomWidget.hxx"
#include "Debugger.hxx"
#include "DebuggerParser.hxx"
#include "PNGLibrary.hxx"
#include "TIADebug.hxx"
#include "TIASurface.hxx"
#include "TIA.hxx"
#include "TIAConstants.hxx"
#include "TimerManager.hxx"
#include "FrameManager.hxx"

#include "TiaOutputWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TiaOutputWidget::TiaOutputWidget(GuiObject* boss, const GUI::Font& font)
  : Widget(boss, font, 0, 0, 0, 0),
    CommandSender(boss)
{
  // Create context menu for commands
  VariantList l;
  VarList::push_back(l, "Fill to scanline", "scanline");
  VarList::push_back(l, "Toggle breakpoint", "bp");
  VarList::push_back(l, "Set zoom position", "zoom");
#ifdef IMAGE_SUPPORT
  VarList::push_back(l, "Save snapshot", "snap");
#endif
  myMenu = std::make_unique<ContextMenu>(this, font, l);

  //setHelpAnchor("TIADisplay", true); // TODO: does not work due to missing focus
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TiaOutputWidget::~TiaOutputWidget()
{
  // The framebuffer keeps a reference to every allocated surface, so release
  // ours explicitly (the dialog can be recreated, which would otherwise leak)
  FrameBuffer& fb = instance().frameBuffer();
  if(myTiaSurface)  fb.deallocateSurface(myTiaSurface);
  if(myMarkSurface) fb.deallocateSurface(myMarkSurface);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TiaOutputWidget::loadConfig()
{
  setEnabled(true);
  setDirty();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TiaOutputWidget::saveSnapshot(int execDepth, string_view execPrefix,
                                   bool mark)
{
#ifdef IMAGE_SUPPORT
  if(execDepth > 0)
    drawWidget(false);

  string sspath = std::format("{}{}", instance().snapshotSaveDir().getPath(),
                              instance().console().properties().get(PropType::Cart_Name));
  if(mark)
  {
    sspath += "_dbg_";
    if(execDepth > 0 && !execPrefix.empty())
      sspath += std::format("{}_", execPrefix);
    sspath += std::format("{:08X}",
      static_cast<uInt32>(TimerManager::getTicks() / 1000));
  }
  else
  {
    // Determine if the file already exists, checking each successive filename
    // until one doesn't exist
    if(FSNode(sspath + ".png").exists())
    {
      for(const uInt32 i: std::views::iota(1U))
      {
        const string candidate = std::format("{}_{}.png", sspath, i);
        if(!FSNode(candidate).exists())
        {
          sspath += std::format("_{}", i);
          break;
        }
      }
    }
  }
  sspath += ".png";

  const uInt32 width = instance().console().tia().width();
  uInt32 height = instance().console().tia().height();
  height = std::min<uInt32>(height, FrameManager::Metrics::baseHeightPAL);

  string message = "Snapshot saved";
  if(myTiaSurface == nullptr)
    message = "Snapshot not available";
  else
  {
    // The image lives in its own surface, at the top-left, horizontally doubled
    const Common::Rect rect(0, 0, width * 2, height);
    try
    {
      PNGLibrary::saveImage(sspath, *myTiaSurface, rect);
    }
    catch(const std::runtime_error& e)
    {
      message = e.what();
    }
  }
  if(execDepth == 0)
    instance().frameBuffer().showTextMessage(message);
#else
  instance().frameBuffer().showTextMessage("PNG image saving not supported");
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TiaOutputWidget::handleMouseDown(int x, int y, MouseButton b, int clickCount)
{
  // Map the (possibly scaled) click to native TIA coordinates.  The zoom widget
  // and the context-menu handlers below work in doubled columns / native rows,
  // matching the original fixed-scale behaviour.
  int col = 0, row = 0;
  widgetToImage(x, y, col, row);

  if(b == MouseButton::LEFT)
    myZoom->setPos(col << 1, row);
  // Grab right mouse button for command context menu
  else if(b == MouseButton::RIGHT)
  {
    myClickX = col << 1;
    myClickY = row;

    // Add menu at current x,y mouse location
    myMenu->show(x + getAbsX(), y + getAbsY(), dialog().surface().dstRect());
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TiaOutputWidget::handleCommand(CommandSender* sender, int cmd, int data, int id)
{
  const uInt32 startLine = instance().console().tia().startLine();

  if(cmd == ContextMenu::kItemSelectedCmd)
  {
    const string& rmb = myMenu->getSelectedTag().toString();

    if(rmb == "scanline")
    {
      int lines = myClickY + startLine - instance().console().tia().scanlines();

      if(lines < 0)
        lines += instance().console().tia().scanlinesLastFrame();
      if(lines > 0)
      {
        const string message = instance().debugger().parser().run(
          std::format("scanLine #{}", lines));
        instance().frameBuffer().showTextMessage(message);
      }
    }
    else if(rmb == "bp")
    {
      const int scanline = myClickY + startLine;
      const string message = instance().debugger().parser().run(
        std::format("breakIf _scan==#{}", scanline));
      instance().frameBuffer().showTextMessage(message);
    }
    else if(rmb == "zoom")
    {
      if(myZoom)
        myZoom->setPos(myClickX, myClickY);
    }
    else if(rmb == "snap")
    {
      instance().debugger().parser().run("saveSnap");
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Common::Point TiaOutputWidget::getToolTipIndex(const Common::Point& pos) const
{
  int col = 0, row = 0;
  if(!widgetToImage(pos.x - getAbsX(), pos.y - getAbsY(), col, row))
    return Common::Point(-1, -1);

  return Common::Point(col, row);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string TiaOutputWidget::getToolTip(const Common::Point& pos) const
{
  const Common::Point& idx = getToolTipIndex(pos);

  if(idx.x < 0)
    return string{};

  const uInt32 startLine = instance().console().tia().startLine();
  const uInt32 height = instance().console().tia().height();
  // limit to 274 lines (PAL default without scaling)
  const uInt32 yStart = height <= FrameManager::Metrics::baseHeightPAL
    ? 0 : (height - FrameManager::Metrics::baseHeightPAL) >> 1;
  const Int32 i = idx.x + (yStart + idx.y) * instance().console().tia().width();
  const uInt8* tiaOutputBuffer = instance().console().tia().outputBuffer();

  return std::format("{}X: #{}\nY: #{}\nC: ${}",
    _toolTipText,
    idx.x,
    idx.y + startLine,
    Common::Base::toString(tiaOutputBuffer[i], Common::Base::Fmt::_16));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool TiaOutputWidget::changedToolTip(const Common::Point& oldPos,
                                     const Common::Point& newPos) const
{
  return getToolTipIndex(oldPos) != getToolTipIndex(newPos);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TiaOutputWidget::drawWidget(bool hilite)
{
  // Lazily create the image + overlay surfaces on first draw, and register the
  // render callback that composites them on top of the dialog's base surface.
  if(myTiaSurface == nullptr)
  {
    FrameBuffer& fb = instance().frameBuffer();

    // 'none' (nearest-neighbour) scaling keeps the TIA pixels crisp at any size
    myTiaSurface = fb.allocateSurface(
      TIAConstants::viewableWidth, TIAConstants::frameBufferHeight,
      ScalingInterpolation::none);
    myTiaSurface->setVisible(true);

    // Pixel-locked overlay for the electron-beam cursor: same size/scaling as
    // the image so it can share the image's src/dst rectangles; blended so its
    // transparent background lets the image show through
    myMarkSurface = fb.allocateSurface(
      TIAConstants::viewableWidth, TIAConstants::frameBufferHeight,
      ScalingInterpolation::none);
    myMarkSurface->setVisible(true);
    myMarkSurface->enableBlend(true);
    myMarkSurface->setBlendLevel(100);

    // Composite order on every render: TIA image first, then the beam overlay
    dialog().addRenderCallback([this]() {
      if(myTiaSurface)  myTiaSurface->render();
      if(myMarkSurface) myMarkSurface->render();
    });
  }

  updateSurface();
  recalcRects();
  drawMarkers();

  // Frame the scaled image on the dialog's base surface (the image itself is
  // composited on top by the render callback)
  FBSurface& s = dialog().surface();
  s.frameRect(_x + myImgX - 1, _y + myImgY - 1, myImgW + 2, myImgH + 2,
              hilite ? kWidColorHi : kColor);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TiaOutputWidget::updateSurface()
{
  const uInt32 width = instance().console().tia().width();
  uInt32 height = instance().console().tia().height();
  // limit to 274 lines (PAL default without scaling); center taller frames
  const uInt32 yStart = height <= FrameManager::Metrics::baseHeightPAL ? 0 :
      (height - FrameManager::Metrics::baseHeightPAL) / 2;
  height = std::min<uInt32>(height, FrameManager::Metrics::baseHeightPAL);

  // Get current scanline position; this determines where the frame greying of
  // the yet-to-be-drawn part of the image should start
  uInt32 scanx = 0, scany = 0;
  instance().console().tia().electronBeamPos(scanx, scany);
  const uInt32 scanoffset = width * scany + scanx;
  const uInt8* tiaOutputBuffer = instance().console().tia().outputBuffer();
  const TIASurface& tiaSurface = instance().frameBuffer().tiaSurface();

  // Copy the frame into the surface's top-left, horizontally doubled (the TIA's
  // 2:1 pixel aspect), so the source rectangle always starts at (0,0)
  for(uInt32 y = 0, i = yStart * width; y < height; ++y)
  {
    uInt32 lineIdx = 0;
    for(uInt32 x = 0; x < width; ++x, ++i)
    {
      const uInt32 pixel = tiaSurface.mapIndexedPixel(
        tiaOutputBuffer[i], i >= scanoffset ? 1 : 0);
      myLineBuffer[lineIdx++] = pixel;
      myLineBuffer[lineIdx++] = pixel;
    }
    myTiaSurface->drawPixels(myLineBuffer.data(), 0, y, width << 1);
  }

  myTiaSurface->setSrcPos(0, 0);
  myTiaSurface->setSrcSize(width << 1, height);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TiaOutputWidget::recalcRects()
{
  const int srcW = static_cast<int>(myTiaSurface->srcRect().w()),
            srcH = static_cast<int>(myTiaSurface->srcRect().h());
  if(srcW <= 0 || srcH <= 0)
    return;

  // Fit the image into the widget area (inside a 1px border), preserving the
  // aspect ratio already baked into the horizontally doubled source
  const int availW = _w - 2, availH = _h - 2;
  const float scale = std::min(static_cast<float>(availW) / srcW,
                               static_cast<float>(availH) / srcH);
  myImgW = static_cast<int>(srcW * scale);
  myImgH = static_cast<int>(srcH * scale);
  // Anchor at the widget's top-left (matching the original TIA image layout)
  myImgX = 1;
  myImgY = 1;

  // Map widget-local logical coordinates to physical surface coordinates,
  // accounting for the dialog's on-screen position and any HiDPI scaling
  const Common::Rect& s_dst = dialog().surface().dstRect();
  const Int32 dpi = instance().frameBuffer().hidpiScaleFactor();
  myTiaSurface->setDstPos(s_dst.x() + (_x + myImgX) * dpi,
                          s_dst.y() + (_y + myImgY) * dpi);
  myTiaSurface->setDstSize(myImgW * dpi, myImgH * dpi);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TiaOutputWidget::drawMarkers()
{
  // Clear the previous frame's marks (fully transparent background)
  myMarkSurface->invalidate();

  uInt32 scanx = 0, scany = 0;
  if(instance().console().tia().electronBeamPos(scanx, scany))
  {
    const uInt32 width = instance().console().tia().width();
    uInt32 height = instance().console().tia().height();
    const uInt32 yStart = height <= FrameManager::Metrics::baseHeightPAL ? 0 :
        (height - FrameManager::Metrics::baseHeightPAL) / 2;
    height = std::min<uInt32>(height, FrameManager::Metrics::baseHeightPAL);

    // Only draw when the beam lies inside the displayed region
    if(scanx < width && scany >= yStart && scany < yStart + height)
    {
      // A small box in (doubled) surface pixels, so it inherits the 2:1 aspect
      // and scales/pans with the image via the shared src/dst rectangles
      const uInt32 bx = scanx << 1, by = scany - yStart;
      myMarkSurface->fillRect(bx, by, std::min<uInt32>(3, (width << 1) - bx),
                              std::min<uInt32>(3, height - by), kColorInfo);
    }
  }

  // Mirror the image surface geometry so the overlay scales/aligns with it
  myMarkSurface->setSrcRect(myTiaSurface->srcRect());
  myMarkSurface->setDstRect(myTiaSurface->dstRect());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool TiaOutputWidget::widgetToImage(int lx, int ly, int& col, int& row) const
{
  const int width = static_cast<int>(instance().console().tia().width());
  uInt32 h = instance().console().tia().height();
  h = std::min<uInt32>(h, FrameManager::Metrics::baseHeightPAL);
  const int height = static_cast<int>(h);

  if(myImgW <= 0 || myImgH <= 0)
  {
    col = row = 0;
    return false;
  }

  const float fx = (lx - myImgX) / static_cast<float>(myImgW);
  const float fy = (ly - myImgY) / static_cast<float>(myImgH);
  const bool inside = fx >= 0.F && fx < 1.F && fy >= 0.F && fy < 1.F;

  // Native TIA column / displayed row, clamped so callers can act on a point
  // in the (letterbox) border too
  col = std::clamp(static_cast<int>(fx * width), 0, width - 1);
  row = std::clamp(static_cast<int>(fy * height), 0, height - 1);
  return inside;
}
