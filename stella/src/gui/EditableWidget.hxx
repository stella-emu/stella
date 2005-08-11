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
// $Id: EditableWidget.hxx,v 1.6 2005-08-11 19:12:39 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#ifndef EDITABLE_WIDGET_HXX
#define EDITABLE_WIDGET_HXX

#include "Widget.hxx"
#include "Rect.hxx"

enum {
  kEditAcceptCmd = 'EDac',
  kEditCancelCmd = 'EDcl'
};

/**
 * Base class for widgets which need to edit text, like ListWidget and
 * EditTextWidget.
 */
class EditableWidget : public Widget, public CommandSender
{
  public:
    EditableWidget(GuiObject *boss, int x, int y, int w, int h);
    virtual ~EditableWidget();

    virtual void setEditString(const string& str);
    virtual const string& getEditString() const { return _editString; }

    bool isEditable() const	         { return _editable; }
    void setEditable(bool editable)  { _editable = editable; }

    virtual bool handleKeyDown(int ascii, int keycode, int modifiers);

    // We only want to focus this widget when we can edit its contents
    virtual bool wantsFocus() { return _editable; }

  protected:
    virtual void startEditMode() = 0;
    virtual void endEditMode() = 0;
    virtual void abortEditMode() = 0;

    virtual GUI::Rect getEditRect() const = 0;
    virtual int getCaretOffset() const;
    void drawCaret();
    bool setCaretPos(int newPos);
    bool adjustOffset();
	
    virtual bool tryInsertChar(char c, int pos);

  protected:
    bool   _editable;
    string _editString;

    bool  _caretVisible;
    int   _caretTime;
    int   _caretPos;

    bool  _caretInverse;

    int   _editScrollOffset;
};

#endif
