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

#ifndef CARTRIDGEE7_WIDGET_HXX
#define CARTRIDGEE7_WIDGET_HXX

class CartridgeE7;
class PopUpWidget;

#include "CartDebugWidget.hxx"

class CartridgeE7Widget : public CartDebugWidget
{
  public:
    CartridgeE7Widget(GuiObject* boss, const GUI::Font& lfont,
                      const GUI::Font& nfont,
                      int x, int y, int w, int h,
                      CartridgeE7& cart);
    virtual ~CartridgeE7Widget() { }

    void loadConfig();
    void handleCommand(CommandSender* sender, int cmd, int data, int id);

    string bankState();

  private:
    CartridgeE7& myCart;
    PopUpWidget *myLower2K, *myUpper256B;

    enum {
      kLowerChanged = 'lwCH',
      kUpperChanged = 'upCH'
    };
};

#endif
