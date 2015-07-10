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
// Copyright (c) 1995-2015 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

#ifndef CARTRIDGEE0_WIDGET_HXX
#define CARTRIDGEE0_WIDGET_HXX

class CartridgeE0;
class PopUpWidget;

#include "CartDebugWidget.hxx"

class CartridgeE0Widget : public CartDebugWidget
{
  public:
    CartridgeE0Widget(GuiObject* boss, const GUI::Font& lfont,
                      const GUI::Font& nfont,
                      int x, int y, int w, int h,
                      CartridgeE0& cart);
    virtual ~CartridgeE0Widget() { }

  private:
    CartridgeE0& myCart;
    PopUpWidget *mySlice0, *mySlice1, *mySlice2;

    enum {
      kSlice0Changed = 's0CH',
      kSlice1Changed = 's1CH',
      kSlice2Changed = 's2CH'
    };

  private:
    void loadConfig() override;
    void handleCommand(CommandSender* sender, int cmd, int data, int id) override;

    string bankState() override;

    // Following constructors and assignment operators not supported
    CartridgeE0Widget() = delete;
    CartridgeE0Widget(const CartridgeE0Widget&) = delete;
    CartridgeE0Widget(CartridgeE0Widget&&) = delete;
    CartridgeE0Widget& operator=(const CartridgeE0Widget&) = delete;
    CartridgeE0Widget& operator=(CartridgeE0Widget&&) = delete;
};

#endif
