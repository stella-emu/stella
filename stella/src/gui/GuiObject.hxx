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
// $Id: GuiObject.hxx,v 1.4 2005-03-14 04:08:15 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#ifndef GUI_OBJECT_HXX
#define GUI_OBJECT_HXX

class OSystem;
class Widget;
class Menu;

#include "Command.hxx"
#include "bspf.hxx"

/**
  This is the base class for all GUI objects/widgets.
  
  @author  Stephen Anthony
  @version $Id: GuiObject.hxx,v 1.4 2005-03-14 04:08:15 stephena Exp $
*/
class GuiObject : public CommandReceiver
{
  friend class Widget;
  friend class Menu;

  public:
    GuiObject(OSystem* osystem, int x, int y, int w, int h)
        : myOSystem(osystem),
          _x(x),
          _y(y),
          _w(w),
          _h(h),
          _firstWidget(0) { }

    OSystem* instance() { return myOSystem; }

    virtual Int16  getAbsX() const     { return _x; }
    virtual Int16  getAbsY() const     { return _y; }
    virtual Int16  getChildX() const   { return getAbsX(); }
    virtual Int16  getChildY() const   { return getAbsY(); }
    virtual uInt16 getWidth() const    { return _w; }
    virtual uInt16 getHeight() const   { return _h; }

    virtual bool isVisible() const = 0;
    virtual void draw() = 0;

  protected:
    OSystem* myOSystem;

    Int16   _x, _y;
    uInt16  _w, _h;

    Widget* _firstWidget;

  protected:
    virtual void releaseFocus() = 0;
};

#endif
