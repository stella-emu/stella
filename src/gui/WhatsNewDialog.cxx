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
// Copyright (c) 1995-2020 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include "Version.hxx"

#include "WhatsNewDialog.hxx"

constexpr int MAX_CHARS = 64; // maximum number of chars per line

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
WhatsNewDialog::WhatsNewDialog(OSystem& osystem, DialogContainer& parent, const GUI::Font& font,
                               int max_w, int max_h)
#if defined(RETRON77)
  : Dialog(osystem, parent, font, "What's New in Stella " + string(STELLA_VERSION) + " for RetroN 77?")
#else
  : Dialog(osystem, parent, font, "What's New in Stella " + string(STELLA_VERSION) + "?")
#endif
{
  const int fontHeight = _font.getFontHeight(),
    fontWidth = _font.getMaxCharWidth(),
    buttonHeight = _font.getLineHeight() * 1.25;
  const int VGAP = fontHeight / 4;
  const int VBORDER = fontHeight / 2;
  const int HBORDER = fontWidth * 1.25;
  int ypos = _th + VBORDER;

  // Set preliminary dimensions
  setSize(MAX_CHARS * fontWidth + HBORDER * 2, max_h,
          max_w, max_h);

#if defined(RETRON77)
  add(ypos, "increased sample size for CDFJ+");
  add(ypos, "fixed navigation bug in Video & Audio settings dialog");
#else
  add(ypos, "enhanced cut/copy/paste for text editing");
  add(ypos, "added undo and redo to text editing");
  add(ypos, "added wildcard support to launcher dialog filter");
  add(ypos, "added tooltips to many UI items");
  add(ypos, "increased sample size for CDFJ+");
  add(ypos, ELLIPSIS + " (for a complete list see 'docs/Changes.txt')");
#endif

  // Set needed dimensions
  setSize(MAX_CHARS * fontWidth + HBORDER * 2,
          ypos + VGAP * 2 + buttonHeight + VBORDER,
          max_w, max_h);

  WidgetArray wid;
  addOKBGroup(wid, _font);
  addBGroupToFocusList(wid);

  // We don't have a close/cancel button, but we still want the cancel
  // event to be processed
  processCancelWithoutWidget();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void WhatsNewDialog::add(int& ypos, const string& text)
{
  const int lineHeight = _font.getLineHeight(),
    fontHeight = _font.getFontHeight(),
    fontWidth = _font.getMaxCharWidth(),
    HBORDER = fontWidth * 1.25;
  const string DOT = "\x1f";
  string txt = DOT + " " + text;

  // automatically wrap too long texts
  while(txt.length() > MAX_CHARS)
  {
    int i = MAX_CHARS;

    while(--i && txt[i] != ' ');
    new StaticTextWidget(this, _font, HBORDER, ypos, txt.substr(0, i));
    txt = " " + txt.substr(i);
    ypos += fontHeight;
  }
  new StaticTextWidget(this, _font, HBORDER, ypos, txt);
  ypos += lineHeight;
}
