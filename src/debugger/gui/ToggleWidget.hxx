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

#ifndef TOGGLE_WIDGET_HXX
#define TOGGLE_WIDGET_HXX

#include "Widget.hxx"
#include "Command.hxx"

/* ToggleWidget */
class ToggleWidget : public Widget, public CommandSender
{
  public:
    // Commands emitted by this commandsender
    enum {
      kItemDataChangedCmd   = 'TWch',
      kSelectionChangedCmd  = 'TWsc'
    };

  public:
    ToggleWidget(GuiObject* boss, const GUI::Font& font,
                 int x, int y, int cols, int rows);
    virtual ~ToggleWidget();

    const BoolArray& getState()    { return _stateList; }
    bool getSelectedState() const  { return _stateList[_selectedItem]; }

    virtual bool wantsFocus() { return true; }

    int colWidth() const { return _colWidth; }
    void setEditable(bool editable) { _editable = editable; }

  protected:

  protected:
    int  _rows;
    int  _cols;
    int  _currentRow;
    int  _currentCol;
    int  _rowHeight;   // explicitly set in child classes
    int  _colWidth;    // explicitly set in child classes
    int  _selectedItem;
    bool _editable;

    BoolArray  _stateList;
    BoolArray  _changedList;

  private:
    void drawWidget(bool hilite) override = 0;
    int findItem(int x, int y);

    void handleMouseDown(int x, int y, int button, int clickCount) override;
    void handleMouseUp(int x, int y, int button, int clickCount) override;
    bool handleKeyDown(StellaKey key, StellaMod mod) override;
    void handleCommand(CommandSender* sender, int cmd, int data, int id) override;

    // Following constructors and assignment operators not supported
    ToggleWidget() = delete;
    ToggleWidget(const ToggleWidget&) = delete;
    ToggleWidget(ToggleWidget&&) = delete;
    ToggleWidget& operator=(const ToggleWidget&) = delete;
    ToggleWidget& operator=(ToggleWidget&&) = delete;
};

#endif
