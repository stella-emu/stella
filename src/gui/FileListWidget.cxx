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

#include "ScrollBarWidget.hxx"
#include "FileListWidget.hxx"

#include "Bankswitch.hxx"
#include "MD5.hxx"
#include "bspf.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FileListWidget::FileListWidget(GuiObject* boss, const GUI::Font& font,
                               int x, int y, int w, int h)
  : StringListWidget(boss, font, x, y, w, h),
    _fsmode(FilesystemNode::ListMode::All),
    _selectedPos(0)
{
  // This widget is special, in that it catches signals and redirects them
  setTarget(this);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FileListWidget::setLocation(const FilesystemNode& node, string select)
{
  _node = node;

  // Generally, we always want a directory listing
  if(!_node.isDirectory() && _node.hasParent())
  {
    select = _node.getName();
    _node = _node.getParent();
  }

  // Start with empty list
  _gameList.clear();

  // Read in the data from the file system
  FSList content;
  content.reserve(512);
  _node.getChildren(content, _fsmode);

  // Add '[..]' to indicate previous folder
  if(_node.hasParent())
    _gameList.appendGame(" [..]", _node.getParent().getPath(), "", true);

  // Now add the directory entries
  for(const auto& file: content)
  {
    string name = file.getName();
    bool isDir = file.isDirectory();
    if(isDir)
      name = " [" + name + "]";
    else if(!BSPF::endsWithIgnoreCase(name, _extension))
      continue;

    _gameList.appendGame(name, file.getPath(), "", isDir);
  }
  _gameList.sortByName();

  // Now fill the list widget with the contents of the GameList
  StringList l;
  for(uInt32 i = 0; i < _gameList.size(); ++i)
    l.push_back(_gameList.name(i));

  setList(l);
  setSelected(select);

  ListWidget::recalc();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FileListWidget::selectParent()
{
  if(_node.hasParent())
  {
    const string& curr = " [" + _node.getName() + "]";
    setLocation(_node.getParent(), curr);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FileListWidget::reload()
{
  if(_node.exists())
    setLocation(_node, _gameList.name(_selectedPos));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const string& FileListWidget::selectedMD5()
{
  if(_selected.isDirectory() || !Bankswitch::isValidRomName(_selected))
    return EmptyString;

  // Make sure we have a valid md5 for this ROM
  if(_gameList.md5(_selectedPos) == "")
    _gameList.setMd5(_selectedPos, MD5::hash(_selected));

  return _gameList.md5(_selectedPos);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FileListWidget::handleCommand(CommandSender* sender, int cmd, int data, int id)
{
  switch (cmd)
  {
    case ListWidget::kPrevDirCmd:
      selectParent();
      break;

    case ListWidget::kSelectionChangedCmd:
      cmd = ItemChanged;
      _selected = FilesystemNode(_gameList.path(data));
      _selectedPos = data;
      break;

    case ListWidget::kActivatedCmd:
    case ListWidget::kDoubleClickedCmd:
      if(_gameList.isDir(data))
      {
        cmd = ItemChanged;
        if(_gameList.name(data) == " [..]")
          selectParent();
        else
          setLocation(FilesystemNode(_gameList.path(data)));
      }
      else
        cmd = ItemActivated;
      break;

    default:
      // If we don't know about the command, send it to the parent and exit
      StringListWidget::handleCommand(sender, cmd, data, id);
      return;
  }

  // Send command to boss, then revert to target 'this'
  setTarget(_boss);
  sendCommand(cmd, data, id);
  setTarget(this);
}
