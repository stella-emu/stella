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
// Copyright (c) 1995-2018 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#ifndef CARTRIDGEE7_WIDGET_HXX
#define CARTRIDGEE7_WIDGET_HXX

#include "CartMNetworkWidget.hxx"

class CartridgeE7Widget : public CartridgeMNetworkWidget
{
  public:
    CartridgeE7Widget(GuiObject* boss, const GUI::Font& lfont,
                      const GUI::Font& nfont,
                      int x, int y, int w, int h,
                      CartridgeMNetwork& cart);
    virtual ~CartridgeE7Widget() = default;

  protected:
    const char* getSpotLower(int idx);
    const char* getSpotUpper(int idx);

  private:
    // Following constructors and assignment operators not supported
    CartridgeE7Widget() = delete;
    CartridgeE7Widget(const CartridgeE7Widget&) = delete;
    CartridgeE7Widget(CartridgeE7Widget&&) = delete;
    CartridgeE7Widget& operator=(const CartridgeE7Widget&) = delete;
    CartridgeE7Widget& operator=(CartridgeE7Widget&&) = delete;
};

#endif
