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

#ifndef BROWSER_DIALOG_HXX
#define BROWSER_DIALOG_HXX

class GuiObject;
class ButtonWidget;
class EditTextWidget;
class FileListWidget;
class NavigationWidget;
class LabelWidget;

#include "Dialog.hxx"
#include "Command.hxx"
#include "FSNode.hxx"
#include "bspf.hxx"

/**
  A file/directory browser, invoked via one of its static show() methods
  for loading, saving, or selecting a directory.

  @author  Stephen Anthony and Thomas Jentzsch
*/
class BrowserDialog : public Dialog
{
  public:
    enum class Mode: uInt8 {
      FileLoad,       // File selector, no input from user
      FileLoadNoDirs, // File selector, no input from user, fixed directory
      FileSave,       // File selector, filename changable by user
      Directories     // Directories only, no input from user
    };

    /** Function which is run when the user clicks OK or Cancel.
        Boolean parameter is passed as 'true' when OK is clicked, else 'false'.
        FSNode parameter is what is currently selected in the browser.
    */
    using Command = std::function<void(bool, const FSNode&)>;

  public:
    ~BrowserDialog() override = default;

    /**
      Place the browser window onscreen, using the given attributes.

      @param parent     The parent object of the browser (cannot be nullptr)
      @param font       The font to use in the browser
      @param title      The title of the browser window
      @param startpath  The initial path to select in the browser
      @param mode       The functionality to use (load/save/display)
      @param command    The command to run when 'OK' or 'Cancel' is clicked
      @param namefilter Filter files/directories in browser display
    */
    static void show(Dialog* parent, const GUI::Font& font,
                     string_view title,string_view startpath,
                     BrowserDialog::Mode mode,
                     const Command& command,
                     const FSNode::NameFilter& namefilter = {
                      [](const FSNode&) { return true; }});

    /**
      Place the browser window onscreen, using the given attributes.

      @param parent     The parent object of the browser (cannot be nullptr)
      @param title      The title of the browser window
      @param startpath  The initial path to select in the browser
      @param mode       The functionality to use (load/save/display)
      @param command    The command to run when 'OK' or 'Cancel' is clicked
      @param namefilter Filter files/directories in browser display
    */
    static void show(Dialog* parent,
                     string_view title, string_view startpath,
                     BrowserDialog::Mode mode,
                     const Command& command,
                     const FSNode::NameFilter& namefilter = {
                      [](const FSNode&) { return true; } });

    /**
      Place the browser window onscreen with no parent dialog (e.g. from TIA
      emulation mode).  Uses the overlay menu as the DialogContainer.

      @param osystem    The OSystem instance
      @param title      The title of the browser window
      @param startpath  The initial path to select in the browser
      @param mode       The functionality to use (load/save/display)
      @param command    The command to run when 'OK' or 'Cancel' is clicked
      @param namefilter Filter files/directories in browser display
    */
    static void show(OSystem& osystem,
                     string_view title, string_view startpath,
                     BrowserDialog::Mode mode,
                     const Command& command,
                     const FSNode::NameFilter& namefilter = {
                      [](const FSNode&) { return true; } });

    /**
      Since the show methods allocate a static BrowserDialog, at some
      point we need to manually de-allocate it.  This method must be
      called from one of the lowest-level destructors to do that.
      Currently this is called from the OSystem destructor.
    */
    static void hide();

  protected:
    void layout() override;
    // Routes navigation keys to the file list first; the dialog only sees a
    // key that the list doesn't handle itself
    void handleKeyDown(StellaKey key, StellaMod mod, bool repeated) override;
    // OK/selection and Close invoke the stored callback and close(); the
    // navigation buttons jump directories; edits/selection changes refresh
    // the UI (see updateUI())
    void handleCommand(CommandSender* sender, GuiCmd::Code cmd, int data, int id) override;

  private:
    // No point calling these directly: the result can't be gotten back
    // that way. Use the static show() methods above instead.
    BrowserDialog(GuiObject* boss, const GUI::Font& font, int max_w, int max_h);
    BrowserDialog(OSystem& osystem, DialogContainer& parent,
                  const GUI::Font& font, int max_w, int max_h);

    // Creates all widgets; layout() derives their real geometry from
    // max_w/max_h and the current font
    void initialize(int max_w, int max_h);

    /** Place the browser window onscreen, using the given attributes */
    void show(string_view startpath,
              BrowserDialog::Mode mode,
              const Command& command,
              const FSNode::NameFilter& namefilter);

    /** Get resulting file node (called after receiving Cmd::Choose) */
    FSNode getResult() const;

    // Refreshes the up button, nav bar and OK-enabled state; 'fileSelected'
    // also fills in the selected-item field from the list's current entry
    void updateUI(bool fileSelected);

  private:
    // Command ids dispatched in handleCommand()
    struct Cmd {
      static constexpr GuiCmd::Code
        Choose  = GuiCmd::of("BrowserDialog.Choose"),
        GoUp    = GuiCmd::of("BrowserDialog.GoUp"),
        BaseDir = GuiCmd::of("BrowserDialog.BaseDir"),
        HomeDir = GuiCmd::of("BrowserDialog.HomeDir");
    };

    // Called when the user selects OK (bool is true) or Cancel (bool is false)
    // FSNode will be set to whatever is active (basically, getResult())
    Command _command;

    // File listing
    FileListWidget*   _fileList{nullptr};
    // Current-path display, kept in sync with _fileList
    NavigationWidget* _navigationBar{nullptr};
    // Currently selected item: its label, and its editable name field
    LabelWidget*      _name{nullptr};
    EditTextWidget*   _selected{nullptr};
    // Directory-navigation buttons
    ButtonWidget*     _goUpButton{nullptr};
    ButtonWidget*     _baseDirButton{nullptr};
    ButtonWidget*     _homeDirButton{nullptr};
    // Saves the current path as the default when checked (FileLoad/FileSave
    // modes only)
    CheckboxWidget*   _savePathBox{nullptr};

    // Which show() mode is active; governs which controls are visible (see
    // the instance show())
    BrowserDialog::Mode _mode{Mode::Directories};

    // The single lazily-created instance behind the static show() methods
    static unique_ptr<BrowserDialog> ourBrowser;

  private:
    // Following constructors and assignment operators not supported
    BrowserDialog() = delete;
    BrowserDialog(const BrowserDialog&) = delete;
    BrowserDialog(BrowserDialog&&) = delete;
    BrowserDialog& operator=(const BrowserDialog&) = delete;
    BrowserDialog& operator=(BrowserDialog&&) = delete;
};

#endif  // BROWSER_DIALOG_HXX
