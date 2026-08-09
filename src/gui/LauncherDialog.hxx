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
class LabelWidget;

namespace Common {
  struct Size;
}  // namespace Common

#include <unordered_map>
#include <unordered_set>

#include "bspf.hxx"
#include "Dialog.hxx"
#include "FSNode.hxx"
#include "Variant.hxx"

/**
  The main ROM launcher: file listing, filtering/navigation, ROM info/
  image preview, and the sub-dialogs it opens (settings, properties,
  high scores, What's New).

  @author  Stephen Anthony and Thomas Jentzsch
*/
class LauncherDialog : public Dialog, CommandSender
{
  public:
    // These must be accessible from dialogs created by this class
    struct Cmd {
      static constexpr GuiCmd::Code
        LoadROM           = GuiCmd::of("LauncherDialog.LoadROM"),
        RomDirChosen      = GuiCmd::of("LauncherDialog.RomDirChosen"),
        FavouritesChanged = GuiCmd::of("LauncherDialog.FavouritesChanged"),
        ExtensionsChanged = GuiCmd::of("LauncherDialog.ExtensionsChanged"),
        RomViewerChanged  = GuiCmd::of("LauncherDialog.RomViewerChanged"),
        FontChanged       = GuiCmd::of("LauncherDialog.FontChanged"),
        ButtonsChanged    = GuiCmd::of("LauncherDialog.ButtonsChanged"),
        // The rest are dispatched only within handleCommand()
        SubDirs           = GuiCmd::of("LauncherDialog.SubDirs"),
        LoadRandomRom     = GuiCmd::of("LauncherDialog.LoadRandomRom"),
        Options           = GuiCmd::of("LauncherDialog.Options"),
        Quit              = GuiCmd::of("LauncherDialog.Quit"),
        Reload            = GuiCmd::of("LauncherDialog.Reload"),
        RomWidthChanged   = GuiCmd::of("LauncherDialog.RomWidthChanged");
    };

  public:
    LauncherDialog(OSystem& osystem, DialogContainer& parent, int w, int h);
    // Out-of-line: myContextMenu (unique_ptr<ContextMenu>) needs its
    // complete type here
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

    // Shows 'What's New' on a new Stella version; on the very first call
    // (list still empty) also loads the last-visited directory/ROM
    void loadConfig() override;
    // Persists the current directory (if 'follow launcher' is set) and
    // favorites
    void saveConfig() override;

    // Always centers (position 0), ignoring the dialogpos setting ordinary
    // dialogs use
    void setPosition() override { positionAt(0); }

    // Fires the delayed reload (after filter typing) and delayed ROM-info
    // load (after list navigation) once their timers expire
    void tick() override;

  protected:
    // Ctrl+letter shortcuts (subdirs/extensions/favorite/properties/high
    // scores/options/global props/reload/sorting/remove-favorite) and
    // Alt+R for a random ROM, checked before the key reaches the list or
    // the dialog's own handling
    void handleKeyDown(StellaKey key, StellaMod mod, bool repeated) override;
    // Right-click inside the list opens the context menu; everything else
    // goes to the base class
    void handleMouseUp(int x, int y, MouseButton b, int clickCount) override;
    // Cmd::RomWidthChanged resizes the ROM-info column as the divider is dragged;
    // item/directory selection loads the ROM or reloads the directory; the
    // remaining ids toggle subdirs/reload/settings/favorites/extensions/
    // font, or dispatch context-menu and ROM-info-link clicks
    void handleCommand(CommandSender* sender, GuiCmd::Code cmd, int data, int id) override;
    // Lets the list see a long button press (ListWidget::Cmd::LongButtonPress)
    // before restoring its normal flags
    void handleJoyDown(int stick, int button, bool longPress) override;
    // Opens power-on options / settings on the 2nd/4th button when nothing
    // else claims them
    void handleJoyUp(int stick, int button) override;
    // Remaps the launcher's Tab-prev/next axis events to Page Up/Down
    Event::Type getJoyAxisEvent(int stick, JoyAxis axis, JoyDir adir, int button) override;
    void layout() override;

