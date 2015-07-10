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

#ifndef CART_RAM_WIDGET_HXX
#define CART_RAM_WIDGET_HXX

class GuiObject;
class InputTextDialog;
class ButtonWidget;
class DataGridWidget;

#include "Base.hxx"
#include "Font.hxx"
#include "CartDebugWidget.hxx"
#include "Command.hxx"
#include "DataGridWidget.hxx"
#include "Debugger.hxx"
#include "RomWidget.hxx"
#include "Widget.hxx"
#include "EditTextWidget.hxx"
#include "StringListWidget.hxx"
#include "StringParser.hxx"

class CartRamWidget : public Widget, public CommandSender
{
  public:
    CartRamWidget(GuiObject* boss, const GUI::Font& lfont,
                  const GUI::Font& nfont,
                  int x, int y, int w, int h, CartDebugWidget& cartDebug);

    virtual ~CartRamWidget() { };

  private:
    void loadConfig() override;
    void handleCommand(CommandSender* sender, int cmd, int data, int id) override;
    void fillGrid(bool updateOld);
  
    void showInputBox(int cmd);
    string doSearch(const string& str);
    string doCompare(const string& str);
    void doRestart(); 
    void showSearchResults();

  protected:
    // Font used for 'normal' text; _font is for 'label' text
    const GUI::Font& _nfont;

    // These will be needed by most of the child classes;
    // we may as well make them protected variables
    int myFontWidth, myFontHeight, myLineHeight, myButtonHeight;

  private:
    enum {
      kUndoCmd     = 'RWud',
      kRevertCmd   = 'RWrv',
      kSearchCmd   = 'RWse',
      kCmpCmd      = 'RWcp',
      kRestartCmd  = 'RWrs',
      kSValEntered = 'RWsv',
      kCValEntered = 'RWcv'
    };

    int myUndoAddress;
    int myUndoValue;

    CartDebugWidget& myCart;
    StringListWidget* myDesc;

    DataGridWidget* myRamGrid;
    StaticTextWidget* myRamStart;
    StaticTextWidget* myRamLabels[16];
    EditTextWidget* myBinValue;
    EditTextWidget* myDecValue;
    EditTextWidget* myLabel;

    uInt32 myCurrentRamBank;
    uInt32 myRamSize;
    uInt32 myPageSize;
    uInt32 myNumRows;

    ButtonWidget* myRevertButton;
    ButtonWidget* myUndoButton;
    ButtonWidget* mySearchButton;
    ButtonWidget* myCompareButton;
    ButtonWidget* myRestartButton;

    InputTextDialog* myInputBox;  

    ByteArray myOldValueList;
    IntArray mySearchAddr;
    IntArray mySearchValue;
    BoolArray mySearchState;  

  private:
    // Following constructors and assignment operators not supported
    CartRamWidget() = delete;
    CartRamWidget(const CartRamWidget&) = delete;
    CartRamWidget(CartRamWidget&&) = delete;
    CartRamWidget& operator=(const CartRamWidget&) = delete;
    CartRamWidget& operator=(CartRamWidget&&) = delete;
};

#endif
