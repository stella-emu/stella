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
  add(ypos, "added adjustable autofire (see 'Advanced Settings')");
  add(ypos, "added new UI theme 'Dark' (see 'Advanced Settings')");
#else
  add(ypos, "added adjustable autofire");
  add(ypos, "added 'Dark' UI theme");
  //add(ypos, "extended global hotkeys for debug options");
  add(ypos, "added option to playback a game using the Time Machine");
  //add(ypos, "allow taking snapshots from within the Time Machine dialog");
  add(ypos, "added the ability to access most files that Stella uses from within a ZIP file");
  add(ypos, "extended AtariVox support to handle flow control, so that long phrases are no longer corrupted / cut off");
  add(ypos, "added QuadTari controller support");
  add(ypos, "added option to select the audio device");
  //add(ypos, "added option to display detected settings info when a ROM is loaded");
  //add(ypos, "added another oddball TIA glitch option for delayed background color");
  //add(ypos, "replaced 'Re-disassemble' with 'Disassemble @ current line' in debugger");
  //add(ypos, "fixed bug when taking fullscreen snapshots; the dimensions were sometimes cut");
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