  private:
    // Minimum ROM-list width, in characters, kept visible when the ROM
    // info column grows
    static constexpr int MIN_LAUNCHER_CHARS = 24;
    // Minimum ROM-info column width, in characters
    static constexpr int MIN_ROMINFO_CHARS = 30;
    static constexpr int MIN_ROMINFO_ROWS = 7; // full lines
    static constexpr int MIN_ROMINFO_LINES = 4; // extra lines

    // Refreshes the Go-Up button, navigation bar, ROM count, and ROM info
    // after the list or directory changes
    void updateUI();
    // Clamp a desired ROM info column width to keep both the list usable
    // (horizontal) and the image + text fitting in the column (vertical)
    int clampRomInfoWidth(int imageWidth, int colHeight) const;
    // Shows/hides the ROM image/info widgets and their divider
    void showRomWidgets(bool show);
    // Shows/hides the bottom button row, disabling (not just hiding) so
    // it drops out of focus cycling
    void showButtonWidgets(bool show);
    // Updates the "<n> items found" label from the current list
    void updateRomCount();
    // These create the widgets and their non-geometry state (tooltips, focus
    // order, structural choices); layout() assigns all geometry
    void addFilteringWidgets();
    void addPathWidgets();
    int addRomWidgets();
    void addButtonWidgets();
    // The effective ROM directory: the commandline temp directory if set,
    // else the configured one
    string getRomDir();

    /**
      Search if string contains pattern including wildcard '*'
      and '?' as joker, ignoring case.

      @param str      The searched string
      @param pattern  The pattern to search for

      @return True if pattern was found.
    */
    static bool matchWithWildcardsIgnoreCase(string_view str, string_view pattern);

    // Installs the list's name filter: valid ROM extensions only (no bare
    // ZIPs), further narrowed by the filter text pattern
    void applyFiltering();

    // Shrinks the requested zoom level just enough to keep the list and
    // ROM info column both usable at the current dialog size
    float getRomInfoZoom(int listHeight, float zoom) const;
    // Picks the largest registered ROM-info font that fits 'area', unless
    // the user has set one explicitly
    void setRomInfoFont(const Common::Size& area);
    // Show/hide the ROM info viewer at runtime (without rebuilding the launcher)
    void setRomInfoEnabled(bool enable);
    // Show/hide the bottom button row at runtime (without rebuilding the launcher)
    void setButtonsEnabled(bool enable);

    // Saves favorites/config and starts emulation of the currently
    // selected ROM
    void loadRom();
    // Schedules a delayed ROM-info/image update for the current selection
    // (see tick()/loadPendingRomInfo())
    void loadRomInfo();
    // Loads the current selection's properties into the ROM image/info
    // widgets once the delay from loadRomInfo() has elapsed
    void loadPendingRomInfo();
    // Picks a random non-directory entry from the current list and loads it
    void loadRandomRom();
    // Opens the basic or advanced settings dialog, whichever the user has
    // selected
    void openSettings();
    // Opens the Game Properties dialog for the currently selected ROM
    void openGameProperties();
    // Builds and shows the ROM-list context menu at (x, y), or at the
    // selected item if invoked from a long button press (x/y < 0)
    void openContextMenu(int x = -1, int y = -1);
    // Opens the dialog for temporarily overriding this ROM's properties
    void openGlobalProps();
    // Opens the High Scores dialog for the currently selected ROM
    void openHighScores();
    // Opens the dialog describing what's new in this Stella version
    void openWhatsNew();
    // Updates the subdirs button's icon and the list's include-subdirs
    // flag; 'toggle' also flips and persists the setting, then reloads
    void toggleSubDirs(bool toggle = true);
    // Dispatches the tag of whichever context-menu item was chosen
    void handleContextMenu();
    // Closes the launcher and quits Stella
    void handleQuit();
    // Flips whether file extensions are shown, persists it, and reloads
    void toggleExtensions();
    // In a virtual directory (Favorites/Popular/Recent), flips between
    // normal and alternative sort order and reloads
    void toggleSorting();
    // Reloads or clears favorites tracking after the 'favorites' setting
    // changed elsewhere (e.g. the UI options dialog)
    void handleFavoritesChanged();
    // Confirms, then clears the entire Favorites list
    void removeAllFavorites();
    // Confirms, then runs 'action' and reloads; shared by
    // removeAllPopular()/removeAllRecent()
    void removeAll(string_view name, const std::function<void()>& action);
    // Confirms, then clears the entire Most Popular list
    void removeAllPopular();
    // Confirms, then clears the entire Recently Played list
    void removeAllRecent();

