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

#ifndef CART_ELF_INFO_WIDGET_HXX
#define CART_ELF_INFO_WIDGET_HXX

#include "CartDebugWidget.hxx"

class CartridgeELF;
class DataGridWidget;
class ToggleBitWidget;
class EditTextWidget;
class StaticTextWidget;

class CartridgeELFStateWidget : public CartDebugWidget {
  public:
    CartridgeELFStateWidget(GuiObject* boss, const GUI::Font& lfont,
                            const GUI::Font& nfont,
                            int x, int y, int w, int h,
                            CartridgeELF& cart);

    ~CartridgeELFStateWidget() override = default;

  private:
    void initialize();

    void loadConfig() override;

  private:
    CartridgeELF& myCart;
    BoolArray myFlagValues;

    DataGridWidget* myArmRegisters{nullptr};
    ToggleBitWidget* myFlags{nullptr};
    EditTextWidget* myCurrentCyclesVcs{nullptr};
    EditTextWidget* myCurrentCyclesArm{nullptr};
    EditTextWidget* myQueueSize{nullptr};
    StaticTextWidget* myNextTransaction{nullptr};

  private:
    CartridgeELFStateWidget() = delete;
    CartridgeELFStateWidget(const CartridgeELFStateWidget&) = delete;
    CartridgeELFStateWidget(CartridgeELFStateWidget&&) = delete;
    CartridgeELFStateWidget& operator=(const CartridgeELFStateWidget&) = delete;
    CartridgeELFStateWidget& operator=(CartridgeELFStateWidget&&) = delete;
};

#endif // CART_ELF_INFO_WIDGET_HXX
