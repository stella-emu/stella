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
DataGridOpsWidget::DataGridOpsWidget(GuiObject* boss, const GUI::Font& font,
                                     int x, int y)
  : Widget(boss, font, x, y, 16, 16),
    CommandSender(boss)
{
  // Create every button at a placeholder position/size; reflow() sizes and
  // arranges them for the current font
  // NOLINTBEGIN(cppcoreguidelines-prefer-member-initializer)
  _zeroButton = new ButtonWidget(boss, font, 0, 0, 1, 1, "0", kDGZeroCmd);
  _zeroButton->setToolTip("Zero currently selected value (Z)");

  _invButton = new ButtonWidget(boss, font, 0, 0, 1, 1, "Inv", kDGInvertCmd);
  _invButton->setToolTip("Invert currently selected value (I)");

  _incButton = new ButtonWidget(boss, font, 0, 0, 1, 1, "++", kDGIncCmd);
  _incButton->setToolTip("Increase currently selected value. (=, Keypad +)");

  _shiftLeftButton = new ButtonWidget(boss, font, 0, 0, 1, 1, "<<", kDGShiftLCmd);
  _shiftLeftButton->setToolTip("Shift currently selected value left (,)");

  _negButton = new ButtonWidget(boss, font, 0, 0, 1, 1, "Neg", kDGNegateCmd);
  _negButton->setToolTip("Negate currently selected value (N)");

  _decButton = new ButtonWidget(boss, font, 0, 0, 1, 1, "--", kDGDecCmd);
  _decButton->setToolTip("Decrease currently selected value (-, Keypad -)");

  _shiftRightButton = new ButtonWidget(boss, font, 0, 0, 1, 1, ">>", kDGShiftRCmd);
  _shiftRightButton->setToolTip("Shift currently selected value right (.)");
  // NOLINTEND(cppcoreguidelines-prefer-member-initializer)

  reflow();

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
void DataGridOpsWidget::setPos(const Common::Point& pos)
{
  Widget::setPos(pos);
  // The buttons are sibling widgets, not children, so they must be moved along
  reflow();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DataGridOpsWidget::refreshFontMetrics()
{
  Widget::refreshFontMetrics();
  // ButtonWidget::refreshFontMetrics() is metrics-only, so the buttons must be
  // resized here
  reflow();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DataGridOpsWidget::reflow()
{
  using GUI::GridLayout;
  using GUI::widgetItem;

  const int bwidth = _fontWidth * 4 + 2,
            bheight = _font.getFontHeight() + 3;
  constexpr int space = 4;

  // Two columns of operations; the right column starts one row down
  GridLayout grid(2, 4, space, space, 0, 0);
  grid.columnFixed(0, bwidth);
  grid.columnFixed(1, bwidth);
  for(int row = 0; row < 4; ++row)
    grid.rowFixed(row, bheight);

  grid.place(0, 0, widgetItem(_zeroButton));
  grid.place(0, 1, widgetItem(_invButton));
  grid.place(0, 2, widgetItem(_incButton));
  grid.place(0, 3, widgetItem(_shiftLeftButton));
  grid.place(1, 1, widgetItem(_negButton));
  grid.place(1, 2, widgetItem(_decButton));
  grid.place(1, 3, widgetItem(_shiftRightButton));

  grid.doLayout(_x, _y, 2 * bwidth + space, 4 * bheight + 3 * space);

  // Calculate real dimensions
  _w = 2 * (bwidth + space);
  _h = 4 * (bheight + space);
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
