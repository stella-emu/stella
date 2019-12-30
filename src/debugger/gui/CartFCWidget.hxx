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
// Copyright (c) 1995-2019 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#ifndef CARTRIDGEFC_WIDGET_HXX
#define CARTRIDGEFC_WIDGET_HXX

class CartridgeFC;
class PopUpWidget;

#include "CartDebugWidget.hxx"

class CartridgeFCWidget : public CartDebugWidget
{
  public:
    CartridgeFCWidget(GuiObject* boss, const GUI::Font& lfont,
                      const GUI::Font& nfont,
                      int x, int y, int w, int h,
                      CartridgeFC& cart);
    virtual ~CartridgeFCWidget() = default;

  private:
    CartridgeFC& myCart;
    PopUpWidget* myBank{nullptr};

    enum { kBankChanged = 'bkCH' };

  private:
    void loadConfig() override;
    void handleCommand(CommandSender* sender, int cmd, int data, int id) override;

    string bankState() override;

    // Following constructors and assignment operators not supported
    CartridgeFCWidget() = delete;
    CartridgeFCWidget(const CartridgeFCWidget&) = delete;
    CartridgeFCWidget(CartridgeFCWidget&&) = delete;
    CartridgeFCWidget& operator=(const CartridgeFCWidget&) = delete;
    CartridgeFCWidget& operator=(CartridgeFCWidget&&) = delete;
};

#endif
