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

#ifndef CARTRIDGE3E_WIDGET_HXX
#define CARTRIDGE3E_WIDGET_HXX

class Cartridge3E;
class PopUpWidget;

#include "CartDebugWidget.hxx"

class Cartridge3EWidget : public CartDebugWidget
{
  public:
    Cartridge3EWidget(GuiObject* boss, const GUI::Font& lfont,
                      const GUI::Font& nfont,
                      int x, int y, int w, int h,
                      Cartridge3E& cart);
    virtual ~Cartridge3EWidget() { }

  private:
    Cartridge3E& myCart;
    const uInt32 myNumRomBanks;
    const uInt32 myNumRamBanks;
    PopUpWidget *myROMBank, *myRAMBank;
  
    struct CartState {
      ByteArray internalram;
    };  
    CartState myOldState;   

    enum {
      kROMBankChanged = 'rmCH',
      kRAMBankChanged = 'raCH'
    };

  private:
    void saveOldState() override;
    void loadConfig() override;
    void handleCommand(CommandSender* sender, int cmd, int data, int id) override;

    string bankState() override;
  
    // start of functions for Cartridge RAM tab
    uInt32 internalRamSize() override;
    uInt32 internalRamRPort(int start) override;
    string internalRamDescription() override;
    const ByteArray& internalRamOld(int start, int count) override;
    const ByteArray& internalRamCurrent(int start, int count) override;
    void internalRamSetValue(int addr, uInt8 value) override;
    uInt8 internalRamGetValue(int addr) override;
    // end of functions for Cartridge RAM tab

    // Following constructors and assignment operators not supported
    Cartridge3EWidget() = delete;
    Cartridge3EWidget(const Cartridge3EWidget&) = delete;
    Cartridge3EWidget(Cartridge3EWidget&&) = delete;
    Cartridge3EWidget& operator=(const Cartridge3EWidget&) = delete;
    Cartridge3EWidget& operator=(Cartridge3EWidget&&) = delete;
};

#endif
