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
// $Id: Widget.hxx,v 1.10 2005-05-16 00:02:32 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#ifndef WIDGET_HXX
#define WIDGET_HXX

class Dialog;

#include <assert.h>

#include "OSystem.hxx"
#include "StellaFont.hxx"
#include "FrameBuffer.hxx"
#include "GuiObject.hxx"
#include "bspf.hxx"

enum {
  WIDGET_ENABLED      = 1 << 0,
  WIDGET_INVISIBLE    = 1 << 1,
  WIDGET_HILITED      = 1 << 2,
  WIDGET_BORDER       = 1 << 3,
  WIDGET_INV_BORDER   = 1 << 4,
  WIDGET_CLEARBG      = 1 << 5,
  WIDGET_WANT_TICKLE  = 1 << 7,
  WIDGET_TRACK_MOUSE  = 1 << 8,
  WIDGET_RETAIN_FOCUS = 1 << 9
};

enum {
  kStaticTextWidget = 'TEXT',
  kEditTextWidget   = 'EDIT',
  kButtonWidget     = 'BTTN',
  kCheckboxWidget   = 'CHKB',
  kSliderWidget     = 'SLDE',
  kListWidget       = 'LIST',
  kScrollBarWidget  = 'SCRB',
  kPopUpWidget      = 'POPU',
  kTabWidget        = 'TABW'
};

enum {
  kButtonWidth  = 50,
  kButtonHeight = 16
};

/**
  This is the base class for all widgets.
  
  @author  Stephen Anthony
  @version $Id: Widget.hxx,v 1.10 2005-05-16 00:02:32 stephena Exp $
*/
class Widget : public GuiObject
{
  friend class Dialog;

  public:
    Widget(GuiObject* boss, int x, int y, int w, int h);
    virtual ~Widget();

    virtual int getAbsX() const  { return _x + _boss->getChildX(); }
    virtual int getAbsY() const  { return _y + _boss->getChildY(); }

    virtual void handleMouseDown(int x, int y, int button, int clickCount) {}
    virtual void handleMouseUp(int x, int y, int button, int clickCount) {}
    virtual void handleMouseEntered(int button) {}
    virtual void handleMouseLeft(int button) {}
    virtual void handleMouseMoved(int x, int y, int button) {}
    virtual void handleMouseWheel(int x, int y, int direction) {}
    virtual bool handleKeyDown(int ascii, int keycode, int modifiers) { return false; }
    virtual bool handleKeyUp(int ascii, int keycode, int modifiers) { return false; }
    virtual void handleTickle() {}

    void draw();
    void receivedFocus() { _hasFocus = true; receivedFocusWidget(); }
    void lostFocus() { _hasFocus = false; lostFocusWidget(); }
    virtual bool wantsFocus() { return false; };

    void setFlags(int flags)    { _flags |= flags;
                                  _boss->instance()->frameBuffer().refresh();
                                }
    void clearFlags(int flags)  { _flags &= ~flags;
                                  _boss->instance()->frameBuffer().refresh();
                                }
    int getFlags() const        { return _flags; }

    void setEnabled(bool e)     { if (e) setFlags(WIDGET_ENABLED); else clearFlags(WIDGET_ENABLED); }
    bool isEnabled() const      { return _flags & WIDGET_ENABLED; }
    bool isVisible() const      { return !(_flags & WIDGET_INVISIBLE); }

  protected:
    virtual void drawWidget(bool hilite) {}

    virtual void receivedFocusWidget() {}
    virtual void lostFocusWidget() {}

    virtual Widget* findWidget(int x, int y) { return this; }

    void releaseFocus() { assert(_boss); _boss->releaseFocus(); }

    // By default, delegate unhandled commands to the boss
    void handleCommand(CommandSender* sender, int cmd, int data)
         { assert(_boss); _boss->handleCommand(sender, cmd, data); }

  protected:
    int        _type;
    GuiObject* _boss;
    Widget*    _next;
    int        _id;
    int        _flags;
    bool       _hasFocus;

  public:
    static Widget* findWidgetInChain(Widget* start, int x, int y);
};


/* StaticTextWidget */
class StaticTextWidget : public Widget
{
  public:
    StaticTextWidget(GuiObject* boss,
                     int x, int y, int w, int h,
                     const string& text, TextAlignment align);
    void setValue(int value);
    void setColor(OverlayColor color)   { _color = color; }
    void setAlign(TextAlignment align)  { _align = align; }
    void setLabel(const string& label)  { _label = label;
                                          _boss->instance()->frameBuffer().refresh();
                                        }
    const string& getLabel() const      { return _label; }

  protected:
    void drawWidget(bool hilite);

  protected:
    string        _label;
    TextAlignment _align;
    OverlayColor  _color;
};


/* ButtonWidget */
class ButtonWidget : public StaticTextWidget, public CommandSender
{
  public:
    ButtonWidget(GuiObject* boss,
                 int x, int y, int w, int h,
                 const string& label, int cmd = 0, uInt8 hotkey = 0);

    void setCmd(int cmd)  { _cmd = cmd; }
    int getCmd() const    { return _cmd; }

    void handleMouseUp(int x, int y, int button, int clickCount);
    void handleMouseEntered(int button) { setFlags(WIDGET_HILITED); draw(); }
    void handleMouseLeft(int button)    { clearFlags(WIDGET_HILITED); draw(); }

  protected:
    void drawWidget(bool hilite);

  protected:
    uint	_cmd;
    uInt8	_hotkey;
};


/* CheckboxWidget */
class CheckboxWidget : public ButtonWidget
{
  public:
    CheckboxWidget(GuiObject* boss, int x, int y, int w, int h,
                   const string& label, int cmd = 0, uInt8 hotkey = 0);

    void handleMouseUp(int x, int y, int button, int clickCount);
    virtual void handleMouseEntered(int button)	{}
    virtual void handleMouseLeft(int button)	{}

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
                 int labelWidth = 0, int cmd = 0, uInt8 hotkey = 0);

    void  setValue(int value) { _value = value; }
    int getValue() const      { return _value; }

    void  setMinValue(int value) { _valueMin = value; }
    int getMinValue() const      { return _valueMin; }
    void  setMaxValue(int value) { _valueMax = value; }
    int getMaxValue() const      { return _valueMax; }

    void handleMouseMoved(int x, int y, int button);
    void handleMouseDown(int x, int y, int button, int clickCount);
    void handleMouseUp(int x, int y, int button, int clickCount);

  protected:
    void drawWidget(bool hilite);

    int valueToPos(int value);
    int posToValue(int pos);

  protected:
    int  _value, _oldValue;
    int  _valueMin, _valueMax;
    bool _isDragging;
    int  _labelWidth;
};

#endif
