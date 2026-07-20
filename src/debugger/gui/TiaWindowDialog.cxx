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
#include "TiaDisplayWidget.hxx"
#include "TiaWindow.hxx"
#include "TiaWindowDialog.hxx"

// The dialog's border, inside which the display widget sits
static constexpr int BORDER = 2;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TiaWindowDialog::TiaWindowDialog(OSystem& osystem, DialogContainer& parent,
                                 int x, int y, int w, int h)
  : Dialog(osystem, parent, x, y, w, h)
{
  // Created at a placeholder size; layout() fits it to the window
  myTiaDisplay = new TiaDisplayWidget(this, _font);
  addToFocusList(myTiaDisplay->getFocusList());

  TiaWindowDialog::layout();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TiaWindowDialog::layout()
{
  // The companion window owns its (resizable) size; take ours from it every
  // time, which is what makes a live resize re-flow this dialog.  The display
  // widget then scales its image to fit, preserving the TIA's pixel aspect
  const Common::Size& size = static_cast<const TiaWindow&>(parent()).size();

  _w = static_cast<int>(size.w);
  _h = static_cast<int>(size.h);

  myTiaDisplay->setArea(BORDER, BORDER, _w - 2 * BORDER, _h - 2 * BORDER);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TiaWindowDialog::loadConfig()
{
  myTiaDisplay->loadConfig();
}
