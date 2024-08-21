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

#ifndef CART_ELF_WIDGET_HXX
#define CART_ELF_WIDGET_HXX

#include "CartDebugWidget.hxx"

class CartridgeELF;
class EditTextWidget;
class FSNode;

class CartridgeELFWidget: public CartDebugWidget
{
  public:
    CartridgeELFWidget(GuiObject* boss, const GUI::Font& lfont,
                       const GUI::Font& nfont,
                       int x, int y, int w, int h,
                       CartridgeELF& cart);

    ~CartridgeELFWidget() override = default;

  private:
    void initialize();
    void saveArmImage(const FSNode& node);

    void handleCommand(CommandSender* sender, int cmd, int data, int id) override;

  private:
    CartridgeELF& myCart;

  private:
    CartridgeELFWidget() = delete;
    CartridgeELFWidget(const CartridgeELFWidget&) = delete;
    CartridgeELFWidget(CartridgeELFWidget&&) = delete;
    CartridgeELFWidget& operator=(const CartridgeELFWidget&) = delete;
    CartridgeELFWidget& operator=(CartridgeELFWidget&&) = delete;
};

#endif // CART_ELF_WIDGET_HXX
