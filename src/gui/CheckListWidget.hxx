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

#ifndef CHECK_LIST_WIDGET_HXX
#define CHECK_LIST_WIDGET_HXX

class CheckboxWidget;

#include "ListWidget.hxx"

using CheckboxArray = vector<CheckboxWidget*>;


/** CheckListWidget */
class CheckListWidget : public ListWidget
{
  public:
    struct Cmd {
      static constexpr GuiCmd::Code
        ListItemChecked = GuiCmd::of("CheckListWidget.ListItemChecked");  // checkbox toggled on current line
    };

  public:
    CheckListWidget(GuiObject* boss, const GUI::Font& font);
    ~CheckListWidget() override = default;

    // Sets the row text and checked state together (parallel arrays)
    void setList(const StringList& list, const BoolArray& state);
    // Updates one row's text and checked state
    void setLine(int line, string_view str, bool state);

    bool getState(int line) const;
    bool getSelectedState() const { return getState(_selectedItem); }

    // UISelect simulates a click on the selected row's checkbox
    bool handleEvent(Event::Type e) override;

    // Brings ListWidget's setPos(int,int)/setPos(Point) into scope alongside
    // our own override below, which would otherwise hide them
    using ListWidget::setPos;
    // Also repositions the per-row checkboxes (see reflowCheckboxes())
    void setPos(const Common::Point& pos) override;
    void setHeight(int h) override;
    void refreshFont() override;

  protected:
    // Reacts to a row's checkbox being toggled (CheckboxWidget::Cmd::CheckAction)
    void handleCommand(CommandSender* sender, GuiCmd::Code cmd, int data, int id) override;

    void drawWidget(bool hilite) override;
    Common::Rect getEditRect() const override;

  protected:
    // Checked state for each row, parallel to _list
    BoolArray     _stateList;
    // Pool of checkbox widgets, one per visible row (grow-only; see reflowCheckboxes())
    CheckboxArray _checkList;

  private:
    // Grow the checkbox pool to one per visible row (grow-only; widgets can't
    // be removed) and position every checkbox against the list's current
    // origin, hiding any beyond the visible row count
    void reflowCheckboxes();

  private:
    // Following constructors and assignment operators not supported
    CheckListWidget() = delete;
    CheckListWidget(const CheckListWidget&) = delete;
    CheckListWidget(CheckListWidget&&) = delete;
    CheckListWidget& operator=(const CheckListWidget&) = delete;
    CheckListWidget& operator=(CheckListWidget&&) = delete;
};

#endif  // CHECK_LIST_WIDGET_HXX
