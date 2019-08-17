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
// Copyright (c) 1995-2019 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#ifndef LAUNCHER_DIALOG_HXX
#define LAUNCHER_DIALOG_HXX

class ButtonWidget;
class CommandSender;
class ContextMenu;
class DialogContainer;
class BrowserDialog;
class OptionsDialog;
class GlobalPropsDialog;
class StellaSettingsDialog;
class OSystem;
class Properties;
class EditTextWidget;
class FileListWidget;
class RomInfoWidget;
class StaticTextWidget;
namespace GUI {
  class MessageBox;
}

#include <unordered_map>

#include "bspf.hxx"
#include "Dialog.hxx"
#include "FSNode.hxx"
#include "Stack.hxx"

class LauncherDialog : public Dialog
{
  public:
    // These must be accessible from dialogs created by this class
    enum {
      kLoadROMCmd      = 'STRT',  // load currently selected ROM
      kRomDirChosenCmd = 'romc'   // rom dir chosen
    };

  public:
    LauncherDialog(OSystem& osystem, DialogContainer& parent,
                   int x, int y, int w, int h);
    virtual ~LauncherDialog() = default;

    /**
      Get path for the currently selected file.

      @return path if a valid ROM file, else the empty string
    */
    const string& selectedRom() const;

    /**
      Get MD5sum for the currently selected file.
      If the MD5 hasn't already been calculated, it will be
      calculated (and cached) for future use.

      @return md5sum if a valid ROM file, else the empty string
    */
    const string& selectedRomMD5();

    /**
      Get node for the currently selected directory.

      @return FilesystemNode currently active
    */
    const FilesystemNode& currentNode() const;

    /**
      Reload the current listing
    */
    void reload();

  private:
    void center() override { positionAt(0); }
    void handleKeyDown(StellaKey key, StellaMod mod, bool repeated) override;
    void handleMouseDown(int x, int y, MouseButton b, int clickCount) override;
    void handleCommand(CommandSender* sender, int cmd, int data, int id) override;
    void handleJoyDown(int stick, int button, bool longPress) override;
    void handleJoyUp(int stick, int button) override;
    Event::Type getJoyAxisEvent(int stick, JoyAxis axis, JoyDir adir, int button) override;

    void loadConfig() override;
    void updateUI();
    void applyFiltering();

    void loadRom();
    void loadRomInfo();
    void handleContextMenu();
    void showOnlyROMs(bool state);
    void openSettings();

  private:
    unique_ptr<OptionsDialog> myOptionsDialog;
    unique_ptr<StellaSettingsDialog> myStellaSettingsDialog;
    unique_ptr<ContextMenu> myMenu;
    unique_ptr<GlobalPropsDialog> myGlobalProps;
    unique_ptr<BrowserDialog> myRomDir;

    ButtonWidget* myStartButton;
    ButtonWidget* myPrevDirButton;
    ButtonWidget* myOptionsButton;
    ButtonWidget* myQuitButton;

    FileListWidget*   myList;
    StaticTextWidget* myDirLabel;
    EditTextWidget*   myDir;
    StaticTextWidget* myRomCount;
    EditTextWidget*   myPattern;
    CheckboxWidget*   myAllFiles;

    RomInfoWidget* myRomInfoWidget;
    std::unordered_map<string,string> myMD5List;

    int mySelectedItem;

    bool myShowOnlyROMs;
    bool myUseMinimalUI;
    bool myEventHandled;

    enum {
      kAllfilesCmd = 'lalf',  // show all files (or ROMs only)
      kPrevDirCmd  = 'PRVD',
      kOptionsCmd  = 'OPTI',
      kQuitCmd     = 'QUIT'
    };

  private:
    // Following constructors and assignment operators not supported
    LauncherDialog() = delete;
    LauncherDialog(const LauncherDialog&) = delete;
    LauncherDialog(LauncherDialog&&) = delete;
    LauncherDialog& operator=(const LauncherDialog&) = delete;
    LauncherDialog& operator=(LauncherDialog&&) = delete;
};

#endif
