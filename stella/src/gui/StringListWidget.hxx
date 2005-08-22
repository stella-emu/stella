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
// $Id: StringListWidget.hxx,v 1.1 2005-08-22 23:09:13 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#ifndef STRING_LIST_WIDGET_HXX
#define STRING_LIST_WIDGET_HXX

#include "ListWidget.hxx"

enum NumberingMode {
  kListNumberingOff  = -1,
  kListNumberingZero = 0,
  kListNumberingOne  = 1
};

/** StringListWidget */
class StringListWidget : public ListWidget
{
  public:
    StringListWidget(GuiObject* boss, const GUI::Font& font,
                     int x, int y, int w, int h);
    virtual ~StringListWidget();

    void setNumberingMode(NumberingMode numberingMode) { _numberingMode = numberingMode; }

  protected:
    void drawWidget(bool hilite);
    GUI::Rect getEditRect() const;

  protected:
    NumberingMode _numberingMode;
};

#endif
