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
// $Id: TiaZoomWidget.cxx,v 1.8 2006-05-04 17:45:24 stephena Exp $
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
TiaZoomWidget::TiaZoomWidget(GuiObject* boss, const GUI::Font& font,
                             int x, int y)
  : Widget(boss, font, x, y, 16, 16),
    CommandSender(boss),
    myMenu(NULL)
{
  _flags = WIDGET_ENABLED | WIDGET_CLEARBG | WIDGET_RETAIN_FOCUS |
           WIDGET_WANTS_RAWDATA;

  _w = 210;
  _h = 120;

  addFocusWidget(this);

  // Initialize positions
  myZoomLevel = 2;
  myNumCols = ((_w - 4) >> 1) / myZoomLevel;
  myNumRows = (_h - 4) / myZoomLevel;
  myXoff = 0;
  myYoff = 0;
  myXCenter = myNumCols >> 1;
  myYCenter = myNumRows >> 1;

  // Create context menu for zoom levels
  myMenu = new ContextMenu(this, font);

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
  // Center zoom on given x,y point
  myXCenter = x >> 1;
  myYCenter = y;

//cerr << " ==> myXCenter = " << myXCenter << ", myYCenter = " << myYCenter << endl;

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
  const int width  = instance()->console().mediaSource().width(),
            height = instance()->console().mediaSource().height();

  // Figure out the bounding rectangle for the current center coords
  const int xoff = myNumCols >> 1,
            yoff = myNumRows >> 1;

  if(myXCenter < xoff)
    myXCenter = xoff;
  else if(myXCenter + xoff >= width)
    myXCenter = width - xoff - 1;
  else if(myYCenter < yoff)
    myYCenter = yoff;
  else if(myYCenter + yoff >= height)
    myYCenter = height - yoff - 1;

  // Only redraw when necessary
  int oldXoff = myXoff, oldYoff = myYoff;
  myXoff = myXCenter - (myNumCols >> 1);
  myYoff = myYCenter - (myNumRows >> 1);
  if(oldXoff != myXoff || oldYoff != myYoff)
  {
    setDirty(); draw();
//cerr << " OLD ==> myXoff: " << oldXoff << ", myYoff = " << oldYoff << endl;
//cerr << " NEW ==> myXoff: " << myXoff << ", myYoff = " << myYoff << endl << endl;
  }
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

  switch(ascii)
  {
    case 256+17:  // up arrow
      myYCenter -= 4;
      handled = true;
      break;

    case 256+18:  // down arrow
      myYCenter += 4;
      handled = true;
      break;

    case 256+20:  // left arrow
      myXCenter -= 2;
      handled = true;
      break;

    case 256+19:  // right arrow
      myXCenter += 2;
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
  uInt8* currentFrame  = instance()->console().mediaSource().currentFrameBuffer();
  const int pitch  = instance()->console().mediaSource().width(),
            width  = myZoomLevel << 1,
            height = myZoomLevel;

  int x, y, col, row;
  for(y = myYoff, row = 0; y < myNumRows+myYoff; ++y, row += height)
  {
    for(x = myXoff, col = 0; x < myNumCols+myXoff; ++x, col += width)
    {
      fb.fillRect(_x + col + 2, _y + row + 2, width, height,
                  currentFrame[y*pitch + x]);
    }
  }
}
