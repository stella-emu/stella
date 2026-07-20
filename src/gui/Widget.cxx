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
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#include "bspf.hxx"
#include "Command.hxx"
#include "Dialog.hxx"
#include "ToolTip.hxx"
#include "FBSurface.hxx"
#include "GuiObject.hxx"
#include "OSystem.hxx"
#include "EventHandler.hxx"

#include "Widget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Widget::Widget(GuiObject* boss, const GUI::Font& font,
               int x, int y, int w, int h)
  : GuiObject(boss->instance(), boss->parent(), boss->dialog(), x, y, w, h),
    _boss{boss},
    _font{font},
    _fontWidth{_font.getMaxCharWidth()},
    _fontHeight{_font.getFontHeight()},
    _lineHeight{_font.getLineHeight()}
{
  // Insert into the widget list of the boss, which owns this widget from
  // here on; everything else holds non-owning aliases
  _boss->_children.emplace_back(this);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Widget::setDirty()
{
  _dirty = true;

  // Inform the parent object that its children chain is dirty
  _boss->setDirtyChain();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Widget::setDirtyChain()
{
  _dirtyChain = true;

  // Inform the parent object that its children chain is dirty
  _boss->setDirtyChain();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Widget::tick()
{
  if(isEnabled())
  {
    if(wantsToolTip())
      dialog().tooltip().request();

    // Recursively tick all child widgets
    for(const auto& w: _children)
      w->tick();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Widget::draw()
{
  if(!isVisible() || !_boss->isVisible())
    return;

  if(isDirty())
  {
  #ifdef DEBUG_BUILD
    //cerr << "  *** draw widget " << typeid(*this).name() << " ***" << '\n';
    cerr << "w";
  #endif

    FBSurface& s = _boss->dialog().surface();
    const int oldX = _x, oldY = _y;

    // Account for our relative position in the dialog
    _x = getAbsX();
    _y = getAbsY();

    // Clear background (unless alpha blending is enabled)
    if(clearsBackground())
    {
      int x = _x, y = _y, w = _w, h = _h;
      if(hasBorder())
      {
        x++; y++; w -= 2; h -= 2;
      }
      if(hasBackground())
        s.fillRect(x, y, w, h, (_flags & Widget::FLAG_HILITED) && isEnabled()
                   ? _bgcolorhi : _bgcolor);
      else
        s.invalidateRect(x, y, w, h);
    }

    // Draw border
    if(hasBorder())
    {
      s.frameRect(_x, _y, _w, _h, (_flags & Widget::FLAG_HILITED) && isEnabled()
                  ? kWidColorHi : kColor);
      _x += 4;
      _y += 4;
      _w -= 8;
      _h -= 8;
    }

    // Now perform the actual widget draw
    drawWidget((_flags & Widget::FLAG_HILITED) != 0);

    // Restore w/hy
    if(hasBorder())
    {
      _w += 8;
      _h += 8;
    }

    _x = oldX;
    _y = oldY;
  }
  clearDirty();

  // Draw all children
  drawChain();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Widget::drawChain()
{
  // Clear chain *before* drawing, because some widgets may set it again when
  //   being drawn (e.g. RomListWidget)
  clearDirtyChain();

  for(const auto& w: _children)
    if(w->needsRedraw())
      w->draw();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Widget::setPosX(int x)
{
  setPos(x, _y);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Widget::setPosY(int y)
{
  setPos(_x, y);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Widget::setPos(int x, int y)
{
  setPos(Common::Point(x, y));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Widget::setPos(const Common::Point& pos)
{
  if(pos != Common::Point(_x, _y))
  {
    _x = pos.x;
    _y = pos.y;
    // we have to redraw the whole dialog!
    dialog().setDirty();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Widget::setWidth(int w)
{
  setSize(w, _h);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Widget::setHeight(int h)
{
  setSize(_w, h);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Widget::setSize(int w, int h)
{
  setSize(Common::Point(w, h));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Widget::setSize(const Common::Point& pos)
{
  if(pos != Common::Point(_w, _h))
  {
    _w = pos.x;
    _h = pos.y;
    // we have to redraw the whole dialog!
    dialog().setDirty();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Widget::setArea(int x, int y, int w, int h)
{
  setPos(x, y);
  setWidth(w);
  setHeight(h);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Widget::handleMouseEntered()
{
  if(isEnabled())
    setFlags(Widget::FLAG_HILITED | Widget::FLAG_MOUSE_FOCUS);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Widget::handleMouseLeft()
{
  if(isEnabled())
    clearFlags(Widget::FLAG_HILITED | Widget::FLAG_MOUSE_FOCUS);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Widget::receivedFocus()
{
  if(_hasFocus)
    return;

  _hasFocus = true;
  setFlags(Widget::FLAG_HILITED);
  receivedFocusWidget();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Widget::lostFocus()
{
  if(!_hasFocus)
    return;

  _hasFocus = false;
  clearFlags(Widget::FLAG_HILITED);
  lostFocusWidget();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Widget::setEnabled(bool e)
{
  if(e) setFlags(Widget::FLAG_ENABLED);
  else  clearFlags(Widget::FLAG_ENABLED);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Widget::setToolTip(string_view text, Event::Type event1, EventMode mode)
{
  setToolTip(text, event1, Event::Type::NoType, mode);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Widget::setToolTip(string_view text, Event::Type event1, Event::Type event2, EventMode mode)
{
  assert(text.length() <= ToolTip::MAX_LEN);

  _toolTipText = text;
  setToolTip(event1, event2, mode);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Widget::setToolTip(Event::Type event1, EventMode mode)
{
  setToolTip(event1, Event::Type::NoType, mode);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Widget::setToolTip(Event::Type event1, Event::Type event2, EventMode mode)
{
  _toolTipEvent1 = event1;
  _toolTipEvent2 = event2;
  _toolTipMode = mode;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string Widget::getToolTip(const Common::Point& pos) const
{
  string hotkey = instance().eventHandler().keyHandler().getMappingDesc(
    _toolTipEvent1, _toolTipMode);
  const string hotkey2 = instance().eventHandler().keyHandler().getMappingDesc(
    _toolTipEvent2, _toolTipMode);

  if(!hotkey.empty() && !hotkey2.empty())
  {
    // Merge hotkeys if they only differ by "-Shift"
    const string mod = "-Shift";
    const size_t p = BSPF::findIgnoreCase(hotkey, mod);

    if(p != string::npos)
    {
      const string testKey = hotkey.substr(0, p) +
                             hotkey.substr(p + string(mod).length());
      if(testKey == hotkey2)
        hotkey = hotkey.substr(0, p) + "[" + mod + "]" +
                 hotkey.substr(p + string(mod).length());
      else
        hotkey += ", " + hotkey2;
    }
    else
      hotkey += ", " + hotkey2;
  }
  else
    hotkey += hotkey2;

  if(hotkey.empty())
    return _toolTipText;
  else if(_toolTipText.empty())
    return "(" + hotkey + ")";
  else
    if(_toolTipText.length() + hotkey.length() + 3 <= ToolTip::MAX_COLUMNS)
      return _toolTipText + " (" + hotkey + ")";
    else
      return _toolTipText + "\n(" + hotkey + ")";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Widget::hasToolTip() const
{
  return !_toolTipText.empty() || _toolTipEvent1 != Event::Type::NoType;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Widget::setHelpAnchor(string_view helpAnchor, bool debugger)
{
  _helpAnchor = helpAnchor;
  _debuggerHelp = debugger;

  dialog().initHelp();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Widget::setHelpURL(string_view helpURL)
{
  _helpURL = helpURL;

  dialog().initHelp();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string Widget::getHelpURL() const
{
  if(!_helpURL.empty())
    return _helpURL;

  if(!_helpAnchor.empty())
  {
    if(_debuggerHelp)
      return "https://stella-emu.github.io/docs/debugger.html#" + _helpAnchor;
    else
      return "https://stella-emu.github.io/docs/index.html#" + _helpAnchor;
  }
  return {};
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Widget* Widget::findWidgetInList(const WidgetList& list, int x, int y)
{
  // Search newest-first, so where widgets overlap the one added last wins
  for(const auto& w: std::views::reverse(list))
  {
    // Stop as soon as we find a widget that contains the point (x,y)
    if(x >= w->_x && x < w->_x + w->_w &&
       y >= w->_y && y < w->_y + w->_h)
      return w->findWidget(x - w->_x, y - w->_y);
  }
  return nullptr;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Widget::isWidgetInList(const WidgetArray& list, Widget* find)
{
  return BSPF::contains(list, find);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Widget* Widget::setFocusForList(const GuiObject* boss, WidgetArray& arr,
                                const Widget* wid, int direction,
                                bool emitFocusEvents)
{
  FBSurface& s = boss->dialog().surface();
  const int size = static_cast<int>(arr.size());
  int pos = -1;
  Widget* tmp = nullptr;

  for(int i = 0; i < size; ++i)
  {
    tmp = arr[i];

    // Determine position of widget 'w'
    if(wid == tmp)
      pos = i;

    // Get area around widget
    // Note: we must use getXXX() methods and not access the variables
    // directly, since in some cases (notably those widgets with embedded
    // ScrollBars) the two quantities may be different
    const int x = tmp->getAbsX() - 1,  y = tmp->getAbsY() - 1,
              w = tmp->getWidth() + 2, h = tmp->getHeight() + 2;

    // First clear area surrounding all widgets
    if(tmp->_hasFocus)
    {
      if(emitFocusEvents)
        tmp->lostFocus();
      else
        tmp->_hasFocus = false;

      s.frameRect(x, y, w, h, kDlgColor);
    }
  }

  // Figure out which which should be active
  if(pos == -1)
    return nullptr;
  else
  {
    const int oldPos = pos;
    do
    {
      switch(direction)
      {
        case -1:  // previous widget
          pos--;
          if(pos < 0)
            pos = size - 1;
          break;

        case +1:  // next widget
          pos++;
          if(pos >= size)
            pos = 0;
          break;

        default:
          // pos already set
          break;
      }
      // break if all widgets should be disabled
      if(oldPos == pos)
        break;
    } while(!arr[pos]->isEnabled());
  }

  // Now highlight the active widget
  tmp = arr[pos];

  // Get area around widget
  // Note: we must use getXXX() methods and not access the variables
  // directly, since in some cases (notably those widgets with embedded
  // ScrollBars) the two quantities may be different
  const int x = tmp->getAbsX() - 1,  y = tmp->getAbsY() - 1,
            w = tmp->getWidth() + 2, h = tmp->getHeight() + 2;

  if(emitFocusEvents)
    tmp->receivedFocus();
  else {
    tmp->_hasFocus = true;
    tmp->setFlags(Widget::FLAG_HILITED);
  }

  s.frameRect(x, y, w, h, kWidFrameColor, FrameStyle::Dashed);

  return tmp;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Widget::setDirtyInList(const WidgetList& list)
{
  for(const auto& w: list)
    w->setDirty();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Widget::refreshFontMetrics()
{
  _fontWidth = _font.getMaxCharWidth();
  _fontHeight = _font.getFontHeight();
  _lineHeight = _font.getLineHeight();
  setDirty();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Widget::refreshFontMetricsInList(const WidgetList& list)
{
  for(const auto& w: list)
  {
    w->refreshFontMetrics();
    // Composite widgets parent their children to themselves, forming separate
    // child lists that the boss-level walk does not reach; recurse into them
    refreshFontMetricsInList(w->_children);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
StaticTextWidget::StaticTextWidget(GuiObject* boss, const GUI::Font& font,
                                   int w, int h,
                                   string_view text, TextAlign align,
                                   ColorId shadowColor)
  : Widget(boss, font, 0, 0, w, h),
    CommandSender(boss),
    _label{text},
    _align{align}
{
  _flags = Widget::FLAG_ENABLED | FLAG_CLEARBG;

  _bgcolor = kDlgColor;
  _bgcolorhi = kDlgColor;
  _textcolor = kTextColor;
  _textcolorhi = kTextColor;
  _shadowcolor = shadowColor;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
StaticTextWidget::StaticTextWidget(GuiObject* boss, const GUI::Font& font,
                                   
                                   string_view text, TextAlign align,
                                   ColorId shadowColor)
  : StaticTextWidget(boss, font, font.getStringWidth(text),
                     font.getLineHeight(), text, align, shadowColor)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StaticTextWidget::refreshFontMetrics()
{
  Widget::refreshFontMetrics();

  // Recompute the natural (font + label derived) size, matching the short ctor.
  // Runs only during a live font-change broadcast; the owning dialog's layout()
  // re-runs immediately after and overrides this where it sets an explicit size.
  _w = _font.getStringWidth(_label);
  _h = _font.getLineHeight();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StaticTextWidget::setValue(int value)
{
  setLabel(std::to_string(value));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StaticTextWidget::setLabel(string_view label)
{
  if(_label != label)
  {
    _label = label;
    if(_autoResize)
      _w = _font.getStringWidth(label);
    setDirty();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StaticTextWidget::setLink(size_t start, int len, bool underline)
{
  if(_linkStart != start || _linkLen != len || _linkUnderline != underline)
  {
    _linkStart = start;
    _linkLen = len;
    _linkUnderline = underline;
    setCmd(len ? kClickedCmd : 0);
    setDirty();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool StaticTextWidget::setUrl(string_view url, string_view label,
                              string_view placeHolder)
{
  size_t start = string::npos, len = 0;
  const string_view text = !label.empty() ? label : url;

  if(!text.empty())
  {
    if(!placeHolder.empty())
      BSPF::replaceAll(_label, placeHolder, text);

    // determine position of link
    if(start = BSPF::findIgnoreCase(_label, text); start != string::npos)
    {
      len = text.size();
      _url = url;
    }
  }
  else
  {
    // extract URL from _label
    start = BSPF::findIgnoreCase(_label, "http://");

    if(start == string::npos)
      start = BSPF::findIgnoreCase(_label, "https://");
    if(start == string::npos)
      start = BSPF::findIgnoreCase(_label, "www.");


    if(start != string::npos)
    {
      // find end of URL
      for(size_t i = start; i < _label.size(); ++i)
      {
        const char ch = _label[i];

        if(ch == ' ' || ch == ')' || ch == '>')
        {
          len = i - start;
          _url = _label.substr(start, len);
          break;
        }
      }
      if(!len)
      {
        len = _label.size() - start;
        _url = _label.substr(start);
      }
    }
  }

  if(len)
  {
    setLink(start, static_cast<int>(len), true);
    setCmd(kOpenUrlCmd);
    return true;
  }
  else
  {
    setLink(); // clear link
    _url.clear();
    return false;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StaticTextWidget::handleMouseEntered()
{
  if(isEnabled())
    setFlags(Widget::FLAG_HILITED | Widget::FLAG_MOUSE_FOCUS, _linkLen);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StaticTextWidget::handleMouseLeft()
{
  if(isEnabled())
    clearFlags(Widget::FLAG_HILITED | Widget::FLAG_MOUSE_FOCUS, _linkLen);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StaticTextWidget::handleMouseUp(int x, int y, MouseButton b, int clickCount)
{
  if(_cmd && isEnabled() && x >= 0 && x < _w && y >= 0 && y < _h)
  {
    clearFlags(Widget::FLAG_HILITED);
    sendCommand(_cmd, 0, _id);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StaticTextWidget::drawWidget(bool hilite)
{
  FBSurface& s = _boss->dialog().surface();

  s.drawString(_font, _label, _x, _y + firstTextY(), _w,
                isEnabled() ? _textcolor : kColor, _align, 0, true,
                _shadowcolor, _linkStart, _linkLen, _linkUnderline && hilite);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ButtonWidget::ButtonWidget(GuiObject* boss, const GUI::Font& font,
                           int w, int h,
                           string_view label, int cmd, bool repeat)
  : StaticTextWidget(boss, font, w, h, label, TextAlign::Center),
    _repeat{repeat}
{
  _cmd = cmd;
  _flags = Widget::FLAG_ENABLED | Widget::FLAG_CLEARBG;
  _bgcolor = kBtnColor;
  _bgcolorhi = kBtnColorHi;
  _bgcolorlo = kColor;
  _textcolor = kBtnTextColor;
  _textcolorhi = kBtnTextColorHi;
  _textcolorlo = kBGColorLo;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ButtonWidget::ButtonWidget(GuiObject* boss, const GUI::Font& font,
                           int w,
                           string_view label, int cmd, bool repeat)
  : ButtonWidget(boss, font, w, calcHeight(font), label, cmd, repeat)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ButtonWidget::ButtonWidget(GuiObject* boss, const GUI::Font& font,
                           
                           string_view label, int cmd, bool repeat)
  : ButtonWidget(boss, font, calcWidth(font, label), calcHeight(font),
                 label, cmd, repeat)
{
  _autoSize = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ButtonWidget::ButtonWidget(GuiObject* boss, const GUI::Font& font,
                           int w, int h,
                           const uInt32* bitmap, int bmw, int bmh,
                           int cmd, bool repeat)
  : ButtonWidget(boss, font, w, h, "", cmd, repeat)
{
  _useBitmap = true;
  _useText = false;
  _bitmap = bitmap;
  _bmw = bmw;
  _bmh = bmh;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ButtonWidget::ButtonWidget(GuiObject* boss, const GUI::Font& font,
                           
                           const GUI::Icon& icon,
                           int cmd, bool repeat)
  : ButtonWidget(boss, font, icon.width() + iconGap(font), calcHeight(font),
                 icon.bitmap(), icon.width(), icon.height(), cmd, repeat)
{
  _autoSize = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ButtonWidget::ButtonWidget(GuiObject* boss, const GUI::Font& font,
                           
                           const GUI::Icon& icon, string_view label,
                           int cmd, bool repeat)
  : ButtonWidget(boss, font,
                 icon.width() + iconGap(font) * 1.5 + font.getStringWidth(label),
                 calcHeight(font), label, cmd, repeat)
{
  _useBitmap = true;
  _bitmap = icon.bitmap();
  _bmw = icon.width();
  _bmh = icon.height();
  _bmx = iconGap(font);
  _align = TextAlign::Left;
  _autoSize = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ButtonWidget::refreshFontMetrics()
{
  // Deliberately skips StaticTextWidget's recompute, which would shrink a button
  // to its bare label width (and would be inherited by the checkbox, radio button
  // and slider below).
  // NOLINTNEXTLINE(bugprone-parent-virtual-call)
  Widget::refreshFontMetrics();

  // A button that sized itself from its content re-derives that size, so it
  // follows the font on its own.  Any other size came from outside — a width
  // shared with its neighbours — and is re-applied by the owning layout().  An
  // icon-and-label button keeps whichever icon it holds; a dialog that swaps in a
  // different variant for the new font does so with setIcon(), which re-sizes it
  if(_autoSize)
  {
    _w = autoWidth();
    _h = autoHeight();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ButtonWidget::handleMouseEntered()
{
  if(isEnabled())
    setFlags(Widget::FLAG_HILITED | Widget::FLAG_MOUSE_FOCUS);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ButtonWidget::handleMouseLeft()
{
  if(isEnabled())
    clearFlags(Widget::FLAG_HILITED | Widget::FLAG_MOUSE_FOCUS);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ButtonWidget::handleEvent(Event::Type e)
{
  if(!isEnabled() || e != Event::UISelect)
    return false;

  // Simulate mouse event
  handleMouseUp(0, 0, MouseButton::LEFT, 0);
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ButtonWidget::handleMouseClicks(int x, int y, MouseButton b)
{
  return _repeat;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ButtonWidget::handleMouseDown(int x, int y, MouseButton b, int clickCount)
{
  if(_repeat && isEnabled() && x >= 0 && x < _w && y >= 0 && y < _h)
  {
    clearFlags(Widget::FLAG_HILITED);
    sendCommand(_cmd, 0, _id);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ButtonWidget::handleMouseUp(int x, int y, MouseButton b, int clickCount)
{
  if(!_repeat && isEnabled() && x >= 0 && x < _w && y >= 0 && y < _h)
  {
    clearFlags(Widget::FLAG_HILITED);
    sendCommand(_cmd, 0, _id);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ButtonWidget::setBitmap(const uInt32* bitmap, int bmw, int bmh)
{
  _useBitmap = true;
  _bitmap = bitmap;
  _bmh = bmh;
  _bmw = bmw;

  setDirty();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ButtonWidget::setIcon(const GUI::Icon& icon)
{
  setBitmap(icon.bitmap(), icon.width(), icon.height());

  // A button that sized itself around its icon re-sizes around the new one; one
  // whose width came from outside keeps it (the layout re-applies that)
  if(_autoSize)
    setWidth(autoWidth());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ButtonWidget::drawWidget(bool hilite)
{
  FBSurface& s = _boss->dialog().surface();

  s.frameRect(_x, _y, _w, _h, hilite && isEnabled() ? kBtnBorderColorHi : kBtnBorderColor);

  int x = _x;
  if(_useBitmap)
  {
    const int xb = _useText ? _x + _bmx / 2 : _x + (_w - _bmw) / 2;
    s.drawBitmap(_bitmap, xb, _y + (_h - _bmh) / 2,
                 !isEnabled() ? _textcolorlo :
                 hilite ? _textcolorhi : _textcolor,
                 _bmw, _bmh);
    x = _x + _bmw + _bmx;
  }
  if(_useText)
    s.drawString(_font, _label, x, _y + firstTextY(), _w,
                 !isEnabled() ? _textcolorlo :
                 hilite ? _textcolorhi : _textcolor, _align);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CheckboxWidget::CheckboxWidget(GuiObject* boss, const GUI::Font& font,
                               string_view label,
                               int cmd)
  : ButtonWidget(boss, font, font.isLarge() ? 24 : 16,
                 font.isLarge() ? 24 : 16, label, cmd),
    _boxSize{boxSize(font)}
{
  _flags = Widget::FLAG_ENABLED;
  _bgcolor = _bgcolorhi = kWidColor;
  _bgcolorlo = kDlgColor;

  _editable = true;

  if(label.empty())
    _w = _boxSize;
  else
    _w = font.getStringWidth(label) + _boxSize + font.getMaxCharWidth() * 0.75;
  alignBox(_boxSize);

  setFill(CheckboxWidget::FillType::Normal);  // NOLINT(clang-analyzer-optin.cplusplus.VirtualCall)
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CheckboxWidget::alignBox(int boxSize)
{
  // The label is centered in the height, as any other control centers its text
  // (firstTextY()), and the box is centered on the label.  With a small enough
  // font the box is the taller of the two, and it sets the height instead
  _h = std::max(_lineHeight, boxSize);
  _boxY = (_h - boxSize) / 2;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CheckboxWidget::refreshFontMetrics()
{
  ButtonWidget::refreshFontMetrics();

  // Recompute the box + label geometry from the live font (mirrors the ctor).
  // A checkbox is fully font + label derived, so this is the complete size.
  _boxSize = boxSize(_font);

  if(_label.empty())
    _w = _boxSize;
  else
    _w = _font.getStringWidth(_label) + _boxSize + _font.getMaxCharWidth() * 0.75;
  alignBox(_boxSize);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CheckboxWidget::handleMouseUp(int x, int y, MouseButton b, int clickCount)
{
  if(isEnabled() && _editable && x >= 0 && x < _w && y >= 0 && y < _h)
  {
    toggleState();

    // We only send a command when the widget has been changed interactively
    sendCommand(_cmd, _state, _id);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CheckboxWidget::setEditable(bool editable)
{
  _editable = editable;
  if(_editable)
  {
    _bgcolor = kWidColor;
  }
  else
  {
    _bgcolor = kBGColorHi;
    setFill(CheckboxWidget::FillType::Inactive);
  }
  setDirty();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CheckboxWidget::setFill(FillType type)
{
  /* 10x10 checkbox bitmap */
  // small versions
  static constexpr std::array<uInt32, 10> checked_img_active = {
    0b1111111111,  0b1111111111,  0b1111111111,  0b1111111111,  0b1111111111,
    0b1111111111,  0b1111111111,  0b1111111111,  0b1111111111,  0b1111111111
  };

  static constexpr std::array<uInt32, 10> checked_img_inactive = {
    0b1111111111,  0b1111111111,  0b1111001111,  0b1110000111,  0b1100000011,
    0b1100000011,  0b1110000111,  0b1111001111,  0b1111111111,  0b1111111111
  };

  static constexpr std::array<uInt32, 10> checked_img_circle = {
    0b0001111000,  0b0111111110,  0b0111111110,  0b1111111111,  0b1111111111,
    0b1111111111,  0b1111111111,  0b0111111110,  0b0111111110,  0b0001111000
  };

  /* 18x18 checkbox bitmap */
  // large versions
  static constexpr std::array<uInt32, 18> checked_img_active_large = {
    0b111111111111111111,  0b111111111111111111,  0b111111111111111111,  0b111111111111111111,
    0b111111111111111111,  0b111111111111111111,  0b111111111111111111,  0b111111111111111111,
    0b111111111111111111,  0b111111111111111111,  0b111111111111111111,  0b111111111111111111,
    0b111111111111111111,  0b111111111111111111,  0b111111111111111111,  0b111111111111111111,
    0b111111111111111111,  0b111111111111111111
  };

  static constexpr std::array<uInt32, 18> checked_img_inactive_large = {
    0b111111111111111111, 0b111111111111111111, 0b111111111111111111,
    0b111111110011111111, 0b111111100001111111, 0b111111000000111111, 0b111110000000011111,
    0b111100000000001111, 0b111000000000000111, 0b111000000000000111, 0b111100000000001111,
    0b111110000000011111, 0b111111000000111111, 0b111111100001111111, 0b111111110011111111,
    0b111111111111111111, 0b111111111111111111, 0b111111111111111111
  };

  switch(type)
  {
    case CheckboxWidget::FillType::Normal:
      _img = _boxSize == 14 ? checked_img_active.data() : checked_img_active_large.data();
      _drawBox = true;
      break;
    case CheckboxWidget::FillType::Inactive:
      _img = _boxSize == 14 ? checked_img_inactive.data() : checked_img_inactive_large.data();
      _drawBox = true;
      break;
    case CheckboxWidget::FillType::Circle:
      // only used in debugger which only has smaller fonts
      _img = checked_img_circle.data();
      _drawBox = false;
      break;
    default:
      break;  // Not supposed to get here
  }
  setDirty();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CheckboxWidget::setState(bool state, bool changed)
{
  if(_state != state || _changed != changed)
  {
    setDirty();

    _state = state;
    _changed = changed;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CheckboxWidget::drawWidget(bool hilite)
{
  FBSurface& s = _boss->dialog().surface();

  if(_drawBox)
    s.frameRect(_x, _y + _boxY, _boxSize, _boxSize, hilite && isEnabled() && isEditable() ? kWidColorHi : kColor);
  // Do we draw a square or cross?
  s.fillRect(_x + 1, _y + _boxY + 1, _boxSize - 2, _boxSize - 2,
      _changed ? kDbgChangedColor : isEnabled() ? _bgcolor : kDlgColor);
  if(_state)
    s.drawBitmap(_img, _x + 2, _y + _boxY + 2, isEnabled() ? hilite && isEditable() ? kWidColorHi : kCheckColor
                 : kColor, _boxSize - 4);

  // Finally draw the label
  s.drawString(_font, _label, _x + prefixSize(_font), _y + firstTextY(), _w,
               isEnabled() ? kTextColor : kColor);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SliderWidget::SliderWidget(GuiObject* boss, const GUI::Font& font,
                           int w, int h,
                           string_view label, int labelWidth, int cmd,
                           int valueLabelWidth, string_view valueUnit, int valueLabelGap,
                           bool forceLabelSign)
  : ButtonWidget(boss, font, w, h, label, cmd),
    _labelWidth{labelWidth},
    _valueUnit{valueUnit},
    _valueLabelGap{valueLabelGap},
    _valueLabelWidth{valueLabelWidth},
    _forceLabelSign{forceLabelSign}
{
  _flags = Widget::FLAG_ENABLED | Widget::FLAG_TRACK_MOUSE | Widget::FLAG_CLEARBG;
  _bgcolor = kDlgColor;
  _bgcolorhi = kDlgColor;

  if(!_label.empty() && _labelWidth == 0)
    _labelWidth = _font.getStringWidth(_label);

  if(_valueLabelWidth == 0)
    _valueLabelGap = 0;
  if(_valueLabelGap == 0)
    _valueLabelGap = font.getMaxCharWidth() / 2;

  _w = w + _labelWidth + _valueLabelGap + _valueLabelWidth;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SliderWidget::SliderWidget(GuiObject* boss, const GUI::Font& font,
                           int w,
                           string_view label, int labelWidth, int cmd,
                           int valueLabelWidth, string_view valueUnit, int valueLabelGap,
                           bool forceLabelSign)
  : SliderWidget(boss, font, w, font.getLineHeight(),
                 label, labelWidth, cmd, valueLabelWidth, valueUnit, valueLabelGap,
                 forceLabelSign)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SliderWidget::SliderWidget(GuiObject* boss, const GUI::Font& font,
                           
                           string_view label, int labelWidth, int cmd,
                           int valueLabelWidth, string_view valueUnit, int valueLabelGap,
                           bool forceLabelSign)
  : SliderWidget(boss, font, font.getMaxCharWidth() * 10,
                 label, labelWidth, cmd, valueLabelWidth, valueUnit, valueLabelGap,
                 forceLabelSign)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SliderWidget::setValue(int value)
{
  if(value < _valueMin)      value = _valueMin;
  else if(value > _valueMax) value = _valueMax;

  if(value != _value)
  {
    _value = value;
    setDirty();
    if(_valueLabelWidth)
      setValueLabel(_value); // update label
    sendCommand(_cmd, _value, _id);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SliderWidget::setMinValue(int value)
{
  _valueMin = value;
  setDirty();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SliderWidget::setMaxValue(int value)
{
  _valueMax = value;
  setDirty();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SliderWidget::setStepValue(int value)
{
  _stepValue = value;
  setDirty();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SliderWidget::setValueLabel(string_view valueLabel)
{
  _valueLabel = valueLabel;
  setDirty();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SliderWidget::setValueLabel(int value)
{
  _valueLabel = (_forceLabelSign && value > 0 ? "+" : "") + std::to_string(value);
  setDirty();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SliderWidget::setValueUnit(string_view valueUnit)
{
  _valueUnit = valueUnit;
  setDirty();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SliderWidget::setTickmarkIntervals(int numIntervals)
{
  _numIntervals = numIntervals;
  setDirty();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SliderWidget::handleMouseMoved(int x, int y)
{
  // TODO: when the mouse is dragged outside the widget, the slider should
  // snap back to the old value.
  if(isEnabled() && _isDragging &&
     x >= (_labelWidth - 4) &&
     x <= (_w - _valueLabelGap - _valueLabelWidth + 4))
    setValue(posToValue(x - _labelWidth));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SliderWidget::handleMouseDown(int x, int y, MouseButton b, int clickCount)
{
  if(isEnabled() && b == MouseButton::LEFT)
  {
    _isDragging = true;
    handleMouseMoved(x, y);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SliderWidget::handleMouseUp(int x, int y, MouseButton b, int clickCount)
{
  if(isEnabled() && _isDragging)
    sendCommand(_cmd, _value, _id);

  _isDragging = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SliderWidget::handleMouseWheel(int x, int y, int direction)
{
  if(isEnabled())
  {
    if(direction < 0)
      handleEvent(Event::UIUp);
    else if(direction > 0)
      handleEvent(Event::UIDown);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SliderWidget::handleEvent(Event::Type e)
{
  if(!isEnabled())
    return false;

  switch(e)
  {
    case Event::UIDown:
    case Event::UILeft:
    case Event::UIPgDown:
      setValue(_value - _stepValue);
      break;

    case Event::UIUp:
    case Event::UIRight:
    case Event::UIPgUp:
      setValue(_value + _stepValue);
      break;

    case Event::UIHome:
      setValue(_valueMin);
      break;

    case Event::UIEnd:
      setValue(_valueMax);
      break;

    default:
      return false;
  }
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SliderWidget::setLabelWidth(int w)
{
  // Keep the track the width it already has: the label column grows or shrinks
  // and we grow or shrink with it, rather than eating into the track
  _w += w - _labelWidth;
  _labelWidth = w;
  setDirty();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SliderWidget::drawWidget(bool hilite)
{
  FBSurface& s = _boss->dialog().surface();

  // Draw the label, if any
  if(_labelWidth > 0)
    s.drawString(_font, _label, _x, _y + firstTextY(), _labelWidth,
                 isEnabled() ? kTextColor : kColor);

  const int p = valueToPos(_value),
    h = _h - _font.getFontHeight() / 2 - 1,
    x = _x + _labelWidth,
    y = _y + 2 + _font.desc().ascent - (_font.getFontHeight() + 1) / 2 - 1; // align to bottom of font

  // Fill the box
  s.fillRect(x, y, _w - _labelWidth - _valueLabelGap - _valueLabelWidth, h,
             !isEnabled() ? kSliderBGColorLo : hilite ? kSliderBGColorHi : kSliderBGColor);
  // Draw the 'bar'
  s.fillRect(x, y, p, h,
             !isEnabled() ? kColor : hilite ? kSliderColorHi : kSliderColor);

  // Draw the 'tickmarks'
  for(int i = 1; i < _numIntervals; ++i)
  {
    const int xt = x + (_w - _labelWidth - _valueLabelGap - _valueLabelWidth) * i / _numIntervals - 1;
    ColorId color = kNone;

    if(isEnabled())
    {
      if(xt > x + p)
        color = hilite ? kSliderColorHi : kSliderColor;
      else
        color = hilite ? kSliderBGColorHi : kSliderBGColor;
    }
    else
    {
      if(xt > x + p)
        color = kColor;
      else
        color = kSliderBGColorLo;
    }
    s.vLine(xt, y + h / 2, y + h - 1, color);
  }

  // Draw the 'handle'
  s.fillRect(x + p, y - 2, 2, h + 4,
             !isEnabled() ? kColor : hilite ? kSliderColorHi : kSliderColor);

  if(_valueLabelWidth > 0)
    s.drawString(_font, _valueLabel + _valueUnit, _x + _w - _valueLabelWidth,
                 _y + firstTextY(), _valueLabelWidth,
                 isEnabled() ? kTextColor : kColor);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int SliderWidget::valueToPos(int value) const
{
  if(value < _valueMin)      value = _valueMin;
  else if(value > _valueMax) value = _valueMax;
  const int range = std::max(_valueMax - _valueMin, 1);  // don't divide by zero

  return ((_w - _labelWidth - _valueLabelGap - _valueLabelWidth - 2) * (value - _valueMin) / range);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int SliderWidget::posToValue(int pos) const
{
  const int value = pos * (_valueMax - _valueMin) /
      (_w - _labelWidth - _valueLabelGap - _valueLabelWidth - 4) + _valueMin;

  // Scale the position to the correct interval (according to step value)
  return value - (value % _stepValue);
}
