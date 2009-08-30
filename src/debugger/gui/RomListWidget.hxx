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
// Copyright (c) 1995-2009 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#ifndef ROM_LIST_WIDGET_HXX
#define ROM_LIST_WIDGET_HXX

class CheckboxWidget;
class ContextMenu;

#include "CheckListWidget.hxx"


/** RomListWidget */
class RomListWidget : public CheckListWidget
{
  friend class RomWidget;

  public:
    RomListWidget(GuiObject* boss, const GUI::Font& font,
                    int x, int y, int w, int h);
    virtual ~RomListWidget();

    void setList(StringList& label, StringList& bytes, StringList& disasm,
                 BoolArray& state);

  protected:
    void handleMouseDown(int x, int y, int button, int clickCount);
    bool handleEvent(Event::Type e);

    void drawWidget(bool hilite);
    GUI::Rect getLineRect() const;
    GUI::Rect getEditRect() const;

    bool tryInsertChar(char c, int pos);

  private:
    ContextMenu* myMenu;
    int myLabelWidth;
    int myBytesWidth;

    StringList myLabel, myDisasm;
};

#endif
