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

  private:
    Cartridge4KSC& myCart;
    struct CartState {
      ByteArray internalram;
    };  
    CartState myOldState; 

  private:
    // No implementation for non-bankswitched ROMs
    void loadConfig() override { }
    void handleCommand(CommandSender* sender, int cmd, int data, int id) override { }
  
    void saveOldState() override;
  
    // start of functions for Cartridge RAM tab
    uInt32 internalRamSize() override;
    uInt32 internalRamRPort(int start) override;
    string internalRamDescription() override;
    const ByteArray& internalRamOld(int start, int count) override;
    const ByteArray& internalRamCurrent(int start, int count) override;
    void internalRamSetValue(int addr, uInt8 value) override;
    uInt8 internalRamGetValue(int addr) override;
    string internalRamLabel(int addr) override;
    // end of functions for Cartridge RAM tab
  
    // Following constructors and assignment operators not supported
    Cartridge4KSCWidget() = delete;
    Cartridge4KSCWidget(const Cartridge4KSCWidget&) = delete;
    Cartridge4KSCWidget(Cartridge4KSCWidget&&) = delete;
    Cartridge4KSCWidget& operator=(const Cartridge4KSCWidget&) = delete;
    Cartridge4KSCWidget& operator=(Cartridge4KSCWidget&&) = delete;
};

#endif
