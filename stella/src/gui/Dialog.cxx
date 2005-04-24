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
// Copyright (c) 1995-2005 by Bradford W. Mott
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: Dialog.cxx,v 1.10 2005-04-24 01:57:47 stephena Exp $
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

/*
 * TODO list
 * - add some sense of the window being "active" (i.e. in front) or not. If it 
 *   was inactive and just became active, reset certain vars (like who is focused).
 *   Maybe we should just add lostFocus and receivedFocus methods to Dialog, just
 *   like we have for class Widget?
 * ...
 */
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Dialog::Dialog(OSystem* instance, uInt16 x, uInt16 y, uInt16 w, uInt16 h)
    : GuiObject(instance, x, y, w, h),
      _mouseWidget(0),
      _focusedWidget(0),
      _dragWidget(0),
      _visible(true),
      _openCount(0)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Dialog::~Dialog()
{
  delete _firstWidget;
  _firstWidget = NULL;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int Dialog::runModal() // FIXME
{
  // Open up
  open();

  // Start processing events
  //  g_gui.runLoop();

  // Return the result code
  return _result;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Dialog::open()
{
  _result = 0;
  _visible = true;

  // Keep count of how many times this dialog was opened
  // Load the config only on the first open (ie, since close was last called)
  if(_openCount++ == 0)
    loadConfig();

  Widget* w = _firstWidget;

  // Search for the first objects that wantsFocus() (if any) and give it the focus
  while(w && !w->wantsFocus())
    w = w->_next;

  if(w)
  {
    w->receivedFocus();
    _focusedWidget = w;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Dialog::close()
{
  if (_mouseWidget) {
    _mouseWidget->handleMouseLeft(0);
    _mouseWidget = 0;
  }

  releaseFocus();
  instance()->menu().removeDialog();

  _openCount = 0;
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
void Dialog::draw()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Dialog::drawDialog()
{
  if(!isVisible())
    return;

  FrameBuffer& fb = instance()->frameBuffer();

  fb.blendRect(_x, _y, _w, _h, kBGColor);
  fb.box(_x, _y, _w, _h, kColor, kShadowColor);

  // Draw all children
  Widget* w = _firstWidget;
  while(w)
  {
    w->draw();
    w = w->_next;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Dialog::handleMouseDown(int x, int y, int button, int clickCount)
{
  Widget* w;
  w = findWidget(x, y);

  _dragWidget = w;

  // If the click occured inside a widget which is not the currently
  // focused one, change the focus to that widget.
  if(w && w != _focusedWidget && w->wantsFocus())
  {
    // The focus will change. Tell the old focused widget (if any)
    // that it lost the focus.
    releaseFocus();

    // Tell the new focused widget (if any) that it just gained the focus.
    if(w)
      w->receivedFocus();

    _focusedWidget = w;
  }

  if(w)
    w->handleMouseDown(x - (w->getAbsX() - _x), y - (w->getAbsY() - _y), button, clickCount);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Dialog::handleMouseUp(int x, int y, int button, int clickCount)
{
  Widget* w;

  if(_focusedWidget)
  {
    //w = _focusedWidget;

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
void Dialog::handleKeyDown(uInt16 ascii, Int32 keycode, Int32 modifiers)
{
  if(_focusedWidget)
    if (_focusedWidget->handleKeyDown(ascii, keycode, modifiers))
      return;

  // Hotkey handling
  if(ascii != 0)
  {
    Widget* w = _firstWidget;
    ascii = toupper(ascii);
    while(w)
    {
      if(0)//FIXME - don't let it access a protected variable w->_type == kButtonWidget && ascii == toupper(((ButtonWidget *)w)->_hotkey))
      {
        // The hotkey for widget w was pressed. We fake a mouse click into the
        // button by invoking the appropriate methods.
        w->handleMouseDown(0, 0, 1, 1);
        w->handleMouseUp(0, 0, 1, 1);
        return;
      }
      w = w->_next;
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Dialog::handleKeyUp(uInt16 ascii, Int32 keycode, Int32 modifiers)
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
void Dialog::handleTickle()
{
  // Focused widget receives tickle notifications
  if(_focusedWidget && _focusedWidget->getFlags() & WIDGET_WANT_TICKLE)
    _focusedWidget->handleTickle();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Dialog::handleCommand(CommandSender* sender, uInt32 cmd, uInt32 data)
{
  switch(cmd)
  {
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
ButtonWidget* Dialog::addButton(Int32 x, Int32 y, const string& label,
                                uInt32 cmd, char hotkey)
{
  return new ButtonWidget(this, x, y, kButtonWidth, 16, label, cmd, hotkey);
}
