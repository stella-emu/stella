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

#ifndef CONTROLLER_WIDGET_HXX
#define CONTROLLER_WIDGET_HXX

class GuiObject;
class ButtonWidget;

#include "Widget.hxx"
#include "Command.hxx"

class ControllerWidget : public Widget, public CommandSender
{
  public:
    ControllerWidget(GuiObject* boss, const GUI::Font& font, int x, int y,
                     Controller& controller)
      : Widget(boss, font, x, y, 16, 16),
        CommandSender(boss),
        myController(controller)
    {
      _type = kControllerWidget;
/* FIXME - add this to controllers that won't have their own widget class

      bool leftport = controller.jack() == Controller::Left;
      const string& label = leftport ? "Left (Unknown):" : "Right (Unknown):";
      const int fontHeight = font.getFontHeight(),
                lineHeight = font.getLineHeight(),
                lwidth = font.getStringWidth("Controller not implemented");
      new StaticTextWidget(boss, font, x, y+2, lwidth,
                           fontHeight, label, kTextAlignLeft);
      new StaticTextWidget(boss, font, x, y+2+2*lineHeight, lwidth,
                           fontHeight, "Controller not implemented",
                           kTextAlignLeft);
      _w = lwidth + 10;
      _h = 6 * lineHeight;
*/
    }

    virtual ~ControllerWidget() { };

    virtual void loadConfig() { };
    virtual void handleCommand(CommandSender* sender, int cmd, int data, int id) { };

  protected:
    Controller& myController;
};

#endif
