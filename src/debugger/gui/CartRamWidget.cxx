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

#include "EditTextWidget.hxx"
#include "GuiObject.hxx"
#include "CartDebug.hxx"
#include "Widget.hxx"
#include "Font.hxx"
#include "WrappedTextWidget.hxx"
#include "Layout.hxx"
#include "CartDebugWidget.hxx"
#include "CartRamWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartRamWidget::CartRamWidget(
      GuiObject* boss, const GUI::Font& lfont, const GUI::Font& nfont,
      CartDebugWidget& cartDebug)
  : Widget(boss, lfont, 0, 0, 0, 0),
    CommandSender(boss),
    _nfont{nfont}
{
  // Everything is created at a placeholder position; reflow() positions and
  // sizes it, and the description re-wraps itself, whenever our area changes
  myRamSizeLabel = new StaticTextWidget(_boss, _font, "RAM size");

  const uInt32 ramsize = cartDebug.internalRamSize();
  myRamSize = new EditTextWidget(boss, nfont, 1,
    ramsize >= 1024
      ? std::format("{} bytes / {}KB", ramsize, ramsize / 1024)
      : std::format("{} bytes", ramsize));
  myRamSize->setEditable(false);

  myDescLabel = new StaticTextWidget(_boss, _font, "Description");
  myDesc = new WrappedTextWidget(boss, nfont,
                                 cartDebug.internalRamDescription(), MAX_DESC_LINES);
  myDesc->setEditable(false);
  myDesc->setEnabled(false);

  // The RAM view fills whatever is left below the fields
  myRam = new InternalRamWidget(boss, lfont, nfont, cartDebug);
  addToFocusList(myRam->getFocusList());

  reflow();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
unique_ptr<GUI::Layout> CartRamWidget::buildLayout() const
{
  using GUI::BoxLayout;
  using GUI::labeledRow;
  using GUI::widgetItem;

  // The two rows share one label column, as wide as the longer of their labels
  GUI::alignLabels({{myRamSizeLabel}, {myDescLabel}});

  const int contentW = CartDebugWidget::contentWidth(_w);

  // Word wrap couples width to height: the description only knows how tall it is
  // once it knows how wide it is, so it is given its width before the column is
  // built (see the heightForWidth note in Layout.hxx).  Its width is the one the
  // filling row below will hand it -- the content, less the shared label column
  myDesc->setWidth(contentW - myDescLabel->getWidth());

  auto col = std::make_unique<BoxLayout>(BoxLayout::Dir::Vertical,
                CartDebugWidget::VGAP, 0, CartDebugWidget::VBORDER);

  col->addAuto(labeledRow(myRamSizeLabel, myRamSize, 0, 0, true));

  // The description scrolls, so it is squeezable: a stretching cell between the
  // floor it always shows and the height of its own text, exactly as on the
  // cart tab (see CartDebugWidget::layoutBaseInformation).  Only the floor is
  // width-independent, which is what lets this column be measured before it has
  // been sized.  The RAM view below is the last stretching cell, so it is what
  // takes the slack the description's cap declines
  auto descRow = std::make_unique<BoxLayout>(BoxLayout::Dir::Horizontal);
  descRow->addFixed(GUI::anchoredItem(myDescLabel), myDescLabel->getWidth());
  descRow->addStretch(widgetItem(myDesc, 0, myDesc->minHeight()));
  col->add(std::move(descRow), GUI::SizePolicy::Stretch, 1,
           static_cast<int>(myDesc->naturalSize().h), myDesc->minHeight());

  col->addSpace(_fontHeight / 2);
  col->addStretch(widgetItem(myRam));

  return col;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartRamWidget::reflow()
{
  buildLayout()->doLayout(_x + CartDebugWidget::HBORDER, _y,
                          CartDebugWidget::contentWidth(_w), _h);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Common::Size CartRamWidget::naturalSize() const
{
  // The tree is laid out in the CONTENT rect, so what it comes to is my size
  // less the horizontal margins reflow() insets it by
  const Common::Size content = buildLayout()->naturalSize();

  return Common::Size(content.w + CartDebugWidget::HBORDER
                                + CartDebugWidget::RBORDER, content.h);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartRamWidget::loadConfig()
{
  myRam->loadConfig();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartRamWidget::setOpsWidget(DataGridOpsWidget* w)
{
  myRam->setOpsWidget(w);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartRamWidget::setArea(int x, int y, int w, int h)
{
  Widget::setArea(x, y, w, h);
  reflow();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartRamWidget::handleCommand(CommandSender* sender, int cmd, int data, int id)
{
  myRam->handleCommand(sender, cmd, data, id);
}

///////////////////////////////////
// Internal RAM implementation
///////////////////////////////////

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartRamWidget::InternalRamWidget::InternalRamWidget(GuiObject* boss,
        const GUI::Font& lfont, const GUI::Font& nfont,
        CartDebugWidget& dbg)
  : RamWidget(boss, lfont, nfont,
      dbg.internalRamSize(), std::min(dbg.internalRamSize() / 16, 16U),
      std::min(dbg.internalRamSize() / 16, 16U) * 16, "CartridgeRAMInformation"),
    myCart(dbg)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 CartRamWidget::InternalRamWidget::getValue(int addr) const
{
  return myCart.internalRamGetValue(addr);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartRamWidget::InternalRamWidget::setValue(int addr, uInt8 value)
{
  myCart.internalRamSetValue(addr, value);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string CartRamWidget::InternalRamWidget::getLabel(int addr) const
{
  return myCart.internalRamLabel(addr);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartRamWidget::InternalRamWidget::fillList(uInt32 start, uInt32 size,
          IntArray& alist, IntArray& vlist, BoolArray& changed) const
{
  const ByteArray& oldRam  = myCart.internalRamOld(start, size);
  const ByteArray& currRam = myCart.internalRamCurrent(start, size);

  for(uInt32 i = 0; i < size; ++i)
    alist.push_back(i + start);

  std::ranges::copy(currRam, std::back_inserter(vlist));
  std::ranges::transform(currRam, oldRam, std::back_inserter(changed),
                         std::not_equal_to{});
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 CartRamWidget::InternalRamWidget::readPort(uInt32 start) const
{
  return myCart.internalRamRPort(start);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const ByteArray& CartRamWidget::InternalRamWidget::currentRam(uInt32 start) const
{
  return myCart.internalRamCurrent(start, myCart.internalRamSize());
}
