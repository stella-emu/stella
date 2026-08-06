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
#include "WrappedTextWidget.hxx"

#include "WhatsNewDialog.hxx"

// The longest line we are prepared to draw before wrapping, and the most lines
// to show before the text scrolls rather than growing the dialog further.  The
// dialog is sized to its text, so there is no floor to keep: one line of news
// gets one line of box
static constexpr int MAX_CHARS = 64;
static constexpr uInt16 MAX_LINES = 20, MIN_LINES = 1;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
WhatsNewDialog::WhatsNewDialog(OSystem& osystem, DialogContainer& parent)
  : Dialog(osystem, parent, osystem.frameBuffer().font(),
           "What's New in Stella " + string(STELLA_VERSION) + "?")
{
  string text;
  const auto add = [&text](string_view entry) {
    if(!text.empty())
      text += '\n';
    text += "\x1f ";
    text += entry;
  };

  // This release only, however we are reached: from the first run after an
  // upgrade, or from the About dialog.  Anything older belongs in Changes.txt
  add("ported Stella to SDL3");
  add(ELLIPSIS + " (for a complete list see 'docs/Changes.txt')");

  // One bullet per line: the parser breaks on the newlines above and wraps
  // whatever is still too long for the width layout() ends up giving it
  myText = new WrappedTextWidget(this, _font, text, MAX_LINES, MIN_LINES);
  myText->setEditable(false);
  myText->setEnabled(false);

  WidgetArray wid;
  addOKBGroup(wid, _font);
  addBGroupToFocusList(wid);

  // We don't have a close/cancel button, but we still want the cancel
  // event to be processed
  processCancelWithoutWidget();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void WhatsNewDialog::layout()
{
  using GUI::BoxLayout;
  using GUI::widgetItem;
  using GUI::anchoredItem;
  using Dir = BoxLayout::Dir;

  const int fontWidth = Dialog::fontWidth(),
            VBORDER   = Dialog::vBorder(),
            HBORDER   = Dialog::hBorder(),
            VGAP      = Dialog::vGap();

  // As wide as there is room for, up to a comfortable reading length.  The
  // window we open over may be narrower than that at a large font, and it is
  // the one that has the final say
  uInt32 availW = 0, availH = 0;
  getDynamicBounds(availW, availH);
  _w = std::min(static_cast<int>(availW), MAX_CHARS * fontWidth + HBORDER * 2);

  // Wrapping is the widget's own concern; it just has to be told the width,
  // and only then can it say how tall the wrapped text came to
  myText->setWidth(_w - HBORDER * 2);

  // The OK button keeps the width its label needs, centered on its row
  auto okRow = std::make_unique<BoxLayout>(Dir::Horizontal);
  okRow->addStretchSpace();
  okRow->addAuto(anchoredItem(_okWidget));
  okRow->addStretchSpace();

  auto root = std::make_unique<BoxLayout>(Dir::Vertical, 0, HBORDER, VBORDER);
  root->addAuto(widgetItem(myText, 0,
                           static_cast<int>(myText->naturalSize().h)));
  root->addSpace(VGAP * 2);
  root->addAuto(std::move(okRow));

  _h = std::min(static_cast<int>(availH),
                _th + static_cast<int>(root->naturalSize().h));

  root->doLayout(0, _th, _w, _h - _th);
}
