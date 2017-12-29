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

#include "Dialog.hxx"
#include "Font.hxx"
#include "EventHandler.hxx"
#include "FrameBuffer.hxx"
#include "FBSurface.hxx"
#include "OSystem.hxx"
#include "Widget.hxx"
#include "TimeMachineDialog.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TimeMachineDialog::TimeMachineDialog(OSystem& osystem, DialogContainer& parent,
                                     int max_w, int max_h)
  : Dialog(osystem, parent)
{
  const GUI::Font& font = instance().frameBuffer().font();
  const int buttonWidth = font.getStringWidth("   ") + 20,
//            buttonHeight = font.getLineHeight() + 6,
            rowHeight = font.getLineHeight() + 10;

  WidgetArray wid;

  // Set real dimensions
  _w = 10 * (buttonWidth + 5) + 20;
  _h = 2 * rowHeight + 15;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TimeMachineDialog::center()
{
  // Place on the bottom of the screen, centered horizontally
  const GUI::Size& screen = instance().frameBuffer().screenSize();
  const GUI::Rect& dst = surface().dstRect();
  surface().setDstPos((screen.w - dst.width()) >> 1, screen.h - dst.height() - 10);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TimeMachineDialog::loadConfig()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TimeMachineDialog::handleCommand(CommandSender* sender, int cmd,
                                      int data, int id)
{
cerr << cmd << endl;
  switch(cmd)
  {
    default:
      Dialog::handleCommand(sender, cmd, data, 0);
  }
}
