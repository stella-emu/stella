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

#include "bspf.hxx"
#include "FrameBuffer.hxx"
#include "FBSurface.hxx"
#include "Font.hxx"
#include "ContextMenu.hxx"
#include "Dialog.hxx"
#include "DialogContainer.hxx"
#include "PopUpWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PopUpWidget::PopUpWidget(GuiObject* boss, const GUI::Font& font,
                         int w, const VariantList& items, GuiCmd::Code cmd)
  : EditableWidget(boss, font, w, font.getLineHeight() + 2)
{
  _flags = Widget::Flag::Enabled | Widget::Flag::RetainFocus
    | Widget::Flag::TrackMouse;
  _bgcolor = kDlgColor;
  _bgcolorhi = kDlgColor;     // do not highlight the background
  _textcolor = kTextColor;
  _textcolorhi = kTextColor;  // do not highlight the label

  setTextFilter([](char c) {
    return (isprint(c) && c != '\"') || c == '\x1c' || c == '\x1d'; // DEGREE || ELLIPSIS
  });
  setEditable(false);

  setArrow();

  _w = w + dropDownWidth(font);

  // vertically center the arrows (the text centers itself, see firstTextY())
  myArrowsY = (_h - _arrowHeight) / 2;

  myMenu = std::make_unique<ContextMenu>(this, font, items, cmd,
                                    w + dropDownWidth(font));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
PopUpWidget::PopUpWidget(GuiObject* boss, const GUI::Font& font,
                         const VariantList& items, GuiCmd::Code cmd)
  : PopUpWidget(boss, font, calcWidth(font, items), items, cmd)
{
  // Nobody chose that width but me, so nobody else will restore it
  myAutoWidth = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int PopUpWidget::calcWidth(const GUI::Font& font, const VariantList& items)
{
  int width = 0;

  for(const auto& item: items)
    width = std::max(width, font.getStringWidth(item.first));

  return width;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PopUpWidget::setBoxWidth(int w)
{
  _w = w + dropDownWidth(_font);
  myMenu->setMaxWidth(_w);
  setDirty();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PopUpWidget::refreshFont()
{
  // Also refreshes the inherited (right-click) mouse menu, if allocated
  EditableWidget::refreshFont();

  // Re-pick the arrow bitmap/dimensions for the live font, restore the framed
  // height and vertically re-center the arrows (mirrors the ctor).
  setArrow();
  _h = _font.getLineHeight() + 2;

  // A width I derived from my own items is mine to restore; one a dialog chose
  // is re-applied by the owning layout(), which runs straight after this
  if(myAutoWidth)
    setBoxWidth(calcWidth(_font, myMenu->entries()));
  myArrowsY = (_h - _arrowHeight) / 2;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PopUpWidget::setID(uInt32 id)
{
  myMenu->setID(id);

  Widget::setID(id);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PopUpWidget::setWidth(int w)
{
  Widget::setWidth(w);
  // Keep the drop-down menu as wide as the value box
  myMenu->setMaxWidth(_w);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PopUpWidget::addItems(const VariantList& items)
{
  myMenu->addItems(items);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PopUpWidget::setSelected(const Variant& tag, const Variant& def)
{
  myMenu->setSelected(tag, def);
  setText(myMenu->getSelectedName());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PopUpWidget::setSelectedIndex(int idx, bool changed)
{
  if(_changed != changed)
  {
    _changed = changed;
    setDirty();
  }
  myMenu->setSelectedIndex(idx);
  setText(myMenu->getSelectedName());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PopUpWidget::setSelectedMax(bool changed)
{
  if(_changed != changed)
  {
    _changed = changed;
    setDirty();
  }
  myMenu->setSelectedMax();
  setText(myMenu->getSelectedName());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PopUpWidget::clearSelection()
{
  myMenu->clearSelection();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int PopUpWidget::getSelected() const
{
  return myMenu->getSelected();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const string& PopUpWidget::getSelectedName() const
{
  return myMenu->getSelectedName();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PopUpWidget::setSelectedName(string_view name)
{
  myMenu->setSelectedName(name);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const Variant& PopUpWidget::getSelectedTag() const
{
  return myMenu->getSelectedTag();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PopUpWidget::handleMouseDown(int x, int y, MouseButton b, int clickCount)
{
  if(b == MouseButton::LEFT)
  {
    resetSelection();
    if(!isEditable() || x > _w - dropDownWidth(_font))
    {
      if(isEnabled() && !myMenu->isVisible())
      {
        // Add menu just underneath parent widget
        myMenu->show(getAbsX(), getAbsY() + getHeight(),
                     dialog().surface().dstRect(), myMenu->getSelected());
      }
    }
    else
    {
      if(setCaretPos(toCaretPos(x)))
        setDirty();
    }
  }
  EditableWidget::handleMouseDown(x, y, b, clickCount);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PopUpWidget::handleMouseWheel(int x, int y, int direction)
{
  if(isEnabled() && !myMenu->isVisible())
  {
    if(direction < 0)
      myMenu->sendSelectionUp();
    else
      myMenu->sendSelectionDown();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool PopUpWidget::handleEvent(Event::Type e)
{
  if(!isEnabled())
    return false;

  switch(e)
  {
    case Event::UISelect:
      handleMouseDown(0, 0, MouseButton::LEFT, 0);
      return true;
    case Event::UIUp:
    case Event::UILeft:
    case Event::UIPgUp:
      return myMenu->sendSelectionUp();
    case Event::UIDown:
    case Event::UIRight:
    case Event::UIPgDown:
      return myMenu->sendSelectionDown();
    case Event::UIHome:
      return myMenu->sendSelectionFirst();
    case Event::UIEnd:
      return myMenu->sendSelectionLast();
    default:
      return false;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PopUpWidget::handleCommand(CommandSender* sender, GuiCmd::Code cmd,
                                int data, int id)
{
  // Intercept all events sent through the PromptWidget
  // They're likely from our ContextMenu, indicating a redraw is required
  setText(myMenu->getSelectedName());
  dialog().setDirty();

  // Pass the cmd on to our parent
  sendCommand(cmd, data, id);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PopUpWidget::setArrow()
{
  // The arrow is sized from the font, so it grows with the text instead of
  // stepping.  At the default 9x18 this reproduces the old 9x7 bitmap
  _arrowWidth = arrowWidth(_font);
  _arrowHeight = _arrowWidth - 2;
  _arrowThickness = ((_arrowWidth - 1) / 4) + 1;

  _textOfs = textInset(_font);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PopUpWidget::drawWidget(bool hilite)
{
  FBSurface& s = dialog().surface();

  const int x = _x;
  int w = _w;

  // Draw a thin frame around us.
  s.frameRect(x, _y, w, _h, isEnabled() && hilite ? kWidColorHi : kColor);
  if(isEnabled() && hilite)
    s.frameRect(x + w - (_arrowWidth * 2 - 1), _y, (_arrowWidth * 2 - 1), _h, kWidColorHi);

  // Fill the background
  const ColorId bgCol = isEditable() ? kWidColor : kDlgColor;
  s.fillRect(x + 1, _y + 1, w - (_arrowWidth * 2 - 1), _h - 2,
             _changed ? kDbgChangedColor : bgCol);
  s.fillRect(x + w - (_arrowWidth * 2 - 2), _y + 1, (_arrowWidth * 2 - 3), _h - 2,
             isEnabled() && hilite ? kBtnColorHi : bgCol);
  // Draw an arrow pointing down at the right end to signal this is a dropdown/popup
  s.drawArrow(x + w - (_arrowWidth * 1.5 - 1), _y + myArrowsY + 1,
              _arrowWidth, _arrowHeight, ArrowDirection::Down,
              !isEnabled() ? kColor : kTextColor, _arrowThickness);

  // Draw the selected entry, if any
  const string& name = editString();
  const bool editable = isEditable();

  w -= dropDownWidth(_font);
  const TextAlign align = (_font.getStringWidth(name) > w && !editable) ?
                           TextAlign::Right : TextAlign::Left;
  adjustOffset();
  s.drawString(_font, name, x + _textOfs, _y + firstTextY(), w,
               !isEnabled() ? kColor : _changed ? kDbgChangedTextColor : kTextColor,
               align, editable ? -_editScrollOffset : 0, !editable);

  if(editable)
    drawCaretSelection();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Common::Rect PopUpWidget::getEditRect() const
{
  return {
    static_cast<uInt32>(_textOfs), 1,
    static_cast<uInt32>(_w - _textOfs - dropDownWidth(_font)),
    static_cast<uInt32>(_h)
  };
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PopUpWidget::endEditMode()
{
  // Editing is always enabled
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void PopUpWidget::abortEditMode()
{
  // Editing is always enabled
}
