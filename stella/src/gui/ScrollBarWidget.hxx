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
// $Id: ScrollBarWidget.hxx,v 1.1 2005-04-04 02:19:22 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#ifndef SCROLL_BAR_WIDGET_HXX
#define SCROLL_BAR_WIDGET_HXX

#include "GuiObject.hxx"
#include "Widget.hxx"
#include "Command.hxx"
#include "bspf.hxx"


class ScrollBarWidget : public Widget, public CommandSender
{
  protected:
    typedef enum {
      kNoPart,
      kUpArrowPart,
      kDownArrowPart,
      kSliderPart,
      kPageUpPart,
      kPageDownPart
    } Part;

  public:
    ScrollBarWidget(GuiObject* boss, Int32 x, Int32 y, Int32 w, Int32 h);

    virtual void handleMouseDown(Int32 x, Int32 y, Int32 button, Int32 clickCount);
    virtual void handleMouseUp(Int32 x, Int32 y, Int32 button, Int32 clickCount);
    virtual void handleMouseWheel(Int32 x, Int32 y, Int32 direction);
    virtual void handleMouseMoved(Int32 x, Int32 y, Int32 button);
    virtual void handleMouseEntered(Int32 button) { setFlags(WIDGET_HILITED); }
    virtual void handleMouseLeft(Int32 button)    { clearFlags(WIDGET_HILITED); _part = kNoPart; draw(); }
    virtual void handleTickle();

    // FIXME - this should be private, but then we also have to add accessors
    // for _numEntries, _entriesPerPage and _currentPos. This again leads to the question:
    // should these accessors force a redraw?
    void recalc();

  public:
    Int32 _numEntries;
    Int32 _entriesPerPage;
    Int32 _currentPos;

  protected:
    void drawWidget(bool hilite);
    void checkBounds(int old_pos);

  protected:
    Part  _part;
    Int32 _sliderHeight;
    Int32 _sliderPos;
    Part  _draggingPart;
    Int32 _sliderDeltaMouseDownPos;
};

#endif
