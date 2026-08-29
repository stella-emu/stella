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
#include "Widget.hxx"
#include "ScrollBarWidget.hxx"
#include "Dialog.hxx"
#include "FrameBuffer.hxx"
#include "StellaKeys.hxx"
#include "EventHandler.hxx"
#include "ListWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ListWidget::ListWidget(GuiObject* boss, const GUI::Font& font, bool useScrollbar)
  : EditableWidget(boss, font),
    _useScrollbar{useScrollbar}
{
  _flags = Widget::Flag::Enabled | Widget::Flag::ClearBG | Widget::Flag::RetainFocus;
  _bgcolor = kWidColor;
  _bgcolorhi = kWidColor;
  _textcolor = kTextColor;
  _textcolorhi = kTextColor;

  _editMode = false;

  // My real dimensions -- and the row count that follows from them -- arrive
  // via setWidth()/setHeight(), which reserve the scrollbar's room the same way
  if(_useScrollbar)
  {
    _scrollBar = new ScrollBarWidget(boss, font);
    _scrollBar->setTarget(this);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ListWidget::setPos(const Common::Point& pos)
{
  Widget::setPos(pos);
  // The scrollbar is a sibling widget, not a child, so it must be moved to
  // track the list (it sits flush against the list's right edge)
  if(_useScrollbar)
    _scrollBar->setPos(_x + _w, _y);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ListWidget::setWidth(int w)
{
  // getWidth() reports the full footprint (drawable area + scrollbar), so
  // setWidth() must subtract the scrollbar again to stay its inverse
  // (mirrors the constructor); otherwise repeated resizes accumulate the
  // scrollbar width into the list
  _fullWidth = w;

  if(_useScrollbar)
  {
    // The scrollbar sizes its own (font-derived) width; we just reserve room
    // for it and keep it flush against the list's right edge.  It only asks
    // for that room while it is needed: with everything in view there is
    // nothing to scroll, so it hides and the width is the list's to use
    const bool needed = scrollBarNeeded();

    _scrollBar->setVisible(needed);
    Widget::setWidth(w - (needed ? ScrollBarWidget::scrollBarWidth(_font) : 1));
    _scrollBar->setPosX(_x + _w);
  }
  else
    Widget::setWidth(w - 1);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ListWidget::scrollBarNeeded() const
{
  return _useScrollbar && std::cmp_greater(_list.size(), _rows);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ListWidget::updateScrollBarRoom()
{
  // Nothing to re-split before we have been given a footprint.  The guard is
  // for the subclass that re-wraps its text to the new width: that lands back
  // in recalc(), and the answer there cannot change again -- a wider list
  // never needs MORE lines -- so one pass is always enough
  if(_fullWidth == 0 || _inScrollBarRoom)
    return;

  _inScrollBarRoom = true;
  setWidth(_fullWidth);
  _inScrollBarRoom = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ListWidget::setHeight(int h)
{
  Widget::setHeight(h);
  if(_useScrollbar)
    _scrollBar->setHeight(h);

  _rows = (h - 2) / _lineHeight;
  recalc();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ListWidget::setSelected(int item)
{
  setDirty();

  if(item < 0 || std::cmp_greater_equal(item, _list.size()))
    return;

  if(isEnabled())
  {
    if(_editMode)
      abortEditMode();

    _selectedItem = item;
    sendCommand(Cmd::SelectionChanged, _selectedItem, _id);

    _currentPos = _selectedItem - _rows / 2;
    scrollToSelected();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ListWidget::setSelected(string_view item)
{
  int selected = -1;
  if(!_list.empty())
  {
    if(item.empty())
      selected = 0;
    else
    {
      uInt32 itemToSelect = 0;
      for(const auto& iter: _list)
      {
        if(item == iter)
        {
          selected = itemToSelect;
          break;
        }
        ++itemToSelect;
      }
      if(itemToSelect > _list.size() || selected == -1)
        selected = 0;
    }
  }
  setSelected(selected);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ListWidget::setHighlighted(int item)
{
  if(item < -1 || std::cmp_greater_equal(item, _list.size()))
    return;

  if(isEnabled())
  {
    if(_editMode)
      abortEditMode();

    _highlightedItem = item;

    // Only scroll the list if we're about to pass the page boundary
    if(_currentPos == 0)
      _currentPos = _highlightedItem;
    else if(_highlightedItem == _currentPos + _rows)
      _currentPos += _rows;

    scrollToHighlighted();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const string& ListWidget::getSelectedString() const
{
  return (_selectedItem >= 0 && std::cmp_less(_selectedItem, _list.size()))
    ? _list[_selectedItem]
    : EmptyString();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ListWidget::scrollTo(int item)
{
  item = BSPF::clamp(item, 0, static_cast<int>(_list.size() - 1));

  if(_currentPos != item)
  {
    _currentPos = item;
    scrollBarRecalc();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int ListWidget::getWidth() const
{
  // Our footprint is what setWidth() was given, however we have since split it
  // with the scrollbar -- a hidden bar must not shrink what we report, or the
  // focus rect (and anything else measuring us) would no longer fit us
  return _fullWidth != 0
    ? _fullWidth
    : _w + ScrollBarWidget::scrollBarWidth(_font);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ListWidget::recalc()
{
  const int size = static_cast<int>(_list.size());

  if(_currentPos >= size - _rows)
  {
    if(size <= _rows)
      _currentPos = 0;
    else
      _currentPos = size - _rows;
  }
  _currentPos = std::max(_currentPos, 0);
  _selectedItem = BSPF::clamp(_selectedItem, 0, std::max(size - 1, 0));
  _editMode = false;

  if(_useScrollbar)
  {
    _scrollBar->setNumEntries(static_cast<int>(_list.size()));
    _scrollBar->setEntriesPerPage(_rows);
    // hide the scrollbar if no longer necessary, which hands its room back to
    // the list (and take it again once there is something to scroll)
    if(_scrollBar->isVisible() != scrollBarNeeded())
      updateScrollBarRoom();
    scrollBarRecalc();
  }

  // Reset to normal data entry
  abortEditMode();

  setDirty();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ListWidget::scrollBarRecalc()
{
  if(_useScrollbar)
  {
    _scrollBar->setCurrentPos(_currentPos);
    _scrollBar->recalc();
    sendCommand(Cmd::Scrolled, _currentPos, _id);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ListWidget::handleMouseDown(int x, int y, MouseButton b, int clickCount)
{
  if(!isEnabled())
    return;

  resetSelection();
  // First check whether the selection changed
  const int newSelectedItem = findItem(x, y);
  if(std::cmp_greater_equal(newSelectedItem, _list.size()))
    return;

  if(_selectedItem != newSelectedItem)
  {
    if(_editMode)
      abortEditMode();
    _selectedItem = newSelectedItem;
    sendCommand(Cmd::SelectionChanged, _selectedItem, _id);
    setDirty();
  }

  // TODO: Determine where inside the string the user clicked and place the
  // caret accordingly. See _editScrollOffset and EditTextWidget::handleMouseDown.
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ListWidget::handleMouseUp(int x, int y, MouseButton b, int clickCount)
{
  // If this was a double click and the mouse is still over the selected item,
  // send the double click command
  if(clickCount == 2 && (_selectedItem == findItem(x, y)))
  {
    sendCommand(Cmd::DoubleClicked, _selectedItem, _id);

    // Start edit mode
    if(isEditable() && !_editMode)
      startEditMode();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ListWidget::handleMouseWheel(int x, int y, int direction)
{
  if(_useScrollbar)
    _scrollBar->handleMouseWheel(x, y, direction);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int ListWidget::findItem(int x, int y) const
{
  return (y - 1) / _lineHeight + _currentPos;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ListWidget::handleText(char text)
{
  // Class EditableWidget handles all text editing related key presses for us
  return _editMode ? EditableWidget::handleText(text) : true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ListWidget::handleKeyDown(StellaKey key, StellaMod mod)
{
  // Ignore all Alt-mod keys
  if(StellaModTest::isAlt(mod))
    return false;

  bool handled = true;
  if(!_editMode)
  {
    switch(key)
    {
      case StellaKey::SPACE:
        // Snap list back to currently highlighted line
        if(_highlightedItem >= 0)
        {
          _currentPos = _highlightedItem;
          scrollToHighlighted();
        }
        break;

      default:
        handled = false;
    }
  }

  return handled;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ListWidget::handleJoyDown(int stick, int button, bool longPress)
{
  if(longPress)
    sendCommand(Cmd::LongButtonPress, _selectedItem, _id);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ListWidget::handleJoyUp(int stick, int button)
{
  const Event::Type e = _boss->instance().eventHandler().eventForJoyButton(EventMode::kMenuMode, stick, button);

  handleEvent(e);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ListWidget::handleEvent(Event::Type e)
{
  if(!isEnabled() || _editMode)
    return false;

  bool handled = true;
  const int oldSelectedItem = _selectedItem;
  const int size = static_cast<int>(_list.size());

  switch(e)
  {
    case Event::UISelect:
      if(_selectedItem >= 0)
      {
        if(isEditable())
          startEditMode();
        else
          sendCommand(Cmd::Activated, _selectedItem, _id);
      }
      break;

    case Event::UIUp:
      if(_selectedItem > 0)
        _selectedItem--;
      break;

    case Event::UIDown:
      if(_selectedItem < size - 1)
        _selectedItem++;
      break;

    case Event::UIPgUp:
    case Event::UILeft:
      _selectedItem = std::max(_selectedItem - (_rows - 1), 0);
      break;

    case Event::UIPgDown:
    case Event::UIRight:
      _selectedItem = std::min(_selectedItem + (_rows - 1), size - 1);
      break;

    case Event::UIHome:
      _selectedItem = 0;
      break;

    case Event::UIEnd:
      _selectedItem = size - 1;
      break;

    case Event::UIPrevDir:
      sendCommand(Cmd::ParentDir, _selectedItem, _id);
      break;

    default:
      handled = false;
  }

  if(_selectedItem != oldSelectedItem)
  {
    if(_useScrollbar)
    {
      _scrollBar->draw();
      scrollToSelected();
    }

    sendCommand(Cmd::SelectionChanged, _selectedItem, _id);
  }

  return handled;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ListWidget::lostFocusWidget()
{
  _editMode = false;

  // Reset to normal data entry
  abortEditMode();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ListWidget::handleCommand(CommandSender* sender, GuiCmd::Code cmd,
                               int data, int id)
{
  if(cmd == GuiObject::Cmd::SetPosition)
  {
    if(_currentPos != data)
    {
      _currentPos = data;
      setDirty();

      // Let boss know the list has scrolled
      sendCommand(Cmd::Scrolled, _currentPos, _id);
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ListWidget::scrollToCurrent(int item)
{
  // Only do something if the current item is not in our view port
  if(item < _currentPos)
  {
    // it's above our view
    _currentPos = item;
  }
  else if(item >= _currentPos + _rows)
  {
    // it's below our view
    _currentPos = item - _rows + 1;
  }

  if(_currentPos < 0 || std::cmp_greater_equal(_rows, _list.size()))
    _currentPos = 0;
  else if(_currentPos + _rows > static_cast<int>(_list.size()))
    _currentPos = static_cast<int>(_list.size()) - _rows;

  if(_useScrollbar)
  {
    const int oldScrollPos = _scrollBar->currentPos();
    _scrollBar->setCurrentPos(_currentPos);
    _scrollBar->recalc();

    setDirty();

    if(oldScrollPos != _currentPos)
      sendCommand(Cmd::Scrolled, _currentPos, _id);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ListWidget::startEditMode()
{
  if(isEditable() && !_editMode && _selectedItem >= 0)
  {
    _editMode = true;
    setText(_list[_selectedItem]);

    // Widget gets raw data while editing
    EditableWidget::startEditMode();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ListWidget::endEditMode()
{
  if(!_editMode)
    return;

  // Send a message that editing finished with a return/enter key press
  _editMode = false;
  _list[_selectedItem] = editString();
  sendCommand(Cmd::DataChanged, _selectedItem, _id);

  // Reset to normal data entry
  EditableWidget::endEditMode();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ListWidget::abortEditMode()
{
  // Undo any changes made
  _editMode = false;

  // Reset to normal data entry
  EditableWidget::abortEditMode();
}
