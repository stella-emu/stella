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
// Copyright (c) 1995-2009 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#include <sstream>

#include "OSystem.hxx"
#include "FrameBuffer.hxx"
#include "Widget.hxx"
#include "GuiObject.hxx"
#include "ContextMenu.hxx"
#include "TiaZoomWidget.hxx"
#include "Debugger.hxx"
#include "DebuggerParser.hxx"
#include "TIADebug.hxx"

#include "TiaOutputWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TiaOutputWidget::TiaOutputWidget(GuiObject* boss, const GUI::Font& font,
                                 int x, int y, int w, int h)
  : Widget(boss, font, x, y, w, h),
    CommandSender(boss),
    myMenu(NULL),
    myZoom(NULL)
{
  _type = kTiaOutputWidget;

  // Create context menu for commands
  StringMap l;
  l.push_back("Fill to scanline", "scanline");
  l.push_back("Set breakpoint", "bp");
  l.push_back("Set zoom position", "zoom");
  myMenu = new ContextMenu(this, font, l);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TiaOutputWidget::~TiaOutputWidget()
{
  delete myMenu;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TiaOutputWidget::loadConfig()
{
  setDirty(); draw();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TiaOutputWidget::handleMouseDown(int x, int y, int button, int clickCount)
{
  // Grab right mouse button for command context menu
  if(button == 2)
  {
    myClickX = x;
    myClickY = y;

    // Add menu at current x,y mouse location
    myMenu->show(x + getAbsX(), y + getAbsY());
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TiaOutputWidget::handleCommand(CommandSender* sender, int cmd, int data, int id)
{
  int ystart = atoi(instance().console().properties().get(Display_YStart).c_str());

  switch(cmd)
  {
    case kCMenuItemSelectedCmd:
    {
      const string& rmb = myMenu->getSelectedTag();

      if(rmb == "scanline")
      {
        ostringstream command;
        int lines = myClickY + ystart -
            instance().debugger().tiaDebug().scanlines();
        if(lines > 0)
        {
          command << "scanline #" << lines;
          instance().debugger().parser().run(command.str());
        }
      }
      else if(rmb == "bp")
      {
        ostringstream command;
        int scanline = myClickY + ystart;
        command << "breakif _scan==#" << scanline;
        instance().debugger().parser().run(command.str());
      }
      else if(rmb == "zoom")
      {
        if(myZoom)
          myZoom->setPos(myClickX, myClickY);
      }
      break;
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TiaOutputWidget::drawWidget(bool hilite)
{
//cerr << "TiaOutputWidget::drawWidget\n";
  // FIXME - maybe 'greyed out mode' should be done here, not in the TIA class
  FBSurface& s = dialog().surface();

  const uInt32 width = 160,  // width is always 160
               height = instance().console().tia().height();
  for(uInt32 y = 0; y < height; ++y)
  {
    uInt32* line_ptr = myLineBuffer;
    for(uInt32 x = 0; x < width; ++x)
    {
      uInt32 pixel = instance().frameBuffer().tiaPixel(y*width+x);
      *line_ptr++ = pixel;
      *line_ptr++ = pixel;
    }
    s.drawPixels(myLineBuffer, _x, _y+y, width << 1);
  }
}
