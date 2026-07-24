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

#include "Dialog.hxx"
#include "Widget.hxx"
#include "Font.hxx"
#include "OSystem.hxx"
#include "EventHandler.hxx"
#include "FrameBuffer.hxx"
#include "StringParser.hxx"
#include "Layout.hxx"
#include "MessageBox.hxx"

namespace GUI {

unique_ptr<MessageBox> MessageBox::ourBox;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
MessageBox::MessageBox(OSystem& osystem, DialogContainer& parent,
                       const GUI::Font& font, string_view text,
                       const std::function<void(bool ok)>& callback,
                       string_view okText, string_view cancelText,
                       string_view title, bool focusOKButton, bool transient)
  : Dialog(osystem, parent, font, title),
    myCallback{callback},
    myTransient{transient}
{
  createText(font, text);

  WidgetArray wid;
  addOKCancelBGroup(wid, font, okText, cancelText, focusOKButton);
  addToFocusList(wid);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// static
void MessageBox::confirm(GuiObject* boss, string_view text,
                         const std::function<void(bool ok)>& callback,
                         string_view title, string_view okText,
                         string_view cancelText, bool focusOKButton)
{
  ourBox.reset(new MessageBox(boss->instance(), boss->parent(),
                              boss->instance().frameBuffer().font(), text,
                              callback, okText, cancelText, title,
                              focusOKButton, /*transient=*/false));
  ourBox->open();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// static
unique_ptr<Dialog> MessageBox::create(OSystem& osystem, DialogContainer& parent,
                         const GUI::Font& font, string_view text,
                         const std::function<void(bool ok)>& callback,
                         string_view okText, string_view cancelText,
                         string_view title)
{
  return unique_ptr<Dialog>(new MessageBox(osystem, parent, font, text, callback,
                            okText, cancelText, title,
                            /*focusOKButton=*/true, /*transient=*/true));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// static
void MessageBox::hide()
{
  ourBox.reset();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MessageBox::createText(const GUI::Font& font, string_view text)
{
  myText = StringParser(text).stringList();
  for(const auto& s: myText)
    myTextWidgets.push_back(new StaticTextWidget(this, font, s,
                                                 TextAlign::Left));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MessageBox::layout()
{
  using GUI::BoxLayout;
  using GUI::stretchedItem;
  using Dir = BoxLayout::Dir;

  const int fontWidth    = Dialog::fontWidth(),
            buttonHeight = Dialog::buttonHeight(),
            VBORDER      = Dialog::vBorder(),
            HBORDER      = Dialog::hBorder(),
            VGAP         = Dialog::vGap();

  // Vertical stack of the text lines below the title bar; the button group sits
  // below it, positioned by layoutButtonGroup()
  auto root = std::make_unique<BoxLayout>(Dir::Vertical, 0, HBORDER, VBORDER);
  for(auto* w: myTextWidgets)
    root->addAuto(stretchedItem(w));
  root->addSpace(VGAP * 2);

  // As wide as the longest line, and as tall as the lines need -- but never
  // narrower than the button group below them. If this ends up bigger than
  // the screen, Dialog::open()'s exceedsScreen() check reports it, the same
  // as any other dialog.
  int str_w = 0;
  for(const auto& s: myText)
    str_w = std::max(static_cast<int>(s.length()), str_w);

  _w = std::max(str_w * fontWidth + HBORDER * 2, Dialog::buttonGroupWidth());
  _h = _th + static_cast<int>(root->naturalSize().h) + buttonHeight + VBORDER;

  root->doLayout(0, _th, _w, _h - _th);

  // Standard OK/Cancel button group along the bottom edge
  layoutButtonGroup();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void MessageBox::handleCommand(CommandSender* sender, int cmd, int data, int id)
{
  if(cmd == GuiObject::kOKCmd || cmd == GuiObject::kCloseCmd)
  {
    const bool ok = cmd == GuiObject::kOKCmd;

    if(myTransient)
      // Over TIA mode with no boss to return to: leaving menu mode (which
      // removes us from the dialog stack) IS the close
      instance().eventHandler().leaveMenuMode();
    else
      close();  // Owned by a boss dialog; behave like any other dialog

    myCallback(ok);
  }
  else
    Dialog::handleCommand(sender, cmd, data, id);
}

}  // namespace GUI
