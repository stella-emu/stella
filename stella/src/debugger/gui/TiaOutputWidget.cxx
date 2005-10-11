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
// $Id: TiaOutputWidget.cxx,v 1.7 2005-10-11 19:38:10 stephena Exp $
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
TiaOutputWidget::TiaOutputWidget(GuiObject* boss, int x, int y, int w, int h)
  : Widget(boss, x, y, w, h),
    CommandSender(boss),
    myMenu(NULL),
    myZoom(NULL)
{
  // Create context menu for commands
  myMenu = new ContextMenu(this, instance()->consoleFont());

  StringList l;
  l.push_back("Fill to scanline");
  l.push_back("Set breakpoint");
  l.push_back("Set zoom position");

  myMenu->setList(l);

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
void TiaOutputWidget::advanceScanline(int lines)
{
  while(lines)
  {
    instance()->console().mediaSource().updateScanline();
    --lines;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TiaOutputWidget::advance(int frames)
{
  while(frames)
  {
    instance()->console().mediaSource().update();
    --frames;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TiaOutputWidget::handleMouseDown(int x, int y, int button, int clickCount)
{
  // Grab right mouse button for command context menu
  if(button == 2)
  {
    myClickX = x;
    myClickY = y;

    myMenu->setPos(x + getAbsX(), y + getAbsY());
    myMenu->show();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TiaOutputWidget::handleCommand(CommandSender* sender, int cmd, int data, int id)
{
  int ystart = atoi(instance()->console().properties().get("Display.YStart").c_str());

  switch(cmd)
  {
    case kCMenuItemSelectedCmd:
      switch(myMenu->getSelected())
      {
        case 0:
        {
          ostringstream command;
          int lines = myClickY + ystart -
              instance()->debugger().tiaDebug().scanlines();
          if(lines > 0)
          {
            command << "scanline #" << lines;
            instance()->debugger().parser()->run(command.str());
          }
          break;
        }

        case 1:
        {
          ostringstream command;
          int scanline = myClickY + ystart;
          command << "breakif _scan==#" << scanline;
          instance()->debugger().parser()->run(command.str());
          break;
        }

        case 2:
          if(myZoom)
            myZoom->setPos(myClickX, myClickY);
          break;
      }
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TiaOutputWidget::drawWidget(bool hilite)
{
  // FIXME - check if we're in 'greyed out mode' and act accordingly
  instance()->frameBuffer().refresh();
  instance()->frameBuffer().drawMediaSource();
}
