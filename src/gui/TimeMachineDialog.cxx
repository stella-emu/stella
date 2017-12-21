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
// Copyright (c) 1995-2017 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include "Dialog.hxx"
#include "Font.hxx"
#include "EventHandler.hxx"
#include "FrameBuffer.hxx"
#include "OSystem.hxx"
#include "Widget.hxx"
#include "TimeMachineDialog.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TimeMachineDialog::TimeMachineDialog(OSystem& osystem, DialogContainer& parent,
                                     int max_w, int max_h)
  : Dialog(osystem, parent)
{
  const GUI::Font& font = instance().frameBuffer().font();
  const int buttonWidth = font.getStringWidth("Right Diff B") + 20,
            buttonHeight = font.getLineHeight() + 6,
            rowHeight = font.getLineHeight() + 10;

  // Set real dimensions
  _w = 3 * (buttonWidth + 5) + 20;
  _h = 6 * rowHeight + 15;
}
