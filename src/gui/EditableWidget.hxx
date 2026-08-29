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
// Copyright (c) 1995-2026 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#ifndef EDITABLE_WIDGET_HXX
#define EDITABLE_WIDGET_HXX

#include <functional>

#include "Widget.hxx"
#include "Rect.hxx"
#include "ContextMenu.hxx"
#include "UndoHandler.hxx"

/**
 * Base class for widgets which need to edit text, like ListWidget and
 * EditTextWidget.
 *
 * Widgets wishing to enforce their own editing restrictions are able
 * to use a 'TextFilter' as described below.
 */
class EditableWidget : public Widget, public CommandSender
{
  public:
    /** Function used to test if a specified character can be inserted
        into the internal buffer */
    using TextFilter = std::function<bool(char)>;

    struct Cmd {
      static constexpr GuiCmd::Code
        Accept  = GuiCmd::of("EditableWidget.Accept"),
        Cancel  = GuiCmd::of("EditableWidget.Cancel"),
        Changed = GuiCmd::of("EditableWidget.Changed");
    };

  protected:
    /**
      Take this size.  Nothing builds a bare EditableWidget: it is the shared base
      of the edit field, the lists, the data grid and the pop-up, each of which
      derives its own size (from its character count, its items, its rows) and
      passes the result down.  A subclass with no size of its own -- a list, which
      is whatever the layout gives it -- passes none.
    */
    EditableWidget(GuiObject* boss, const GUI::Font& font,
                   int w = 0, int h = 0, string_view str = "");

  public:
    ~EditableWidget() override = default;

    // Replaces the text (filtered through the current TextFilter), resetting
    // the caret to its end and the undo history to just this value
    virtual void setText(string_view str, bool changed = false);
    void setMaxLen(int len) { _maxLen = len; }
    const string& getText() const { return _editString; }

    bool isEditable() const	{ return _editable; }
    // Whether the text differs from what it was when last committed/loaded
    bool isChanged() { return editString() != backupString(); }
    /**
      The gap a framed text box keeps between its frame and its first glyph,
      and so also the padding its width must allow either side.  Derived from
      the font, so it grows with the text instead of stepping; at the default
      9x18 font this is 3, the value these widgets used to hard-code.
    */
    static int textInset(const GUI::Font& font) {
      return font.getMaxCharWidth() / 3;
    }

    // Whether the text can currently be changed; hiliteBG additionally tints
    // the background while uneditable, rather than leaving it looking active
    virtual void setEditable(bool editable, bool hiliteBG = false);

    // Insert typed text / dispatch editing keys (move/select/delete/undo/cut/
    // copy/paste/accept/abort) to the private line-editing helpers below
    bool handleText(char text) override;
    bool handleKeyDown(StellaKey key, StellaMod mod) override;

    // We only want to focus this widget when we can edit its contents
    bool wantsFocus() const override { return _editable; }

    // Set filter used to test whether a character can be inserted
    void setTextFilter(const TextFilter& filter) { _filter = filter; }

    // Right mouse button opens the cut/copy/paste context menu; left starts a
    // drag-select, extended by handleMouseMoved() and ended by handleMouseUp()
    void handleMouseDown(int x, int y, MouseButton b, int clickCount) override;
    void handleMouseUp(int x, int y, MouseButton b, int clickCount) override;
    void handleMouseMoved(int x, int y) override;
    void tick() override;

  protected:
    // Reacts to a selection from the mouse context menu (cut/copy/paste)
    void handleCommand(CommandSender* sender, GuiCmd::Code cmd, int data, int id) override;

    // Horizontal scroll offset to subtract when mapping a caret position to
    // pixels; overridden by a subclass whose edit rect isn't at x=0
    virtual int caretOfs() const { return _editScrollOffset; }
    // Maps a pixel x-coordinate (relative to the edit rect) to a caret position
    int toCaretPos(int x) const;

    // Resets the caret blink / clears the undo history & selection
    void receivedFocusWidget() override;
    void lostFocusWidget() override;
    // Suppresses the tooltip while actively editing
    bool wantsToolTip() const override;

