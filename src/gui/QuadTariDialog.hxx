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

#ifndef QUADTARI_DIALOG_HXX
#define QUADTARI_DIALOG_HXX

class CommandSender;
class PopUpWidget;

#include "Dialog.hxx"

/**
 * Allow assigning controllers to the four QuadTari ports.
 */
class QuadTariDialog: public Dialog
{
  public:
    QuadTariDialog(GuiObject* boss, const GUI::Font& font, Properties& properties);
    ~QuadTariDialog() override = default;

    /** Place the dialog onscreen; also enables/disables each port's controls
        per enableLeft/enableRight */
    void show(bool enableLeft, bool enableRight);

    // Populates the four pop-ups (and their detected-controller labels) from myGameProperties
    void loadConfig() override;
    // Writes the four pop-ups' selections back to myGameProperties
    void saveConfig() override;
    // Loads this ROM's default properties and re-populates from them
    void setDefaults() override;

  protected:
    // OK saves, closes, and asks the launcher to load the ROM; Defaults resets
    void handleCommand(CommandSender* sender, GuiCmd::Code cmd, int data, int id) override;
    void layout() override;

  private:
    // Populates all four pop-ups/detected-labels from 'props'
    void loadControllerProperties(const Properties& props);
    // Selects 'key's controller in 'popup' and fills 'label' with what was
    // auto-detected (from the live console, or by scanning the ROM image)
    void defineController(const Properties& props, PropType key,
      Controller::Jack jack, PopUpWidget* popup, LabelWidget* label, bool first = true);

  private:
    // Kept so layout() can re-derive the pop-up width from the live font; the
    // pop-ups do not re-derive it themselves (see PopUpWidget::refreshFont)
    VariantList myCtrls;

    // Left port: header, plus two controller pop-ups each with a
    // detected-controller label below it
    LabelWidget* myLeftPortLbl{nullptr};
    LabelWidget* myLeft1PortLbl{nullptr};
    PopUpWidget* myLeft1Port{nullptr};
    LabelWidget* myLeft1PortDetected{nullptr};
    LabelWidget* myLeft2PortLbl{nullptr};
    PopUpWidget* myLeft2Port{nullptr};
    LabelWidget* myLeft2PortDetected{nullptr};

    // Right port (mirrors the left)
    LabelWidget* myRightPortLbl{nullptr};
    LabelWidget* myRight1PortLbl{nullptr};
    PopUpWidget* myRight1Port{nullptr};
    LabelWidget* myRight1PortDetected{nullptr};
    LabelWidget* myRight2PortLbl{nullptr};
    PopUpWidget* myRight2Port{nullptr};
    LabelWidget* myRight2PortDetected{nullptr};

    // Game properties for currently loaded ROM
    Properties& myGameProperties;

  private:
    // Following constructors and assignment operators not supported
    QuadTariDialog() = delete;
    QuadTariDialog(const QuadTariDialog&) = delete;
    QuadTariDialog(QuadTariDialog&&) = delete;
    QuadTariDialog& operator=(const QuadTariDialog&) = delete;
    QuadTariDialog& operator=(QuadTariDialog&&) = delete;
};

#endif  // QUADTARI_DIALOG_HXX
