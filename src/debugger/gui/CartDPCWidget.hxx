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

  private:
    struct CartState {
      ByteArray tops;
      ByteArray bottoms;
      IntArray counters;
      ByteArray flags;
      BoolArray music;
      uInt8 random;
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
