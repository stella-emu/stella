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

#ifndef LAUNCHER_FILE_LIST_WIDGET_HXX
#define LAUNCHER_FILE_LIST_WIDGET_HXX

class FavoritesManager;
class FSNode;
class ProgressDialog;
class Settings;

#include "FileListWidget.hxx"

/**
  Specialization of the general FileListWidget which provides support for
  user defined favorites, recently played ROMs and most popular ROMs.

  @author  Thomas Jentzsch
*/

class LauncherFileListWidget : public FileListWidget
{
  public:
    LauncherFileListWidget(GuiObject* boss, const GUI::Font& font);
    ~LauncherFileListWidget() override = default;

    // Load/save/clear the favorites database, and refresh it for the ROM
    // just selected (updateFavorites(), called on activation)
    void loadFavorites();
    void saveFavorites(bool force = false);
    void clearFavorites();
    void updateFavorites();
    bool isUserFavorite(string_view path) const;
    // Adds/removes the selected entry from the user's favorites
    void toggleUserFavorite();
    // Removes the selected entry from the recent/popular list, whichever it's in
    void removeFavorite();
    void removeAllUserFavorites();
    void removeAllPopular();
    void removeAllRecent();

    // Whether we're currently showing one of the virtual folders below,
    // rather than a real directory
    bool inVirtualDir() const { return myInVirtualDir; }
    bool inUserDir() const { return myVirtualDir == user_name; }
    bool inRecentDir() const { return myVirtualDir == recent_name; }
    bool inPopularDir() const { return myVirtualDir == popular_name; }
    // Whether 'name' is one of the virtual folders' display names
    static bool isUserDir(string_view name) { return name == user_name; }
    static bool isRecentDir(string_view name) { return name == recent_name; }
    static bool isPopularDir(string_view name) { return name == popular_name; }

    // Also treats the (non-existent) virtual folders as directories
    bool isDirectory(const FSNode& node) const override;

  protected:
    // Populates a real directory as the base class does, or (if 'favorites' is
    // enabled and this is a virtual folder) the corresponding favorites list
    void getChildren(const FSNode::CancelCheck& isCancelled) override;
    // Adds the virtual-folder entries (Favorites/Popular/Recent) to the ROM
    // start directory's own listing, via addFolder()
    void extendLists(StringList& list) override;
    IconType getIconType(const FSNode& node) const override;
    const GUI::Icon* getIcon(int i) const override;
    bool fullPathToolTip() const override { return myInVirtualDir; }

  private:
    // Display names of the three virtual folders
    static constexpr string_view user_name = "Favorites";
    static constexpr string_view recent_name = "Recently Played";
    static constexpr string_view popular_name = "Most Popular";

    unique_ptr<FavoritesManager> myFavorites;
    // Whether _node is currently one of the virtual folders
    bool myInVirtualDir{false};
    // Which virtual folder, if myInVirtualDir (one of the *_name values above)
    string myVirtualDir;

  private:
    // The directory extendLists() anchors the virtual folders to (normally
    // the configured ROM start dir, adjusted if that itself is virtual/a ZIP)
    FSNode startRomNode() const;
    // Refreshes one row's icon after its favorite status changes
    void userFavor(string_view path);
    // Inserts a virtual-folder entry at 'offset' (advancing it) into the
    // file list, display list, dir list, and icon list together
    void addFolder(StringList& list, int& offset, string_view name, IconType icon);

  private:
    // Following constructors and assignment operators not supported
    LauncherFileListWidget(const LauncherFileListWidget&) = delete;
    LauncherFileListWidget(LauncherFileListWidget&&) = delete;
    LauncherFileListWidget& operator=(const LauncherFileListWidget&) = delete;
    LauncherFileListWidget& operator=(LauncherFileListWidget&&) = delete;
};

#endif  // LAUNCHER_FILE_LIST_WIDGET_HXX
