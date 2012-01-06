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

#ifndef PADDLE_WIDGET_HXX
#define PADDLE_WIDGET_HXX

#include "Control.hxx"
#include "Event.hxx"
#include "ControllerWidget.hxx"

class PaddleWidget : public ControllerWidget
{
  public:
    PaddleWidget(GuiObject* boss, const GUI::Font& font, int x, int y,
                 Controller& controller);
    virtual ~PaddleWidget();

    void loadConfig();
    void handleCommand(CommandSender* sender, int cmd, int data, int id);

  private:
    enum { kP0Changed = 'P0ch', kP1Changed = 'P1ch',
           kP0Fire = 'P0fr', kP1Fire = 'P1fr' };

    SliderWidget *myP0Resistance, *myP1Resistance;
    CheckboxWidget *myP0Fire, *myP1Fire;
};

#endif
