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
// $Id: PopUpWidget.hxx,v 1.2 2005-03-26 04:19:56 stephena Exp $
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

enum {
  kPopUpItemSelectedCmd = 'POPs'
};

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
    uInt32 tag;
  };

  typedef Array<Entry> EntryList;

  protected:
    EntryList _entries;
    Int32     _selectedItem;
    string    _label;
	uInt32    _labelWidth;

  public:
    PopUpWidget(GuiObject* boss, Int32 x, Int32 y, Int32 w, Int32 h,
                const string& label, uInt32 labelWidth = 0, Int32 cmd = 0);

    void handleMouseDown(Int32 x, Int32 y, Int32 button, Int32 clickCount);

    void appendEntry(const string& entry, uInt32 tag = (uInt32)-1);
    void clearEntries();

    /** Select the entry at the given index. */
    void setSelected(Int32 item);
	
    /** Select the first entry matching the given tag. */
    void setSelectedTag(uInt32 tag);

    Int32 getSelected() const               { return _selectedItem; }
    uInt32 getSelectedTag() const           { return (_selectedItem >= 0) ? _entries[_selectedItem].tag : (uInt32)-1; }
    const string& getSelectedString() const { return (_selectedItem >= 0) ? _entries[_selectedItem].name : EmptyString; }

  protected:
    void drawWidget(bool hilite);

  protected:
    uInt32	_cmd;

  private:
    PopUpDialog* myPopUpDialog;
};

//
// PopUpDialog
//
class PopUpDialog : public Dialog
{
  protected:
    PopUpWidget* _popUpBoss;
    Int32        _clickX, _clickY;
    uInt8*       _buffer;
    Int32        _selection;
    uInt32       _openTime;

  public:
    PopUpDialog(PopUpWidget* boss, Int32 clickX, Int32 clickY);
	
    void drawDialog();

    void handleMouseDown(Int32 x, Int32 y, Int32 button, Int32 clickCount);
    void handleMouseWheel(Int32 x, Int32 y, Int32 direction);           // Scroll through entries with scroll wheel
    void handleMouseMoved(Int32 x, Int32 y, Int32 button);              // Redraw selections depending on mouse position
    void handleKeyDown(uInt16 ascii, Int32 keycode, Int32 modifiers);   // Scroll through entries with arrow keys etc.

  protected:
    void drawMenuEntry(Int32 entry, bool hilite);
	
    Int32 findItem(Int32 x, Int32 y) const;
    void setSelection(Int32 item);
    bool isMouseDown();
	
    void moveUp();
    void moveDown();
};

#endif
