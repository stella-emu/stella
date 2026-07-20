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

#include "Font.hxx"
#include "Layout.hxx"
#include "DataGridOpsWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DataGridOpsWidget::DataGridOpsWidget(GuiObject* boss, const GUI::Font& font)
  : Widget(boss, font, 0, 0, 0, 0),
    CommandSender(boss)
{
  // This widget only holds the buttons (siblings parented to the boss) and wires
  // their target/enabled state; DebuggerDialog lays them out via buildLayout()
  // NOLINTBEGIN(cppcoreguidelines-prefer-member-initializer)
  _zeroButton = new ButtonWidget(boss, font, 0, 0, "0", kDGZeroCmd);
  _zeroButton->setToolTip("Zero currently selected value (Z)");

  _invButton = new ButtonWidget(boss, font, 0, 0, "Inv", kDGInvertCmd);
  _invButton->setToolTip("Invert currently selected value (I)");

  _incButton = new ButtonWidget(boss, font, 0, 0, "++", kDGIncCmd);
  _incButton->setToolTip("Increase currently selected value. (=, Keypad +)");

  _shiftLeftButton = new ButtonWidget(boss, font, 0, 0, "<<", kDGShiftLCmd);
  _shiftLeftButton->setToolTip("Shift currently selected value left (,)");

  _negButton = new ButtonWidget(boss, font, 0, 0, "Neg", kDGNegateCmd);
  _negButton->setToolTip("Negate currently selected value (N)");

  _decButton = new ButtonWidget(boss, font, 0, 0, "--", kDGDecCmd);
  _decButton->setToolTip("Decrease currently selected value (-, Keypad -)");

  _shiftRightButton = new ButtonWidget(boss, font, 0, 0, ">>", kDGShiftRCmd);
  _shiftRightButton->setToolTip("Shift currently selected value right (.)");
  // NOLINTEND(cppcoreguidelines-prefer-member-initializer)

  // These op buttons are small; trim their self-size margin
  for(auto* b: {_zeroButton, _invButton, _incButton, _shiftLeftButton,
                _negButton, _decButton, _shiftRightButton})
    b->setCompact();

  // We don't enable the buttons until the DataGridWidget is attached
  // Don't call setEnabled(false), since that does an immediate redraw
  _zeroButton->clearFlags(Widget::FLAG_ENABLED);
  _invButton->clearFlags(Widget::FLAG_ENABLED);
  _negButton->clearFlags(Widget::FLAG_ENABLED);
  _incButton->clearFlags(Widget::FLAG_ENABLED);
  _decButton->clearFlags(Widget::FLAG_ENABLED);
  _shiftLeftButton->clearFlags(Widget::FLAG_ENABLED);
  _shiftRightButton->clearFlags(Widget::FLAG_ENABLED);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
unique_ptr<GUI::GridLayout> DataGridOpsWidget::buildLayout(int vGap, int hGap)
{
  using GUI::GridLayout;
  using GUI::alignedItem;
  using GUI::HAlign;
  using GUI::VAlign;

  // Every button takes the widest one's (compact) width, so the two columns line up
  GUI::alignButtons({_zeroButton, _invButton, _incButton, _shiftLeftButton,
                     _negButton, _decButton, _shiftRightButton});

  // Two columns of operations; the right column starts one row down.  The rows
  // SHARE the height of the band between them and the gaps are a fixed vGap, so
  // this column ends level with the register grids beside it whatever height the
  // band comes to, and the height goes into the buttons rather than the space
  // between them.  A button keeps its compact WIDTH and fills the row
  auto grid = std::make_unique<GridLayout>(2, ROWS * 2 - 1, hGap, 0);
  grid->columnAuto(0).columnAuto(1);
  for(int row = 0; row < ROWS * 2 - 1; ++row)
  {
    if(row % 2)
      grid->rowFixed(row, vGap);
    else
      grid->rowStretch(row);
  }

  const auto opButton = [](ButtonWidget* b) {
    return alignedItem(b, HAlign::Left, VAlign::Fill);
  };
  grid->place(0, 2, opButton(_zeroButton));
  grid->place(0, 4, opButton(_invButton));
  grid->place(0, 6, opButton(_incButton));
  grid->place(0, 8, opButton(_shiftLeftButton));
  grid->place(1, 4, opButton(_negButton));
  grid->place(1, 6, opButton(_decButton));
  grid->place(1, 8, opButton(_shiftRightButton));

  return grid;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DataGridOpsWidget::setTarget(CommandReceiver* target)
{
  _zeroButton->setTarget(target);
  _invButton->setTarget(target);
  _negButton->setTarget(target);
  _incButton->setTarget(target);
  _decButton->setTarget(target);
  _shiftLeftButton->setTarget(target);
  _shiftRightButton->setTarget(target);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DataGridOpsWidget::setEnabled(bool e)
{
  _zeroButton->setEnabled(e);
  _invButton->setEnabled(e);
  _negButton->setEnabled(e);
  _incButton->setEnabled(e);
  _decButton->setEnabled(e);
  _shiftLeftButton->setEnabled(e);
  _shiftRightButton->setEnabled(e);
}
