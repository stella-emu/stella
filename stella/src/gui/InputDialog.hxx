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
// Copyright (c) 1995-2005 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: InputDialog.hxx,v 1.1 2005-11-13 22:25:47 stephena Exp $
//============================================================================

#ifndef INPUT_DIALOG_HXX
#define INPUT_DIALOG_HXX

class OSystem;
class GuiObject;
class TabWidget;
class EventMappingWidget;
class StaticTextWidget;
class PopUpWidget;

#include "Dialog.hxx"
#include "bspf.hxx"

class InputDialog : public Dialog
{
  public:
    InputDialog(OSystem* osystem, DialogContainer* parent,
                int x, int y, int w, int h);
    ~InputDialog();

    virtual void handleKeyDown(int ascii, int keycode, int modifiers);
    virtual void handleJoyDown(int stick, int button);

  protected:
    virtual void handleCommand(CommandSender* sender, int cmd, int data, int id);

    void loadConfig();
    void saveConfig();

  private:
    TabWidget* myTab;

    EventMappingWidget* myEventMapper;
    PopUpWidget*        myPaddleModePopup;
    StaticTextWidget*   myPaddleModeText;
};

#endif
