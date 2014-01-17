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
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#include "OSystem.hxx"
#include "FrameBuffer.hxx"
#include "Menu.hxx"
#include "Dialog.hxx"
#include "Widget.hxx"
#include "TabWidget.hxx"

/*
 * TODO list
 * - add some sense of the window being "active" (i.e. in front) or not. If it 
 *   was inactive and just became active, reset certain vars (like who is focused).
 *   Maybe we should just add lostFocus and receivedFocus methods to Dialog, just
 *   like we have for class Widget?
 * ...
 */
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Dialog::Dialog(OSystem* instance, DialogContainer* parent,
               int x, int y, int w, int h, bool isBase)
  : GuiObject(*instance, *parent, *this, x, y, w, h),
    _mouseWidget(0),
    _focusedWidget(0),
    _dragWidget(0),
    _okWidget(0),
    _cancelWidget(0),
    _visible(false),
    _isBase(isBase),
    _processCancel(false),
    _surface(0),
    _tabID(0)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Dialog::~Dialog()
{
  _myFocus.list.clear();
  _myTabList.clear();

  delete _firstWidget;
  _firstWidget = NULL;

  _buttonGroup.clear();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Dialog::open(bool refresh)
{
  // Make sure we have a valid surface to draw into
  // Technically, this shouldn't be needed until drawDialog(), but some
  // dialogs cause drawing to occur within loadConfig()
  // Base surfaces are typically large, and will probably cause slow
  // performance if we update the whole area each frame
  // Instead, dirty rectangle updates should be performed
  // However, this policy is left entirely to the framebuffer
  // We suggest the hint here, but specific framebuffers are free to
  // ignore it
  if(_surface == NULL)
  {
    uInt32 surfaceID = instance().frameBuffer().allocateSurface(_w, _h, _isBase);
    _surface = instance().frameBuffer().surface(surfaceID);
  }
  parent().addDialog(this);

  center();
  loadConfig();

  // (Re)-build the focus list to use for the widgets which are currently
  // onscreen
  buildCurrentFocusList();

  _visible = true;

  if(refresh)
    instance().frameBuffer().refresh();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Dialog::close(bool refresh)
{
  if (_mouseWidget)
  {
    _mouseWidget->handleMouseLeft(0);
    _mouseWidget = 0;
  }

  releaseFocus();

  _visible = false;

  parent().removeDialog();

  if(refresh)
    instance().frameBuffer().refresh();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Dialog::center()
{
  if(_surface)
  {
    const GUI::Rect& screen = instance().frameBuffer().screenRect();
    uInt32 x = (screen.width() - getWidth()) >> 1;
    uInt32 y = (screen.height() - getHeight()) >> 1;
    _surface->setPos(x, y);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Dialog::releaseFocus()
{
  if(_focusedWidget)
  {
    _focusedWidget->lostFocus();
    _focusedWidget = 0;
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
  for(uInt32 i = 0; i < list.size(); ++i)
    list[i]->setFlags(WIDGET_RETAIN_FOCUS);

  _myFocus.list.push_back(list);
  _focusList = _myFocus.list;

  if(list.size() > 0)
    _myFocus.widget = list[0];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Dialog::addToFocusList(WidgetArray& list, TabWidget* w, int tabId)
{
  // Only add the list if the tab actually exists
  if(!w || w->getID() < 0 || (uInt32)w->getID() >= _myTabList.size())
    return;

  assert(w == _myTabList[w->getID()].widget);

  // All focusable widgets should retain focus
  for(uInt32 i = 0; i < list.size(); ++i)
    list[i]->setFlags(WIDGET_RETAIN_FOCUS);

  // First get the appropriate focus list
  FocusList& focus = _myTabList[w->getID()].focus;

  // Now insert in the correct place in that focus list
  uInt32 id = tabId;
  if(id < focus.size())
    focus[id].list.push_back(list);
  else
  {
    // Make sure the array is large enough
    while(focus.size() <= id)
      focus.push_back(Focus());

    focus[id].list.push_back(list);
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
  Widget* tabFocusWidget = 0;
  if(tabID >= 0 && tabID < (int)_myTabList.size())
  {
    // Save focus in previously selected tab column,
    // and get focus for new tab column
    TabFocus& tabfocus = _myTabList[tabID];
    tabfocus.saveCurrentFocus(_focusedWidget);
    tabFocusWidget = tabfocus.getNewFocus();

    _tabID = tabID;
  }

  // Add appropriate items from tablist (if present)
  for(uInt32 id = 0; id < _myTabList.size(); ++id)
    _myTabList[id].appendFocusList(_focusList);

  // Add remaining items from main focus list
  _focusList.push_back(_myFocus.list);

  // Add button group at end of current focus list
  // We do it this way for TabWidget, so that buttons are scanned
  // *after* the widgets in the current tab
  if(_buttonGroup.size() > 0)
    _focusList.push_back(_buttonGroup);

  // Finally, the moment we've all been waiting for :)
  // Set the actual focus widget
  if(tabFocusWidget)
    _focusedWidget = tabFocusWidget;
  else if(!_focusedWidget && _focusList.size() > 0)
    _focusedWidget = _focusList[0];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Dialog::redrawFocus()
{
  if(_focusedWidget)
    _focusedWidget = Widget::setFocusForChain(this, getFocusList(), _focusedWidget, 0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Dialog::draw()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Dialog::drawDialog()
{
  if(!isVisible())
    return;

  FBSurface& s = surface();

  if(_dirty)
  {
//    cerr << "Dialog::drawDialog(): w = " << _w << ", h = " << _h << " @ " << &s << endl << endl;

    s.fillRect(_x, _y, _w, _h, kDlgColor);
    s.box(_x, _y, _w, _h, kColor, kShadowColor);

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
    redrawFocus();

    // Tell the surface this area is dirty
    s.addDirtyRect(_x, _y, _w, _h);
    _dirty = false;
  }

  // Commit surface changes to screen
  s.update();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Dialog::handleKeyDown(StellaKey key, StellaMod mod, char ascii)
{
  // Test for TAB character
  // Shift-left/shift-right cursor selects next tab
  // Tab sets next widget in current tab
  // Shift-Tab sets previous widget in current tab
  Event::Type e = Event::NoType;

  // Detect selection of previous and next tab headers and objects
  // For some strange reason, 'tab' needs to be interpreted as keycode,
  // not ascii??
  if(instance().eventHandler().kbdShift(mod))
  {
    if(key == KBDK_LEFT && cycleTab(-1))
      return;
    else if(key == KBDK_RIGHT && cycleTab(+1))
      return;
    else if(key == KBDK_TAB)
      e = Event::UINavPrev;
  }
  else if(key == KBDK_TAB)
    e = Event::UINavNext;

  // Check the keytable now, since we might get one of the above events,
  // which must always be processed before any widget sees it.
  if(e == Event::NoType)
    e = instance().eventHandler().eventForKey(key, kMenuMode);

  // Unless a widget has claimed all responsibility for data, we assume
  // that if an event exists for the given data, it should have priority.
  if(!handleNavEvent(e) && _focusedWidget)
  {
    if(_focusedWidget->wantsRaw() || e == Event::NoType)
      _focusedWidget->handleKeyDown(key, mod, ascii);
    else
      _focusedWidget->handleEvent(e);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Dialog::handleKeyUp(StellaKey key, StellaMod mod, char ascii)
{
  // Focused widget receives keyup events
  if(_focusedWidget)
    _focusedWidget->handleKeyUp(key, mod, ascii);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Dialog::handleMouseDown(int x, int y, int button, int clickCount)
{
  Widget* w = findWidget(x, y);

  _dragWidget = w;
  setFocus(w);

  if(w)
    w->handleMouseDown(x - (w->getAbsX() - _x), y - (w->getAbsY() - _y),
                       button, clickCount);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Dialog::handleMouseUp(int x, int y, int button, int clickCount)
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
                     button, clickCount);

  _dragWidget = 0;
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
void Dialog::handleMouseMoved(int x, int y, int button)
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
        _mouseWidget->handleMouseLeft(button);
      _mouseWidget = w;
      w->handleMouseEntered(button);
    }
    else if (!mouseInFocusedWidget && _mouseWidget == w)
    {
      _mouseWidget = 0;
      w->handleMouseLeft(button);
    }

    w->handleMouseMoved(x - wx, y - wy, button);
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
      _mouseWidget->handleMouseLeft(button);
    if (w)
      w->handleMouseEntered(button);
    _mouseWidget = w;
  } 

  if (w && (w->getFlags() & WIDGET_TRACK_MOUSE))
    w->handleMouseMoved(x - (w->getAbsX() - _x), y - (w->getAbsY() - _y), button);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Dialog::handleMouseClicks(int x, int y, int button)
{
  Widget* w = findWidget(x, y);

  if(w)
    return w->handleMouseClicks(x - (w->getAbsX() - _x),
                                y - (w->getAbsY() - _y), button);
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
bool Dialog::handleJoyHat(int stick, int hat, int value)
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
      break;
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
  if(_tabID >= 0 && _tabID < (int)_myTabList.size())
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
    case kTabChangedCmd:
      if(_visible)
        buildCurrentFocusList(id);
      break;

    case kCloseCmd:
      close();
      break;
  }
}

/*
 * Determine the widget at location (x,y) if any. Assumes the coordinates are
 * in the local coordinate system, i.e. relative to the top left of the dialog.
 */
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Widget* Dialog::findWidget(int x, int y)
{
  return Widget::findWidgetInChain(_firstWidget, x, y);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Dialog::addOKCancelBGroup(WidgetArray& wid, const GUI::Font& font,
                               const string& okText, const string& cancelText)
{

  int buttonWidth  = BSPF_max(font.getStringWidth("Cancel"),
                      BSPF_max(font.getStringWidth(okText),
                      font.getStringWidth(okText))) + 15;
  int buttonHeight = font.getLineHeight() + 4;
  ButtonWidget* b;
#ifndef MAC_OSX
  b = new ButtonWidget(this, font, _w - 2 * (buttonWidth + 7), _h - buttonHeight - 10,
                       buttonWidth, buttonHeight,
                       okText == "" ? "OK" : okText, kOKCmd);
  wid.push_back(b);
  addOKWidget(b);
  b = new ButtonWidget(this, font, _w - (buttonWidth + 10), _h - buttonHeight - 10,
                       buttonWidth, buttonHeight,
                       cancelText == "" ? "Cancel" : cancelText, kCloseCmd);
  wid.push_back(b);
  addCancelWidget(b);
#else
  b = new ButtonWidget(this, font, _w - 2 * (buttonWidth + 7), _h - buttonHeight - 10,
                       buttonWidth, buttonHeight,
                       cancelText == "" ? "Cancel" : cancelText, kCloseCmd);
  wid.push_back(b);
  addCancelWidget(b);
  b = new ButtonWidget(this, font, _w - (buttonWidth + 10), _h - buttonHeight - 10,
                       buttonWidth, buttonHeight,
                       okText == "" ? "OK" : okText, kOKCmd);
  wid.push_back(b);
  addOKWidget(b);
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Dialog::Focus::Focus(Widget* w)
  : widget(w)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Dialog::Focus::~Focus()
{
  list.clear();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Dialog::TabFocus::TabFocus(TabWidget* w)
  : widget(w),
    currentTab(0)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Dialog::TabFocus::~TabFocus()
{
  focus.clear();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Dialog::TabFocus::appendFocusList(WidgetArray& list)
{
  int active = widget->getActiveTab();

  if(active >= 0 && active < (int)focus.size())
    list.push_back(focus[active].list);
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

  return (currentTab < focus.size()) ? focus[currentTab].widget : 0;
}
