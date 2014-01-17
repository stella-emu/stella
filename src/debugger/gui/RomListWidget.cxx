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
// Copyright (c) 1995-2014 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

#include "bspf.hxx"
#include "Debugger.hxx"
#include "DiStella.hxx"
#include "PackedBitArray.hxx"
#include "Widget.hxx"
#include "ScrollBarWidget.hxx"
#include "RomListSettings.hxx"
#include "RomListWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RomListWidget::RomListWidget(GuiObject* boss, const GUI::Font& lfont,
                             const GUI::Font& nfont,
                             int x, int y, int w, int h)
  : EditableWidget(boss, nfont, x, y, 16, 16),
    myMenu(NULL),
    _rows(0),
    _cols(0),
    _currentPos(0),
    _selectedItem(-1),
    _highlightedItem(-1),
    _editMode(false),
    _currentKeyDown(KBDK_UNKNOWN)
{
  _flags = WIDGET_ENABLED | WIDGET_CLEARBG | WIDGET_RETAIN_FOCUS;
  _bgcolor = kWidColor;
  _bgcolorhi = kWidColor;
  _textcolor = kTextColor;
  _textcolorhi = kTextColor;

  _cols = w / _fontWidth;
  _rows = h / _fontHeight;

  // Set real dimensions
  _w = w - kScrollBarWidth;
  _h = h + 2;

  // Create scrollbar and attach to the list
  myScrollBar = new ScrollBarWidget(boss, lfont, _x + _w, _y, kScrollBarWidth, _h);
  myScrollBar->setTarget(this);

  // Add settings menu
  myMenu = new RomListSettings(this, lfont);

  // Take advantage of a wide debugger window when possible
  const int fontWidth = lfont.getMaxCharWidth(),
            numchars = w / fontWidth;

  _labelWidth = BSPF_max(16, int(0.20 * (numchars - 12))) * fontWidth - 1;
  _bytesWidth = 12 * fontWidth;

  //////////////////////////////////////////////////////
  // Add checkboxes
  int ypos = _y + 2;

  // rowheight is determined by largest item on a line,
  // possibly meaning that number of rows will change
  _fontHeight = BSPF_max(_fontHeight, CheckboxWidget::boxSize());
  _rows = h / _fontHeight;

  // Create a CheckboxWidget for each row in the list
  CheckboxWidget* t;
  for(int i = 0; i < _rows; ++i)
  {
    t = new CheckboxWidget(boss, lfont, _x + 2, ypos, "", kCheckActionCmd);
    t->setTarget(this);
    t->setID(i);
    t->setFill(CheckboxWidget::Circle);
    t->setTextColor(kTextColorEm);
    ypos += _fontHeight;

    myCheckList.push_back(t);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RomListWidget::~RomListWidget()
{
  delete myMenu;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RomListWidget::setList(const CartDebug::Disassembly& disasm,
                            const PackedBitArray& state)
{
  myDisasm = &disasm;
  myBPState = &state;

  // Enable all checkboxes
  for(int i = 0; i < _rows; ++i)
    myCheckList[i]->setFlags(WIDGET_ENABLED);

  // Then turn off any extras
  if((int)myDisasm->list.size() < _rows)
    for(int i = myDisasm->list.size(); i < _rows; ++i)
      myCheckList[i]->clearFlags(WIDGET_ENABLED);

  recalc();

  setDirty(); draw();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RomListWidget::setSelected(int item)
{
  if(item < -1 || item >= (int)myDisasm->list.size())
    return;

  if(isEnabled())
  {
    if(_editMode)
      abortEditMode();

    _currentPos = _selectedItem = item;
    scrollToSelected();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RomListWidget::setHighlighted(int item)
{
  if(item < -1 || item >= (int)myDisasm->list.size())
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
const string& RomListWidget::getText() const
{
  if(_selectedItem < -1 || _selectedItem >= (int)myDisasm->list.size())
    return EmptyString;
  else
    return _editString;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int RomListWidget::findItem(int x, int y) const
{
  return (y - 1) / _fontHeight + _currentPos;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RomListWidget::recalc()
{
  int size = myDisasm->list.size();

  if (_currentPos >= size)
    _currentPos = size - 1;
  if (_currentPos < 0)
    _currentPos = 0;

  if(_selectedItem < 0 || _selectedItem >= size)
    _selectedItem = 0;

  _editMode = false;

  myScrollBar->_numEntries     = myDisasm->list.size();
  myScrollBar->_entriesPerPage = _rows;

  // Reset to normal data entry
  abortEditMode();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RomListWidget::scrollToCurrent(int item)
{
  // Only do something if the current item is not in our view port
  if (item < _currentPos)
  {
    // it's above our view
    _currentPos = item;
  }
  else if (item >= _currentPos + _rows )
  {
    // it's below our view
    _currentPos = item - _rows + 1;
  }

  if (_currentPos < 0 || _rows > (int)myDisasm->list.size())
    _currentPos = 0;
  else if (_currentPos + _rows > (int)myDisasm->list.size())
    _currentPos = myDisasm->list.size() - _rows;

  myScrollBar->_currentPos = _currentPos;
  myScrollBar->recalc();

  setDirty(); draw();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RomListWidget::handleMouseDown(int x, int y, int button, int clickCount)
{
  if (!isEnabled())
    return;

  // Grab right mouse button for context menu, left for selection/edit mode
  if(button == 2)
  {
    // Set selected and add menu at current x,y mouse location
    _selectedItem = findItem(x, y);
    scrollToSelected();
    myMenu->show(x + getAbsX(), y + getAbsY(), _selectedItem);
  }
  else
  {
    // First check whether the selection changed
    int newSelectedItem;
    newSelectedItem = findItem(x, y);
    if (newSelectedItem > (int)myDisasm->list.size() - 1)
      newSelectedItem = -1;

    if (_selectedItem != newSelectedItem)
    {
      if (_editMode)
        abortEditMode();
      _selectedItem = newSelectedItem;
      setDirty(); draw();
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RomListWidget::handleMouseUp(int x, int y, int button, int clickCount)
{
  // If this was a double click and the mouse is still over the selected item,
  // send the double click command
  if (clickCount == 2 && (_selectedItem == findItem(x, y)))
  {
    // Start edit mode
    if(_editable && !_editMode)
      startEditMode();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RomListWidget::handleMouseWheel(int x, int y, int direction)
{
  myScrollBar->handleMouseWheel(x, y, direction);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool RomListWidget::handleKeyDown(StellaKey key, StellaMod mod, char ascii)
{
  // Ignore all Alt-mod keys
  if(instance().eventHandler().kbdAlt(mod))
    return true;

  bool handled = true;
  int oldSelectedItem = _selectedItem;

  if (_editMode)
  {
    // Class EditableWidget handles all text editing related key presses for us
    handled = EditableWidget::handleKeyDown(key, mod, ascii);
  }
  else
  {
    // not editmode
    switch (key)
    {
      case KBDK_SPACE:
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

  if (_selectedItem != oldSelectedItem)
  {
    myScrollBar->draw();
    scrollToSelected();
  }

  _currentKeyDown = key;
  return handled;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool RomListWidget::handleKeyUp(StellaKey key, StellaMod mod, char ascii)
{
  if (key == _currentKeyDown)
    _currentKeyDown = KBDK_UNKNOWN;
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool RomListWidget::handleEvent(Event::Type e)
{
  if(!isEnabled() || _editMode)
    return false;

  bool handled = true;
  int oldSelectedItem = _selectedItem;

  switch(e)
  {
    case Event::UISelect:
      if (_selectedItem >= 0)
      {
        if (_editable)
          startEditMode();
      }
      break;

    case Event::UIUp:
      if (_selectedItem > 0)
        _selectedItem--;
      break;

    case Event::UIDown:
      if (_selectedItem < (int)myDisasm->list.size() - 1)
        _selectedItem++;
      break;

    case Event::UIPgUp:
      _selectedItem -= _rows - 1;
      if (_selectedItem < 0)
        _selectedItem = 0;
      break;

    case Event::UIPgDown:
      _selectedItem += _rows - 1;
      if (_selectedItem >= (int)myDisasm->list.size())
        _selectedItem = myDisasm->list.size() - 1;
      break;

    case Event::UIHome:
      _selectedItem = 0;
      break;

    case Event::UIEnd:
      _selectedItem = myDisasm->list.size() - 1;
      break;

    default:
      handled = false;
  }

  if (_selectedItem != oldSelectedItem)
  {
    myScrollBar->draw();
    scrollToSelected();
  }

  return handled;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RomListWidget::handleCommand(CommandSender* sender, int cmd, int data, int id)
{
  switch (cmd)
  {
    case kCheckActionCmd:
      // We let the parent class handle this
      // Pass it as a kRLBreakpointChangedCmd command, since that's the intent
      sendCommand(RomListWidget::kBPointChangedCmd, _currentPos+id,
                  myCheckList[id]->getState());
      break;

    case kSetPositionCmd:
      if (_currentPos != (int)data)
      {
        _currentPos = data;
        setDirty(); draw();
      }
      break;

    default:
      // Let the parent class handle all other commands directly
      sendCommand(cmd, data, id);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RomListWidget::lostFocusWidget()
{
  _editMode = false;

  // Reset to normal data entry
  abortEditMode();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RomListWidget::drawWidget(bool hilite)
{
//cerr << "RomListWidget::drawWidget\n";
  FBSurface& s = _boss->dialog().surface();
  const CartDebug::DisassemblyList& dlist = myDisasm->list;
  int i, pos, xpos, ypos, len = dlist.size();
  int deltax;

  const GUI::Rect& r = getEditRect();
  const GUI::Rect& l = getLineRect();

  // Draw a thin frame around the list and to separate columns
  s.hLine(_x, _y, _x + _w - 1, kColor);
  s.hLine(_x, _y + _h - 1, _x + _w - 1, kShadowColor);
  s.vLine(_x, _y, _y + _h - 1, kColor);
  s.vLine(_x + CheckboxWidget::boxSize() + 5, _y, _y + _h - 1, kColor);

  // Draw the list items
  int ccountw = _fontWidth << 2,
      large_disasmw = _w - l.x() - _labelWidth,
      medium_disasmw = large_disasmw - r.width(),
      small_disasmw = medium_disasmw - (ccountw << 1),
      actualwidth = myDisasm->fieldwidth * _fontWidth;
  if(actualwidth < small_disasmw)
    small_disasmw = actualwidth;

  xpos = _x + CheckboxWidget::boxSize() + 10;  ypos = _y + 2;
  for (i = 0, pos = _currentPos; i < _rows && pos < len; i++, pos++, ypos += _fontHeight)
  {
    // Draw checkboxes for correct lines (takes scrolling into account)
    myCheckList[i]->setState(myBPState->isSet(dlist[pos].address));
    myCheckList[i]->setDirty();
    myCheckList[i]->draw();

    // Draw highlighted item in a frame
    if (_highlightedItem == pos)
      s.frameRect(_x + l.x() - 3, ypos - 1, _w - l.x(), _fontHeight, kTextColorHi);

    // Draw the selected item inverted, on a highlighted background.
    if (_selectedItem == pos && _hasFocus)
    {
      if (!_editMode)
        s.fillRect(_x + r.x() - 3, ypos - 1, r.width(), _fontHeight, kTextColorHi);
      else
        s.frameRect(_x + r.x() - 3, ypos - 1, r.width(), _fontHeight, kTextColorHi);
    }

    // Draw labels
    s.drawString(_font, dlist[pos].label, xpos, ypos, _labelWidth,
                 dlist[pos].hllabel ? kTextColor : kColor);

    // Bytes are only editable if they represent code, graphics, or accessible data
    // Otherwise, the disassembly should get all remaining space
    if(dlist[pos].type & (CartDebug::CODE|CartDebug::GFX|CartDebug::PGFX|CartDebug::DATA))
    {
      if(dlist[pos].type == CartDebug::CODE)
      {
        // Draw disassembly and cycle count
        s.drawString(_font, dlist[pos].disasm, xpos + _labelWidth, ypos,
                     small_disasmw, kTextColor);
        s.drawString(_font, dlist[pos].ccount, xpos + _labelWidth + small_disasmw, ypos,
                     ccountw, kTextColor);
      }
      else
      {
        // Draw disassembly only
        s.drawString(_font, dlist[pos].disasm, xpos + _labelWidth, ypos,
                     medium_disasmw, kTextColor);
      }

      // Draw separator
      s.vLine(_x + r.x() - 7, ypos, ypos + _fontHeight - 1, kColor);

      // Draw bytes
      {
        if (_selectedItem == pos && _editMode)
        {
          adjustOffset();
          deltax = -_editScrollOffset;

          s.drawString(_font, _editString, _x + r.x(), ypos, r.width(), kTextColor,
                       kTextAlignLeft, deltax, false);

          drawCaret();
        }
        else
        {
          deltax = 0;
          s.drawString(_font, dlist[pos].bytes, _x + r.x(), ypos, r.width(), kTextColor);
        }
      }
    }
    else
    {
      // Draw disassembly, giving it all remaining horizontal space
      s.drawString(_font, dlist[pos].disasm, xpos + _labelWidth, ypos,
                   large_disasmw, kTextColor);
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
GUI::Rect RomListWidget::getLineRect() const
{
  GUI::Rect r(2, 1, _w, _fontHeight);
  const int yoffset = (_selectedItem - _currentPos) * _fontHeight,
            xoffset = CheckboxWidget::boxSize() + 10;
  r.top    += yoffset;
  r.bottom += yoffset;
  r.left   += xoffset;
  r.right  -= xoffset - 15;
	
  return r;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
GUI::Rect RomListWidget::getEditRect() const
{
  GUI::Rect r(2, 1, _w, _fontHeight);
  const int yoffset = (_selectedItem - _currentPos) * _fontHeight;
  r.top    += yoffset;
  r.bottom += yoffset;
  r.left   += _w - _bytesWidth;
  r.right   = _w;
	
  return r;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool RomListWidget::tryInsertChar(char c, int pos)
{
  // Not sure how efficient this is, or should we even care?
  bool insert = false;
  c = tolower(c);
  switch(_base)
  {
    case Common::Base::F_16:
    case Common::Base::F_16_1:
    case Common::Base::F_16_2:
    case Common::Base::F_16_4:
    case Common::Base::F_16_8:
      insert = (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || c == ' ';
      break;
    case Common::Base::F_2:
    case Common::Base::F_2_8:
    case Common::Base::F_2_16:
      insert = c == '0' || c == '1' || c == ' ';
      break;
    case Common::Base::F_10:
      if((c >= '0' && c <= '9') || c == ' ')
        insert = true;
      break;
    case Common::Base::F_DEFAULT:
      break;
  }

  if(insert)
    _editString.insert(pos, 1, c);

  return insert;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RomListWidget::startEditMode()
{
  if (_editable && !_editMode && _selectedItem >= 0)
  {
    // Does this line represent an editable area?
    if(myDisasm->list[_selectedItem].bytes == "")
      return;

    _editMode = true;
    switch(myDisasm->list[_selectedItem].type)
    {
      case CartDebug::GFX:
      case CartDebug::PGFX:
        _base = DiStella::settings.gfx_format;
        break;
      default:
        _base = Common::Base::format();
    }

    // Widget gets raw data while editing
    EditableWidget::startEditMode();
    setText(myDisasm->list[_selectedItem].bytes);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RomListWidget::endEditMode()
{
  if (!_editMode)
    return;

  // Send a message that editing finished with a return/enter key press
  // The parent then calls getText() to get the newly entered data
  _editMode = false;
  sendCommand(RomListWidget::kRomChangedCmd, _selectedItem, _base);

  // Reset to normal data entry
  EditableWidget::endEditMode();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RomListWidget::abortEditMode()
{
  // Undo any changes made
  _editMode = false;

  // Reset to normal data entry
  EditableWidget::abortEditMode();
}
