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

#include "bspf.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FileListWidget::FileListWidget(GuiObject* boss, const GUI::Font& font,
                               int x, int y, int w, int h)
  : StringListWidget(boss, font, x, y, w, h),
    _fsmode(FilesystemNode::ListMode::All),
    _selected(0)
{
  // This widget is special, in that it catches signals and redirects them
  setTarget(this);

  // By default, all filenames are valid
  _filter = [](const FilesystemNode& node) { return true; };
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FileListWidget::setDirectory(const FilesystemNode& node, string select)
{
  _node = node;

  // We always want a directory listing
  if(!_node.isDirectory() && _node.hasParent())
  {
    select = _node.getName();
    _node = _node.getParent();
  }

  // Initialize history
  FilesystemNode tmp = _node;
  while(tmp.hasParent())
  {
    string name = tmp.getName();
    if(name.back() == '/' || name.back() == '\\')
      name.pop_back();
    if(!BSPF::startsWithIgnoreCase(name, " ["))
      name = " [" + name + "]";

    _history.push(name);
    tmp = tmp.getParent();
  }
  // History is in reverse order; we need to fix that
  _history.reverse();

  // Finally, go to this location
  setLocation(_node, select);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FileListWidget::setLocation(const FilesystemNode& node, string select)
{
  _node = node;

  // Read in the data from the file system (start with an empty list)
  _fileList.clear();
  _fileList.reserve(512);
  _node.getChildren(_fileList, _fsmode, _filter);

  // Now fill the list widget with the names from the file list
  StringList l;
  for(const auto& file: _fileList)
    l.push_back(file.getName());

  setList(l);
  setSelected(select);

  ListWidget::recalc();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FileListWidget::selectDirectory()
{
  _history.push(selected().getName());
  setLocation(selected());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FileListWidget::selectParent()
{
  if(_node.hasParent())
    setLocation(_node.getParent(), !_history.empty() ? _history.pop() : EmptyString);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FileListWidget::reload()
{
  if(_node.exists())
    setLocation(_node, selected().getName());
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
      _selected = data;
      cmd = ItemChanged;
      break;

    case ListWidget::kActivatedCmd:
    case ListWidget::kDoubleClickedCmd:
      _selected = data;
      if(selected().isDirectory())
      {
        cmd = ItemChanged;
        selectDirectory();
      }
      else
        cmd = ItemActivated;
      break;

    case ListWidget::kLongButtonPressCmd:
      // do nothing, let boss handle this one
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
