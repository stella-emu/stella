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
// Copyright (c) 1995-2010 by Bradford W. Mott and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#ifndef EDITABLE_WIDGET_HXX
#define EDITABLE_WIDGET_HXX

#include "Widget.hxx"
#include "Rect.hxx"

enum {
  kEditAcceptCmd  = 'EDac',
  kEditCancelCmd  = 'EDcl',
  kEditChangedCmd = 'EDch'
};

/**
 * Base class for widgets which need to edit text, like ListWidget and
 * EditTextWidget.
 */
class EditableWidget : public Widget, public CommandSender
{
  public:
    EditableWidget(GuiObject *boss, const GUI::Font& font,
                   int x, int y, int w, int h);
    virtual ~EditableWidget();

    virtual void setEditString(const string& str);
    virtual const string& getEditString() const { return _editString; }

    bool isEditable() const	 { return _editable; }
    void setEditable(bool editable);

    virtual bool handleKeyDown(int ascii, int keycode, int modifiers);

    // We only want to focus this widget when we can edit its contents
    virtual bool wantsFocus() { return _editable; }

  protected:
    virtual void startEditMode() { setFlags(WIDGET_WANTS_RAWDATA);   }
    virtual void endEditMode()   { clearFlags(WIDGET_WANTS_RAWDATA); }
    virtual void abortEditMode() { clearFlags(WIDGET_WANTS_RAWDATA); }

    virtual GUI::Rect getEditRect() const = 0;
    virtual int getCaretOffset() const;
    void drawCaret();
    bool setCaretPos(int newPos);
    bool adjustOffset();
	
    virtual bool tryInsertChar(char c, int pos);

  private:
    // Line editing
    bool specialKeys(int ascii, int keycode);
    bool killChar(int direction);
    bool killLine(int direction);
    bool killLastWord();
    bool moveWord(int direction);

    // Clipboard
    void copySelectedText();
    void pasteSelectedText();

  protected:
    bool   _editable;
    string _editString;

    bool  _caretVisible;
    int   _caretTime;
    int   _caretPos;

    bool  _caretInverse;

    int   _editScrollOffset;

    static string _clippedText;
};

#endif
