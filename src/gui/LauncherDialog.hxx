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

#ifndef LAUNCHER_DIALOG_HXX
#define LAUNCHER_DIALOG_HXX

class ButtonWidget;
class CommandSender;
class ContextMenu;
class DialogContainer;
class OSystem;
class Properties;
class EditTextWidget;
class NavigationWidget;
class LauncherFileListWidget;
class RomImageWidget;
class RomInfoWidget;
class StaticTextWidget;

namespace Common {
  struct Size;
}  // namespace Common
namespace GUI {
  class MessageBox;
}  // namespace GUI

#include <unordered_map>
#include <unordered_set>

#include "bspf.hxx"
#include "Dialog.hxx"
#include "FSNode.hxx"
#include "Variant.hxx"

class LauncherDialog : public Dialog, CommandSender
{
  public:
    // These must be accessible from dialogs created by this class
    enum {
      kLoadROMCmd      = 'STRT',  // load currently selected ROM
      kRomDirChosenCmd = 'romc',  // ROM dir chosen
      kFavChangedCmd   = 'favc',  // Favorite tracking changed
      kExtChangedCmd   = 'extc',  // File extension display changed
      kRomViewerChangedCmd = 'rmvc',  // ROM info viewer enabled/disabled
      kFontChangedCmd  = 'fntc',  // Launcher font changed
    };

  public:
    LauncherDialog(OSystem& osystem, DialogContainer& parent,
                   int w, int h);
    ~LauncherDialog() override;

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
      Get node for the currently selected entry.

      @return FSNode currently selected
    */
    const FSNode& currentNode() const;

    /**
      Get node for the current directory.

      @return FSNode (directory) currently active
    */
    const FSNode& currentDir() const;

    /**
      Reload the current listing
    */
    void reload();

    /**
      Quit the dialog
    */
    void quit();

    void loadConfig() override;
    void saveConfig() override;

    void setPosition() override { positionAt(0); }

    void tick() override;

  protected:
    void handleKeyDown(StellaKey key, StellaMod mod, bool repeated) override;
    void handleMouseUp(int x, int y, MouseButton b, int clickCount) override;
    void handleCommand(CommandSender* sender, int cmd, int data, int id) override;
    void handleJoyDown(int stick, int button, bool longPress) override;
    void handleJoyUp(int stick, int button) override;
    Event::Type getJoyAxisEvent(int stick, JoyAxis axis, JoyDir adir, int button) override;
    void layout() override;

  private:
    static constexpr int MIN_LAUNCHER_CHARS = 24;
    static constexpr int MIN_ROMINFO_CHARS = 30;
    static constexpr int MIN_ROMINFO_ROWS = 7; // full lines
    static constexpr int MIN_ROMINFO_LINES = 4; // extra lines

    void updateUI();
    // Clamp a desired ROM info column width to keep both the list usable
    // (horizontal) and the image + text fitting in the column (vertical)
    int clampRomInfoWidth(int imageWidth, int colHeight) const;
    void showRomWidgets(bool show);
    void updateRomCount();
    // These create the widgets and their non-geometry state (tooltips, focus
    // order, structural choices); layout() assigns all geometry
    void addFilteringWidgets();
    void addPathWidgets();
    int addRomWidgets();
    void addButtonWidgets();
    string getRomDir();

    /**
      Search if string contains pattern including wildcard '*'
      and '?' as joker, ignoring case.

      @param str      The searched string
      @param pattern  The pattern to search for

      @return True if pattern was found.
    */
    static bool matchWithWildcardsIgnoreCase(string_view str, string_view pattern);

    void applyFiltering();

    float getRomInfoZoom(int listHeight, float zoom) const;
    void setRomInfoFont(const Common::Size& area);
    // Show/hide the ROM info viewer at runtime (without rebuilding the launcher)
    void setRomInfoEnabled(bool enable);

    void loadRom();
    void loadRomInfo();
    void loadPendingRomInfo();
    void loadRandomRom();
    void openSettings();
    void openGameProperties();
    void openContextMenu(int x = -1, int y = -1);
    void openGlobalProps();
    void openHighScores();
    void openWhatsNew();
    void toggleSubDirs(bool toggle = true);
    void handleContextMenu();
    void handleQuit();
    void toggleExtensions();
    void toggleSorting();
    void handleFavoritesChanged();
    void removeAllFavorites();
    void removeAll(string_view name);
    void removeAllPopular();
    void removeAllRecent();

    ContextMenu& contextMenu();

  private:
    unique_ptr<Dialog> myDialog;
    unique_ptr<ContextMenu> myContextMenu;

    // automatically sized font for ROM info viewer
    unique_ptr<GUI::Font> myROMInfoFont;

    ButtonWidget*     mySettingsButton{nullptr};
    StaticTextWidget* myFilterLabel{nullptr};
    EditTextWidget*   myPattern{nullptr};
    ButtonWidget*     mySubDirsButton{nullptr};
    ButtonWidget*     myRandomRomButton{nullptr};
    StaticTextWidget* myRomCount{nullptr};
    ButtonWidget*     myHelpButton{nullptr};

    NavigationWidget* myNavigationBar{nullptr};
    ButtonWidget*     myReloadButton{nullptr};

    LauncherFileListWidget* myList{nullptr};

    ButtonWidget*     myStartButton{nullptr};
    ButtonWidget*     myGoUpButton{nullptr};
    ButtonWidget*     myOptionsButton{nullptr};
    ButtonWidget*     myQuitButton{nullptr};

    RomImageWidget*   myRomImageWidget{nullptr};
    RomInfoWidget*    myRomInfoWidget{nullptr};

    // ROM info column width as a fraction of the launcher content width.
    // Keeps the ROM info area scaling proportionally as the window resizes,
    // and is adjusted by dragging the divider.
    float             myRomInfoFraction{0.F};

    // The minimum content size, recomputed at the end of each layout() from the
    // laid-out widgets and used to clamp the next one (the widgets carry no
    // meaningful geometry until layout() runs, so it cannot be read up front)
    Common::Size      myMinSize;

    // Draggable divider between the list and the ROM info column
    Widget*           myDivider{nullptr};

    std::unordered_map<string, string, BSPF::StringHash, std::equal_to<>> myMD5List;

    // Show a message about the dangers of using this function
    unique_ptr<GUI::MessageBox> myConfirmMsg;

    int mySelectedItem{0};

    bool myShowRomInfo{false};
    bool myEventHandled{false};
    bool myPendingReload{false};
    uInt64 myReloadTime{0};
    bool myPendingRomInfo{false};
    uInt64 myRomInfoTime{0};

    enum {
      kSubDirsCmd    = 'lred',
      kLoadRndRomCmd = 'lrnd',  // load random ROM
      kOptionsCmd    = 'OPTI',
      kQuitCmd       = 'QUIT',
      kReloadCmd     = 'relc',
      kRomWidthCmd   = 'lrwd',
      kRmAllFav      = 'rmaf',
      kRmAllPop      = 'rmap',
      kRmAllRec      = 'rmar'
    };

  private:
    // Following constructors and assignment operators not supported
    LauncherDialog() = delete;
    LauncherDialog(const LauncherDialog&) = delete;
    LauncherDialog(LauncherDialog&&) = delete;
    LauncherDialog& operator=(const LauncherDialog&) = delete;
    LauncherDialog& operator=(LauncherDialog&&) = delete;
};

#endif  // LAUNCHER_DIALOG_HXX
