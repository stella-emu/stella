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
// $Id: Dialog.hxx,v 1.25 2006-01-08 02:28:03 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#ifndef DIALOG_HXX
#define DIALOG_HXX

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
  @version $Id: Dialog.hxx,v 1.25 2006-01-08 02:28:03 stephena Exp $
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
           int x, int y, int w, int h);

    virtual ~Dialog();

    bool isVisible() const { return _visible; }

    virtual void open();
    virtual void close();
    virtual void drawDialog();
    virtual void loadConfig() {}
    virtual void saveConfig() {}
    virtual void setDefaults() {}

    void addFocusWidget(Widget* w);
    void addToFocusList(WidgetArray& list, int id = -1);
    void redrawFocus();
    void addTabWidget(TabWidget* w) { _ourTab = w; }
    void setFocus(Widget* w);

  protected:
    virtual void draw();
    void releaseFocus();

    virtual void handleKeyDown(int ascii, int keycode, int modifiers);
    virtual void handleKeyUp(int ascii, int keycode, int modifiers);
    virtual void handleMouseDown(int x, int y, int button, int clickCount);
    virtual void handleMouseUp(int x, int y, int button, int clickCount);
    virtual void handleMouseWheel(int x, int y, int direction);
    virtual void handleMouseMoved(int x, int y, int button);
    virtual void handleJoyDown(int stick, int button);
    virtual void handleJoyUp(int stick, int button);
    virtual void handleJoyAxis(int stick, int axis, int value);
    virtual void handleCommand(CommandSender* sender, int cmd, int data, int id);
    virtual void handleScreenChanged() {}

    /** The dialog wants all events except those that have some special function */
    virtual bool wantsEvents();

    /** The dialog wants all events, without exception */
    virtual bool wantsAllEvents();
	
    Widget* findWidget(int x, int y); // Find the widget at pos x,y if any

    ButtonWidget* addButton(int x, int y, const string& label, int cmd, char hotkey);

    void setResult(int result) { _result = result; }
    int getResult() const { return _result; }

  private:
    void buildFocusWidgetList(int id);

  protected:
    Widget* _mouseWidget;
    Widget* _focusedWidget;
    Widget* _dragWidget;
    bool    _visible;

  private:
    FocusList  _ourFocusList;
    TabWidget* _ourTab;

    int _result;
    int _focusID;
};

#endif
