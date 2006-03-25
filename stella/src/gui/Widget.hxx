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
// $Id: Widget.hxx,v 1.49 2006-03-25 00:34:17 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#ifndef WIDGET_HXX
#define WIDGET_HXX

class Dialog;

#include <assert.h>

#include "OSystem.hxx"
#include "FrameBuffer.hxx"
#include "GuiObject.hxx"
#include "GuiUtils.hxx"
#include "Array.hxx"
#include "Rect.hxx"
#include "Font.hxx"
#include "bspf.hxx"

enum {
  WIDGET_ENABLED      = 1 << 0,
  WIDGET_INVISIBLE    = 1 << 1,
  WIDGET_HILITED      = 1 << 2,
  WIDGET_BORDER       = 1 << 3,
  WIDGET_INV_BORDER   = 1 << 4,
  WIDGET_CLEARBG      = 1 << 5,
  WIDGET_TRACK_MOUSE  = 1 << 6,
  WIDGET_RETAIN_FOCUS = 1 << 7,
  WIDGET_NODRAW_FOCUS = 1 << 8,
  WIDGET_STICKY_FOCUS = 1 << 9,
  WIDGET_WANTS_TAB    = 1 << 10,
  WIDGET_WANTS_EVENTS = 1 << 11
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
  kTabWidget        = 'TABW',
  kPromptWidget     = 'PROM',
  kDataGridWidget   = 'BGRI',
  kToggleWidget     = 'TOGL',
  kColorWidget      = 'COLR'
};

enum {
  kButtonWidth  = 50,
  kButtonHeight = 16
};

/**
  This is the base class for all widgets.
  
  @author  Stephen Anthony
  @version $Id: Widget.hxx,v 1.49 2006-03-25 00:34:17 stephena Exp $
*/
class Widget : public GuiObject
{
  friend class Dialog;

  public:
    Widget(GuiObject* boss, const GUI::Font& font, int x, int y, int w, int h);
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
    virtual bool handleKeyUp(int ascii, int keycode, int modifiers)   { return false; }
    virtual void handleJoyDown(int stick, int button) {}
    virtual void handleJoyUp(int stick, int button) {}
    virtual void handleJoyAxis(int stick, int axis, int value) {}
    virtual bool handleJoyHat(int stick, int hat, int value) { return false; }

    void draw();
    void receivedFocus();
    void lostFocus();
    void addFocusWidget(Widget* w) { _focusList.push_back(w); }

    virtual GUI::Rect getRect() const;
    virtual bool wantsFocus()  { return false; }

    /** Set/clear WIDGET_ENABLED flag and immediately redraw */
    void setEnabled(bool e);

    void setFlags(int flags)    { _flags |= flags;  }
    void clearFlags(int flags)  { _flags &= ~flags; }
    int  getFlags() const       { return _flags;    }

    bool isEnabled() const   { return _flags & WIDGET_ENABLED;      }
    bool isVisible() const   { return !(_flags & WIDGET_INVISIBLE); }
    bool isSticky() const    { return _flags & WIDGET_STICKY_FOCUS; }
    bool wantsEvents() const { return _flags & WIDGET_WANTS_EVENTS; }

    void setID(int id)  { _id = id;   }
    int  getID()        { return _id; }

    void setColor(int color)        { _color = color; }
    virtual const GUI::Font* font() { return _font; }

    virtual void loadConfig() {}

  protected:
    virtual void drawWidget(bool hilite) {}

    virtual void receivedFocusWidget() {}
    virtual void lostFocusWidget() {}

    virtual Widget* findWidget(int x, int y) { return this; }

    void releaseFocus() { assert(_boss); _boss->releaseFocus(); }

    // By default, delegate unhandled commands to the boss
    void handleCommand(CommandSender* sender, int cmd, int data, int id)
         { assert(_boss); _boss->handleCommand(sender, cmd, data, id); }

  protected:
    int        _type;
    GuiObject* _boss;
    GUI::Font* _font;
    Widget*    _next;
    int        _id;
    int        _flags;
    bool       _hasFocus;
    int        _color;
    int        _fontWidth;
    int        _fontHeight;

  public:
    static Widget* findWidgetInChain(Widget* start, int x, int y);

    /** Determine if 'find' is in the chain pointed to by 'start' */
    static bool isWidgetInChain(Widget* start, Widget* find);

    /** Determine if 'find' is in the widget array */
    static bool isWidgetInChain(WidgetArray& list, Widget* find);

    /** Select either previous, current, or next widget in chain to have
        focus, and deselects all others */
    static Widget* setFocusForChain(GuiObject* boss, WidgetArray& arr,
                                    Widget* w, int direction);

    /** Sets all widgets in this chain to be dirty (must be redrawn) */
    static void setDirtyInChain(Widget* start);
};


/* StaticTextWidget */
class StaticTextWidget : public Widget
{
  public:
    StaticTextWidget(GuiObject* boss, const GUI::Font& font,
                     int x, int y, int w, int h,
                     const string& text, TextAlignment align);
    void setValue(int value);
    void setLabel(const string& label);
    void setAlign(TextAlignment align)  { _align = align; }
    const string& getLabel() const      { return _label; }
    void setEditable(bool editable);

  protected:
    void drawWidget(bool hilite);

  protected:
    string        _label;
    bool          _editable;
    TextAlignment _align;
};


/* ButtonWidget */
class ButtonWidget : public StaticTextWidget, public CommandSender
{
  public:
    ButtonWidget(GuiObject* boss, const GUI::Font& font,
                 int x, int y, int w, int h,
                 const string& label, int cmd = 0, uInt8 hotkey = 0);

    void setCmd(int cmd)  { _cmd = cmd; }
    int getCmd() const    { return _cmd; }

    void handleMouseUp(int x, int y, int button, int clickCount);
    void handleMouseEntered(int button);
    void handleMouseLeft(int button);
    bool handleKeyDown(int ascii, int keycode, int modifiers);
    void handleJoyDown(int stick, int button);

    bool wantsFocus();
    void setEditable(bool editable);

  protected:
    void drawWidget(bool hilite);

  protected:
    int    _cmd;
    bool   _editable;
    uInt8  _hotkey;
};


/* CheckboxWidget */
class CheckboxWidget : public ButtonWidget
{
  public:
    CheckboxWidget(GuiObject* boss, const GUI::Font& font, int x, int y,
                   const string& label, int cmd = 0);

    void handleMouseUp(int x, int y, int button, int clickCount);
    virtual void handleMouseEntered(int button)	{}
    virtual void handleMouseLeft(int button)	{}
    virtual bool handleKeyDown(int ascii, int keycode, int modifiers);

    bool wantsFocus();
    void holdFocus(bool status) { _holdFocus = status; }

    void setEditable(bool editable);
    void setFill(bool fill) { _fillRect = fill; }
    void drawBox(bool draw) { _drawBox = draw;  }

    void setState(bool state);
    void toggleState()     { setState(!_state); }
    bool getState() const  { return _state;     }

    static int boxSize() { return 14; }  // box is square

  protected:
    void drawWidget(bool hilite);

  protected:
    bool _state;
    bool _editable;
    bool _holdFocus;
    bool _fillRect;
    bool _drawBox;

    int _fillColor;

  private:
    int _boxY;
    int _textY;
};


/* SliderWidget */
class SliderWidget : public ButtonWidget
{
  public:
    SliderWidget(GuiObject *boss, const GUI::Font& font,
                 int x, int y, int w, int h, const string& label = "",
                 int labelWidth = 0, int cmd = 0, uInt8 hotkey = 0);

    void setValue(int value);
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
