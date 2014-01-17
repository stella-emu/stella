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

#ifndef CARTRIDGE4K_WIDGET_HXX
#define CARTRIDGE4K_WIDGET_HXX

class Cartridge4K;
#include "CartDebugWidget.hxx"

class Cartridge4KWidget : public CartDebugWidget
{
  public:
    Cartridge4KWidget(GuiObject* boss, const GUI::Font& lfont,
                      const GUI::Font& nfont,
                      int x, int y, int w, int h,
                      Cartridge4K& cart);
    virtual ~Cartridge4KWidget() { }

    // No implementation for non-bankswitched ROMs
    void loadConfig() { }
    void handleCommand(CommandSender* sender, int cmd, int data, int id) { }
};

#endif
