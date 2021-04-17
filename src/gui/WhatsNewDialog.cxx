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
// Copyright (c) 1995-2021 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include "OSystem.hxx"
#include "FrameBuffer.hxx"
#include "Version.hxx"

#include "WhatsNewDialog.hxx"

constexpr int MAX_CHARS = 64; // maximum number of chars per line

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
WhatsNewDialog::WhatsNewDialog(OSystem& osystem, DialogContainer& parent,
                               int max_w, int max_h)
#if defined(RETRON77)
  : Dialog(osystem, parent, osystem.frameBuffer().font(),
           "What's New in Stella " + string(STELLA_VERSION) + " for RetroN 77?")
#else
  : Dialog(osystem, parent, osystem.frameBuffer().font(),
           "What's New in Stella " + string(STELLA_VERSION) + "?")
#endif
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

  const string& version = instance().settings().getString("stella.version");
#if defined(RETRON77)
  if(version < "6.5")
  {
    add(ypos, "increased sample size for CDFJ+");
    add(ypos, "fixed navigation bug in Video & Audio settings dialog");
    add(ypos, "fixed autofire bug for trackball controllers");
    add(ypos, "fixed paddle button bug for jittering controllers");
    add(ypos, "improved switching between joysticks and paddles");
    add(ypos, "improved memory usage in UI mode");
    add(ypos, "fixed broken Driving Controller support for Stelladaptor/2600-daptor devices");
    add(ypos, "fixed missing QuadTari option in UI");
  }
  add(ypos, "improved analog input reading");
  add(ypos, "fixed QuadTari support for some controller types");
  add(ypos, "fixed palette and TV effects saving");
#else
  if(version < "6.5")
  {
    add(ypos, "added high scores saving");
    add(ypos, "enhanced cut/copy/paste and undo/redo for text editing");
    add(ypos, "added mouse support for text editing");
    add(ypos, "added wildcard support to launcher dialog filter");
    add(ypos, "added option to search subdirectories in launcher");
    add(ypos, "added tooltips to many UI items");
    add(ypos, "added sound to Time Machine playback");
    add(ypos, "moved settings, properties etc. to an SQLite database");
    add(ypos, "fixed paddle button bug for jittering controllers");
    add(ypos, "fixed broken Driving Controller support for Stelladaptor/2600-daptor devices");
    add(ypos, "fixed missing QuadTari option in UI");
  }
  add(ypos, "added context-sensitive help");
  add(ypos, "improved analog input reading");
  add(ypos, "improved multi-monitor support");
  add(ypos, "fixed QuadTari support for some controller types");
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
  const int lineHeight = Dialog::lineHeight(),
            fontHeight = Dialog::fontHeight(),
            HBORDER    = Dialog::hBorder();
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
