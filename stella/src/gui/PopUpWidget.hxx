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
// $Id: PopUpWidget.hxx,v 1.13 2006-05-04 17:45:25 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#ifndef POPUP_WIDGET_HXX
#define POPUP_WIDGET_HXX

class GUIObject;
class PopUpDialog;

#include "Dialog.hxx"

#include "Widget.hxx"
#include "Command.hxx"
#include "Array.hxx"
#include "GuiUtils.hxx"

#include "bspf.hxx"

/**
 * Popup or dropdown widget which, when clicked, "pop up" a list of items and
 * lets the user pick on of them.
 *
 * Implementation wise, when the user selects an item, then a kPopUpItemSelectedCmd 
 * is broadcast, with data being equal to the tag value of the selected entry.
 */
class PopUpWidget : public Widget, public CommandSender
{
  friend class PopUpDialog;

  struct Entry {
    string name;
    int tag;
  };

  typedef Common::Array<Entry> EntryList;

  protected:
    EntryList _entries;
    int       _selectedItem;
    string    _label;
    int       _labelWidth;

  public:
    PopUpWidget(GuiObject* boss, const GUI::Font& font,
                int x, int y, int w, int h,
                const string& label, int labelWidth = 0, int cmd = 0);
    ~PopUpWidget();

    void handleMouseDown(int x, int y, int button, int clickCount);
    bool handleEvent(Event::Type e);

    bool wantsFocus()  { return true; }

    void appendEntry(const string& entry, int tag = (int)-1);
    void clearEntries();

    /** Select the entry at the given index. */
    void setSelected(int item);
	
    /** Select the first entry matching the given tag. */
    void setSelectedTag(int tag);

    int getSelected() const
      { return _selectedItem; }
    int getSelectedTag() const
      { return (_selectedItem >= 0) ? _entries[_selectedItem].tag : (int)-1; }
    const string& getSelectedString() const
      { return (_selectedItem >= 0) ? _entries[_selectedItem].name : EmptyString; }

  protected:
    void drawWidget(bool hilite);

  protected:
    int	_cmd;

  private:
    PopUpDialog* myPopUpDialog;
    int myArrowsY;
    int myTextY;
};

//
// PopUpDialog
//
class PopUpDialog : public Dialog
{
  friend class PopUpWidget;

  public:
    PopUpDialog(PopUpWidget* boss, int clickX, int clickY);
	
    void drawDialog();

    void handleMouseDown(int x, int y, int button, int clickCount);
    void handleMouseWheel(int x, int y, int direction); // Scroll through entries with scroll wheel
    void handleMouseMoved(int x, int y, int button);    // Redraw selections depending on mouse position
    void handleKeyDown(int ascii, int keycode, int modifiers);  // Scroll through entries with arrow keys etc

  protected:
    void drawMenuEntry(int entry, bool hilite);

    void recalc();
    int findItem(int x, int y) const;
    void setSelection(int item);
    bool isMouseDown();
	
    void moveUp();
    void moveDown();

  private:
    void sendSelection();
    void cancelSelection();
    void handleEvent(Event::Type e);

  protected:
    PopUpWidget* _popUpBoss;
    int          _clickX, _clickY;
    uInt8*       _buffer;
    int          _selection;
    int          _oldSelection;
    int          _openTime;
    bool         _twoColumns;
    int          _entriesPerColumn;
};

#endif
