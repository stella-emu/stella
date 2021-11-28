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
// Copyright (c) 1995-2021 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#ifndef LAUNCHER_FILE_LIST_WIDGET_HXX
#define LAUNCHER_FILE_LIST_WIDGET_HXX

class FavoritesManager;
class FilesystemNode;
class ProgressDialog;
class Settings;

#include "FileListWidget.hxx"

/**
  Specialization of the general FileListWidget which procides support for
  user defined favorites, recently played ROMs and most popular ROMs.

  @author  Thomas Jentzsch
*/

class LauncherFileListWidget : public FileListWidget
{
  public:
    LauncherFileListWidget(GuiObject* boss, const GUI::Font& font,
      int x, int y, int w, int h);
    ~LauncherFileListWidget() override = default;

    void loadFavorites();
    void saveFavorites();
    void updateFavorites();
    bool isUserFavorite(const string& path) const;
    void toggleUserFavorite();

    bool isDirectory(const FilesystemNode& node) const override;
    bool inVirtualDir() const { return myInVirtualDir; }

  private:
    static const string user_name;
    static const string recent_name;
    static const string popular_name;

    unique_ptr<FavoritesManager> myFavorites;
    bool myInVirtualDir{false};
    string myRomDir;

  private:
    void getChildren(const FilesystemNode::CancelCheck& isCancelled) override;
    void userFavor(const string& path, bool enable = true);
    void addFolder(StringList& list, int& offset, const string& name, IconType icon);
    void extendLists(StringList& list) override;
    IconType romIconType(const FilesystemNode& file) const override;
    const Icon* getIcon(int i) const override;
    bool fullPathToolTip() const override { return myInVirtualDir; }
};

#endif