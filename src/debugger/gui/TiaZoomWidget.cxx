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

#include <cmath>

#include "OSystem.hxx"
#include "Console.hxx"
#include "Debugger.hxx"
#include "DebuggerParser.hxx"
#include "TIA.hxx"
#include "FrameBuffer.hxx"
#include "FBSurface.hxx"
#include "Widget.hxx"
#include "GuiObject.hxx"
#include "Dialog.hxx"
#include "ToolTip.hxx"
#include "ContextMenu.hxx"
#include "TiaZoomWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TiaZoomWidget::TiaZoomWidget(GuiObject* boss, const GUI::Font& font)
  : Widget(boss, font, 16, 16),
    CommandSender(boss)
{
  _flags = Widget::FLAG_ENABLED | Widget::FLAG_CLEARBG |
           Widget::FLAG_RETAIN_FOCUS | Widget::FLAG_TRACK_MOUSE;
  _bgcolor = _bgcolorhi = kDlgColor;

  addFocusWidget(this);

  // Size the view and grid from the placeholder area; setArea() redoes this
  // for the real one
  recomputeGrid(_w, _h);

  // Create context menu for zoom levels
  VariantList l;
  VarList::push_back(l, "Fill to scanline", "scanline");
  VarList::push_back(l, "Toggle breakpoint", "bp");
  VarList::push_back(l, "2x zoom", "2");
  VarList::push_back(l, "4x zoom", "4");
  VarList::push_back(l, "8x zoom", "8");
  myMenu = std::make_unique<ContextMenu>(this, font, l);

  setHelpAnchor("TIAZoom", true);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TiaZoomWidget::loadConfig()
{
  setDirty();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TiaZoomWidget::setPos(int x, int y)
{
  // Center on given x,y point
  myOffX = x - (myNumCols >> 1);
  myOffY = y - (myNumRows >> 1);

  recalc();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TiaZoomWidget::setArea(int x, int y, int w, int h)
{
  // Our setPos(int,int) override is hijacked to re-centre the zoom on a TIA
  // pixel, so move the widget itself via the geometry setPos explicitly
  Widget::setPos(Common::Point(x, y));
  recomputeGrid(w, h);
  recalc();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TiaZoomWidget::recomputeGrid(int w, int h)
{
  // Use all the space available; the zoom view need not preserve the TIA aspect
  // ratio, and once the view outgrows the image drawWidget() simply stops at the
  // image's edge, leaving the rest of the view blank
  _w = w;
  _h = h;

  myNumCols = (_w - 4) / myZoomLevel;
  myNumRows = (_h - 4) / myZoomLevel;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TiaZoomWidget::zoom(int level)
{
  if(myZoomLevel == level)
    return;

  // zoom towards mouse position
  const auto clickx = static_cast<double>(myClickX),
             clicky = static_cast<double>(myClickY);
  myOffX = round(myOffX + clickx / myZoomLevel - clickx / level);
  myOffY = round(myOffY + clicky / myZoomLevel - clicky / level);

  myZoomLevel = level;
  myNumCols = (_w - 4) / myZoomLevel & 0xfffe; // must be even!
  myNumRows = (_h - 4) / myZoomLevel;

  recalc();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TiaZoomWidget::recalc()
{
  const int tw = instance().console().tia().width(),
            th = instance().console().tia().height();

  // Don't go past end of framebuffer.  When the viewport is larger than the
  // image (e.g. the companion TIA window at low zoom), the available range can
  // go negative, so pin the upper bound at 0 to keep the offset at top-left.
  myOffX = BSPF::clamp(myOffX, 0, std::max(0, (tw << 1) - myNumCols));
  myOffY = BSPF::clamp(myOffY, 0, std::max(0, th - myNumRows));

  setDirty();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TiaZoomWidget::handleMouseDown(int x, int y, MouseButton b, int clickCount)
{
  myClickX = x;
  myClickY = y - 1;

  // Button 1 is for 'drag'/movement of the image
  // Button 2 is for context menu
  if(b == MouseButton::LEFT)
  {
    // Indicate mouse drag started/in progress
    myMouseMoving = true;
    myOffXLo = myOffYLo = 0;
  }
  else if(b == MouseButton::RIGHT)
  {
    // Add menu at current x,y mouse location
    myMenu->show(x + getAbsX(), y + getAbsY(), dialog().surface().dstRect());
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TiaZoomWidget::handleMouseUp(int x, int y, MouseButton b, int clickCount)
{
  myMouseMoving = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TiaZoomWidget::handleMouseWheel(int x, int y, int direction)
{
  dialog().tooltip().hide();

  // zoom towards mouse position
  myClickX = x;
  myClickY = y - 1;

  if(direction > 0)
  {
    if(myZoomLevel > 1)
      zoom(myZoomLevel - 1);
  }
  else
  {
    if(myZoomLevel < 8)
      zoom(myZoomLevel + 1);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TiaZoomWidget::handleMouseMoved(int x, int y)
{
  if(myMouseMoving)
  {
    y--;
    const int diffx = x + myOffXLo - myClickX;
    const int diffy = y + myOffYLo - myClickY;

    myClickX = x;
    myClickY = y;

    myOffX -= diffx / myZoomLevel;
    myOffY -= diffy / myZoomLevel;
    // handle remainder
    myOffXLo = diffx % myZoomLevel;
    myOffYLo = diffy % myZoomLevel;

    recalc();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TiaZoomWidget::handleMouseLeft()
{
  myMouseMoving = false;
  Widget::handleMouseLeft();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool TiaZoomWidget::handleEvent(Event::Type event)
{
  bool handled = true;

  switch(event)
  {
    case Event::UIUp:
      myOffY -= 4;
      break;

    case Event::UIDown:
      myOffY += 4;
      break;

    case Event::UILeft:
      myOffX -= 4;
      break;

    case Event::UIRight:
      myOffX += 4;
      break;

    case Event::UIPgUp:
      myOffY = 0;
      break;

    case Event::UIPgDown:
      myOffY = _h;
      break;

    case Event::UIHome:
      myOffX = 0;
      break;

    case Event::UIEnd:
      myOffX = _w;
      break;

    default:
      handled = false;
      break;
  }

  if(handled)
    recalc();

  return handled;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TiaZoomWidget::handleCommand(CommandSender* sender, int cmd, int data, int id)
{
  if(cmd == ContextMenu::kItemSelectedCmd)
  {
    const uInt32 startLine = instance().console().tia().startLine();
    const string& rmb = myMenu->getSelectedTag().toString();

    if(rmb == "scanline")
    {
      int lines = myClickY / myZoomLevel + myOffY + startLine - instance().console().tia().scanlines();

      if(lines < 0)
        lines += instance().console().tia().scanlinesLastFrame();
      if(lines > 0)
      {
        const string message = instance().debugger().parser().run(
          std::format("scanline #{}", lines));
        instance().frameBuffer().showTextMessage(message);
      }
    }
    else if(rmb == "bp")
    {
      const int scanline = myClickY / myZoomLevel + myOffY + startLine;
      const string message = instance().debugger().parser().run(
        std::format("breakif _scan==#{}", scanline));
      instance().frameBuffer().showTextMessage(message);
    }
    else
    {
      const int level = myMenu->getSelectedTag().toInt();
      if(level > 0)
        zoom(level);
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Common::Point TiaZoomWidget::getToolTipIndex(const Common::Point& pos) const
{
  // A native TIA pixel is drawn 'myZoomLevel << 1' wide and myOffX counts
  // doubled pixels, so 'col' is a native column, as getToolTip() indexes with
  const Int32 width = instance().console().tia().width();
  const Int32 height = instance().console().tia().height();
  const int col = (pos.x - 1 - getAbsX()) / (myZoomLevel << 1) + (myOffX >> 1);
  const int row = (pos.y - 1 - getAbsY()) / myZoomLevel + myOffY;

  if(col < 0 || col >= width || row < 0 || row >= height)
    return Common::Point(-1, -1);
  else
    return Common::Point(col, row);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string TiaZoomWidget::getToolTip(const Common::Point& pos) const
{
  const Common::Point& idx = getToolTipIndex(pos);

  if(idx.x < 0)
    return string{};

  const Int32 i = idx.x + idx.y * instance().console().tia().width();
  const uInt32 startLine = instance().console().tia().startLine();
  const uInt8* tiaOutputBuffer = instance().console().tia().outputBuffer();

  return std::format("{}X: #{}\nY: #{}\nC: ${}",
    _toolTipText,
    idx.x,
    idx.y + startLine,
    Common::Base::toString(tiaOutputBuffer[i], Common::Base::Fmt::_16));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool TiaZoomWidget::changedToolTip(const Common::Point& oldPos,
                                     const Common::Point& newPos) const
{
  return getToolTipIndex(oldPos) != getToolTipIndex(newPos);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TiaZoomWidget::drawWidget(bool hilite)
{
  FBSurface& s = dialog().surface();

  s.fillRect(_x+1, _y+1, _w-2, _h-2, kBGColor);
  s.frameRect(_x, _y, _w, _h, hilite ? kWidColorHi : kColor);

  // Draw the zoomed image
  // This probably isn't as efficient as it can be, but it's a small area
  // and I don't have time to make it faster :)
  const uInt8* currentFrame  = instance().console().tia().outputBuffer();
  const int width = instance().console().tia().width(),
            height = instance().console().tia().height(),
            wzoom = myZoomLevel << 1,
            hzoom = myZoomLevel;

  // Get current scanline position
  // This determines where the frame greying should start
  uInt32 scanx = 0, scany = 0;
  instance().console().tia().electronBeamPos(scanx, scany);
  const uInt32 scanoffset = width * scany + scanx;

  // The view may be larger than the image, in which case it shows all of it and
  // leaves the rest blank; never read beyond the frame buffer
  const int xEnd = std::min((myNumCols + myOffX) >> 1, width),
            yEnd = std::min(myNumRows + myOffY, height);

  for(int y = myOffY, row = 0; y < yEnd; ++y, row += hzoom)
  {
    for(int x = myOffX >> 1, col = 0; x < xEnd; ++x, col += wzoom)
    {
      const uInt32 idx = std::max(y * width + x, 0);
      const auto color = static_cast<ColorId>(currentFrame[idx] | (idx > scanoffset ? 1 : 0));
      s.fillRect(_x + col + 1, _y + row + 1, wzoom, hzoom, color);
    }
  }
}
