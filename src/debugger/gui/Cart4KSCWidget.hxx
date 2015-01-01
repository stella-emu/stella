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

#ifndef CARTRIDGE4KSC_WIDGET_HXX
#define CARTRIDGE4KSC_WIDGET_HXX

class Cartridge4KSC;
#include "CartDebugWidget.hxx"

class Cartridge4KSCWidget : public CartDebugWidget
{
  public:
    Cartridge4KSCWidget(GuiObject* boss, const GUI::Font& lfont,
                      const GUI::Font& nfont,
                      int x, int y, int w, int h,
                      Cartridge4KSC& cart);
    virtual ~Cartridge4KSCWidget() { }

    // No implementation for non-bankswitched ROMs
    void loadConfig() { }
    void handleCommand(CommandSender* sender, int cmd, int data, int id) { }
  
    void saveOldState();
  
    // start of functions for Cartridge RAM tab
    uInt32 internalRamSize();
    uInt32 internalRamRPort(int start);
    string internalRamDescription(); 
    const ByteArray& internalRamOld(int start, int count);
    const ByteArray& internalRamCurrent(int start, int count);
    void internalRamSetValue(int addr, uInt8 value);
    uInt8 internalRamGetValue(int addr);
    string internalRamLabel(int addr);
    // end of functions for Cartridge RAM tab   
  
  private:
    Cartridge4KSC& myCart;
    struct CartState {
      ByteArray internalram;
    };  
    CartState myOldState; 
};

#endif
