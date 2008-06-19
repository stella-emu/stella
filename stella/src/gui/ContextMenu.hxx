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
// Copyright (c) 1995-2008 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: ContextMenu.hxx,v 1.3 2008-06-19 12:01:31 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#ifndef CONTEXT_MENU_HXX
#define CONTEXT_MENU_HXX

#include "bspf.hxx"

#include "Array.hxx"
#include "Command.hxx"
#include "Dialog.hxx"

enum {
  kCMenuItemSelectedCmd = 'CMsl'
};

/**
 * Popup context menu which, when clicked, "pop up" a list of items and
 * lets the user pick on of them.
 *
 * Implementation wise, when the user selects an item, then the given 'cmd'
 * is broadcast, with data being equal to the tag value of the selected entry.
 */
class ContextMenu : public Dialog, public CommandSender
{
  public:
    ContextMenu(GuiObject* boss, const GUI::Font& font,
                const StringList& items, int cmd = 0);
	virtual ~ContextMenu();

    /** Show context menu onscreen at the specified coordinates */
    void show(uInt32 x, uInt32 y, int item = -1);

    /** Select the entry at the given index. */
    void setSelected(int item);
	
    /** Select the first entry matching the given name. */
    void setSelected(const string& name);

    /** Select the highest/last entry in the internal list. */
    void setSelectedMax();

    /** Clear selection (reset to default). */
    void clearSelection();

    /** Accessor methods for the currently selected item. */
    int getSelected() const;
    const string& getSelectedString() const;

    /** This dialog uses its own positioning, so we override Dialog::center() */
    void center() { }

  protected:
    void handleMouseDown(int x, int y, int button, int clickCount);
    void handleMouseWheel(int x, int y, int direction);
    void handleMouseMoved(int x, int y, int button);
    void handleKeyDown(int ascii, int keycode, int modifiers);  // Scroll through entries with arrow keys etc
    void handleJoyDown(int stick, int button);
    void handleJoyAxis(int stick, int axis, int value);
    bool handleJoyHat(int stick, int hat, int value);
    void handleEvent(Event::Type e);

    void drawDialog();

  private:
    void drawMenuEntry(int entry, bool hilite);
	
    int findItem(int x, int y) const;
    void drawCurrentSelection(int item);
	
    void moveUp();
    void moveDown();

    void sendSelection();

  private:
    StringList _entries;

    int _currentItem;
    int _selectedItem;
    int _rowHeight;

    bool _twoColumns;
    int  _entriesPerColumn;

    const GUI::Font* _font;
    int _cmd;
};

#endif
