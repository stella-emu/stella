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

#ifndef INPUT_DIALOG_HXX
#define INPUT_DIALOG_HXX

class OSystem;
class GuiObject;
class TabWidget;
class EventMappingWidget;
class CheckboxWidget;
class EditTextWidget;
class PopUpWidget;
class SliderWidget;
class StaticTextWidget;

#include "Dialog.hxx"
#include "bspf.hxx"

class InputDialog : public Dialog
{
  public:
    InputDialog(OSystem* osystem, DialogContainer* parent,
                const GUI::Font& font, int max_w, int max_h);
    ~InputDialog();

  protected:
    virtual void handleKeyDown(StellaKey key, StellaMod mod, char ascii);
    virtual void handleJoyDown(int stick, int button);
    virtual void handleJoyAxis(int stick, int axis, int value);
    virtual bool handleJoyHat(int stick, int hat, int value);
    virtual void handleCommand(CommandSender* sender, int cmd, int data, int id);

    void loadConfig();
    void saveConfig();
    void setDefaults();

  private:
    void addDevicePortTab(const GUI::Font& font);

  private:
    enum {
      kDeadzoneChanged = 'DZch',
      kDPSpeedChanged  = 'PDch',
      kMPSpeedChanged  = 'PMch'
    };

    TabWidget* myTab;

    EventMappingWidget* myEmulEventMapper;
    EventMappingWidget* myMenuEventMapper;

    PopUpWidget* mySAPort;

    EditTextWidget*   myAVoxPort;

    SliderWidget*     myDeadzone;
    StaticTextWidget* myDeadzoneLabel;
    SliderWidget*     myDPaddleSpeed;
    SliderWidget*     myMPaddleSpeed;
    StaticTextWidget* myDPaddleLabel;
    StaticTextWidget* myMPaddleLabel;
    CheckboxWidget*   myAllowAll4;
    CheckboxWidget*   myGrabMouse;
    PopUpWidget*      myMouseControl;
};

#endif
