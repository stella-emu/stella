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
// $Id: Widget.hxx,v 1.2 2005-03-10 22:59:40 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#ifndef WIDGET_HXX
#define WIDGET_HXX

class Dialog;

#include "GuiObject.hxx"
#include "bspf.hxx"

enum {
	WIDGET_ENABLED		= 1 << 0,
	WIDGET_INVISIBLE	= 1 << 1,
	WIDGET_HILITED		= 1 << 2,
	WIDGET_BORDER		= 1 << 3,
	WIDGET_INV_BORDER	= 1 << 4,
	WIDGET_CLEARBG		= 1 << 5,
	WIDGET_WANT_TICKLE	= 1 << 7,
	WIDGET_TRACK_MOUSE	= 1 << 8,
	WIDGET_RETAIN_FOCUS	= 1 << 9
};

/**
  This is the base class for all widgets.
  
  @author  Stephen Anthony
  @version $Id: Widget.hxx,v 1.2 2005-03-10 22:59:40 stephena Exp $
*/
class Widget : public GuiObject
{
  friend class Dialog;

  public:
    Widget(GuiObject* boss, uInt32 x, uInt32 y, uInt32 w, uInt32 h);
    virtual ~Widget();

    virtual Int16 getAbsX() const  { return _x + _boss->getChildX(); }
    virtual Int16 getAbsY() const  { return _y + _boss->getChildY(); }

    virtual void handleMouseDown(uInt32 x, uInt32 y, uInt32 button, uInt32 clickCount) {}
    virtual void handleMouseUp(uInt32 x, uInt32 y, uInt32 button, uInt32 clickCount) {}
    virtual void handleMouseEntered(uInt32 button) {}
    virtual void handleMouseLeft(uInt32 button) {}
    virtual void handleMouseMoved(uInt32 x, uInt32 y, uInt32 button) {}
    virtual void handleMouseWheel(uInt32 x, uInt32 y, uInt32 direction) {}
    virtual bool handleKeyDown(uInt16 ascii, int keycode, int modifiers) { return false; }
    virtual bool handleKeyUp(uInt16 ascii, int keycode, int modifiers) { return false; }
    virtual void handleTickle() {}

    void draw();
    void receivedFocus() { _hasFocus = true; receivedFocusWidget(); }
    void lostFocus() { _hasFocus = false; lostFocusWidget(); }
    virtual bool wantsFocus() { return false; };

    void setFlags(int flags)    { _flags |= flags; }
    void clearFlags(int flags)  { _flags &= ~flags; }
    uInt32 getFlags() const     { return _flags; }

    void setEnabled(bool e)     { if (e) setFlags(WIDGET_ENABLED); else clearFlags(WIDGET_ENABLED); }
    bool isEnabled() const      { return _flags & WIDGET_ENABLED; }
    bool isVisible() const      { return !(_flags & WIDGET_INVISIBLE); }

  protected:
    virtual void drawWidget(bool hilite) {}

    virtual void receivedFocusWidget() {}
    virtual void lostFocusWidget() {}

    virtual Widget* findWidget(uInt32 x, uInt32 y) { return this; }

    void releaseFocus() { assert(_boss); _boss->releaseFocus(); }

    // By default, delegate unhandled commands to the boss
    void handleCommand(CommandSender* sender, uInt32 cmd, uInt32 data)
         { assert(_boss); _boss->handleCommand(sender, cmd, data); }

  protected:
    uInt32     _type;
    GuiObject* _boss;
    Widget*    _next;
    uInt16     _id;
    uInt16     _flags;
    bool       _hasFocus;

  public:
    static Widget* findWidgetInChain(Widget* start, uInt32 x, uInt32 y);
};


/* StaticTextWidget */
class StaticTextWidget : public Widget
{
  public:
    StaticTextWidget(GuiObject* boss, uInt32 x, uInt32 y, uInt32 w, uInt32 h,
                     const string& text);
    void setValue(uInt32 value);
    void setLabel(const string& label)  { _label = label; }
    const string& getLabel() const      { return _label; }

  protected:
    void drawWidget(bool hilite);

  protected:
    string _label;
};


/* ButtonWidget */
class ButtonWidget : public StaticTextWidget
{
  public:
    ButtonWidget(GuiObject* boss, uInt32 x, uInt32 y, uInt32 w, uInt32 h,
                 const string& label, uInt32 cmd = 0, uInt8 hotkey = 0);

    void setCmd(uInt32 cmd)  { _cmd = cmd; }
    uInt32 getCmd() const    { return _cmd; }

    void handleMouseUp(uInt32 x, uInt32 y, uInt32 button, uInt32 clickCount);
    void handleMouseEntered(uInt32 button) { setFlags(WIDGET_HILITED); draw(); }
    void handleMouseLeft(uInt32 button)    { clearFlags(WIDGET_HILITED); draw(); }

  protected:
    void drawWidget(bool hilite);

  protected:
    uInt32	_cmd;
    uInt8	_hotkey;
};


/* CheckboxWidget */
class CheckboxWidget : public ButtonWidget
{
  public:
    CheckboxWidget(GuiObject* boss, uInt32 x, uInt32 y, uInt32 w, uInt32 h,
                   const string& label, uInt32 cmd = 0, uInt8 hotkey = 0);

    void handleMouseUp(uInt32 x, uInt32 y, uInt32 button, uInt32 clickCount);
    virtual void handleMouseEntered(uInt32 button)	{}
    virtual void handleMouseLeft(uInt32 button)	{}

    void setState(bool state);
    void toggleState()     { setState(!_state); }
    bool getState() const  { return _state; }

  protected:
    void drawWidget(bool hilite);

  protected:
    bool _state;
};


/* SliderWidget */
class SliderWidget : public ButtonWidget
{
  public:
    SliderWidget(GuiObject *boss, int x, int y, int w, int h, const string& label = "",
                 uInt32 labelWidth = 0, uInt32 cmd = 0, uInt8 hotkey = 0);

    void setValue(uInt32 value) { _value = value; }
    uInt32 getValue() const     { return _value; }

    void   setMinValue(uInt32 value) { _valueMin = value; }
    uInt32 getMinValue() const       { return _valueMin; }
    void   setMaxValue(uInt32 value) { _valueMax = value; }
    uInt32 getMaxValue() const       { return _valueMax; }

    void handleMouseMoved(uInt32 x, uInt32 y, uInt32 button);
    void handleMouseDown(uInt32 x, uInt32 y, uInt32 button, uInt32 clickCount);
    void handleMouseUp(uInt32 x, uInt32 y, uInt32 button, uInt32 clickCount);

  protected:
    void drawWidget(bool hilite);

    uInt32 valueToPos(uInt32 value);
    uInt32 posToValue(uInt32 pos);

  protected:
    uInt32 _value, _oldValue;
    uInt32 _valueMin, _valueMax;
    bool   _isDragging;
    uInt32 _labelWidth;
};

#endif
