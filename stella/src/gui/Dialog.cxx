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
// Copyright (c) 1995-2005 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: Dialog.cxx,v 1.37 2005-12-21 01:50:16 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#include <SDL.h>

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
               int x, int y, int w, int h)
  : GuiObject(instance, parent, x, y, w, h),
    _mouseWidget(0),
    _focusedWidget(0),
    _dragWidget(0),
    _visible(true),
    _ourTab(NULL),
    _focusID(0)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Dialog::~Dialog()
{
  for(unsigned int i = 0; i < _ourFocusList.size(); ++i)
    _ourFocusList[i].focusList.clear();

  delete _firstWidget;
  _firstWidget = NULL;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Dialog::open()
{
  _result = 0;
  _visible = true;

  loadConfig();

  // (Re)-build the focus list to use for the widgets which are currently
  // onscreen
  buildFocusWidgetList(_focusID);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Dialog::close()
{
  if (_mouseWidget)
  {
    _mouseWidget->handleMouseLeft(0);
    _mouseWidget = 0;
  }

  releaseFocus();
  parent()->removeDialog();
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
  if(_ourFocusList.size() == 0)
  {
	Focus f;
    f.focusedWidget = 0;
	_ourFocusList.push_back(f);
  }
  _ourFocusList[0].focusedWidget = w;
  _ourFocusList[0].focusList.push_back(w);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Dialog::addToFocusList(WidgetArray& list, int id)
{
  id++;  // Arrays start at 0, not -1.

  // Make sure the array is large enough
  while((int)_ourFocusList.size() <= id)
  {
	Focus f;
	f.focusedWidget = NULL;
	_ourFocusList.push_back(f);
  }

  _ourFocusList[id].focusList.push_back(list);
  if(id == 0 && _ourFocusList.size() > 0)
    _focusList = _ourFocusList[0].focusList;

  if(list.size() > 0 && !(list[0]->getFlags() & WIDGET_NODRAW_FOCUS))
    _ourFocusList[id].focusedWidget = list[0];
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
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Dialog::buildFocusWidgetList(int id)
{
  // Yes, this is hideously complex.  That's the price we pay for
  // tab navigation ...

  // Remember which item previously had focus, but only if it belongs
  // to this focus list
  if(_focusID < (int)_ourFocusList.size() &&
     Widget::isWidgetInChain(_ourFocusList[_focusID].focusList, _focusedWidget))
    _ourFocusList[_focusID].focusedWidget = _focusedWidget;

  _focusID = id;

  // Create a focuslist for items currently onscreen
  // We do this by starting with any dialog focus list (at index 0 in the
  // focus lists, then appending the list indicated by 'id'.
  if(_focusID < (int)_ourFocusList.size())
  {
    _focusList.clear();
    _focusList.push_back(_ourFocusList[0].focusList);

    // Append extra focus list
    if(_focusID > 0)
      _focusList.push_back(_ourFocusList[_focusID].focusList);

    // Only update _focusedWidget if it doesn't belong to the main focus list
    // HACK - figure out how to properly deal with only one focus-able widget
    // in a tab -- TabWidget is the spawn of the devil
    if(_focusList.size() == 1)
      _focusedWidget = _focusList[0];
    else if(!Widget::isWidgetInChain(_ourFocusList[0].focusList, _focusedWidget))
      _focusedWidget = _ourFocusList[_focusID].focusedWidget;
  }
  else
    _focusedWidget = 0;
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

  if(_dirty)
  {
//    cerr << "Dialog::drawDialog()\n";
    FrameBuffer& fb = instance()->frameBuffer();

    fb.fillRect(_x+1, _y+1, _w-2, _h-2, kBGColor);
    fb.box(_x, _y, _w, _h, kColor, kShadowColor);

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

    // Tell the framebuffer this area is dirty
    fb.addDirtyRect(_x, _y, _w, _h);

    _dirty = false;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Dialog::handleMouseDown(int x, int y, int button, int clickCount)
{
  Widget* w;
  w = findWidget(x, y);

  _dragWidget = w;

  setFocus(w);

  if(w)
    w->handleMouseDown(x - (w->getAbsX() - _x), y - (w->getAbsY() - _y), button, clickCount);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Dialog::handleMouseUp(int x, int y, int button, int clickCount)
{
  Widget* w;

  if(_focusedWidget)
  {
    // Lose focus on mouseup unless the widget requested to retain the focus
    if(! (_focusedWidget->getFlags() & WIDGET_RETAIN_FOCUS ))
      releaseFocus();
  }

  w = _dragWidget;

  if(w)
    w->handleMouseUp(x - (w->getAbsX() - _x), y - (w->getAbsY() - _y), button, clickCount);

  _dragWidget = 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Dialog::handleMouseWheel(int x, int y, int direction)
{
  Widget* w;

  // This may look a bit backwards, but I think it makes more sense for
  // the mouse wheel to primarily affect the widget the mouse is at than
  // the widget that happens to be focused.

  w = findWidget(x, y);
  if(!w)
    w = _focusedWidget;
  if (w)
    w->handleMouseWheel(x, y, direction);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Dialog::handleKeyDown(int ascii, int keycode, int modifiers)
{
  // Test for TAB character
  // Shift-left/shift-right cursor selects next tab
  // Tab sets next widget in current tab
  // Shift-Tab sets previous widget in current tab
  //
  // Widgets are only cycled if currently focused key hasn't claimed
  // the TAB key
  // TODO - figure out workaround for this
  if(_ourTab && instance()->eventHandler().kbdShift(modifiers))
  {
    // these key-combos are never passed to the child widget
    if(ascii == 256 + 20)      // left arrow
    {
      _ourTab->cycleTab(-1);
      return;
    }
    else if(ascii == 256 + 19) // right arrow
    {
      _ourTab->cycleTab(+1);
      return;
    }
  }

  if(keycode == 9)  // tab key
  {
    if(_focusedWidget && !(_focusedWidget->getFlags() & WIDGET_WANTS_TAB))
    {
      if(instance()->eventHandler().kbdShift(modifiers))
        _focusedWidget = Widget::setFocusForChain(this, getFocusList(),
                                                  _focusedWidget, -1);
      else
        _focusedWidget = Widget::setFocusForChain(this, getFocusList(),
                                                  _focusedWidget, +1);
      return;  // this key-combo is never passed to the child widget
    }
  }

  if (_focusedWidget)
    _focusedWidget->handleKeyDown(ascii, keycode, modifiers);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Dialog::handleKeyUp(int ascii, int keycode, int modifiers)
{
  // Focused widget receives keyup events
  if(_focusedWidget)
    _focusedWidget->handleKeyUp(ascii, keycode, modifiers);
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
void Dialog::handleJoyDown(int stick, int button)
{
  // Focused widget receives joystick events
  if(_focusedWidget)
    _focusedWidget->handleJoyDown(stick, button);
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
  // Focused widget receives joystick events
  if(_focusedWidget)
    _focusedWidget->handleJoyAxis(stick, axis, value);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Dialog::handleCommand(CommandSender* sender, int cmd, int data, int id)
{
  switch(cmd)
  {
    case kTabChangedCmd:
      // Add this focus list for the given tab to the global focus list
      buildFocusWidgetList(++data);
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
ButtonWidget* Dialog::addButton(int x, int y, const string& label,
                                int cmd, char hotkey)
{
  return new ButtonWidget(this, x, y, kButtonWidth, 16, label, cmd, hotkey);
}
