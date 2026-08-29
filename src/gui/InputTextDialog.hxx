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

#ifndef INPUT_TEXT_DIALOG_HXX
#define INPUT_TEXT_DIALOG_HXX

class GuiObject;
class LabelWidget;
class EditTextWidget;

#include "Dialog.hxx"
#include "Command.hxx"
#include "EditableWidget.hxx"

/**
  A small dialog with one or more label+text-field rows, used to
  prompt for free-form text input.

  @author  Stephen Anthony and Thomas Jentzsch
*/
class InputTextDialog : public Dialog, public CommandSender
{
  public:
    // One label+field row per entry in 'labels'; the dialog width is 39
    // characters unless overridden by the 'widthChars' overload below
    InputTextDialog(GuiObject* boss, const GUI::Font& font,
                    const StringList& labels, string_view title = "");
    // As above, with the labels and the edit fields in different fonts
    InputTextDialog(GuiObject* boss, const GUI::Font& lfont,
                    const GUI::Font& nfont, const StringList& labels,
                    string_view title = "");
    // For a boss-less dialog (no CommandSender target); states its own width
    InputTextDialog(OSystem& osystem, DialogContainer& parent,
                    const GUI::Font& font, const StringList& labels,
                    string_view title, int widthChars);

    ~InputTextDialog() override = default;

    /** Place the input dialog onscreen and center it */
    void show();

    /** Show input dialog onscreen at the specified coordinates */
    void show(uInt32 x, uInt32 y, const Common::Rect& bossRect);

    /** This dialog uses its own positioning, so we override Dialog::center() */
    void setPosition() override;

    // The text entered in field 'idx', or empty if out of range
    const string& getResult(int idx = 0);

    void setText(string_view str, int idx = 0);
    void setTextFilter(const EditableWidget::TextFilter& f, int idx = 0);
    // Also fixes the field's width so layout() doesn't stretch it to fill the row
    void setMaxLen(int len, int idx = 0);
    void setToolTip(string_view str, int idx = 0);

    // Command sent (via CommandSender) when OK/Enter accepts the input
    void setEmitSignal(GuiCmd::Code cmd) { myCmd = cmd; }
    // Shows an error/status message below the fields, cleared on the next edit
    void setMessage(string_view title);

    void setFocus(int idx = 0);
    void setEditable(bool editable, int idx = 0);

  protected:
    // Shared setup for all three ctors: builds the label+field rows and the message line
    void initialize(const GUI::Font& lfont, const GUI::Font& nfont,
                    const StringList& labels, int widthChars = 39);
    void layout() override;
    // OK/Enter sends myCmd (leaving the parent to close/validate); editing
    // after an error clears the message
    void handleCommand(CommandSender* sender, GuiCmd::Code cmd, int data, int id) override;

  private:
    // One label per row
    vector<LabelWidget*> myLbl;
    // One edit field per row, parallel to myLbl
    vector<EditTextWidget*> myInput;
    // Status/error line below the rows (see setMessage())
    LabelWidget* myMessage{nullptr};

    // Dialog width (in characters) and per-input character limit (0 = fill),
    // both needed by layout() to reflow from the current font
    int myWidthChars{39};
    IntArray myMaxLen;

    // True unless positioned via the (x,y) overload of show()
    bool myEnableCenter{true};
    // Set by setMessage(), cleared on the next edit (see handleCommand())
    bool myErrorFlag{false};
    // Command sent on accept (see setEmitSignal())
    GuiCmd::Code myCmd{GuiCmd::None};

    // Screen position from the (x,y) overload of show() (see setPosition())
    uInt32 myXOrig{0}, myYOrig{0};

  private:
    // Following constructors and assignment operators not supported
    InputTextDialog() = delete;
    InputTextDialog(const InputTextDialog&) = delete;
    InputTextDialog(InputTextDialog&&) = delete;
    InputTextDialog& operator=(const InputTextDialog&) = delete;
    InputTextDialog& operator=(InputTextDialog&&) = delete;
};

#endif  // INPUT_TEXT_DIALOG_HXX
