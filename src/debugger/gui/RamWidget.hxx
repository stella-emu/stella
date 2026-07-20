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

#ifndef RAM_WIDGET_HXX
#define RAM_WIDGET_HXX

class GuiObject;
class ButtonWidget;
class DataGridWidget;
class DataGridOpsWidget;
class DataGridRamWidget;
class EditTextWidget;
class StaticTextWidget;
class InputTextDialog;

#include "Widget.hxx"
#include "Command.hxx"

namespace GUI {
  class BoxLayout;
}  // namespace GUI

class RamWidget : public Widget, public CommandSender
{
  friend class CartRamWidget;  // TODO: handleCommand() needs this

  public:
    RamWidget(GuiObject* boss, const GUI::Font& lfont, const GUI::Font& nfont,
                            uInt32 ramsize, uInt32 numrows, uInt32 pagesize,
              string_view helpAnchor);
    ~RamWidget() override;

    void loadConfig() override;
    void setOpsWidget(DataGridOpsWidget* w);

    // Reflow entry point for the resizable debugger: move the widget and lay
    // the RAM grid, its labels/buttons and the detail row out for the width
    void setArea(int x, int y, int w, int h) override;

    // My constructor cannot know how big I am -- that is however big my grid,
    // buttons and detail row make me -- so report what my layout tree comes to
    Common::Size naturalSize() const override;

    virtual string getLabel(int addr) const = 0;

  protected:
    void handleCommand(CommandSender* sender, int cmd, int data, int id) override;

  private:
    // Lay the widgets out within the area the parent layout gave us; shared by
    // the ctor and setArea()
    void reflow();

    // The grid, its row/column labels, the action buttons and the detail row,
    // as the engine sees them.  Asking this tree for its natural size is where
    // the widget's own size comes from, so nothing is added up here by hand
    unique_ptr<GUI::BoxLayout> buildLayout() const;

  private:
    // To be implemented by derived classes
    virtual uInt8 getValue(int addr) const = 0;
    virtual void setValue(int addr, uInt8 value) = 0;

    virtual void fillList(uInt32 start, uInt32 size,
                          IntArray& alist, IntArray& vlist,
                          BoolArray& changed) const = 0;
    virtual uInt32 readPort(uInt32 start) const = 0;
    virtual const ByteArray& currentRam(uInt32 start) const = 0;

  private:
    void fillGrid(bool updateOld);

    void showInputBox(int cmd);
    string_view doSearch(string_view str);
    string_view doCompare(string_view str);
    void doRestart();
    void showSearchResults();

  protected:
    // Font used for 'normal' text; _font is for 'label' text
    const GUI::Font& _nfont;

  private:
    enum {
      kUndoCmd     = 'RWud',
      kRevertCmd   = 'RWrv',
      kSearchCmd   = 'RWse',
      kCmpCmd      = 'RWcp',
      kRestartCmd  = 'RWrs',
      kSValEntered = 'RWsv',
      kCValEntered = 'RWcv',
      kRamGridID,
      kRamHexID,
      kRamDecID,
      kRamSignID,
      kRamBinID
    };

    uInt32 myUndoAddress{0}, myUndoValue{0};
    uInt32 myCurrentRamBank{0};
    uInt32 myRamSize{0};
    uInt32 myNumRows{0};
    uInt32 myPageSize{0};

    unique_ptr<InputTextDialog> myInputBox;

    StaticTextWidget* myRamStart{nullptr};
    std::array<StaticTextWidget*, 16> myRamLabels{nullptr};

    // Promoted from anonymous locals so the reflow can reposition them
    std::array<StaticTextWidget*, 16> myColHeaders{nullptr};  // column headers 0..F
    StaticTextWidget* myLabelText{nullptr};  // "Label"
    StaticTextWidget* myDecPrefix{nullptr};  // "#"
    StaticTextWidget* myBinPrefix{nullptr};  // "%"

    DataGridRamWidget* myRamGrid{nullptr};
    DataGridWidget* myHexValue{nullptr};
    DataGridWidget* myDecValue{nullptr};
    DataGridWidget* myBinValue{nullptr};
    EditTextWidget* myLabel{nullptr};

    ButtonWidget* myRevertButton{nullptr};
    ButtonWidget* myUndoButton{nullptr};
    ButtonWidget* mySearchButton{nullptr};
    ButtonWidget* myCompareButton{nullptr};
    ButtonWidget* myRestartButton{nullptr};

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

#endif  // RAM_WIDGET_HXX
