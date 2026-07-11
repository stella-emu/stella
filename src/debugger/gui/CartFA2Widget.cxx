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
#include "CartFA2.hxx"
#include "Layout.hxx"
#include "CartFA2Widget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeFA2Widget::CartridgeFA2Widget(
      GuiObject* boss, const GUI::Font& lfont, const GUI::Font& nfont,
      int x, int y, int w, int h, CartridgeFA2& cart)
  : CartridgeEnhancedWidget(boss, lfont, nfont, x, y, w, h, cart),
    myCartFA2{cart}
{
  initialize();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeFA2Widget::createExtras()
{
  const int bwidth = _font.getStringWidth("Erase") + 20;

  myFlashLabel = new StaticTextWidget(_boss, _font, 0, 0, "Harmony flash memory ");

  myFlashErase = new ButtonWidget(_boss, _font, 0, 0, bwidth, myButtonHeight,
                                  "Erase", kFlashErase);
  myFlashErase->setTarget(this);
  addFocusWidget(myFlashErase);

  myFlashLoad = new ButtonWidget(_boss, _font, 0, 0, bwidth, myButtonHeight,
                                 "Load", kFlashLoad);
  myFlashLoad->setTarget(this);
  addFocusWidget(myFlashLoad);

  myFlashSave = new ButtonWidget(_boss, _font, 0, 0, bwidth, myButtonHeight,
                                 "Save", kFlashSave);
  myFlashSave->setTarget(this);
  addFocusWidget(myFlashSave);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeFA2Widget::layoutExtra(GUI::BoxLayout& col)
{
  using GUI::BoxLayout;
  using GUI::anchoredItem;
  using GUI::alignedItem;
  using GUI::HAlign;
  using GUI::VAlign;

  // The flash label and the three flash buttons, in a row
  auto row = std::make_unique<BoxLayout>(BoxLayout::Dir::Horizontal, _fontWidth);
  row->addFixed(alignedItem(myFlashLabel, HAlign::Fill, VAlign::Center),
                myFlashLabel->getWidth());
  row->addFixed(anchoredItem(myFlashErase), myFlashErase->getWidth());
  row->addFixed(anchoredItem(myFlashLoad), myFlashLoad->getWidth());
  row->addFixed(anchoredItem(myFlashSave), myFlashSave->getWidth());

  col.addSpace(VGAP * 2);
  col.addFixed(std::move(row), myButtonHeight);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string CartridgeFA2Widget::description()
{
  return std::format(
    "Modified FA RAM+, six or seven 4K banks\n"
    "RAM+ can be loaded/saved to Harmony flash memory by accessing ${:X}\n"
    "{}",
    0xFFF4,
    CartridgeEnhancedWidget::description());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeFA2Widget::handleCommand(CommandSender* sender,
                                       int cmd, int data, int id)
{
  switch(cmd)
  {
    case kFlashErase:
      myCartFA2.flash(0);
      break;

    case kFlashLoad:
      myCartFA2.flash(1);
      break;

    case kFlashSave:
      myCartFA2.flash(2);
      break;

    default:
      CartridgeEnhancedWidget::handleCommand(sender, cmd, data, id);
  }
}
