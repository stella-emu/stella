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
// $Id: TabWidget.hxx,v 1.5 2005-06-16 00:56:00 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#ifndef TAB_WIDGET_HXX
#define TAB_WIDGET_HXX

#include "GuiUtils.hxx"
#include "Widget.hxx"
#include "Command.hxx"
#include "Array.hxx"
#include "bspf.hxx"

class TabWidget : public Widget, public CommandSender
{
  struct Tab {
    string title;
    Widget* firstWidget;
    Widget* activeWidget;
  };
  typedef GUI::Array<Tab> TabList;

  public:
    TabWidget(GuiObject *boss, int x, int y, int w, int h);
    ~TabWidget();

    virtual int getChildY() const;

// use Dialog::releaseFocus() when changing to another tab

// Problem: how to add items to a tab?
// First off, widget should allow non-dialog bosses, (i.e. also other widgets)
// Could add a common base class for Widgets and Dialogs.
// Then you add tabs using the following method, which returns a unique ID
    int addTab(const string& title);
// Maybe we need to remove tabs again? Hm
    //void removeTab(int tabID);
// Setting the active tab:
    void setActiveTab(int tabID);
    void cycleTab(int direction);
    void cycleWidget(int direction);
// setActiveTab changes the value of _firstWidget. This means Widgets added afterwards
// will be added to the active tab.
    void setActiveWidget(int tabID, Widget* widID);

    virtual void handleMouseDown(int x, int y, int button, int clickCount);
    virtual bool handleKeyDown(int ascii, int keycode, int modifiers);
    virtual void handleCommand(CommandSender* sender, int cmd, int data);

  protected:
    virtual void drawWidget(bool hilite);
    virtual Widget *findWidget(int x, int y);

  protected:
    int     _activeTab;
    TabList _tabs;
    int     _tabWidth;

  private:
    void box(int x, int y, int width, int height,
             OverlayColor colorA, OverlayColor colorB, bool omitBottom);
};

#endif
