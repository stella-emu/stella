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
// Copyright (c) 1995-2018 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include <sstream>

#include "OSystem.hxx"
#include "FrameBuffer.hxx"
#include "FBSurface.hxx"
#include "Widget.hxx"
#include "GuiObject.hxx"
#include "ContextMenu.hxx"
#include "TiaZoomWidget.hxx"
#include "Debugger.hxx"
#include "DebuggerParser.hxx"
#include "PNGLibrary.hxx"
#include "TIADebug.hxx"
#include "TIASurface.hxx"
#include "TIA.hxx"

#include "TiaOutputWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TiaOutputWidget::TiaOutputWidget(GuiObject* boss, const GUI::Font& font,
                                 int x, int y, int w, int h)
  : Widget(boss, font, x, y, w, h),
    CommandSender(boss),
    myZoom(nullptr),
    myClickX(0),
    myClickY(0)
{
  // Create context menu for commands
  VariantList l;
  VarList::push_back(l, "Fill to scanline", "scanline");
  VarList::push_back(l, "Toggle breakpoint", "bp");
  VarList::push_back(l, "Set zoom position", "zoom");
  VarList::push_back(l, "Save snapshot", "snap");
  myMenu = make_unique<ContextMenu>(this, font, l);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TiaOutputWidget::loadConfig()
{
  setDirty();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TiaOutputWidget::saveSnapshot(int execDepth, const string& execPrefix)
{
  if (execDepth > 0) {
    drawWidget(false);
  }
  ostringstream sspath;
  sspath << instance().snapshotSaveDir()
         << instance().console().properties().get(Cartridge_Name);
  sspath << "_dbg_";
  if (execDepth > 0 && !execPrefix.empty()) {
    sspath << execPrefix << "_";
  }
  sspath << std::hex << std::setw(8) << std::setfill('0') << uInt32(instance().getTicks()/1000) << ".png";

  const uInt32 width  = instance().console().tia().width(),
               height = instance().console().tia().height();
  FBSurface& s = dialog().surface();

  GUI::Rect rect(_x, _y, _x + width*2, _y + height);
  string message = "Snapshot saved";
  try
  {
    instance().png().saveImage(sspath.str(), s, rect);
  }
  catch(const runtime_error& e)
  {
    message = e.what();
  }
  if (execDepth == 0) {
    instance().frameBuffer().showMessage(message);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TiaOutputWidget::handleMouseDown(int x, int y, MouseButton b, int clickCount)
{
  // Grab right mouse button for command context menu
  if(b == MouseButton::RIGHT)
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
  uInt32 ystart = instance().console().tia().ystart();

  switch(cmd)
  {
    case ContextMenu::kItemSelectedCmd:
    {
      const string& rmb = myMenu->getSelectedTag().toString();

      if(rmb == "scanline")
      {
        ostringstream command;
        int lines = myClickY + ystart;
        if(instance().console().tia().isRendering())
          lines -= instance().console().tia().scanlines();
        if(lines > 0)
        {
          command << "scanline #" << lines;
          string message = instance().debugger().parser().run(command.str());
          instance().frameBuffer().showMessage(message);
        }
      }
      else if(rmb == "bp")
      {
        ostringstream command;
        int scanline = myClickY + ystart;
        command << "breakif _scan==#" << scanline;
        string message = instance().debugger().parser().run(command.str());
        instance().frameBuffer().showMessage(message);
      }
      else if(rmb == "zoom")
      {
        if(myZoom)
          myZoom->setPos(myClickX, myClickY);
      }
      else if(rmb == "snap")
      {
        instance().debugger().parser().run("savesnap");
      }
      break;
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TiaOutputWidget::drawWidget(bool hilite)
{
//cerr << "TiaOutputWidget::drawWidget\n";
  const uInt32 width  = instance().console().tia().width(),
               height = instance().console().tia().height();
  FBSurface& s = dialog().surface();

  s.vLine(_x + _w + 1, _y, height, kColor);
  s.hLine(_x, _y + height + 1, _x +_w + 1, kColor);

  // Get current scanline position
  // This determines where the frame greying should start, and where a
  // scanline 'pointer' should be drawn
  uInt32 scanx, scany, scanoffset;
  bool visible = instance().console().tia().electronBeamPos(scanx, scany);
  scanoffset = width * scany + scanx;

  for(uInt32 y = 0, i = 0; y < height; ++y)
  {
    uInt32* line_ptr = myLineBuffer;
    for(uInt32 x = 0; x < width; ++x, ++i)
    {
      uInt8 shift = i >= scanoffset ? 1 : 0;
      uInt32 pixel = instance().frameBuffer().tiaSurface().pixel(i, shift);
      *line_ptr++ = pixel;
      *line_ptr++ = pixel;
    }
    s.drawPixels(myLineBuffer, _x + 1, _y + 1 + y, width << 1);
  }

  // Show electron beam position
  if(visible && scanx < width && scany+2u < height)
    s.fillRect(_x + 1 + (scanx<<1), _y + 1 + scany, 3, 3, kColorInfo);
}
