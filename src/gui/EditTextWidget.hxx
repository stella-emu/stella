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
// Copyright (c) 1995-2015 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
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
                   int x, int y, int w, int h, const string& text = "");

    void setText(const string& str, bool changed = false) override;

  protected:
    void drawWidget(bool hilite) override;
    void lostFocusWidget() override;

    void startEditMode() override;
    void endEditMode() override;
    void abortEditMode() override;

    GUI::Rect getEditRect() const override;

    void handleMouseDown(int x, int y, int button, int clickCount) override;

  protected:
    string _backupString;
    bool   _changed;

  private:
    // Following constructors and assignment operators not supported
    EditTextWidget() = delete;
    EditTextWidget(const EditTextWidget&) = delete;
    EditTextWidget(EditTextWidget&&) = delete;
    EditTextWidget& operator=(const EditTextWidget&) = delete;
    EditTextWidget& operator=(EditTextWidget&&) = delete;
};

#endif
