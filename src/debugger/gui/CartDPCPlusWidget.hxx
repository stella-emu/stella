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

#ifndef CARTRIDGEDPCPLUS_WIDGET_HXX
#define CARTRIDGEDPCPLUS_WIDGET_HXX

class CartridgeDPCPlus;
class PopUpWidget;
class CheckboxWidget;
class DataGridWidget;

#include "CartDebugWidget.hxx"

class CartridgeDPCPlusWidget : public CartDebugWidget
{
  public:
    CartridgeDPCPlusWidget(GuiObject* boss, const GUI::Font& lfont,
                           const GUI::Font& nfont,
                           int x, int y, int w, int h,
                           CartridgeDPCPlus& cart);
    virtual ~CartridgeDPCPlusWidget() { }

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
      IntArray fraccounters;
      ByteArray fracinc;
      ByteArray param;
      IntArray mcounters;
      IntArray mfreqs;
      IntArray mwaves;
      uInt32 random;
      ByteArray internalram;
    };

  private:
    CartridgeDPCPlus& myCart;
    PopUpWidget* myBank;

    DataGridWidget* myTops;
    DataGridWidget* myBottoms;
    DataGridWidget* myCounters;
    DataGridWidget* myFracCounters;
    DataGridWidget* myFracIncrements;
    DataGridWidget* myParameter;
    DataGridWidget* myMusicCounters;
    DataGridWidget* myMusicFrequencies;
    DataGridWidget* myMusicWaveforms;
    CheckboxWidget* myFastFetch;
    CheckboxWidget* myIMLDA;
    DataGridWidget* myRandom;

    CartState myOldState;

    enum { kBankChanged = 'bkCH' };
};

#endif
