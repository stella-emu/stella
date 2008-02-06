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
// Copyright (c) 1995-2008 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: TiaOutputWidget.hxx,v 1.6 2008-02-06 13:45:20 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#ifndef TIA_OUTPUT_WIDGET_HXX
#define TIA_OUTPUT_WIDGET_HXX

class GuiObject;
class ContextMenu;
class TiaZoomWidget;

#include "Widget.hxx"
#include "Command.hxx"


class TiaOutputWidget : public Widget, public CommandSender
{
  public:
    TiaOutputWidget(GuiObject *boss, const GUI::Font& font,
                    int x, int y, int w, int h);
    virtual ~TiaOutputWidget();

    void loadConfig();
    void setZoomWidget(TiaZoomWidget* w) { myZoom = w; }

// Eventually, these methods will enable access to the onscreen TIA image
// For example, clicking an area may cause an action
// (fill to this scanline, etc).
/*
    virtual void handleMouseUp(int x, int y, int button, int clickCount);
    virtual void handleMouseWheel(int x, int y, int direction);
    virtual bool handleKeyDown(int ascii, int keycode, int modifiers);
    virtual bool handleKeyUp(int ascii, int keycode, int modifiers);
*/
    void advanceScanline(int lines);
    void advance(int frames);

  protected:
    void handleMouseDown(int x, int y, int button, int clickCount);
    void handleCommand(CommandSender* sender, int cmd, int data, int id);

    void drawWidget(bool hilite);
    bool wantsFocus() { return false; }

  private:
    ContextMenu*   myMenu;
    TiaZoomWidget* myZoom;

    int myClickX, myClickY;
};

#endif
