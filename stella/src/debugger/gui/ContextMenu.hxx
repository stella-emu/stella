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
// $Id: ContextMenu.hxx,v 1.2 2005-08-31 22:34:43 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#ifndef CONTEXT_MENU_HXX
#define CONTEXT_MENU_HXX

#include "Dialog.hxx"
#include "Command.hxx"
#include "Array.hxx"
#include "GuiUtils.hxx"
#include "bspf.hxx"

enum {
  kCMenuItemSelectedCmd = 'CMsl'
};

/**
 * Popup context menu which, when clicked, "pop up" a list of items and
 * lets the user pick on of them.
 *
 * Implementation wise, when the user selects an item, then a kCMenuItemSelectedCmd
 * is broadcast, with data being equal to the tag value of the selected entry.
 */
class ContextMenu : public Dialog, public CommandSender
{
  public:
    ContextMenu(GuiObject* boss, const GUI::Font& font);
	virtual ~ContextMenu();

    /** Show context menu onscreen */
    void show();

    void setList(const StringList& list);
    const string& getSelectedString() const;
    int getSelected() const { return _selectedItem; }

  protected:
    void handleMouseDown(int x, int y, int button, int clickCount);
    void handleMouseWheel(int x, int y, int direction);
    void handleMouseMoved(int x, int y, int button);
    void handleKeyDown(int ascii, int keycode, int modifiers);

    void drawDialog();

  private:
    void drawMenuEntry(int entry, bool hilite);
	
    int findItem(int x, int y) const;
    void setSelection(int item);
	
    void moveUp();
    void moveDown();

    void sendSelection();

  protected:
    StringList _entries;

    int _selectedItem;
    int _rowHeight;
};

#endif
