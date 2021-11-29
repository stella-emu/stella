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

#include "Bankswitch.hxx"
#include "FavoritesManager.hxx"
#include "OSystem.hxx"
#include "ProgressDialog.hxx"
#include "Settings.hxx"

#include "LauncherFileListWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
LauncherFileListWidget::LauncherFileListWidget(GuiObject* boss, const GUI::Font& font,
  int x, int y, int w, int h)
  : FileListWidget(boss, font, x, y, w, h)
{
  // This widget is special, in that it catches signals and redirects them
  setTarget(this);
  myFavorites = make_unique<FavoritesManager>(instance().settings());
  myRomDir = instance().settings().getString("romdir");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool LauncherFileListWidget::isDirectory(const FilesystemNode& node) const
{
  bool isDir = node.isDirectory();

  // Check for virtual directories
  if(!isDir && !node.exists())
    return node.getName() == user_name
      || node.getName() == recent_name
      || node.getName() == popular_name;

  return isDir;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherFileListWidget::getChildren(const FilesystemNode::CancelCheck& isCancelled)
{
  if(_node.exists() || !_node.hasParent())
  {
    myInVirtualDir = false;
    myVirtualDir = EmptyString;
    FileListWidget::getChildren(isCancelled);
  }
  else
  {
    myInVirtualDir = true;
    myVirtualDir = _node.getName();

    FilesystemNode parent(_node.getParent());
    parent.setName("..");
    _fileList.emplace_back(parent);

    if(myVirtualDir == user_name)
    {
      for(auto& item : myFavorites->userList())
      {
        FilesystemNode node(item);
        if(_filter(node))
          _fileList.emplace_back(node);
      }
    }
    else if(myVirtualDir == popular_name)
    {
      for(auto& item : myFavorites->popularList())
      {
        FilesystemNode node(item.first);
        if(_filter(node))
          _fileList.emplace_back(node);
      }
    }
    else if(myVirtualDir == recent_name)
    {
      for(auto& item : myFavorites->recentList())
      {
        FilesystemNode node(item);
        if(_filter(node))
          _fileList.emplace_back(node);
      }
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherFileListWidget::addFolder(StringList& list, int& offset, const string& name, IconType icon)
{
  _fileList.insert(_fileList.begin() + offset,
    FilesystemNode(_node.getPath() + name));
  list.insert(list.begin() + offset, name);
  _dirList.insert(_dirList.begin() + offset, "");
  _iconTypeList.insert((_iconTypeList.begin() + offset), icon);

  ++offset;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherFileListWidget::extendLists(StringList& list)
{
  // Only show virtual dirs in "romdir". Except if
  //  "romdir" is virtual or "romdir" is a ZIP
  //  Then show virtual dirs in parent dir of "romdir".
  if(myRomDir == instance().settings().getString("romdir")
      && (myInVirtualDir || BSPF::endsWithIgnoreCase(_node.getPath(), ".zip")))
    myRomDir = _node.getParent().getPath();

  if(_node.getPath() == myRomDir)
  {
    // Add virtual directories behind ".."
    int offset = _fileList.begin()->getName() == ".." ? 1 : 0;

    if(myFavorites->userList().size())
      addFolder(list, offset, user_name, IconType::favdir);
    if(myFavorites->popularList().size())
      addFolder(list, offset, popular_name, IconType::popdir);
    if(myFavorites->recentList().size())
      addFolder(list, offset, recent_name, IconType::recentdir);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherFileListWidget::loadFavorites()
{
  myFavorites->load();

  for(const auto& path : myFavorites->userList())
    userFavor(path);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherFileListWidget::saveFavorites()
{
  myFavorites->save();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherFileListWidget::updateFavorites()
{
  myFavorites->update(selected().getPath());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool LauncherFileListWidget::isUserFavorite(const string& path) const
{
  return myFavorites->existsUser(path);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherFileListWidget::toggleUserFavorite()
{
  if(!selected().isDirectory() && Bankswitch::isValidRomName(selected()))
  {
    bool isUserFavorite = myFavorites->toggleUser(selected().getPath());

    userFavor(selected().getPath(), isUserFavorite);
    // Redraw file list
    setDirty();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherFileListWidget::removeFavorite()
{
  if(inRecentDir())
    myFavorites->removeRecent(selected().getPath());
  else if(inPopularDir())
    myFavorites->removePopular(selected().getPath());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LauncherFileListWidget::userFavor(const string& path, bool isUserFavorite)
{
  size_t pos = 0;

  for(const auto& file : _fileList)
  {
    if(file.getPath() == path)
      break;
    pos++;
  }
  if(pos < _iconTypeList.size())
    _iconTypeList[pos] = isUserFavorite ? IconType::favorite : IconType::rom;
}

FileListWidget::IconType LauncherFileListWidget::romIconType(const FilesystemNode& file) const
{
  if(file.isFile() && Bankswitch::isValidRomName(file.getName()))
    return isUserFavorite(file.getPath()) ? IconType::favorite : IconType::rom;
  else
    return IconType::unknown;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const FileListWidget::Icon* LauncherFileListWidget::getIcon(int i) const
{
  static const Icon favorite_small = {
    //0b0000001'11000000,
    //0b0000011'01100000,
    //0b0000010'00100000,
    //0b0000110'00110000,
    //0b0000100'00010000,
    //0b1111100'00011111,
    //0b1000000'00000001,
    //0b1100000'00000011,
    //0b0110000'00000110,
    //0b0011000'00001100,
    //0b0010000'00000100,
    //0b0110000'10000110,
    //0b0100011'01100010,
    //0b0101110'00111010,
    //0b0111000'00001110,

    0b0000001'10000000,
    0b0000011'11000000,
    0b0000010'01000000,
    0b0000110'01100000,
    0b0111100'00111100,
    0b1100000'00000110,
    0b0110000'00001100,
    0b0011000'00011000,
    0b0001100'00110000,
    0b0011000'00011000,
    0b0010001'10001000,
    0b0110111'11101100,
    0b0111100'00111100,
    0b0011000'00011000,
  };
  static const Icon favdir_small = {
    //0b11111000'0000000,
    //0b11111100'0000000,
    //0b11111111'1111111,
    //0b10000000'0000001,
    //0b10000001'0000001,
    //0b10000011'1000001,
    //0b10001111'1110001,
    //0b10000111'1100001,
    //0b10000011'1000001,
    //0b10000111'1100001,
    //0b10001100'0110001,
    //0b10000000'0000001,
    //0b11111111'1111111

    0b11111000'0000000,
    0b11111100'0000000,
    0b11111101'0111111,
    0b10000011'1000001,
    0b10000011'1000001,
    0b10000111'1100001,
    0b10111111'1111101,
    0b10011111'1111001,
    0b10001111'1110001,
    0b10000111'1100001,
    0b10001111'1110001,
    0b10011100'0111001,
    0b11011000'0011011
  };
  static const Icon recent_small = {
    0b11111000'0000000,
    0b11111100'0000000,
    0b11111111'1111111,
    0b10000011'1000001,
    0b10001110'1110001,
    0b10001110'1110001,
    0b10011110'1111001,
    0b10011110'0111001,
    0b10011111'0011001,
    0b10001111'1110001,
    0b10001111'1110001,
    0b10000011'1000001,
    0b11111111'1111111
  };
  static const Icon popular_small = {
    0b11111000'0000000,
    0b11111100'0000000,
    0b11111111'1111111,
    0b10000000'0000001,
    0b10001100'0110001,
    0b10011110'1111001,
    0b10011111'1111001,
    0b10011111'1111001,
    0b10001111'1110001,
    0b10000111'1100001,
    0b10000011'1000001,
    0b10000001'0000001,
    0b11111111'1111111
  };

  static const Icon favorite_large = {
    //0b0000000'0001000'0000000,
    //0b0000000'0011100'0000000,
    //0b0000000'0011100'0000000,
    //0b0000000'0110110'0000000,
    //0b0000000'0110110'0000000,
    //0b0000000'0110110'0000000,
    //0b0000000'1100011'0000000,
    //0b0111111'1000001'1111110,
    //0b1111111'0000000'1111111,
    //0b0110000'0000000'0000110,
    //0b0011000'0000000'0001100,
    //0b0001100'0000000'0011000,
    //0b0000110'0000000'0110000,
    //0b0000011'0000000'1100000,
    //0b0000111'0000000'1110000,
    //0b0000110'0001000'0110000,
    //0b0001100'0011100'0011000,
    //0b0001100'1110111'0011000,
    //0b0011001'1000001'1001100,
    //0b0011111'0000000'1111100,
    //0b0001100'0000000'0011000

    0b0000000'0001000'0000000,
    0b0000000'0011100'0000000,
    0b0000000'0011100'0000000,
    0b0000000'0111110'0000000,
    0b0000000'0111110'0000000,
    0b0000000'0110110'0000000,
    0b0000000'1100011'0000000,
    0b0111111'1100011'1111110,
    0b1111111'1000001'1111111,
    0b0111000'0000000'0001110,
    0b0011100'0000000'0011100,
    0b0001110'0000000'0111000,
    0b0000111'0000000'1110000,
    0b0000011'1000001'1100000,
    0b0000111'0000000'1110000,
    0b0000111'0011100'1110000,
    0b0001110'0111110'0111000,
    0b0001100'1110111'0011000,
    0b0011111'1000001'1111100,
    0b0011111'0000000'1111100,
    0b0001100'0000000'0011000

  };
  static const Icon favdir_large = {
    0b111111'10000000'0000000,
    0b111111'11000000'0000000,
    0b111111'11100000'0000000,
    0b111111'11111111'1111111,
    0b111111'11111111'1111111,
    0b110000'00001000'0000011,
    0b110000'00011100'0000011,
    0b110000'00011100'0000011,
    0b110000'00111110'0000011,
    0b110001'11111111'1100011,
    0b110011'11111111'1110011,
    0b110001'11111111'1100011,
    0b110000'11111111'1000011,
    0b110000'01111111'0000011,
    0b110000'11111111'1000011,
    0b110001'11110111'1100011,
    0b110001'11100011'1100011,
    0b110000'11000001'1000011,
    0b110000'00000000'0000011,
    0b111111'11111111'1111111,
    0b111111'11111111'1111111
  };
  static const Icon recent_large = {
    0b111111'10000000'0000000,
    0b111111'11000000'0000000,
    0b111111'11100000'0000000,
    0b111111'11111111'1111111,
    0b111111'11111111'1111111,
    0b110000'00000000'0000011,
    0b110000'00111110'0000011,
    0b110000'11110111'1000011,
    0b110001'11110111'1100011,
    0b110001'11110111'1100011,
    0b110011'11110111'1110011,
    0b110011'11110111'1110011,
    0b110011'11110011'1110011,
    0b110011'11111001'1110011,
    0b110001'11111100'1100011,
    0b110001'11111111'1100011,
    0b110000'11111111'1000011,
    0b110000'00111110'0000011,
    0b110000'00000000'0000011,
    0b111111'11111111'1111111,
    0b111111'11111111'1111111
  };
  static const Icon popular_large = {
    0b111111'10000000'0000000,
    0b111111'11000000'0000000,
    0b111111'11100000'0000000,
    0b111111'11111111'1111111,
    0b111111'11111111'1111111,
    0b110000'00000000'0000011,
    0b110000'00000000'0000011,
    0b110000'11100011'1000011,
    0b110001'11110111'1100011,
    0b110011'11111111'1110011,
    0b110011'11111111'1110011,
    0b110011'11111111'1110011,
    0b110001'11111111'1100011,
    0b110000'11111111'1000011,
    0b110000'01111111'0000011,
    0b110000'00111110'0000011,
    0b110000'00011100'0000011,
    0b110000'00001000'0000011,
    0b110000'00000000'0000011,
    0b111111'11111111'1111111,
    0b111111'11111111'1111111
  };
  static const Icon* small_icons[int(IconType::numLauncherTypes)] = {
    &favorite_small, &favdir_small, &recent_small, &popular_small

  };
  static const Icon* large_icons[int(IconType::numLauncherTypes)] = {
    &favorite_large, &favdir_large, &recent_large, &popular_large
  };

  if(int(_iconTypeList[i]) < int(IconType::numTypes))
    return FileListWidget::getIcon(i);

  const bool smallIcon = iconWidth() < 24;
  const int iconType = int(_iconTypeList[i]) - int(IconType::numTypes);

  assert(iconType < int(IconType::numLauncherTypes));

  return smallIcon ? small_icons[iconType] : large_icons[iconType];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const string LauncherFileListWidget::user_name = "Favorites";
const string LauncherFileListWidget::recent_name = "Recently Played";
const string LauncherFileListWidget::popular_name = "Most Popular";
