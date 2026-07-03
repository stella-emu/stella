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

#include "OSystem.hxx"
#include "FrameBuffer.hxx"
#include "Version.hxx"
#include "Layout.hxx"

#include "WhatsNewDialog.hxx"

static constexpr int MAX_CHARS = 64; // maximum number of chars per line

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
WhatsNewDialog::WhatsNewDialog(OSystem& osystem, DialogContainer& parent)
  : Dialog(osystem, parent, osystem.frameBuffer().font(),
           "What's New in Stella " + string(STELLA_VERSION) + "?")
{
  const string_view version = instance().settings().getString("stella.version");
  if(version < "7.0")
  {
    add("accelerated ARM emulation by ~15%");
    add("added user defined CPU cycle timers to debugger");
  }
  add("ported Stella to SDL3");
  add(ELLIPSIS + " (for a complete list see 'docs/Changes.txt')");

  WidgetArray wid;
  addOKBGroup(wid, _font);
  addBGroupToFocusList(wid);

  // We don't have a close/cancel button, but we still want the cancel
  // event to be processed
  processCancelWithoutWidget();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void WhatsNewDialog::add(string_view text)
{
  const int lineHeight = Dialog::lineHeight(),
            fontHeight = Dialog::fontHeight();
  string txt = "\x1f ";
  txt += text;

  // automatically wrap too long texts; continuation lines advance by fontHeight,
  // the final line of each bullet by lineHeight (giving a gap between bullets)
  while(txt.length() > MAX_CHARS)
  {
    int i = MAX_CHARS;

    while(--i && txt[i] != ' ');  // NOLINT(bugprone-inc-dec-in-conditions)
    myLines.push_back(new StaticTextWidget(this, _font, 0, 0, txt.substr(0, i)));
    myLineAdvance.push_back(fontHeight);
    txt = " " + txt.substr(i);
  }
  myLines.push_back(new StaticTextWidget(this, _font, 0, 0, txt));
  myLineAdvance.push_back(lineHeight);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void WhatsNewDialog::layout()
{
  using GUI::BoxLayout;
  using GUI::anchoredItem;
  using GUI::hCentered;
  using Dir = BoxLayout::Dir;

  const int fontWidth    = Dialog::fontWidth(),
            buttonHeight = Dialog::buttonHeight(),
            VBORDER      = Dialog::vBorder(),
            HBORDER      = Dialog::hBorder(),
            VGAP         = Dialog::vGap();

  _w = MAX_CHARS * fontWidth + HBORDER * 2;

  int linesHeight = 0;
  for(const int adv: myLineAdvance)
    linesHeight += adv;
  _h = _th + VBORDER + linesHeight + VGAP * 2 + buttonHeight + VBORDER;
  assert(std::cmp_less_equal(_h, FBMinimum::Height)); // minimal launcher height

  // Stack the (pre-wrapped) bullet lines, then a single centered OK button
  auto root = std::make_unique<BoxLayout>(Dir::Vertical, 0, HBORDER, VBORDER);
  for(size_t i = 0; i < myLines.size(); ++i)
    root->addFixed(anchoredItem(myLines[i]), myLineAdvance[i]);
  root->addSpace(VGAP * 2);
  root->addFixed(hCentered(_okWidget, _okWidget->getWidth()), buttonHeight);
  root->doLayout(0, _th, _w, _h - _th);
}
