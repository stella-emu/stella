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

#ifndef RAM_WIDGET_HXX
#define RAM_WIDGET_HXX

class GuiObject;
class InputTextDialog;
class ButtonWidget;
class DataGridWidget;
class DataGridOpsWidget;
class EditTextWidget;
class StaticTextWidget;

#include "Widget.hxx"
#include "Command.hxx"

class RamWidget : public Widget, public CommandSender
{
  public:
    RamWidget(GuiObject* boss, const GUI::Font& lfont, const GUI::Font& nfont,
              int x, int y, int w, int h,
              uInt32 ramsize, uInt32 numrows, uInt32 pagesize);
    virtual ~RamWidget();

    void loadConfig() override;
    void setOpsWidget(DataGridOpsWidget* w);
    void handleCommand(CommandSender* sender, int cmd, int data, int id) override;

  private:
    // To be implemented by derived classes
    virtual uInt8 getValue(int addr) const = 0;
    virtual void setValue(int addr, uInt8 value) = 0;
    virtual string getLabel(int addr) const = 0;

    virtual void fillList(uInt32 start, uInt32 size,
              IntArray& alist, IntArray& vlist, BoolArray& changed) const = 0;
    virtual uInt32 readPort(uInt32 start) const = 0;
    virtual const ByteArray& currentRam(uInt32 start) const = 0;

  private:
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
      kCValEntered = 'RWcv',
      kRamHexID,
      kRamDecID,
      kRamBinID
    };

    uInt32 myUndoAddress, myUndoValue;
    uInt32 myCurrentRamBank;
    uInt32 myRamSize;
    uInt32 myNumRows;
    uInt32 myPageSize;

    unique_ptr<InputTextDialog> myInputBox;

    StaticTextWidget* myRamStart;
    StaticTextWidget* myRamLabels[16];

    DataGridWidget* myRamGrid;
    DataGridWidget* myDecValue;
    DataGridWidget* myBinValue;
    EditTextWidget* myLabel;

    ButtonWidget* myRevertButton;
    ButtonWidget* myUndoButton;
    ButtonWidget* mySearchButton;
    ButtonWidget* myCompareButton;
    ButtonWidget* myRestartButton;

    ByteArray myOldValueList;
    IntArray mySearchAddr;
    IntArray mySearchValue;
    BoolArray mySearchState;

  private:
    // Following constructors and assignment operators not supported
    RamWidget() = delete;
    RamWidget(const RamWidget&) = delete;
    RamWidget(RamWidget&&) = delete;
    RamWidget& operator=(const RamWidget&) = delete;
    RamWidget& operator=(RamWidget&&) = delete;
};

#endif
