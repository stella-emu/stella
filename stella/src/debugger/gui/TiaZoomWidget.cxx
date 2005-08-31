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
// $Id: TiaZoomWidget.cxx,v 1.2 2005-08-31 22:34:43 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#include "OSystem.hxx"
#include "FrameBuffer.hxx"
#include "Widget.hxx"
#include "GuiObject.hxx"
#include "ContextMenu.hxx"

#include "TiaZoomWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TiaZoomWidget::TiaZoomWidget(GuiObject* boss, int x, int y)
  : Widget(boss, x, y, 16, 16),
    CommandSender(boss),
    myMenu(NULL)
{
  _flags = WIDGET_ENABLED | WIDGET_CLEARBG | WIDGET_RETAIN_FOCUS;

  _w = 200;
  _h = 120;

  addFocusWidget(this);

  // Initialize positions
  myZoomLevel = 2;
  myNumCols = ((_w - 4) >> 1) / myZoomLevel;
  myNumRows = (_h - 4) / myZoomLevel;
  myXoff = 0;
  myYoff = 0;

  // Create context menu for zoom levels
  myMenu = new ContextMenu(this, instance()->consoleFont());

  StringList l;
  l.push_back("2x zoom");
  l.push_back("4x zoom");
  l.push_back("8x zoom");

  myMenu->setList(l);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TiaZoomWidget::~TiaZoomWidget()
{
  delete myMenu;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TiaZoomWidget::loadConfig()
{
  setDirty(); draw();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TiaZoomWidget::setPos(int x, int y)
{
  myXoff = x;
  myYoff = y;

  recalc();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TiaZoomWidget::zoom(int level)
{
  if(myZoomLevel == level)
    return;

  myZoomLevel = level;
  myNumCols   = ((_w - 4) >> 1) / myZoomLevel;
  myNumRows   = (_h - 4) / myZoomLevel;

  recalc();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TiaZoomWidget::recalc()
{
  // Don't go past end of framebuffer
  int imageWidth  = instance()->console().mediaSource().width();
  int imageHeight = instance()->console().mediaSource().height();

  if(myXoff < 0)
    myXoff = 0;
  else if(myXoff > imageWidth - myNumCols)
    myXoff = imageWidth - myNumCols;
  else if(myYoff < 0)
    myYoff = 0;
  else if(myYoff > imageHeight - myNumRows)
    myYoff = imageHeight - myNumRows;

  setDirty(); draw();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TiaZoomWidget::handleMouseDown(int x, int y, int button, int clickCount)
{
  // Grab right mouse button for zoom context menu
  if(button == 2)
  {
    myMenu->setPos(x + getAbsX(), y + getAbsY());
    myMenu->show();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool TiaZoomWidget::handleKeyDown(int ascii, int keycode, int modifiers)
{
  bool handled = false;

  switch (keycode)
  {
    case 256+17:  // up arrow
      myYoff -= 4;
      handled = true;
      break;

    case 256+18:  // down arrow
      myYoff += 4;
      handled = true;
      break;

    case 256+20:  // left arrow
      myXoff -= 2;
      handled = true;
      break;

    case 256+19:  // right arrow
      myXoff += 2;
      handled = true;
      break;
  }

  if(handled)
    recalc();

  return handled;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TiaZoomWidget::handleCommand(CommandSender* sender, int cmd, int data, int id)
{
  switch(cmd)
  {
    case kCMenuItemSelectedCmd:
    {
      int item = myMenu->getSelected(), level = 0;
      if(item == 0)
        level = 2;
      else if(item == 1)
        level = 4;
      else if(item == 2)
        level = 8;

      if(level > 0)
        zoom(level);
      break;
    }
  }
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
  uInt8* currentFrame = instance()->console().mediaSource().currentFrameBuffer();
  const int pitch  = instance()->console().mediaSource().width(),
            width  = myZoomLevel << 1,
            height = myZoomLevel;

  int x, y, col, row;
  for(y = myYoff, row = 0; y < myNumRows+myYoff; ++y, row += height)
  {
    for(x = myXoff, col = 0; x < myNumCols+myXoff; ++x, col += width)
    {
      SDL_Rect temp;

      temp.x = _x + col;
      temp.y = _y + row;
      temp.w = width;
      temp.h = height;

      fb.fillRect(_x + col + 2, _y + row + 2, width, height,
                  (OverlayColor)currentFrame[y*pitch + x]);
    }
  }
}
