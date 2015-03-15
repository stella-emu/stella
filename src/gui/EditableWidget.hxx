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

#ifndef EDITABLE_WIDGET_HXX
#define EDITABLE_WIDGET_HXX

#include <functional>

#include "Widget.hxx"
#include "Rect.hxx"

/**
 * Base class for widgets which need to edit text, like ListWidget and
 * EditTextWidget.
 */
class EditableWidget : public Widget, public CommandSender
{
  public:
    /** Function used by 'tryInsertChar' to test validity of a character */
    using TextFilter = std::function<bool(char)>;

    enum {
      kAcceptCmd  = 'EDac',
      kCancelCmd  = 'EDcl',
      kChangedCmd = 'EDch'
    };

  public:
    EditableWidget(GuiObject *boss, const GUI::Font& font,
                   int x, int y, int w, int h, const string& str = "");
    virtual ~EditableWidget();

    virtual void setText(const string& str, bool changed = false);
    virtual const string& getText() const { return _editString; }

    bool isEditable() const	 { return _editable; }
    void setEditable(bool editable);

    virtual bool handleText(char text);
    virtual bool handleKeyDown(StellaKey key, StellaMod mod);

    // We only want to focus this widget when we can edit its contents
    virtual bool wantsFocus() { return _editable; }

    // Set filter used by 'tryInsertChar'
    void setTextFilter(TextFilter& filter) { _filter = filter; }

  protected:
    virtual void startEditMode() { setFlags(WIDGET_WANTS_RAWDATA);   }
    virtual void endEditMode()   { clearFlags(WIDGET_WANTS_RAWDATA); }
    virtual void abortEditMode() { clearFlags(WIDGET_WANTS_RAWDATA); }

    virtual GUI::Rect getEditRect() const = 0;
    virtual int getCaretOffset() const;
    void drawCaret();
    bool setCaretPos(int newPos);
    bool adjustOffset();
	
    // This method will use the current TextFilter to insert a character
    // Note that classes which override this method will no longer use the
    // current TextFilter, and will assume all responsibility for filtering
    virtual bool tryInsertChar(char c, int pos);

  private:
    // Line editing
    bool specialKeys(StellaKey key);
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

  private:
    TextFilter _filter;
};

#endif
