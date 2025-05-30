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
// Copyright (c) 1995-2025 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include "OSystem.hxx"
#include "FrameBuffer.hxx"
#include "Version.hxx"

#include "WhatsNewDialog.hxx"

static constexpr int MAX_CHARS = 64; // maximum number of chars per line

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
WhatsNewDialog::WhatsNewDialog(OSystem& osystem, DialogContainer& parent,
                               int max_w, int max_h)
  : Dialog(osystem, parent, osystem.frameBuffer().font(),
           "What's New in Stella " + string(STELLA_VERSION) + "?")
{
  const int fontWidth = Dialog::fontWidth(),
    buttonHeight = Dialog::buttonHeight(),
    VBORDER = Dialog::vBorder(),
    HBORDER = Dialog::hBorder(),
    VGAP = Dialog::vGap();
  int ypos = _th + VBORDER;

  // Set preliminary dimensions
  setSize(MAX_CHARS * fontWidth + HBORDER * 2, max_h,
          max_w, max_h);

  const string_view version = instance().settings().getString("stella.version");
  if(version < "7.0")
  {
    add(ypos, "accelerated ARM emulation by ~15%");
    add(ypos, "added user defined CPU cycle timers to debugger");
  }
  add(ypos, "ported Stella to SDL3");
  add(ypos, ELLIPSIS + " (for a complete list see 'docs/Changes.txt')");

  // Set needed dimensions
  ypos += VGAP * 2 + buttonHeight + VBORDER;
  assert(std::cmp_less_equal(ypos, FBMinimum::Height)); // minimal launcher height
  setSize(MAX_CHARS * fontWidth + HBORDER * 2, ypos, max_w, max_h);

  WidgetArray wid;
  addOKBGroup(wid, _font);
  addBGroupToFocusList(wid);

  // We don't have a close/cancel button, but we still want the cancel
  // event to be processed
  processCancelWithoutWidget();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void WhatsNewDialog::add(int& ypos, string_view text)
{
  const int lineHeight = Dialog::lineHeight(),
            fontHeight = Dialog::fontHeight(),
            HBORDER    = Dialog::hBorder();
  string txt = "\x1f ";
  txt += text;

  // automatically wrap too long texts
  while(txt.length() > MAX_CHARS)
  {
    int i = MAX_CHARS;

    while(--i && txt[i] != ' ');  // NOLINT: bugprone-inc-dec-in-conditions
    new StaticTextWidget(this, _font, HBORDER, ypos, txt.substr(0, i));
    txt = " " + txt.substr(i);
    ypos += fontHeight;
  }
  new StaticTextWidget(this, _font, HBORDER, ypos, txt);
  ypos += lineHeight;
}
