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
#include "Layout.hxx"
#include "NullControlWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
NullControlWidget::NullControlWidget(GuiObject* boss, const GUI::Font& font,
                                     Controller& controller,
                                     bool embedded)
  : ControllerWidget(boss, font, controller)
{
  // Create the text at a placeholder position; reflow() lays it out.  Embedded
  // in a QuadTari there is only room for a terse "not avail."
  // NOLINTBEGIN(cppcoreguidelines-prefer-member-initializer)
  myLine1 = new StaticTextWidget(boss, font,
                                 embedded ? "not" : "Controller input");
  myLine2 = new StaticTextWidget(boss, font,
                                 embedded ? "avail." : "not available");
  // NOLINTEND(cppcoreguidelines-prefer-member-initializer)

  if(!embedded)
    createHeader();
  reflow();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NullControlWidget::layoutContent(GUI::BoxLayout& col)
{
  using GUI::centeredItem;

  col.addAuto(centeredItem(myLine1));
  col.addAuto(centeredItem(myLine2));
}
