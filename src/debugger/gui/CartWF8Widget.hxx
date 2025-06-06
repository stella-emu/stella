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
// Copyright (c) 1995-2025 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#ifndef CARTRIDGEWF8_WIDGET_HXX
#define CARTRIDGEWF8_WIDGET_HXX

class CartridgeWF8;
class PopUpWidget;

#include "CartEnhancedWidget.hxx"

class CartridgeWF8Widget : public CartridgeEnhancedWidget
{
public:
  CartridgeWF8Widget(GuiObject* boss, const GUI::Font& lfont,
                     const GUI::Font& nfont,
                     int x, int y, int w, int h,
                     CartridgeWF8& cart);
  ~CartridgeWF8Widget() override = default;

private:
  string manufacturer() override { return "Coleco"; }

  string description() override;

private:
  // Following constructors and assignment operators not supported
  CartridgeWF8Widget() = delete;
  CartridgeWF8Widget(const CartridgeWF8Widget&) = delete;
  CartridgeWF8Widget(CartridgeWF8Widget&&) = delete;
  CartridgeWF8Widget& operator=(const CartridgeWF8Widget&) = delete;
  CartridgeWF8Widget& operator=(CartridgeWF8Widget&&) = delete;
};

#endif
