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

#include "FBSurface.hxx"
#include "Dialog.hxx"
#include "ToolTip.hxx"
#include "Font.hxx"
#include "EditTextWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EditTextWidget::EditTextWidget(GuiObject* boss, const GUI::Font& font,
                               int w, int h, string_view text)
  : EditableWidget(boss, font, w, h + 2, text)
{
  _flags = Widget::FLAG_ENABLED | Widget::FLAG_CLEARBG
    | Widget::FLAG_RETAIN_FOCUS | Widget::FLAG_TRACK_MOUSE;

  EditableWidget::startEditMode();  // We're always in edit mode

  // A box taller than one line was built to show several (each further line
  // adds a font height to the first line's row); remember how many, so a font
  // change can restore the height
  _lines = std::max(1, 1 + (h - font.getLineHeight()) / font.getFontHeight());

  if(_font.isLarge())
    _textOfs = 5;
  else
    _textOfs = 3;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EditTextWidget::EditTextWidget(GuiObject* boss, const GUI::Font& font,
                               int w, string_view text)
  : EditTextWidget(boss, font, w, calcHeight(font), text)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EditTextWidget::refreshFontMetrics()
{
  Widget::refreshFontMetrics();

  // Restore the framed height (lineHeight + 2, plus a font height for each line
  // beyond the first) and the text offset for the live font; the width is
  // dialog-chosen and re-applied by the owning layout().
  _h = _font.getLineHeight() + 2 + _font.getFontHeight() * (_lines - 1);
  _textOfs = _font.isLarge() ? 5 : 3;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EditTextWidget::setText(string_view str, bool changed)
{
  EditableWidget::setText(str, changed);

  if(_changed != changed)
  {
    _changed = changed;
    setDirty();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EditTextWidget::handleMouseDown(int x, int y, MouseButton b, int clickCount)
{
  if(b == MouseButton::LEFT)
  {
    if(!isEditable())
      return;

    resetSelection();
    if(setCaretPos(toCaretPos(x)))
      setDirty();
  }
  EditableWidget::handleMouseDown(x, y, b, clickCount);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EditTextWidget::drawWidget(bool hilite)
{
  FBSurface& s = _boss->dialog().surface();

  // Highlight changes
  if(_changed)
    s.fillRect(_x, _y, _w, _h, kDbgChangedColor);
  else if(!isEditable() || !isEnabled())
    s.fillRect(_x, _y, _w, _h, kDlgColor);

  // Draw a thin frame around us.
  s.frameRect(_x, _y, _w, _h, hilite && isEditable() && isEnabled() ? kWidColorHi : kColor);

  // Draw the text
  adjustOffset();
  const Common::Rect editRect = getEditRect();
  s.drawString(_font, editString(), _x + _textOfs, _y + firstTextY(), editRect.w(), editRect.h(),
               _changed && isEnabled()
               ? kDbgChangedTextColor
               : isEnabled() ? _textcolor : kColor,
               TextAlign::Left, scrollOffset(), !isEditable());

  // Draw the caret and selection
  drawCaretSelection();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Common::Rect EditTextWidget::getEditRect() const
{
  return {
    static_cast<uInt32>(_textOfs), 1,
    static_cast<uInt32>(_w - _textOfs), static_cast<uInt32>(_h)
  };
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EditTextWidget::lostFocusWidget()
{
  EditableWidget::lostFocusWidget();
  // If we loose focus, 'commit' the user changes
  commit();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EditTextWidget::endEditMode()
{
  // Editing is always enabled
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void EditTextWidget::abortEditMode()
{
  // Editing is always enabled
  abort();
}
