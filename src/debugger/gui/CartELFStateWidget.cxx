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
// Copyright (c) 1995-2024 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include "CartELFStateWidget.hxx"

#include <sstream>

#include "CartELF.hxx"
#include "DataGridWidget.hxx"
#include "ToggleBitWidget.hxx"

namespace {
  string registerName(uInt8 reg) {
    switch (reg) {
      case 11:
        return "FP (R11) = ";

      case 12:
        return "IP (R12) = ";

      case 13:
        return "SP (R13) = ";

      case 14:
        return "LR (R14) = ";

      case 15:
        return "PC (R15) = ";

      default: {
        ostringstream s;
        s << "R" << static_cast<int>(reg) << " = ";

        return s.str();
      }
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CartridgeELFStateWidget::CartridgeELFStateWidget(GuiObject* boss, const GUI::Font& lfont,
                       const GUI::Font& nfont,
                       int x, int y, int w, int h,
                       CartridgeELF& cart)
  : CartDebugWidget(boss, lfont, nfont, x, y, w, h), myCart(cart), myFlagValues(4)
{
  initialize();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CartridgeELFStateWidget::initialize()
{
  constexpr int x0 = 2;
  const int lineHeight = _font.getLineHeight();
  int y = lineHeight / 2;

  new StaticTextWidget(_boss, _font, x0, y, "ARM registers:");
  const int indent = _font.getMaxCharWidth() * 15;

  myArmRegisters = new DataGridWidget(_boss, _font, x0 + indent, y, 4, 4, 8, 8, Common::Base::Fmt::_16_8);
  y += myArmRegisters->getHeight() + lineHeight / 2;

  myArmRegisters->setEditable(false);
  for (uInt8 i = 0; i < 16; i++) myArmRegisters->setToolTip(i % 4, i / 4, registerName(i));

  new StaticTextWidget(_boss, _font, x0, y, "ARM flags:");
  myFlags = new ToggleBitWidget(_boss, _font, x0 + indent, y + lineHeight, 4, 1, 1);
  myFlags->setEditable(false);

  new StaticTextWidget(_boss, _font, x0 + indent + 3, y, "N");
  new StaticTextWidget(_boss, _font, x0 + indent + 3 + myFlags->colWidth(), y, "Z");
  new StaticTextWidget(_boss, _font, x0 + indent + 3 + 2 * myFlags->colWidth(), y, "C");
  new StaticTextWidget(_boss, _font, x0 + indent + 3 + 3 * myFlags->colWidth(), y, "V");
}

void CartridgeELFStateWidget::loadConfig()
{
  for (uInt8 i = 0; i < 16; i++)
    myArmRegisters->setValue(i, myCart.myCortexEmu.getRegister(i));

  BoolArray flags(4);
  flags[0] = myCart.myCortexEmu.getN();
  flags[1] = myCart.myCortexEmu.getZ();
  flags[2] = myCart.myCortexEmu.getC();
  flags[3] = myCart.myCortexEmu.getV();

  BoolArray flagsChanged(4);
  for (uInt8 i = 0; i < 4; i++) flagsChanged[i] = flags[i] != myFlagValues[i];
  myFlagValues = flags;

  myFlags->setState(flags, flagsChanged);
}
