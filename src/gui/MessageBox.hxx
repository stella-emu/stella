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

#ifndef MESSAGE_BOX_HXX
#define MESSAGE_BOX_HXX

class GuiObject;
class StaticTextWidget;

#include "Dialog.hxx"
#include "Command.hxx"
#include "DialogContainer.hxx"

/**
 * Show a simple message box containing the given text, with buttons 
 * prompting the user to accept or reject.  If the user selects 'OK',
 * the value of 'cmd' is returned.
 */
class MessageBox : public Dialog, public CommandSender
{
  public:
    MessageBox(GuiObject* boss, const GUI::Font& font, const StringList& text,
               int max_w, int max_h, int cmd = 0,
               const string& okText = "", const string& cancelText = "");
    MessageBox(GuiObject* boss, const GUI::Font& font, const string& text,
               int max_w, int max_h, int cmd = 0,
               const string& okText = "", const string& cancelText = "");
    virtual ~MessageBox();

    /** Place the input dialog onscreen and center it */
    void show() { parent().addDialog(this); }

  private:
    void addText(const GUI::Font& font, const StringList& text);
    void handleCommand(CommandSender* sender, int cmd, int data, int id);

  private:
    int myCmd;
};

#endif
