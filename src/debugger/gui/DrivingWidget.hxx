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
// Copyright (c) 1995-2012 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

#ifndef DRIVING_WIDGET_HXX
#define DRIVING_WIDGET_HXX

class ButtonWidget;
class CheckboxWidget;
class DataGridWidget;

#include "Control.hxx"
#include "Event.hxx"
#include "ControllerWidget.hxx"

class DrivingWidget : public ControllerWidget
{
  public:
    DrivingWidget(GuiObject* boss, const GUI::Font& font, int x, int y,
                  Controller& controller);
    virtual ~DrivingWidget();

    void loadConfig();
    void handleCommand(CommandSender* sender, int cmd, int data, int id);

  private:
    enum {
      kGreyUpCmd   = 'DWup',
      kGreyDownCmd = 'DWdn',
      kFireCmd     = 'DWfr'
    };
    ButtonWidget *myGreyUp, *myGreyDown;
    DataGridWidget* myGreyValue;
    CheckboxWidget* myFire;

    int myGreyIndex;

    static uInt8 ourGreyTable[4];
};

#endif
