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
// Copyright (c) 1995-2006 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: TiaZoomWidget.hxx,v 1.5 2006-12-08 16:49:18 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#ifndef TIA_ZOOM_WIDGET_HXX
#define TIA_ZOOM_WIDGET_HXX

class GuiObject;
class ContextMenu;

#include "Widget.hxx"
#include "Command.hxx"


class TiaZoomWidget : public Widget, public CommandSender
{
  public:
    TiaZoomWidget(GuiObject *boss, const GUI::Font& font, int x, int y);
    virtual ~TiaZoomWidget();

    void loadConfig();
    void setPos(int x, int y);

  protected:
    void handleMouseDown(int x, int y, int button, int clickCount);
    bool handleKeyDown(int ascii, int keycode, int modifiers);
    void handleCommand(CommandSender* sender, int cmd, int data, int id);

    void drawWidget(bool hilite);
    bool wantsFocus() { return true; }

  private:
    void zoom(int level);
    void recalc();

  private:
    ContextMenu* myMenu;

    int myZoomLevel;
    int myNumCols, myNumRows;
    int myXoff, myYoff;
    int myXCenter, myYCenter;
};

#endif
