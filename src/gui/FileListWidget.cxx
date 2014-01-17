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
// Copyright (c) 1995-2014 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

#include "ScrollBarWidget.hxx"
#include "FileListWidget.hxx"

#include "bspf.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FileListWidget::FileListWidget(GuiObject* boss, const GUI::Font& font,
                               int x, int y, int w, int h)
  : StringListWidget(boss, font, x, y, w, h),
    _fsmode(FilesystemNode::kListAll),
    _extension("")
{
  // This widget is special, in that it catches signals and redirects them
  setTarget(this);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FileListWidget::~FileListWidget()
{
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
  for(unsigned int idx = 0; idx < content.size(); idx++)
  {
    string name = content[idx].getName();
    bool isDir = content[idx].isDirectory();
    if(isDir)
      name = " [" + name + "]";
    else if(!BSPF_endsWithIgnoreCase(name, _extension))
      continue;

    _gameList.appendGame(name, content[idx].getPath(), "", isDir);
  }
  _gameList.sortByName();

  // Now fill the list widget with the contents of the GameList
  StringList l;
  for (int i = 0; i < (int) _gameList.size(); ++i)
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
