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
// Copyright (c) 1995-2010 by Bradford W. Mott, Stephen Anthony
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

#ifndef DIALOG_HXX
#define DIALOG_HXX

class FBSurface;
class OSystem;
class DialogContainer;
class TabWidget;

#include "Command.hxx"
#include "Widget.hxx"
#include "GuiObject.hxx"

#include "bspf.hxx"

/**
  This is the base class for all dialog boxes.
  
  @author  Stephen Anthony
  @version $Id$
*/
class Dialog : public GuiObject
{
  friend class DialogContainer;

  struct Focus {
    Widget* focusedWidget;
    WidgetArray focusList;
  };
  typedef Common::Array<Focus> FocusList;

  public:
    Dialog(OSystem* instance, DialogContainer* parent,
           int x, int y, int w, int h, bool isBase = false);

    virtual ~Dialog();

    bool isVisible() const { return _visible; }
    bool isBase() const    { return _isBase;  }

    virtual void open();
    virtual void close();
    virtual void center();
    virtual void drawDialog();
    virtual void loadConfig() {}
    virtual void saveConfig() {}
    virtual void setDefaults() {}

    void addFocusWidget(Widget* w);
    void addToFocusList(WidgetArray& list, int id = -1);
    void addBGroupToFocusList(WidgetArray& list) { _ourButtonGroup = list; }
    void redrawFocus();
    void addTabWidget(TabWidget* w) { _ourTab = w; }
    void addOKWidget(Widget* w)     { _okWidget = w; }
    void addCancelWidget(Widget* w) { _cancelWidget = w; }
    void setFocus(Widget* w);

    inline FBSurface& surface() { return *_surface; }

  protected:
    virtual void draw();
    void releaseFocus();

    virtual void handleKeyDown(int ascii, int keycode, int modifiers);
    virtual void handleKeyUp(int ascii, int keycode, int modifiers);
    virtual void handleMouseDown(int x, int y, int button, int clickCount);
    virtual void handleMouseUp(int x, int y, int button, int clickCount);
    virtual void handleMouseWheel(int x, int y, int direction);
    virtual void handleMouseMoved(int x, int y, int button);
    virtual bool handleMouseClicks(int x, int y, int button);
    virtual void handleJoyDown(int stick, int button);
    virtual void handleJoyUp(int stick, int button);
    virtual void handleJoyAxis(int stick, int axis, int value);
    virtual bool handleJoyHat(int stick, int hat, int value);
    virtual void handleCommand(CommandSender* sender, int cmd, int data, int id);

    Widget* findWidget(int x, int y); // Find the widget at pos x,y if any

    void addOKCancelBGroup(WidgetArray& wid, const GUI::Font& font,
                           const string& okText = "",
                           const string& cancelText = "");

    void setResult(int result) { _result = result; }
    int getResult() const { return _result; }

  private:
    void buildFocusWidgetList(int id);
    inline bool handleNavEvent(Event::Type e);

  protected:
    Widget* _mouseWidget;
    Widget* _focusedWidget;
    Widget* _dragWidget;
    Widget* _okWidget;
    Widget* _cancelWidget;
    bool    _visible;
    bool    _isBase;

  private:
    FocusList   _ourFocusList;
    TabWidget*  _ourTab;
    WidgetArray _ourButtonGroup;
    FBSurface*  _surface;

    int _result;
    int _focusID;
    int _surfaceID;
};

#endif
