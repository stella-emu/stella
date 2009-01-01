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
// $Id: EditTextWidget.hxx,v 1.9 2009-01-01 18:13:38 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#ifndef EDIT_TEXT_WIDGET_HXX
#define EDIT_TEXT_WIDGET_HXX

#include "Rect.hxx"
#include "EditableWidget.hxx"


/* EditTextWidget */
class EditTextWidget : public EditableWidget
{
  public:
    EditTextWidget(GuiObject* boss, const GUI::Font& font,
                   int x, int y, int w, int h, const string& text);

    void setEditString(const string& str);

    virtual void handleMouseDown(int x, int y, int button, int clickCount);

  protected:
    void drawWidget(bool hilite);
    void lostFocusWidget();

    void startEditMode();
    void endEditMode();
    void abortEditMode();

    GUI::Rect getEditRect() const;

  protected:
    string _backupString;
    int    _editable;
};

#endif
