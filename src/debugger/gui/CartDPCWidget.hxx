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

#ifndef CARTRIDGEDPC_WIDGET_HXX
#define CARTRIDGEDPC_WIDGET_HXX

class CartridgeDPC;
class PopUpWidget;
class DataGridWidget;

#include "CartDebugWidget.hxx"

class CartridgeDPCWidget : public CartDebugWidget
{
  public:
    CartridgeDPCWidget(GuiObject* boss, const GUI::Font& lfont,
                       const GUI::Font& nfont,
                       int x, int y, int w, int h,
                       CartridgeDPC& cart);
    virtual ~CartridgeDPCWidget() { }

    void saveOldState();

    void loadConfig();
    void handleCommand(CommandSender* sender, int cmd, int data, int id);

    string bankState();

    // start of functions for Cartridge RAM tab
    uInt32 internalRamSize();
    uInt32 internalRamRPort(int start);
    string internalRamDescription(); 
    const ByteArray& internalRamOld(int start, int count);
    const ByteArray& internalRamCurrent(int start, int count);
    void internalRamSetValue(int addr, uInt8 value);
    uInt8 internalRamGetValue(int addr);
    // end of functions for Cartridge RAM tab   

  private:
    struct CartState {
      ByteArray tops;
      ByteArray bottoms;
      IntArray counters;
      ByteArray flags;
      BoolArray music;
      uInt8 random;
      ByteArray internalram;
    };

  private:
    CartridgeDPC& myCart;
    PopUpWidget* myBank;

    DataGridWidget* myTops;
    DataGridWidget* myBottoms;
    DataGridWidget* myCounters;
    DataGridWidget* myFlags;
    DataGridWidget* myMusicMode;
    DataGridWidget* myRandom;

    CartState myOldState;

    enum { kBankChanged = 'bkCH' };
};

#endif