    // The lazily-created context menu shared by every ROM-list
    // right-click/long-press
    ContextMenu& contextMenu();

  private:
    // Whichever sub-dialog (Settings/GameInfo/GlobalProps/HighScores/
    // WhatsNew) is currently open
    unique_ptr<Dialog> myDialog;
    // The ROM-list right-click/long-press menu, created on first use
    // (see contextMenu())
    unique_ptr<ContextMenu> myContextMenu;

    // Filtering row: reload/random-ROM/subdirs/options buttons, the
    // filter text field, and the "<n> items found" label
    ButtonWidget*   mySettingsButton{nullptr};
    LabelWidget*    myFilterLbl{nullptr};
    EditTextWidget* myPattern{nullptr};
    ButtonWidget*   mySubDirsButton{nullptr};
    ButtonWidget*   myRandomRomButton{nullptr};
    LabelWidget*    myRomCount{nullptr};
    ButtonWidget*   myHelpButton{nullptr};

    // Current-path display and its reload button
    NavigationWidget* myNavigationBar{nullptr};
    ButtonWidget*     myReloadButton{nullptr};

    // The ROM/directory listing
    LauncherFileListWidget* myList{nullptr};

    // Bottom button row (optional, see myShowButtons)
    ButtonWidget*   myStartButton{nullptr};
    ButtonWidget*   myGoUpButton{nullptr};
    ButtonWidget*   myOptionsButton{nullptr};
    ButtonWidget*   myQuitButton{nullptr};

    // ROM info column (optional, see myShowRomInfo): the image/snapshot
    // and the text info beneath it
    RomImageWidget* myRomImageWidget{nullptr};
    RomInfoWidget*  myRomInfoWidget{nullptr};

    // ROM info column width as a fraction of the launcher content width.
    // Keeps the ROM info area scaling proportionally as the window resizes,
    // and is adjusted by dragging the divider.
    float myRomInfoFraction{0.F};

    // The minimum content size, recomputed at the end of each layout() from the
    // laid-out widgets and used to clamp the next one (the widgets carry no
    // meaningful geometry until layout() runs, so it cannot be read up front)
    Common::Size myMinSize;

    // Draggable divider between the list and the ROM info column
    Widget* myDivider{nullptr};

    // Cache of ROM path -> MD5, to avoid recomputing on every selection
    // change (see selectedRomMD5())
    std::unordered_map<string, string, BSPF::StringHash, std::equal_to<>> myMD5List;

    // Index into the focus list of the widget to focus when the dialog
    // (re)opens (the ROM listing, by default)
    int mySelectedItem{0};

    // Whether the ROM info column is currently shown (toggleable at
    // runtime, see setRomInfoEnabled())
    bool myShowRomInfo{false};
    // Whether the bottom button row is currently shown (toggleable at
    // runtime, see setButtonsEnabled())
    bool myShowButtons{false};
    // Set by a joystick button already handled in handleJoyDown()/
    // openGlobalProps()/openSettings(), so handleJoyUp() doesn't also
    // dispatch it
    bool myEventHandled{false};
    // A directory reload delayed until this tick (in ms, see tick()), so
    // rapid filter-text typing doesn't reload on every keystroke
    bool myPendingReload{false};
    uInt64 myReloadTime{0};
    // A ROM-info/image update delayed until this tick (in ms, see tick()
    // and loadRomInfo()/loadPendingRomInfo())
    bool myPendingRomInfo{false};
    uInt64 myRomInfoTime{0};

  private:
    // Following constructors and assignment operators not supported
    LauncherDialog() = delete;
    LauncherDialog(const LauncherDialog&) = delete;
    LauncherDialog(LauncherDialog&&) = delete;
    LauncherDialog& operator=(const LauncherDialog&) = delete;
    LauncherDialog& operator=(LauncherDialog&&) = delete;
};

#endif  // LAUNCHER_DIALOG_HXX
