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
// Copyright (c) 1995-2018 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#include "OSystem.hxx"
#include "EventHandler.hxx"
#include "FrameBuffer.hxx"
#include "FBSurface.hxx"
#include "Font.hxx"
#include "Menu.hxx"
#include "Dialog.hxx"
#include "Widget.hxx"
#include "TabWidget.hxx"

#include "ContextMenu.hxx"
#include "PopUpWidget.hxx"
#include "Settings.hxx"
#include "Console.hxx"

#include "Vec.hxx"

/*
 * TODO list
 * - add some sense of the window being "active" (i.e. in front) or not. If it
 *   was inactive and just became active, reset certain vars (like who is focused).
 *   Maybe we should just add lostFocus and receivedFocus methods to Dialog, just
 *   like we have for class Widget?
 * ...
 */
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Dialog::Dialog(OSystem& instance, DialogContainer& parent, const GUI::Font& font,
               const string& title, int x, int y, int w, int h)
  : GuiObject(instance, parent, *this, x, y, w, h),
    _font(font),
    _mouseWidget(nullptr),
    _focusedWidget(nullptr),
    _dragWidget(nullptr),
    _defaultWidget(nullptr),
    _okWidget(nullptr),
    _cancelWidget(nullptr),
    _visible(false),
    _processCancel(false),
    _title(title),
    _th(0),
    _surface(nullptr),
    _tabID(0),
    _flags(WIDGET_ENABLED | WIDGET_BORDER | WIDGET_CLEARBG)
{
  setTitle(title);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Dialog::Dialog(OSystem& instance, DialogContainer& parent,
               int x, int y, int w, int h)
  : Dialog(instance, parent, instance.frameBuffer().font(), "", x, y, w, h)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Dialog::~Dialog()
{
  _myFocus.list.clear();
  _myTabList.clear();

  delete _firstWidget;
  _firstWidget = nullptr;

  _buttonGroup.clear();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Dialog::open(bool refresh)
{
  // Make sure we have a valid surface to draw into
  // Technically, this shouldn't be needed until drawDialog(), but some
  // dialogs cause drawing to occur within loadConfig()
  if(_surface == nullptr)
    _surface = instance().frameBuffer().allocateSurface(_w, _h);
  parent().addDialog(this);

  center();
  loadConfig();

  // (Re)-build the focus list to use for the widgets which are currently
  // onscreen
  buildCurrentFocusList();

  _visible = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Dialog::close(bool refresh)
{
  if (_mouseWidget)
  {
    _mouseWidget->handleMouseLeft();
    _mouseWidget = nullptr;
  }

  releaseFocus();

  _visible = false;

  parent().removeDialog();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Dialog::setTitle(const string& title)
{
  _title = title;
  _h -= _th;
  if(title.empty())
    _th = 0;
  else
    _th = _font.getLineHeight() + 4;
  _h += _th;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Dialog::center()
{
  if(_surface)
  {
    const GUI::Size& screen = instance().frameBuffer().screenSize();
    const GUI::Rect& dst = _surface->dstRect();
    _surface->setDstPos((screen.w - dst.width()) >> 1, (screen.h - dst.height()) >> 1);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Dialog::releaseFocus()
{
  if(_focusedWidget)
  {
    _focusedWidget->lostFocus();
    _focusedWidget = nullptr;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Dialog::addFocusWidget(Widget* w)
{
  if(!w)
    return;

  // All focusable widgets should retain focus
  w->setFlags(WIDGET_RETAIN_FOCUS);

  _myFocus.widget = w;
  _myFocus.list.push_back(w);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Dialog::addToFocusList(WidgetArray& list)
{
  // All focusable widgets should retain focus
  for(const auto& w: list)
    w->setFlags(WIDGET_RETAIN_FOCUS);

  Vec::append(_myFocus.list, list);
  _focusList = _myFocus.list;

  if(list.size() > 0)
    _myFocus.widget = list[0];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Dialog::addToFocusList(WidgetArray& list, TabWidget* w, int tabId)
{
  // Only add the list if the tab actually exists
  if(!w || w->getID() < 0 || uInt32(w->getID()) >= _myTabList.size())
    return;

  assert(w == _myTabList[w->getID()].widget);

  // All focusable widgets should retain focus
  for(const auto& fw: list)
    fw->setFlags(WIDGET_RETAIN_FOCUS);

  // First get the appropriate focus list
  FocusList& focus = _myTabList[w->getID()].focus;

  // Now insert in the correct place in that focus list
  uInt32 id = tabId;
  if(id < focus.size())
    Vec::append(focus[id].list, list);
  else
  {
    // Make sure the array is large enough
    while(focus.size() <= id)
      focus.push_back(Focus());

    Vec::append(focus[id].list, list);
  }

  if(list.size() > 0)
    focus[id].widget = list[0];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Dialog::addTabWidget(TabWidget* w)
{
  if(!w || w->getID() < 0)
    return;

  // Make sure the array is large enough
  uInt32 id = w->getID();
  while(_myTabList.size() < id)
    _myTabList.push_back(TabFocus());

  _myTabList.push_back(TabFocus(w));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Dialog::setFocus(Widget* w)
{
  // If the click occured inside a widget which is not the currently
  // focused one, change the focus to that widget.
  if(w && w != _focusedWidget && w->wantsFocus())
  {
    // Redraw widgets for new focus
    _focusedWidget = Widget::setFocusForChain(this, getFocusList(), w, 0);

    // Update current tab based on new focused widget
    getTabIdForWidget(_focusedWidget);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Dialog::buildCurrentFocusList(int tabID)
{
  // Yes, this is hideously complex.  That's the price we pay for
  // tab navigation ...
  _focusList.clear();

  // Remember which tab item previously had focus, if applicable
  // This only applies if this method was called for a tab change
  Widget* tabFocusWidget = nullptr;
  if(tabID >= 0 && tabID < int(_myTabList.size()))
  {
    // Save focus in previously selected tab column,
    // and get focus for new tab column
    TabFocus& tabfocus = _myTabList[tabID];
    tabfocus.saveCurrentFocus(_focusedWidget);
    tabFocusWidget = tabfocus.getNewFocus();

    _tabID = tabID;
  }

  // Add appropriate items from tablist (if present)
  for(auto& tabfocus: _myTabList)
    tabfocus.appendFocusList(_focusList);

  // Add remaining items from main focus list
  Vec::append(_focusList, _myFocus.list);

  // Add button group at end of current focus list
  // We do it this way for TabWidget, so that buttons are scanned
  // *after* the widgets in the current tab
  if(_buttonGroup.size() > 0)
    Vec::append(_focusList, _buttonGroup);

  // Finally, the moment we've all been waiting for :)
  // Set the actual focus widget
  if(tabFocusWidget)
    _focusedWidget = tabFocusWidget;
  else if(!_focusedWidget && _focusList.size() > 0)
    _focusedWidget = _focusList[0];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Dialog::addSurface(shared_ptr<FBSurface> surface)
{
  mySurfaceStack.push(surface);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Dialog::drawDialog()
{
  if(!isVisible())
    return;

  FBSurface& s = surface();

  if(_dirty)
  {
    // dialog is still on top if e.g a ContextMenu is opened
    bool onTop = parent().myDialogStack.top() == this
      || (parent().myDialogStack.get(parent().myDialogStack.size() - 2) == this
      && !parent().myDialogStack.top()->hasTitle());

    if(_flags & WIDGET_CLEARBG)
    {
      //    cerr << "Dialog::drawDialog(): w = " << _w << ", h = " << _h << " @ " << &s << endl << endl;
      s.fillRect(_x, _y + _th, _w, _h - _th, onTop ? kDlgColor : kBGColorLo);
      if(_th)
      {
        s.fillRect(_x, _y, _w, _th, onTop ? kColorTitleBar : kColorTitleBarLo);
        s.drawString(_font, _title, _x + 10, _y + 2 + 1, _font.getStringWidth(_title),
                     onTop ? kColorTitleText : kColorTitleTextLo);
      }
    }
    else
      s.invalidate();
    if(_flags & WIDGET_BORDER)
      s.frameRect(_x, _y, _w, _h, onTop ? kColor : kShadowColor);

    // Make all child widget dirty
    Widget* w = _firstWidget;
    Widget::setDirtyInChain(w);

    // Draw all children
    w = _firstWidget;
    while(w)
    {
      w->draw();
      w = w->_next;
    }

    // Draw outlines for focused widgets
    // Don't change focus, since this will trigger lost and received
    // focus events
    if(_focusedWidget)
      _focusedWidget = Widget::setFocusForChain(this, getFocusList(),
                          _focusedWidget, 0, false);

    // Tell the surface(s) this area is dirty
    s.setDirty();

    _dirty = false;
  }

  // Commit surface changes to screen; also render any extra surfaces
  // Extra surfaces must be rendered afterwards, so they are drawn on top
  if(s.render())
  {
    mySurfaceStack.applyAll([](shared_ptr<FBSurface>& surface){
      surface->setDirty();
      surface->render();
    });
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Dialog::handleText(char text)
{
  // Focused widget receives text events
  if(_focusedWidget)
    _focusedWidget->handleText(text);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Dialog::handleKeyDown(StellaKey key, StellaMod mod)
{
  // Test for TAB character
  // Control-Tab selects next tab
  // Shift-Control-Tab selects previous tab
  // Tab sets next widget in current tab
  // Shift-Tab sets previous widget in current tab
  Event::Type e = Event::NoType;

  // Detect selection of previous and next tab headers and objects
  if(key == KBDK_TAB)
  {
    if(StellaModTest::isControl(mod))
    {
      // tab header navigation
      if(StellaModTest::isShift(mod) && cycleTab(-1))
        return;
      else if(!StellaModTest::isShift(mod) && cycleTab(+1))
        return;
    }
    else
    {
      // object navigation
      if(StellaModTest::isShift(mod))
        e = Event::UINavPrev;
      else
        e = Event::UINavNext;
    }
  }

  // Check the keytable now, since we might get one of the above events,
  // which must always be processed before any widget sees it.
  if(e == Event::NoType)
    e = instance().eventHandler().eventForKey(key, kMenuMode);

  // Unless a widget has claimed all responsibility for data, we assume
  // that if an event exists for the given data, it should have priority.
  if(!handleNavEvent(e) && _focusedWidget)
  {
    if(_focusedWidget->wantsRaw() || e == Event::NoType)
      _focusedWidget->handleKeyDown(key, mod);
    else
      _focusedWidget->handleEvent(e);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Dialog::handleKeyUp(StellaKey key, StellaMod mod)
{
  // Focused widget receives keyup events
  if(_focusedWidget)
    _focusedWidget->handleKeyUp(key, mod);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Dialog::handleMouseDown(int x, int y, MouseButton b, int clickCount)
{
  Widget* w = findWidget(x, y);

  _dragWidget = w;
  setFocus(w);

  if(w)
    w->handleMouseDown(x - (w->getAbsX() - _x), y - (w->getAbsY() - _y),
                       b, clickCount);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Dialog::handleMouseUp(int x, int y, MouseButton b, int clickCount)
{
  if(_focusedWidget)
  {
    // Lose focus on mouseup unless the widget requested to retain the focus
    if(! (_focusedWidget->getFlags() & WIDGET_RETAIN_FOCUS ))
      releaseFocus();
  }

  Widget* w = _dragWidget;
  if(w)
    w->handleMouseUp(x - (w->getAbsX() - _x), y - (w->getAbsY() - _y),
                     b, clickCount);

  _dragWidget = nullptr;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Dialog::handleMouseWheel(int x, int y, int direction)
{
  // This may look a bit backwards, but I think it makes more sense for
  // the mouse wheel to primarily affect the widget the mouse is at than
  // the widget that happens to be focused.

  Widget* w = findWidget(x, y);
  if(!w)
    w = _focusedWidget;
  if(w)
    w->handleMouseWheel(x, y, direction);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Dialog::handleMouseMoved(int x, int y)
{
  Widget* w;

  if(_focusedWidget && !_dragWidget)
  {
    w = _focusedWidget;
    int wx = w->getAbsX() - _x;
    int wy = w->getAbsY() - _y;

    // We still send mouseEntered/Left messages to the focused item
    // (but to no other items).
    bool mouseInFocusedWidget = (x >= wx && x < wx + w->_w && y >= wy && y < wy + w->_h);
    if(mouseInFocusedWidget && _mouseWidget != w)
    {
      if(_mouseWidget)
        _mouseWidget->handleMouseLeft();
      _mouseWidget = w;
      w->handleMouseEntered();
    }
    else if (!mouseInFocusedWidget && _mouseWidget == w)
    {
      _mouseWidget = nullptr;
      w->handleMouseLeft();
    }

    w->handleMouseMoved(x - wx, y - wy);
  }

  // While a "drag" is in process (i.e. mouse is moved while a button is pressed),
  // only deal with the widget in which the click originated.
  if (_dragWidget)
    w = _dragWidget;
  else
    w = findWidget(x, y);

  if (_mouseWidget != w)
  {
    if (_mouseWidget)
      _mouseWidget->handleMouseLeft();
    if (w)
      w->handleMouseEntered();
    _mouseWidget = w;
  }

  if (w && (w->getFlags() & WIDGET_TRACK_MOUSE))
    w->handleMouseMoved(x - (w->getAbsX() - _x), y - (w->getAbsY() - _y));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Dialog::handleMouseClicks(int x, int y, MouseButton b)
{
  Widget* w = findWidget(x, y);

  if(w)
    return w->handleMouseClicks(x - (w->getAbsX() - _x),
                                y - (w->getAbsY() - _y), b);
  else
    return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Dialog::handleJoyDown(int stick, int button)
{
  Event::Type e =
    instance().eventHandler().eventForJoyButton(stick, button, kMenuMode);

  // Unless a widget has claimed all responsibility for data, we assume
  // that if an event exists for the given data, it should have priority.
  if(!handleNavEvent(e) && _focusedWidget)
  {
    if(_focusedWidget->wantsRaw() || e == Event::NoType)
      _focusedWidget->handleJoyDown(stick, button);
    else
      _focusedWidget->handleEvent(e);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Dialog::handleJoyUp(int stick, int button)
{
  // Focused widget receives joystick events
  if(_focusedWidget)
    _focusedWidget->handleJoyUp(stick, button);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Dialog::handleJoyAxis(int stick, int axis, int value)
{
  Event::Type e =
    instance().eventHandler().eventForJoyAxis(stick, axis, value, kMenuMode);

  // Unless a widget has claimed all responsibility for data, we assume
  // that if an event exists for the given data, it should have priority.
  if(!handleNavEvent(e) && _focusedWidget)
  {
    if(_focusedWidget->wantsRaw() || e == Event::NoType)
      _focusedWidget->handleJoyAxis(stick, axis, value);
    else if(value != 0)
      _focusedWidget->handleEvent(e);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Dialog::handleJoyHat(int stick, int hat, JoyHat value)
{
  Event::Type e =
    instance().eventHandler().eventForJoyHat(stick, hat, value, kMenuMode);

  // Unless a widget has claimed all responsibility for data, we assume
  // that if an event exists for the given data, it should have priority.
  if(!handleNavEvent(e) && _focusedWidget)
  {
    if(_focusedWidget->wantsRaw() || e == Event::NoType)
      return _focusedWidget->handleJoyHat(stick, hat, value);
    else
      return _focusedWidget->handleEvent(e);
  }
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Dialog::handleNavEvent(Event::Type e)
{
  switch(e)
  {
    case Event::UINavPrev:
      if(_focusedWidget && !_focusedWidget->wantsTab())
      {
        _focusedWidget = Widget::setFocusForChain(this, getFocusList(),
                                                  _focusedWidget, -1);
        // Update current tab based on new focused widget
        getTabIdForWidget(_focusedWidget);

        return true;
      }
      break;

    case Event::UINavNext:
      if(_focusedWidget && !_focusedWidget->wantsTab())
      {
        _focusedWidget = Widget::setFocusForChain(this, getFocusList(),
                                                  _focusedWidget, +1);
        // Update current tab based on new focused widget
        getTabIdForWidget(_focusedWidget);

        return true;
      }
      break;

    case Event::UIOK:
      if(_okWidget && _okWidget->isEnabled())
      {
        // Receiving 'OK' is the same as getting the 'Select' event
        _okWidget->handleEvent(Event::UISelect);
        return true;
      }
      break;

    case Event::UICancel:
      if(_cancelWidget && _cancelWidget->isEnabled())
      {
        // Receiving 'Cancel' is the same as getting the 'Select' event
        _cancelWidget->handleEvent(Event::UISelect);
        return true;
      }
      else if(_processCancel)
      {
        // Some dialogs want the ability to cancel without actually having
        // a corresponding cancel button
        close();
        return true;
      }
      break;

    default:
      return false;
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Dialog::getTabIdForWidget(Widget* w)
{
  if(_myTabList.size() == 0 || !w)
    return;

  for(uInt32 id = 0; id < _myTabList.size(); ++id)
  {
    if(w->_boss == _myTabList[id].widget)
    {
      _tabID = id;
      return;
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Dialog::cycleTab(int direction)
{
  if(_tabID >= 0 && _tabID < int(_myTabList.size()))
  {
    _myTabList[_tabID].widget->cycleTab(direction);
    return true;
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Dialog::handleCommand(CommandSender* sender, int cmd, int data, int id)
{
  switch(cmd)
  {
    case TabWidget::kTabChangedCmd:
      if(_visible)
        buildCurrentFocusList(id);
      break;

    case GuiObject::kCloseCmd:
      close();
      break;
  }
}

/*
 * Determine the widget at location (x,y) if any. Assumes the coordinates are
 * in the local coordinate system, i.e. relative to the top left of the dialog.
 */
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Widget* Dialog::findWidget(int x, int y) const
{
  return Widget::findWidgetInChain(_firstWidget, x, y);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Dialog::addOKCancelBGroup(WidgetArray& wid, const GUI::Font& font,
                               const string& okText, const string& cancelText,
                               bool focusOKButton, int buttonWidth)
{
  const int HBORDER = 10;
  const int VBORDER = 10;
  const int BTN_BORDER = 20;
  const int BUTTON_GAP = 8;
  buttonWidth = std::max(buttonWidth,
                         std::max(font.getStringWidth("Defaults"),
                         std::max(font.getStringWidth(okText),
                         font.getStringWidth(cancelText))) + BTN_BORDER);
  int buttonHeight = font.getLineHeight() + 4;

#ifndef BSPF_MAC_OSX
  addOKWidget(new ButtonWidget(this, font, _w - 2 * buttonWidth - HBORDER - BUTTON_GAP,
      _h - buttonHeight - VBORDER, buttonWidth, buttonHeight, okText, GuiObject::kOKCmd));
  addCancelWidget(new ButtonWidget(this, font, _w - (buttonWidth + HBORDER),
      _h - buttonHeight - VBORDER, buttonWidth, buttonHeight, cancelText, GuiObject::kCloseCmd));
#else
  addCancelWidget(new ButtonWidget(this, font, _w - 2 * buttonWidth - HBORDER - BUTTON_GAP,
      _h - buttonHeight - VBORDER, buttonWidth, buttonHeight, cancelText, GuiObject::kCloseCmd));
  addOKWidget(new ButtonWidget(this, font, _w - (buttonWidth + HBORDER),
      _h - buttonHeight - VBORDER, buttonWidth, buttonHeight, okText, GuiObject::kOKCmd));
#endif

  // Note that 'focusOKButton' only takes effect when there are no other UI
  // elements in the dialog; otherwise, the first widget of the dialog is always
  // automatically focused first
  // Changing this behaviour would require a fairly major refactoring of the UI code
  if(focusOKButton)
  {
    wid.push_back(_okWidget);
    wid.push_back(_cancelWidget);
  }
  else
  {
    wid.push_back(_cancelWidget);
    wid.push_back(_okWidget);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Dialog::addDefaultsOKCancelBGroup(WidgetArray& wid, const GUI::Font& font,
                                       const string& okText, const string& cancelText,
                                       const string& defaultsText,
                                       bool focusOKButton)
{
  const int HBORDER = 10;
  const int VBORDER = 10;
  const int BTN_BORDER = 20;
  int buttonWidth = font.getStringWidth(defaultsText) + BTN_BORDER;
  int buttonHeight = font.getLineHeight() + 4;

  addDefaultWidget(new ButtonWidget(this, font, HBORDER, _h - buttonHeight - VBORDER,
                   buttonWidth, buttonHeight, defaultsText, GuiObject::kDefaultsCmd));
  wid.push_back(_defaultWidget);

  addOKCancelBGroup(wid, font, okText, cancelText, focusOKButton, buttonWidth);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Dialog::TabFocus::appendFocusList(WidgetArray& list)
{
  int active = widget->getActiveTab();

  if(active >= 0 && active < int(focus.size()))
    Vec::append(list, focus[active].list);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Dialog::TabFocus::saveCurrentFocus(Widget* w)
{
  if(currentTab < focus.size() &&
      Widget::isWidgetInChain(focus[currentTab].list, w))
    focus[currentTab].widget = w;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Widget* Dialog::TabFocus::getNewFocus()
{
  currentTab = widget->getActiveTab();

  return (currentTab < focus.size()) ? focus[currentTab].widget : nullptr;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Dialog::getResizableBounds(uInt32& w, uInt32& h) const
{
  const GUI::Rect& r = instance().frameBuffer().imageRect();

  bool ntsc = instance().console().about().InitialFrameRate == "60";
  uInt32 aspect = instance().settings().getInt(ntsc ?"tia.aspectn" : "tia.aspectp");

  if(r.width() <= FrameBuffer::kFBMinW || r.height() <= FrameBuffer::kFBMinH)
  {
    w = uInt32(aspect * FrameBuffer::kTIAMinW) * 2 / 100;
    h = FrameBuffer::kTIAMinH * 2;
    return false;
  }
  else
  {
    w = std::max(uInt32(aspect * r.width() / 100), uInt32(FrameBuffer::kFBMinW));
    h = std::max(uInt32(aspect * r.height() / 100), uInt32(FrameBuffer::kFBMinH));
    return true;
  }
}
