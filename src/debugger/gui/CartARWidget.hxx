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

#ifndef CARTRIDGE_AR_WIDGET_HXX
#define CARTRIDGE_AR_WIDGET_HXX

class CartridgeAR;
class PopUpWidget;

#include "CartDebugWidget.hxx"

class CartridgeARWidget : public CartDebugWidget
{
  public:
    CartridgeARWidget(GuiObject* boss, const GUI::Font& lfont,
                      const GUI::Font& nfont,
                      int x, int y, int w, int h,
                      CartridgeAR& cart);
    ~CartridgeARWidget() override = default;

    void loadConfig() override;
    string bankState() override;

  protected:
    void handleCommand(CommandSender* sender, int cmd, int data, int id) override;

  private:
    CartridgeAR& myCart;
    PopUpWidget* myBank{nullptr};

    enum { kBankChanged = 'bkCH' };

  private:
    // Following constructors and assignment operators not supported
    CartridgeARWidget() = delete;
    CartridgeARWidget(const CartridgeARWidget&) = delete;
    CartridgeARWidget(CartridgeARWidget&&) = delete;
    CartridgeARWidget& operator=(const CartridgeARWidget&) = delete;
    CartridgeARWidget& operator=(CartridgeARWidget&&) = delete;
};

#endif  // CARTRIDGE_AR_WIDGET_HXX
