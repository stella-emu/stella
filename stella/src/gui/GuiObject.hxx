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
// $Id: GuiObject.hxx,v 1.18 2006-01-15 20:46:20 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#ifndef GUI_OBJECT_HXX
#define GUI_OBJECT_HXX

class DialogContainer;
class Widget;

#include "Command.hxx"
#include "OSystem.hxx"
#include "Array.hxx"
#include "Font.hxx"

typedef Common::Array<Widget*> WidgetArray;

enum {
  kCursorUp    = 256+17,
  kCursorDown  = 256+18,
  kCursorRight = 256+19,
  kCursorLeft  = 256+20,
  kCursorHome  = 256+22,
  kCursorEnd   = 256+23,
  kCursorPgUp  = 256+24,
  kCursorPgDn  = 256+25
};

/**
  This is the base class for all GUI objects/widgets.
  
  @author  Stephen Anthony
  @version $Id: GuiObject.hxx,v 1.18 2006-01-15 20:46:20 stephena Exp $
*/
class GuiObject : public CommandReceiver
{
  friend class Widget;
  friend class DialogContainer;

  public:
    GuiObject(OSystem* osystem, DialogContainer* parent, int x, int y, int w, int h)
      : myOSystem(osystem),
        myParent(parent),
        _x(x),
        _y(y),
        _w(w),
        _h(h),
        _dirty(false),
        _font((GUI::Font*)&(osystem->font())),
        _firstWidget(0) {}

    virtual ~GuiObject() {}

    OSystem* instance() { return myOSystem; }
    DialogContainer* parent() { return myParent; }

    virtual int getAbsX() const     { return _x; }
    virtual int getAbsY() const     { return _y; }
    virtual int getChildX() const   { return getAbsX(); }
    virtual int getChildY() const   { return getAbsY(); }
    virtual int getWidth() const    { return _w; }
    virtual int getHeight() const   { return _h; }

    virtual void setPos(int x, int y) { _x = x; _y = y; }
    virtual void setWidth(int w)      { _w = w; }
    virtual void setHeight(int h)     { _h = h; }

    virtual void setDirty()         { _dirty = true; }

    virtual void setFont(const GUI::Font& font) { _font = (GUI::Font*) &font; }
    virtual const GUI::Font* font() { return _font; }

    virtual bool isVisible() const = 0;
    virtual void draw() = 0;

    /** Add given widget to the focus list */
    virtual void addFocusWidget(Widget* w) = 0;

    /** Return focus list for this object */
    WidgetArray& getFocusList() { return _focusList; }

    /** Redraw the focus list */
    virtual void redrawFocus() {}

  protected:
    virtual void releaseFocus() = 0;

  protected:
    OSystem*         myOSystem;
    DialogContainer* myParent;

    int _x, _y;
    int _w, _h;
    bool _dirty;

    GUI::Font* _font;

    Widget* _firstWidget;
    WidgetArray _focusList;
};

#endif
