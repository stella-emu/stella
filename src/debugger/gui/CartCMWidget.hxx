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

#ifndef CARTRIDGECM_WIDGET_HXX
#define CARTRIDGECM_WIDGET_HXX

class CartridgeCM;
class CheckboxWidget;
class DataGridWidget;
class EditTextWidget;
class PopUpWidget;
class ToggleBitWidget;

#include "CartDebugWidget.hxx"

class CartridgeCMWidget : public CartDebugWidget
{
  public:
    CartridgeCMWidget(GuiObject* boss, const GUI::Font& lfont,
                      const GUI::Font& nfont,
                      int x, int y, int w, int h,
                      CartridgeCM& cart);
    virtual ~CartridgeCMWidget() { }

    void saveOldState();

    void loadConfig();
    void handleCommand(CommandSender* sender, int cmd, int data, int id);

    string bankState();

  private:
    struct CartState {
      uInt8 swcha;
      uInt8 column;
    };

  private:
    CartridgeCM& myCart;
    PopUpWidget* myBank;

    ToggleBitWidget* mySWCHA;
    DataGridWidget* myColumn;
    CheckboxWidget *myAudIn, *myAudOut, *myIncrease, *myReset;
    CheckboxWidget* myRow[4];
    CheckboxWidget *myFunc, *myShift;
    EditTextWidget* myRAM;

    CartState myOldState;

    enum { kBankChanged = 'bkCH' };
};

#endif
