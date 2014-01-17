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
// Copyright (c) 1995-2014 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

#ifndef CARTRIDGEMC_WIDGET_HXX
#define CARTRIDGEMC_WIDGET_HXX

class CartridgeMC;
class PopUpWidget;

#include "CartDebugWidget.hxx"

class CartridgeMCWidget : public CartDebugWidget
{
  public:
    CartridgeMCWidget(GuiObject* boss, const GUI::Font& lfont,
                      const GUI::Font& nfont,
                      int x, int y, int w, int h,
                      CartridgeMC& cart);
    virtual ~CartridgeMCWidget() { }

    void loadConfig();
    void handleCommand(CommandSender* sender, int cmd, int data, int id);

    string bankState();

  private:
    CartridgeMC& myCart;
    PopUpWidget *mySlice0, *mySlice1, *mySlice2, *mySlice3;

    enum {
      kSlice0Changed = 's0CH',
      kSlice1Changed = 's1CH',
      kSlice2Changed = 's2CH',
      kSlice3Changed = 's3CH'
    };
};

#endif
