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
// $Id: GuiObject.hxx,v 1.1 2005-02-27 23:41:19 stephena Exp $
//============================================================================

#ifndef GUI_OBJECT_HXX
#define GUI_OBJECT_HXX

class Widget;

#include "bspf.hxx"

/**
  This is the base class for all GUI objects/widgets.
  
  @author  Stephen Anthony
  @version $Id: GuiObject.hxx,v 1.1 2005-02-27 23:41:19 stephena Exp $
*/
class GuiObject
{
  //friend class Widget;

  public:
    GuiObject(int x, int y, int w, int h) : _x(x), _y(y), _w(w), _h(h), _firstWidget(0) { }

    virtual Int16  getAbsX() const     { return _x; }
    virtual Int16  getAbsY() const     { return _y; }
    virtual Int16  getChildX() const   { return getAbsX(); }
    virtual Int16  getChildY() const   { return getAbsY(); }
    virtual uInt16 getWidth() const    { return _w; }
    virtual uInt16 getHeight() const   { return _h; }

    virtual bool isVisible() const = 0;

    virtual void draw() = 0;

  protected:
    Int16   _x, _y;
    uInt16  _w, _h;

    Widget* _firstWidget;

  protected:
    virtual void releaseFocus() = 0;
};

#endif
