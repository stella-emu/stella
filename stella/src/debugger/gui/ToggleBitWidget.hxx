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
// Copyright (c) 1995-2007 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: ToggleBitWidget.hxx,v 1.3 2007-01-01 18:04:44 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#ifndef TOGGLE_BIT_WIDGET_HXX
#define TOGGLE_BIT_WIDGET_HXX

class StringList;

#include "ToggleWidget.hxx"

/* ToggleBitWidget */
class ToggleBitWidget : public ToggleWidget
{
  public:
    ToggleBitWidget(GuiObject* boss, const GUI::Font& font,
                    int x, int y, int cols, int rows, int colchars = 1);
    virtual ~ToggleBitWidget();

    void setList(const StringList& off, const StringList& on);
    void setState(const BoolArray& state, const BoolArray& changed);

  protected:
    void drawWidget(bool hilite);

  protected:
    StringList  _offList;
    StringList  _onList;
};

#endif
