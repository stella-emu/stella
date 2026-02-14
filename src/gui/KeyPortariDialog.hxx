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

#ifndef KEYPORTARI_DIALOG_HXX
#define KEYPORTARI_DIALOG_HXX

class CommandSender;
class PopUpWidget;

#include "Dialog.hxx"

/**
 * Allow assigning controllers to the four KeyPortari ports.
 */
class KeyPortariDialog: public Dialog
{
  public:
    KeyPortariDialog(GuiObject* boss, const GUI::Font& font, int max_w, int max_h,
                   Properties& properties);
    ~KeyPortariDialog() override = default;

  private:
    void loadConfig() override;
    void saveConfig() override;
    void handleCommand(CommandSender* sender, int cmd, int data, int id) override;
    void setDefaults() override;

    void loadControllerProperties(const Properties& props);

  private:
  
    StaticTextWidget* myProtocolLabel{nullptr};
    PopUpWidget*      myProtocol{nullptr};
  
    StaticTextWidget* myLeftPortLabel{nullptr};
    PopUpWidget*      myLeft1Port{nullptr};

    StaticTextWidget* myRightPortLabel{nullptr};
    PopUpWidget*      myRight1Port{nullptr};

    // Game properties for currently loaded ROM
    Properties& myGameProperties;

  private:
    // Following constructors and assignment operators not supported
    KeyPortariDialog() = delete;
    KeyPortariDialog(const KeyPortariDialog&) = delete;
    KeyPortariDialog(KeyPortariDialog&&) = delete;
    KeyPortariDialog& operator=(const KeyPortariDialog&) = delete;
    KeyPortariDialog& operator=(KeyPortariDialog&&) = delete;
};

#endif
