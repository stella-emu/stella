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
// Copyright (c) 1995-2021 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#ifndef CARTRIDGEE78K_WIDGET_HXX
#define CARTRIDGEE78K_WIDGET_HXX

#include "CartMNetworkWidget.hxx"

class CartridgeE78KWidget : public CartridgeMNetworkWidget
{
  public:
    CartridgeE78KWidget(GuiObject* boss, const GUI::Font& lfont,
                        const GUI::Font& nfont,
                        int x, int y, int w, int h,
                        CartridgeMNetwork& cart);
    ~CartridgeE78KWidget() override = default;

  protected:
    const char* getSpotLower(int idx) override;
    const char* getSpotUpper(int idx) override;

  private:
    // Following constructors and assignment operators not supported
    CartridgeE78KWidget() = delete;
    CartridgeE78KWidget(const CartridgeE78KWidget&) = delete;
    CartridgeE78KWidget(CartridgeE78KWidget&&) = delete;
    CartridgeE78KWidget& operator=(const CartridgeE78KWidget&) = delete;
    CartridgeE78KWidget& operator=(CartridgeE78KWidget&&) = delete;
};

#endif
