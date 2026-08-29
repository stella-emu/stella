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
      CartridgeFA2& cart)
  : CartridgeEnhancedWidget(boss, lfont, nfont, cart),
    myCartFA2{cart}
{
  createFlashWidgets();
  initialize();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeFA2Widget::createFlashWidgets()
{
  // Each button sizes itself to its label; layoutContent() gives the group one width
  myFlashLbl = new LabelWidget(_boss, _font, "Harmony flash memory");

  myFlashErase = new ButtonWidget(_boss, _font, "Erase", Cmd::FlashErase);
  myFlashErase->setTarget(this);
  addFocusWidget(myFlashErase);

  myFlashLoad = new ButtonWidget(_boss, _font, "Load", Cmd::FlashLoad);
  myFlashLoad->setTarget(this);
  addFocusWidget(myFlashLoad);

  myFlashSave = new ButtonWidget(_boss, _font, "Save", Cmd::FlashSave);
  myFlashSave->setTarget(this);
  addFocusWidget(myFlashSave);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeFA2Widget::layoutContent(GUI::BoxLayout& col) const
{
  using GUI::BoxLayout;
  using GUI::anchoredItem;

  // The PlusROM fields and bank selector first, then the flash row beneath them
  CartridgeEnhancedWidget::layoutContent(col);

  GUI::alignButtons({myFlashErase, myFlashLoad, myFlashSave});

  // The flash label and the three flash buttons, in a row
  auto row = std::make_unique<BoxLayout>(BoxLayout::Dir::Horizontal, _fontWidth);
  row->addAuto(anchoredItem(myFlashLbl));
  row->addAuto(anchoredItem(myFlashErase));
  row->addAuto(anchoredItem(myFlashLoad));
  row->addAuto(anchoredItem(myFlashSave));

  col.addSpace(VGAP * 2);
  col.addAuto(std::move(row));
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
void CartridgeFA2Widget::handleCommand(CommandSender* sender, GuiCmd::Code cmd,
                                       int data, int id)
{
  switch(cmd)
  {
    case Cmd::FlashErase:
      myCartFA2.flash(0);
      break;

    case Cmd::FlashLoad:
      myCartFA2.flash(1);
      break;

    case Cmd::FlashSave:
      myCartFA2.flash(2);
      break;

    default:
      CartridgeEnhancedWidget::handleCommand(sender, cmd, data, id);
  }
}
