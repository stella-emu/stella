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

#ifndef JOYSTICK_DIALOG_HXX
#define JOYSTICK_DIALOG_HXX

class CommandSender;
class GuiObject;
class ButtonWidget;
class EditTextWidget;
class PopUpWidget;
class StringListWidget;
class LabelWidget;

#include "Dialog.hxx"

/**
 * Show a listing of joysticks currently stored in the eventhandler database,
 * and allow to remove those that aren't currently being used.
 */
class JoystickDialog : public Dialog
{
  public:
    JoystickDialog(GuiObject* boss, const GUI::Font& font);
    ~JoystickDialog() override = default;

    // (Re)populates the list from the eventhandler's physical joystick database
    void loadConfig() override;

  protected:
    void layout() override;
    // Reacts to a list selection, a port change, Remove, or Close
    void handleCommand(CommandSender* sender, GuiCmd::Code cmd, int data, int id) override;
    // UIReload (e.g. a device plugged/unplugged) refreshes the list
    void handleEvent(Event::Type event) override;

  private:
    // The known controllers, one row each
    StringListWidget* myJoyList{nullptr};
    // Read-only controller-ID readout for the selected entry
    LabelWidget*      myIDLbl{nullptr};
    EditTextWidget*   myJoyText{nullptr};
    // Default port assignment for the selected entry
    LabelWidget*      myJoyPortLbl{nullptr};
    PopUpWidget*      myJoyPort{nullptr};

    ButtonWidget* myRemoveBtn{nullptr};
    ButtonWidget* myCloseBtn{nullptr};

    // Numeric device ID if currently plugged in (else negative), parallel to myJoyList
    IntArray myJoyIDs;
    // Default port assignment, parallel to myJoyList
    IntArray myJoyPorts;

    struct Cmd {
      static constexpr GuiCmd::Code
        Remove = GuiCmd::of("JoystickDialog.Remove"),
        Port   = GuiCmd::of("JoystickDialog.Port");
    };

  private:
    // Following constructors and assignment operators not supported
    JoystickDialog() = delete;
    JoystickDialog(const JoystickDialog&) = delete;
    JoystickDialog(JoystickDialog&&) = delete;
    JoystickDialog& operator=(const JoystickDialog&) = delete;
    JoystickDialog& operator=(JoystickDialog&&) = delete;
};

#endif  // JOYSTICK_DIALOG_HXX