    // Enter/leave text-entry mode; endEditMode() keeps the change (commit()),
    // abortEditMode() discards it (abort()) -- a subclass overrides either to
    // add its own effect (e.g. ListWidget writing the row back on commit)
    virtual void startEditMode() { setFlags(Widget::Flag::WantsRawData);   }
    virtual void endEditMode()   {
      clearFlags(Widget::Flag::WantsRawData);
      commit();
    }
    virtual void abortEditMode()
    {
      clearFlags(Widget::Flag::WantsRawData);
      abort();
    }
    // Accepts the current text as the new backup (kept on endEditMode())
    void commit() { _backupString = _editString; }
    // Restores the text to the last committed backup (used by abortEditMode())
    void abort()  { setText(_backupString); }

    // The rectangle text is drawn/scrolled within, in this widget's own coordinates
    virtual Common::Rect getEditRect() const = 0;
    // Pixel offset of the caret from the edit rect's left edge, scroll-adjusted
    virtual int getCaretOffset() const;
    // Draws the caret and, if any, the selection highlight
    void drawCaretSelection();
    // Moves the caret to 'newPos', scrolling the text if needed to keep it visible
    bool setCaretPos(int newPos);
    // Moves the caret by 'direction' positions, extending the selection to match
    bool moveCaretPos(int direction);
    // Scrolls the text horizontally so the caret stays within the edit rect
    bool adjustOffset();

    // This method is used internally by child classes wanting to
    // access/edit the internal buffer
    string& editString() { return _editString; }
    string& backupString() { return _backupString; }
    // The currently selected substring of the text, or empty if none
    string selectString() const;
    void resetSelection() { _selectSize = 0; }
    // Horizontal scroll offset to apply when drawing (0 while not editable)
    int scrollOffset() const;

  private:
    // Line editing.  'direction' is -1 (toward the start) or +1 (toward the end);
    // addEdit says whether this is its own undo step or part of a larger one
    bool killChar(int direction, bool addEdit = true);
    bool killLine(int direction);
    bool killWord(int direction);
    // Moves the caret one word in 'direction'; extends the selection if 'select'
    bool moveWord(int direction, bool select);
    // Selects the whole word the caret is currently within
    bool markWord();

    // Deletes the current selection, if any
    bool killSelectedText(bool addEdit = true);
    // The lower/higher of (caret, caret + selection size)
    int selectStartPos() const;
    int selectEndPos() const;
    // Clipboard
    bool cutSelectedText();
    bool copySelectedText();
    bool pasteSelectedText();

    // Use the current TextFilter to insert a character into the
    // internal buffer
    bool tryInsertChar(char c, int pos);

    // The right-click context menu (cut/copy/paste), created on first use
    ContextMenu& mouseMenu();

  private:
    // Right-click context menu (cut/copy/paste); created lazily by mouseMenu()
    unique_ptr<ContextMenu> myMouseMenu;
    // True while the mouse is held down, extending the selection as it moves
    bool    _isDragging{false};

    // Whether the text can currently be changed (see setEditable())
    bool   _editable{true};
    // The text as currently edited
    string _editString;
    // The text before the current edit, restored by abort()
    string _backupString;
    // Maximum length of the text, or 0 for unlimited
    int    _maxLen{0};
    // Undo/redo history for the edited text
    unique_ptr<UndoHandler> myUndoHandler;

    // Caret position, as an index into _editString
    int    _caretPos{0};
    // Blink timer and current on/off phase for the caret (see tick())
    int    _caretTimer{0};
    bool   _caretEnabled{true};

    // Size of current selected text
    //    0 = no selection
    //   <0 = selected left of caret
    //   >0 = selected right of caret
    int    _selectSize{0};

  protected:
    // Horizontal scroll offset so the caret stays visible in a narrower box
    int   _editScrollOffset{0};
    // Whether text entry is currently active (vs. e.g. a list row merely selected)
    bool  _editMode{true};
    // Vertical draw offset applied to the caret/selection; a subclass whose
    // text isn't at the widget's own baseline overrides it (see RomListWidget)
    int   _dyText{0};

  private:
    // Tests whether a character may be inserted (see setTextFilter())
    TextFilter _filter;

  private:
    // Following constructors and assignment operators not supported
    EditableWidget() = delete;
    EditableWidget(const EditableWidget&) = delete;
    EditableWidget(EditableWidget&&) = delete;
    EditableWidget& operator=(const EditableWidget&) = delete;
    EditableWidget& operator=(EditableWidget&&) = delete;
};

#endif  // EDITABLE_WIDGET_HXX
