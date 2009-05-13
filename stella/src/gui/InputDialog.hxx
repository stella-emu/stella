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
// Copyright (c) 1995-2009 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
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
class CheckBoxWidget;
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
                const GUI::Font& font);
    ~InputDialog();

  protected:
    virtual void handleKeyDown(int ascii, int keycode, int modifiers);
    virtual void handleJoyDown(int stick, int button);
    virtual void handleJoyAxis(int stick, int axis, int value);
    virtual bool handleJoyHat(int stick, int hat, int value);
    virtual void handleCommand(CommandSender* sender, int cmd, int data, int id);

    void loadConfig();
    void saveConfig();

  private:
    void addVDeviceTab(const GUI::Font& font);

  private:
    enum {
      kLeftChanged     = 'LCch',
      kRightChanged    = 'RCch',
      kDeadzoneChanged = 'DZch',
      kPaddleChanged   = 'PDch',
      kPSpeedChanged   = 'PSch'
    };

    TabWidget* myTab;

    EventMappingWidget* myEmulEventMapper;
    EventMappingWidget* myMenuEventMapper;

    PopUpWidget* myLeftPort;
    PopUpWidget* myRightPort;

    SliderWidget*     myDeadzone;
    StaticTextWidget* myDeadzoneLabel;
    SliderWidget*     myPaddleMode;
    StaticTextWidget* myPaddleModeLabel;
    SliderWidget*     myPaddleSpeed;
    StaticTextWidget* myPaddleLabel;
    EditTextWidget*   myAVoxPort;
};

#endif
