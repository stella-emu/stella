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
// Copyright (c) 1995-2005 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: TiaZoomWidget.cxx,v 1.1 2005-08-31 19:15:10 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#include "OSystem.hxx"
#include "FrameBuffer.hxx"
#include "Widget.hxx"
#include "GuiObject.hxx"

#include "TiaZoomWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TiaZoomWidget::TiaZoomWidget(GuiObject* boss, int x, int y)
  : Widget(boss, x, y, 16, 16),
    CommandSender(boss),
    myZoomLevel(2)
{
  _flags = WIDGET_ENABLED | WIDGET_CLEARBG | WIDGET_RETAIN_FOCUS;

  _w = 200;
  _h = 120;

  addFocusWidget(this);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TiaZoomWidget::~TiaZoomWidget()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TiaZoomWidget::loadConfig()
{
  setDirty(); draw();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TiaZoomWidget::zoom(int level)
{
  myZoomLevel = level;


  // Redraw the zoomed image
  loadConfig();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TiaZoomWidget::handleMouseDown(int x, int y, int button, int clickCount)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool TiaZoomWidget::handleKeyDown(int ascii, int keycode, int modifiers)
{
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TiaZoomWidget::handleCommand(CommandSender* sender, int cmd, int data, int id)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TiaZoomWidget::drawWidget(bool hilite)
{
//cerr << "TiaZoomWidget::drawWidget\n";
  FrameBuffer& fb = instance()->frameBuffer();

  fb.fillRect(_x+1, _y+1, _w-2, _h-2, kBGColor);
  fb.box(_x, _y, _w, _h, kColor, kShadowColor);

  // Draw the zoomed image
  // This probably isn't as efficient as it can be, but it's a small area
  // and I don't have time to make it faster :)
  uInt8* currentFrame = myOSystem->console().mediaSource().currentFrameBuffer();

  int numCols = ((_w - 4) >> 1) / myZoomLevel;
  int numRows = (_h - 4) / myZoomLevel;

  int x, y, col, row;
  for(y = 0, row = 0; y < numRows; ++y, row += myZoomLevel)
  {
    for(x = 0, col = 0; x < numCols; ++x, col += (myZoomLevel << 1))
    {
      SDL_Rect temp;

      temp.x = _x + col;
      temp.y = _y + row;
      temp.w = myZoomLevel << 1;
      temp.h = myZoomLevel;

      fb.fillRect(_x + col + 2, _y + row + 2, myZoomLevel << 1, myZoomLevel,
                  (OverlayColor)currentFrame[x*y]);
    }
  }

/*




  // Copy the mediasource framebuffer to the RGB texture
  uInt8* currentFrame  = mediasrc.currentFrameBuffer();
  uInt8* previousFrame = mediasrc.previousFrameBuffer();
  uInt32 width         = mediasrc.width();
  uInt32 height        = mediasrc.height();
  uInt16* buffer       = (uInt16*) myTexture->pixels;

  register uInt32 y;
  for(y = 0; y < height; ++y )
  {
    const uInt32 bufofsY    = y * width;
    const uInt32 screenofsY = y * myTexture->w;

    register uInt32 x;
    for(x = 0; x < width; ++x )
    {
      const uInt32 bufofs = bufofsY + x;
      uInt8 v = currentFrame[bufofs];
      if(v != previousFrame[bufofs] || theRedrawTIAIndicator)
      {
        // If we ever get to this point, we know the current and previous
        // buffers differ.  In that case, make sure the changes are
        // are drawn in postFrameUpdate()
        theRedrawTIAIndicator = true;

        // x << 1 is times 2 ( doubling width )
        const uInt32 pos = screenofsY + (x << 1);
        buffer[pos] = buffer[pos+1] = (uInt16) myPalette[v];
      }
    }
  }
*/
}
