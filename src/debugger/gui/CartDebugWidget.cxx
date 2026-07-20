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
#include "RomWidget.hxx"
#include "EditTextWidget.hxx"
#include "WrappedTextWidget.hxx"
#include "Layout.hxx"
#include "CartDebugWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartDebugWidget::CartDebugWidget(GuiObject* boss, const GUI::Font& lfont,
                                 const GUI::Font& nfont)
  : Widget(boss, lfont, 0, 0, 0, 0),
    CommandSender(boss),
    _nfont{nfont}
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
unique_ptr<GUI::Layout> CartDebugWidget::buildLayout() const
{
  using GUI::BoxLayout;

  // One label column for the whole tab, as wide as the longest label in it
  GUI::alignLabels(myLabelColumn);

  // The horizontal margins are applied via the layout rect (see reflow) so the
  // right margin can be wider than the left; only the vertical margin lives here
  auto col = std::make_unique<BoxLayout>(BoxLayout::Dir::Vertical, VGAP, 0, VBORDER);

  layoutBaseInformation(*col);
  layoutContent(*col);

  // Soak up whatever the rows above did not need.  The description stretches
  // (see layoutBaseInformation) but stops at the height of its own text, and a
  // box hands the leftover to its LAST stretching cell -- so without this the
  // description would be handed back the very slack its cap just declined
  col->addStretchSpace(0);

  return col;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartDebugWidget::reflow()
{
  buildLayout()->doLayout(_x + HBORDER, _y, contentWidth(_w), _h);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Common::Size CartDebugWidget::naturalSize() const
{
  // The tree is laid out in the CONTENT rect, so what it comes to is my size
  // less the horizontal margins reflow() insets it by
  const Common::Size content = buildLayout()->naturalSize();

  return Common::Size(content.w + HBORDER + RBORDER, content.h);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartDebugWidget::createBaseInformation(size_t bytes, string_view manufacturer,
        string_view desc, uInt16 maxlines)
{
  // Everything is created at a placeholder position; layoutBaseInformation()
  // positions it, and the description re-wraps itself, at reflow() time.  The
  // labels carry no padding of their own: they join the tab's label column below,
  // and GUI::alignLabels() supplies the column and its clearance
  myROMSizeLabel = new StaticTextWidget(_boss, _font, "ROM size");
  myROMSize = new EditTextWidget(_boss, _nfont, 1,
    bytes >= 1024
      ? std::format("{} bytes / {}KB", bytes, bytes / 1024)
      : std::format("{} bytes", bytes));
  myROMSize->setEditable(false);

  myManufacturerLabel = new StaticTextWidget(_boss, _font, "Manufacturer");
  myManufacturer = new EditTextWidget(_boss, _nfont, 1, manufacturer);
  myManufacturer->setEditable(false);

  myDescLabel = new StaticTextWidget(_boss, _font, "Description");
  myDesc = new WrappedTextWidget(_boss, _nfont, desc, maxlines);
  myDesc->setEditable(false);
  myDesc->setEnabled(false);

  myLabelColumn.insert(myLabelColumn.end(),
                       {{myROMSizeLabel}, {myManufacturerLabel}, {myDescLabel}});
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartDebugWidget::layoutBaseInformation(GUI::BoxLayout& col) const
{
  using GUI::labeledRow;

  // Not every cart tab has an info block: the ARM carts show theirs on a tab of
  // its own (CartridgeBUSInfoWidget, CartridgeCDFInfoWidget) and this one holds
  // nothing but their registers
  if(myROMSizeLabel == nullptr)
    return;

  // Word wrap couples width to height: the description only knows how tall it is
  // once it knows how wide it is, so it is given its width before the column is
  // built (see the heightForWidth note in Layout.hxx).  Its width is the one the
  // filling row below will hand it -- the content, less the shared label column
  myDesc->setWidth(contentWidth(_w) - myDescLabel->getWidth());

  col.addAuto(labeledRow(myROMSizeLabel, myROMSize, 0, 0, true));
  col.addAuto(labeledRow(myManufacturerLabel, myManufacturer, 0, 0, true));

  // The description is the one row here that can be SQUEEZED: it scrolls, so it
  // gives up height before anything below it is pushed off the tab.  Hence a
  // stretching cell rather than an Auto one -- between the floor it always shows
  // and the height of its own text, so a roomy tab looks as it always did while
  // a short one keeps the rows below visible.  Both ends come from the widget:
  // only the floor is width-independent, which is what lets this column be
  // measured before anything has been sized (see the class comment there)
  auto descRow = std::make_unique<GUI::BoxLayout>(GUI::BoxLayout::Dir::Horizontal);
  descRow->addFixed(GUI::anchoredItem(myDescLabel), myDescLabel->getWidth());
  descRow->addStretch(GUI::widgetItem(myDesc, 0, myDesc->minHeight()));
  col.add(std::move(descRow), GUI::SizePolicy::Stretch, 1,
          static_cast<int>(myDesc->naturalSize().h), myDesc->minHeight());

  // Whatever the cart puts below the info block stands clear of it
  col.addSpace(_lineHeight / 2);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartDebugWidget::setArea(int x, int y, int w, int h)
{
  Widget::setArea(x, y, w, h);
  reflow();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartDebugWidget::invalidate()
{
  sendCommand(RomWidget::kInvalidateListing, -1, -1);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartDebugWidget::loadConfig()
{
}
