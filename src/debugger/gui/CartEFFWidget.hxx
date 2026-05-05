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

#ifndef CARTRIDGE_EFF_WIDGET_HXX
#define CARTRIDGE_EFF_WIDGET_HXX

class CartridgeEFF;

#include "CartEnhancedWidget.hxx"

class CartridgeEFFWidget : public CartridgeEnhancedWidget
{
  public:
    CartridgeEFFWidget(GuiObject* boss, const GUI::Font& lfont,
                       const GUI::Font& nfont,
                       int x, int y, int w, int h,
                       CartridgeEFF& cart);
    ~CartridgeEFFWidget() override = default;

  protected:
    string manufacturer() override { return "Fred Quimby/AtariAge"; }
    string description() override;
    int descriptionLines() override;

  private:
    // Following constructors and assignment operators not supported
    CartridgeEFFWidget() = delete;
    CartridgeEFFWidget(const CartridgeEFFWidget&) = delete;
    CartridgeEFFWidget(CartridgeEFFWidget&&) = delete;
    CartridgeEFFWidget& operator=(const CartridgeEFFWidget&) = delete;
    CartridgeEFFWidget& operator=(CartridgeEFFWidget&&) = delete;
};

#endif  // CARTRIDGE_EFF_WIDGET_HXX
