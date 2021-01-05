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

#ifndef CARTRIDGE_MNETWORK_WIDGET_HXX
#define CARTRIDGE_MNETWORK_WIDGET_HXX

class CartridgeMNetwork;
class PopUpWidget;

#include "CartDebugWidget.hxx"

class CartridgeMNetworkWidget : public CartDebugWidget
{
  public:
    CartridgeMNetworkWidget(GuiObject* boss, const GUI::Font& lfont,
                      const GUI::Font& nfont,
                      int x, int y, int w, int h,
                      //CartridgeE7& cart);
                      CartridgeMNetwork& cart);
    ~CartridgeMNetworkWidget() override = default;

  protected:
    //CartridgeE7& myCart;
    CartridgeMNetwork& myCart;

    PopUpWidget *myLower2K{nullptr}, *myUpper256B{nullptr};

    struct CartState
    {
      ByteArray internalram;
      uInt16 lowerBank{0};
      uInt16 upperBank{0};
    };
    CartState myOldState;

    enum
    {
      kLowerChanged = 'lwCH',
      kUpperChanged = 'upCH'
    };

  protected:
    void initialize(GuiObject* boss, CartridgeMNetwork& cart, ostringstream& info);
    virtual const char* getSpotLower(int idx) = 0;
    virtual const char* getSpotUpper(int idx) = 0;

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

  private:
    // Following constructors and assignment operators not supported
    CartridgeMNetworkWidget() = delete;
    CartridgeMNetworkWidget(const CartridgeMNetworkWidget&) = delete;
    CartridgeMNetworkWidget(CartridgeMNetworkWidget&&) = delete;
    CartridgeMNetworkWidget& operator=(const CartridgeMNetworkWidget&) = delete;
    CartridgeMNetworkWidget& operator=(CartridgeMNetworkWidget&&) = delete;
};

#endif
