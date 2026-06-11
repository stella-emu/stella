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

#include "Widget.hxx"
#include "TiaZoomWidget.hxx"
#include "TiaWindowDialog.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TiaWindowDialog::TiaWindowDialog(OSystem& osystem, DialogContainer& parent,
                                 int x, int y, int w, int h)
  : Dialog(osystem, parent, x, y, w, h)
{
  constexpr int border = 2;
  myTiaZoom = new TiaZoomWidget(this, _font, border, border,
                                w - 2 * border, h - 2 * border);
  addToFocusList(myTiaZoom->getFocusList());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TiaWindowDialog::loadConfig()
{
  myTiaZoom->loadConfig();
}
