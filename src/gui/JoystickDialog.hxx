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

#ifndef JOYSTICK_DIALOG_HXX
#define JOYSTICK_DIALOG_HXX

class GuiObject;
class ButtonWidget;
class EditTextWidgetWidget;
class StringListWidget;

#include "Dialog.hxx"
#include "Command.hxx"
#include "DialogContainer.hxx"

/**
 * Show a listing of joysticks currently stored in the eventhandler database,
 * and allow to remove those that aren't currently being used.
 */
class JoystickDialog : public Dialog
{
  public:
    JoystickDialog(GuiObject* boss, const GUI::Font& font,
                   int max_w, int max_h);
    virtual ~JoystickDialog();

    /** Place the dialog onscreen and center it */
    void show() { open(); }

  private:
    void loadConfig();
    void handleCommand(CommandSender* sender, int cmd, int data, int id);

  private:
    StringListWidget* myJoyList;
    EditTextWidget*   myJoyText;

    ButtonWidget* myRemoveBtn;
    ButtonWidget* myCloseBtn;

    IntArray myJoyIDs;

    enum { kRemoveCmd = 'JDrm' };
};

#endif
