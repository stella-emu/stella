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
// Copyright (c) 1995-2015 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
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
    TiaZoomWidget(GuiObject *boss, const GUI::Font& font,
                  int x, int y, int w, int h);
    virtual ~TiaZoomWidget();

    void loadConfig() override;
    void setPos(int x, int y);

  private:
    void zoom(int level);
    void recalc();

    void handleMouseDown(int x, int y, int button, int clickCount) override;
    void handleMouseUp(int x, int y, int button, int clickCount) override;
    void handleMouseWheel(int x, int y, int direction) override;
    void handleMouseMoved(int x, int y, int button) override;
    void handleMouseLeft(int button) override;
    bool handleEvent(Event::Type event) override;
    void handleCommand(CommandSender* sender, int cmd, int data, int id) override;

    void drawWidget(bool hilite) override;
    bool wantsFocus() { return true; }

  private:
    unique_ptr<ContextMenu> myMenu;

    int myZoomLevel;
    int myNumCols, myNumRows;
    int myXOff, myYOff;

    bool myMouseMoving;
    int myXClick, myYClick;

  private:
    // Following constructors and assignment operators not supported
    TiaZoomWidget() = delete;
    TiaZoomWidget(const TiaZoomWidget&) = delete;
    TiaZoomWidget(TiaZoomWidget&&) = delete;
    TiaZoomWidget& operator=(const TiaZoomWidget&) = delete;
    TiaZoomWidget& operator=(TiaZoomWidget&&) = delete;
};

#endif
