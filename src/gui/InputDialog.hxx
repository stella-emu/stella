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
// Copyright (c) 1995-2026 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#ifndef INPUT_DIALOG_HXX
#define INPUT_DIALOG_HXX

class OSystem;
class GuiObject;
class TabWidget;
class EventMappingWidget;
class CheckboxWidget;
class JoystickDialog;
class PopUpWidget;
class SliderWidget;
class LabelWidget;

#include "Dialog.hxx"
#include "bspf.hxx"

class InputDialog : public Dialog
{
  public:
    InputDialog(OSystem& osystem, DialogContainer& parent, const GUI::Font& font);
    ~InputDialog() override;

    void loadConfig() override;
    void saveConfig() override;
    void setDefaults() override;

  protected:
    void layout() override;

    // disable repeat during and directly after mapping events
    bool repeatEnabled() override;

    void handleKeyDown(StellaKey key, StellaMod mod, bool repeated) override;
    void handleKeyUp(StellaKey key, StellaMod mod) override;
    void handleJoyDown(int stick, int button, bool longPress) override;
    void handleJoyUp(int stick, int button) override;
    void handleJoyAxis(int stick, JoyAxis axis, JoyDir adir, int button) override;
    bool handleJoyHat(int stick, int hat, JoyHatDir hdir, int button) override;
    void handleCommand(CommandSender* sender, int cmd, int data, int id) override;

  private:
    void addDevicePortTab();
    void addMouseTab();

    void handleMouseControlState();
    void handleCursorState();
    void updateDejitterAveraging();
    void updateDejitterReaction();
    void updateAutoFireRate();
    void eraseEEPROM();

  private:
    enum {
      kDDeadzoneChanged   = 'DDch',
      kADeadzoneChanged   = 'ADch',
      kPSpeedChanged      = 'Ppch',
      kDejitterAvChanged  = 'JAch',
      kDejitterReChanged  = 'JRch',
      kDPSpeedChanged     = 'DSch',
      kAutoFireChanged    = 'AFch',
      kAutoFireRate       = 'AFra',
      kTBSpeedChanged     = 'TBch',
      kDCSpeedChanged     = 'DCch',
      kDBButtonPressed    = 'DBbp',
      kEEButtonPressed    = 'EEbp',
      kMouseCtrlChanged   = 'MCch',
      kCursorStateChanged = 'CSch',
      kMPSpeedChanged     = 'PMch',
    };

    TabWidget* myTab{nullptr};

    EventMappingWidget* myEventMapper{nullptr};

    CheckboxWidget*   mySAPort{nullptr};

    LabelWidget* myAVoxPortLbl{nullptr};
    PopUpWidget*      myAVoxPort{nullptr};

    LabelWidget*    myDigitalDeadzoneLbl{nullptr};
    SliderWidget*   myDigitalDeadzone{nullptr};
    LabelWidget*    myAnalogDeadzoneLbl{nullptr};
    SliderWidget*   myAnalogDeadzone{nullptr};
    LabelWidget*    myPaddleSpeedLbl{nullptr};
    SliderWidget*   myPaddleSpeed{nullptr};
    LabelWidget*    myPaddleLinearityLbl{nullptr};
    SliderWidget*   myPaddleLinearity{nullptr};
    LabelWidget*    myDejitterBaseLbl{nullptr};
    SliderWidget*   myDejitterBase{nullptr};
    LabelWidget*    myDejitterDiffLbl{nullptr};
    SliderWidget*   myDejitterDiff{nullptr};
    LabelWidget*    myDPaddleSpeedLbl{nullptr};
    SliderWidget*   myDPaddleSpeed{nullptr};
    LabelWidget*    myAnalogPaddleLbl{nullptr};
    CheckboxWidget* myAutoFire{nullptr};
    LabelWidget*    myAutoFireRateLbl{nullptr};
    SliderWidget*   myAutoFireRate{nullptr};
    CheckboxWidget* myAllowAll4{nullptr};
    CheckboxWidget* myModCombo{nullptr};

    LabelWidget*    myAtariVoxLbl{nullptr};
    ButtonWidget*   myJoyDlgButton{nullptr};
    ButtonWidget*   myEraseEEPROMButton{nullptr};

    LabelWidget*    myMouseControlLbl{nullptr};
    PopUpWidget*    myMouseControl{nullptr};
    LabelWidget*    myMouseSensitivity{nullptr};
    LabelWidget*    myMPaddleSpeedLbl{nullptr};
    SliderWidget*   myMPaddleSpeed{nullptr};
    LabelWidget*    myTrackBallSpeedLbl{nullptr};
    SliderWidget*   myTrackBallSpeed{nullptr};
    LabelWidget*    myDrivingSpeedLbl{nullptr};
    SliderWidget*   myDrivingSpeed{nullptr};
    LabelWidget*    myCursorStateLbl{nullptr};
    PopUpWidget*    myCursorState{nullptr};
    CheckboxWidget* myGrabMouse{nullptr};

    // Show the list of joysticks that the eventhandler knows about
    unique_ptr<JoystickDialog> myJoyDialog;

  private:
    // Following constructors and assignment operators not supported
    InputDialog() = delete;
    InputDialog(const InputDialog&) = delete;
    InputDialog(InputDialog&&) = delete;
    InputDialog& operator=(const InputDialog&) = delete;
    InputDialog& operator=(InputDialog&&) = delete;
};

#endif  // INPUT_DIALOG_HXX
