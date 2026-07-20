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

#ifndef DATA_GRID_OPS_WIDGET_HXX
#define DATA_GRID_OPS_WIDGET_HXX

#include "Widget.hxx"
#include "Command.hxx"

namespace GUI {
  class GridLayout;
}  // namespace GUI

// DataGridWidget operations
enum {
  kDGZeroCmd   = 'DGze',
  kDGInvertCmd = 'DGiv',
  kDGNegateCmd = 'DGng',
  kDGIncCmd    = 'DGic',
  kDGDecCmd    = 'DGdc',
  kDGShiftLCmd = 'DGls',
  kDGShiftRCmd = 'DGrs'
};

class DataGridOpsWidget : public Widget, public CommandSender
{
  public:
    DataGridOpsWidget(GuiObject* boss, const GUI::Font& font);
    ~DataGridOpsWidget() override = default;

    // How many rows the ops column comes to: my four, plus one at the top for
    // the owner's Options button.  Every column in the band is built to this
    // many rows so they all end level
    static constexpr int ROWS = 5;

    // The column the op buttons live in: ROWS rows SHARING the height of the
    // band with a fixed 'vGap' between them, so the column ends level with the
    // register grids however tall the band comes to, and the height it is given
    // goes into the buttons rather than the space between them.  Row 0 is left
    // empty for the owner's Options button to place itself into
    unique_ptr<GUI::GridLayout> buildLayout(int vGap, int hGap);

    void setTarget(CommandReceiver* target) override;
    void setEnabled(bool e) override;

  private:
    ButtonWidget* _zeroButton{nullptr};
    ButtonWidget* _invButton{nullptr};
    ButtonWidget* _negButton{nullptr};
    ButtonWidget* _incButton{nullptr};
    ButtonWidget* _decButton{nullptr};
    ButtonWidget* _shiftLeftButton{nullptr};
    ButtonWidget* _shiftRightButton{nullptr};

  private:
    // Following constructors and assignment operators not supported
    DataGridOpsWidget() = delete;
    DataGridOpsWidget(const DataGridOpsWidget&) = delete;
    DataGridOpsWidget(DataGridOpsWidget&&) = delete;
    DataGridOpsWidget& operator=(const DataGridOpsWidget&) = delete;
    DataGridOpsWidget& operator=(DataGridOpsWidget&&) = delete;
};

#endif  // DATA_GRID_OPS_WIDGET_HXX
