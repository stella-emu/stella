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
// $Id: EditableWidget.hxx,v 1.1 2005-06-14 01:11:48 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#ifndef EDITABLE_WIDGET_HXX
#define EDITABLE_WIDGET_HXX

#include "Widget.hxx"
#include "Rect.hxx"

/**
 * Base class for widgets which need to edit text, like ListWidget and
 * EditTextWidget.
 */
class EditableWidget : public Widget
{
  public:
    EditableWidget(GuiObject *boss, int x, int y, int w, int h);
    virtual ~EditableWidget();

    virtual void setEditString(const string& str);
    virtual const string& getEditString() const { return _editString; }

    virtual bool handleKeyDown(int ascii, int keycode, int modifiers);

  protected:
    virtual void startEditMode() = 0;
    virtual void endEditMode() = 0;
    virtual void abortEditMode() = 0;

    virtual GUI::Rect getEditRect() const = 0;
    virtual int getCaretOffset() const;
    void drawCaret(bool erase);
    bool setCaretPos(int newPos);
    bool adjustOffset();
	
    virtual bool tryInsertChar(char c, int pos);

  protected:
    string _editString;

    bool  _caretVisible;
    int   _caretTime;
    int   _caretPos;

    bool  _caretInverse;

    int   _editScrollOffset;
};

#endif
