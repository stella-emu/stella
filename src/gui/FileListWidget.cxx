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

#include "bspf.hxx"
#include "ScrollBarWidget.hxx"
#include "TimerManager.hxx"
#include "ProgressDialog.hxx"
#include "FBSurface.hxx"
#include "Bankswitch.hxx"

#include "FileListWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FileListWidget::FileListWidget(GuiObject* boss, const GUI::Font& font,
                               int x, int y, int w, int h)
  : StringListWidget(boss, font, x, y, w, h),
    _filter{[](const FSNode&) { return true; }}
{
  // This widget is special, in that it catches signals and redirects them
  setTarget(this);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FileListWidget::setInitialDirectory(const FSNode& node, string_view select)
{
  _node = node;

  // We always want a directory listing, keeping going up a node until
  // a valid one is found
  while(!_node.isDirectory() && _node.hasParent())
    _node = _node.getParent();

  // Initialize history
  _history.clear();
  FSNode tmp = _node;
  while(tmp.hasParent())
  {
    _history.emplace_back(tmp);
    tmp = tmp.getParent();
  }
  // Ensure at least one entry; without this, _node with no parent leaves
  // _history empty, causing underflow in _currentHistoryIdx and _historyHome.
  if(_history.empty())
    _history.emplace_back(_node);

  // History is in reverse order; we need to fix that
  std::ranges::reverse(_history);
  _currentHistoryIdx = _history.size() - 1;
  _historyHome = _currentHistoryIdx;

  // Finally, go to this location
  setLocation(_node, select);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FileListWidget::setLocation(const FSNode& node, string_view select)
{
  progress().resetProgress();
  progress().open();
  const FSNode::CancelCheck isCancelled = [this]() {
    return myProgressDialog->isCancelled();
  };

  _node = node;

  // Read in the data from the file system (start with an empty list)
  _fileList.clear();
  getChildren(isCancelled);

  // Now fill the list widget with the names from the file list,
  // even if cancelled
  StringList fileNames;
  fileNames.reserve(_fileList.size());
  const size_t orgLen = _node.getShortPath().length();

  _dirList.clear();
  _dirList.reserve(_fileList.size());
  _iconTypeList.clear();
  _iconTypeList.reserve(_fileList.size());

  for(const auto& file: _fileList)
  {
    auto path = file.getShortPath();
    const string& name = file.getName();

    // display only relative path in tooltip
    if(path.length() >= orgLen && !fullPathToolTip())
      _dirList.push_back(path.substr(orgLen));
    else
      _dirList.push_back(std::move(path));

    if(file.isDirectory() && !file.hasExtension(".zip"))
    {
      fileNames.push_back(name);
      if(name == "..")
        _iconTypeList.push_back(IconType::updir);
      else
        _iconTypeList.push_back(getIconType(file));
    }
    else
    {
      fileNames.push_back(_showFileExtensions ? name : file.getBaseName());
      _iconTypeList.push_back(getIconType(file));
    }
  }
  extendLists(fileNames);

  setList(fileNames);

  // An explicit select overrides stored history
  const string& nodePath = _node.getPath();
  if(!select.empty())
    _selectionHistory[nodePath] = select;

  // Go to previously selected item, if it exists
  if(const auto it = _selectionHistory.find(nodePath);
                                       it != _selectionHistory.end())
    setSelected(it->second);
  else
    setSelected(0);

  ListWidget::recalc();

  progress().close();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FileListWidget::isDirectory(const FSNode& node) const
{
  return node.isDirectory();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FileListWidget::getChildren(const FSNode::CancelCheck& isCancelled)
{
  if(_includeSubDirs)
  {
    // Actually this could become HUGE
    _fileList.reserve(1000);
    _node.getAllChildren(_fileList, _fsmode, _filter, true, isCancelled);
  }
  else
  {
    _fileList.reserve(200);
    _node.getChildren(_fileList, _fsmode, _filter, false, true, isCancelled);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FileListWidget::IconType FileListWidget::getIconType(const FSNode& node) const
{
  if(node.isDirectory())
    return node.hasExtension(".zip")
      ? IconType::zip : IconType::directory;
  else
    if(node.isFile() && Bankswitch::isValidRomName(node))
    {
      return node.hasExtension(".mp3") || node.hasExtension(".wav")
        ? IconType::cassette : IconType::rom;
    }
    else
      return IconType::unknown;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FileListWidget::selectDirectory()
{
  addHistory(selected());
  setLocation(selected());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FileListWidget::selectDirectory(const FSNode& node)
{
  if(node.getPath() != _node.getPath())
    addHistory(node);
  setLocation(node);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FileListWidget::selectParent()
{
  if(_node.hasParent() && _fsmode != FSNode::ListMode::FilesOnly)
  {
    const auto parent = _node.getParent();

    // When going up, land on the child we came from.
    // getName() can carry a trailing separator when the node was constructed
    // via getParent() (stemPathComponent keeps it), but list entries don't,
    // so strip it.  Then apply the same display-name logic setLocation() uses
    // so the stored name matches what ends up in _list.
    string_view childName = _node.getName();
    if(!childName.empty() && childName.back() == FSNode::PATH_SEPARATOR)
      childName.remove_suffix(1);
    // Real directories always use getName(); ZIPs and files respect _showFileExtensions.
    const bool isRealDir = _node.isDirectory() &&
                           !BSPF::endsWithIgnoreCase(childName, ".zip");
    if(!isRealDir && !_showFileExtensions)
    {
      const size_t dot = childName.find_last_of('.');
      if(dot != string_view::npos)
        childName = childName.substr(0, dot);
    }
    _selectionHistory[parent.getPath()] = childName;
    addHistory(parent);
    setLocation(parent);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FileListWidget::selectHomeDir()
{
  _currentHistoryIdx = _historyHome;
  setLocation(_history[_currentHistoryIdx]);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FileListWidget::selectPrevHistory()
{
  if(_currentHistoryIdx != _historyHome)
    setLocation(_history[--_currentHistoryIdx]);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FileListWidget::selectNextHistory()
{
  if(_currentHistoryIdx + 1 < _history.size())
    setLocation(_history[++_currentHistoryIdx]);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FileListWidget::hasPrevHistory() const
{
  return _currentHistoryIdx != _historyHome;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FileListWidget::hasNextHistory() const
{
  return _currentHistoryIdx + 1 < _history.size();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FileListWidget::addHistory(const FSNode& node)
{
  if(!_history.empty())
    _history.resize(_currentHistoryIdx + 1);

  _history.push_back(node);
  _currentHistoryIdx = _history.size() - 1;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FileListWidget::reload()
{
  if(isDirectory(_node))
    setLocation(_node, _showFileExtensions
      ? selected().getName()
      : selected().getBaseName());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const FSNode& FileListWidget::selected()
{
  if(!_fileList.empty())
  {
    _selected = std::min(_selected, static_cast<uInt32>(_fileList.size() - 1));
    return _fileList[_selected];
  }
  else
  {
    // This should never happen, but we'll error-check out-of-bounds
    // array access anyway
    return defaultNode();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ProgressDialog& FileListWidget::progress()
{
  if(myProgressDialog == nullptr)
    myProgressDialog = std::make_unique<ProgressDialog>(this, _font, "");

  return *myProgressDialog;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FileListWidget::incProgress()
{
  progress().incProgress();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FileListWidget::handleKeyDown(StellaKey key, StellaMod mod)
{
  // Grab the key before passing it to the actual dialog and check for
  // file list navigation keys
  bool handled = false;

  if(StellaModTest::isAlt(mod))
  {
    handled = true;
    switch(key)
    {
      case StellaKey::HOME:
        sendCommand(kHomeDirCmd, 0, 0);
        break;

      case StellaKey::LEFT:
        sendCommand(kPrevDirCmd, 0, 0);
        break;

      case StellaKey::RIGHT:
        sendCommand(kNextDirCmd, 0, 0);
        break;

      case StellaKey::UP:
        sendCommand(kParentDirCmd, 0, 0);
        break;

      case StellaKey::DOWN:
        sendCommand(kActivatedCmd, _selected, 0);
        break;

      default:
        handled = false;
        break;
    }
  }
  // Handle shift input for quick directory selection
  _lastKey = key;
  _lastMod = mod;
  if(_quickSelectTime < TimerManager::getTicks() / 1000)
    _firstMod = mod;
  else if(key == StellaKey::SPACE) // allow searching ROMs with a space without selecting/starting
    handled = true;

  return handled;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FileListWidget::handleText(char text)
{
  // Quick selection mode: Go to first list item starting with this key
  // (or a substring accumulated from the last couple key presses).
  // Only works in a useful fashion if the list entries are sorted.
  const uInt64 time = TimerManager::getTicks() / 1000;
  const bool firstShift = StellaModTest::isShift(_firstMod);

  if(StellaModTest::isShift(_lastMod))
  {
    const string_view key = StellaKeyName::forKey(_lastKey);
    text = !key.empty() ? key.front() : '\0';
  }

  if(_quickSelectTime < time)
    _quickSelectStr = text;
  else
    _quickSelectStr += text;
  _quickSelectTime = time + S_QUICK_SELECT_DELAY;

  int selectedItem = 0;
  for(; std::cmp_less(selectedItem, _list.size()); ++selectedItem)
  {
    const auto icon = _iconTypeList[selectedItem];
    // Select directories when the first character is uppercase
    const bool isDir = icon == IconType::directory || icon == IconType::userdir
                    || icon == IconType::recentdir || icon == IconType::popdir;
    if(BSPF::startsWithIgnoreCase(_list[selectedItem], _quickSelectStr) &&
          firstShift == isDir)
      break;
  }

  if(selectedItem > 0)
    setSelected(selectedItem);

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FileListWidget::handleCommand(CommandSender* sender, int cmd, int data, int id)
{
  switch(cmd)
  {
    case FileListWidget::kHomeDirCmd:
      // Do not let the boss know
      selectHomeDir();
      return;

    case FileListWidget::kPrevDirCmd:
      // Do not let the boss know
      selectPrevHistory();
      return;

    case FileListWidget::kNextDirCmd:
      // Do not let the boss know
      selectNextHistory();
      return;

    case ListWidget::kParentDirCmd:
      selectParent();
      // Do not let the boss know
      return;

    case ListWidget::kSelectionChangedCmd:
      _selected = data;
      if(std::cmp_less(data, _list.size()))
        _selectionHistory[_node.getPath()] = _list[data];
      cmd = ItemChanged;
      break;

    case ListWidget::kActivatedCmd:
      [[fallthrough]];
    case ListWidget::kDoubleClickedCmd:
      _selected = data;
      if(std::cmp_less(data, _list.size()))
        _selectionHistory[_node.getPath()] = _list[data];
      if(isDirectory(selected())/* || !selected().exists()*/)
      {
        if(selected().getName() == "..")
          selectParent();
        else
        {
          cmd = ItemChanged;
          selectDirectory();
        }
      }
      else
      {
        cmd = ItemActivated;
      }
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

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int FileListWidget::drawIcon(int i, int x, int y, ColorId color)
{
  const bool smallIcon = iconWidth() < 24;
  const Icon* icon = getIcon(i);
  const int iconGap = smallIcon ? 2 : 3;
  FBSurface& s = _boss->dialog().surface();

  s.drawBitmap(icon->data(), x + 2 + iconGap,
      y + (_lineHeight - static_cast<int>(icon->size())) / 2 - 1,
      color, iconWidth() - iconGap * 2, static_cast<int>(icon->size()));

  return iconWidth();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const FileListWidget::Icon* FileListWidget::getIcon(int i) const
{
  static const Icon unknown_small = {
    0b00111111'11000000,
    0b00100000'01100000,
    0b00100000'01110000,
    0b00100000'01111000,
    0b00100000'00001000,
    0b00100000'00001000,
    0b00100000'00001000,
    0b00100000'00001000,
    0b00100000'00001000,
    0b00100000'00001000,
    0b00100000'00001000,
    0b00100000'00001000,
    0b00100000'00001000,
    0b00111111'11111000
  };
  static const Icon rom_small = {
    0b00000000000000000,
    0b00001111'11100000,
    0b00001010'10100000,
    0b00001010'10100000,
    0b00001010'10100000,
    0b00001010'10100000,
    0b00001010'10100000,
    0b00011010'10110000,
    0b00110010'10011000,
    0b00100110'11001000,
    0b11101110'11101110,
    0b10001010'10100010,
    0b10011010'10110010,
    0b11110011'10011110
  };
  static const Icon directory_small = {
    0b00000000000000000,
    0b11111000'00000000,
    0b11111100'00000000,
    0b11111111'11111110,
    0b10000000'00000010,
    0b10000000'00000010,
    0b10000000'00000010,
    0b10000000'00000010,
    0b10000000'00000010,
    0b10000000'00000010,
    0b10000000'00000010,
    0b10000000'00000010,
    0b10000000'00000010,
    0b11111111'11111110
  };
  static const Icon zip_small = {
    0b00000000000000000,
    0b11111000'00000000,
    0b11111100'00000000,
    0b11111111'11111110,
    0b10000000'00000010,
    0b10001111'11100010,
    0b10000000'11100010,
    0b10000001'11000010,
    0b10000011'10000010,
    0b10000111'00000010,
    0b10001110'00000010,
    0b10001111'11100010,
    0b10000000'00000010,
    0b11111111'11111110
  };
  static const Icon cassette_small = {
    0b00000000000000000,
    0b00000000000000000,
//    0b00000000000000000,
    0b11111111'11111110,
    0b11000000'00000110,
    0b11000000'00000110,
    0b11111111'11111110,
    0b11001100'01100110,
    0b11010100'01010110,
    0b11001111'11100110,
    0b11111111'11111110,
    0b11110000'00011110,
    0b11101111'11101110,
    0b11011111'11110110,
    0b00000000000000000,
  };

  static const Icon up_small = {
    0b00000000000000000,
    0b11111000'00000000,
    0b11111100'00000000,
    0b11111111'11111110,
    0b10000001'00000010,
    0b10000011'10000010,
    0b10000111'11000010,
    0b10001111'11100010,
    0b10011111'11110010,
    0b10000011'10000010,
    0b10000011'10000010,
    0b10000011'10000010,
    0b10000011'10000010,
    0b11111111'11111110
  };

  static const Icon unknown_large = {
    0b00000000000'00000000000,
    0b00111111111'11110000000,
    0b00111111111'11111000000,
    0b00110000000'00011100000,
    0b00110000000'00001110000,
    0b00110000000'00000111000,
    0b00110000000'00000011000,
    0b00110000000'00000011000,
    0b00110000000'00000011000,
    0b00110000000'00000011000,
    0b00110000000'00000011000,
    0b00110000000'00000011000,
    0b00110000000'00000011000,
    0b00110000000'00000011000,
    0b00110000000'00000011000,
    0b00110000000'00000011000,
    0b00110000000'00000011000,
    0b00110000000'00000011000,
    0b00110000000'00000011000,
    0b00110000000'00000011000,
    0b00111111111'11111111000,
    0b00111111111'11111111000
  };
  static const Icon rom_large = {
    0b00000000000'00000000000,
    0b00000011111'11110000000,
    0b00000011111'11110000000,
    0b00000011010'10110000000,
    0b00000011010'10110000000,
    0b00000011010'10110000000,
    0b00000011010'10110000000,
    0b00000011010'10110000000,
    0b00000011010'10110000000,
    0b00000111010'10111000000,
    0b00000110010'10011000000,
    0b00000110010'10011000000,
    0b00001110010'10011100000,
    0b00001100010'10001100000,
    0b00011100110'11001110000,
    0b00011000110'11000110000,
    0b11111001110'11100111110,
    0b111100111101'1110011110,
    0b110000111101'1110000110,
    0b110001111101'1111000110,
    0b111111101111'1011111110,
    0b111111001111'1001111110
  };
  static const Icon directory_large = {
    0b00000000000'00000000000,
    0b11111110000'00000000000,
    0b11111111000'00000000000,
    0b11111111100'00000000000,
    0b11111111111'11111111110,
    0b11111111111'11111111110,
    0b11000000000'00000000110,
    0b11000000000'00000000110,
    0b11000000000'00000000110,
    0b11000000000'00000000110,
    0b11000000000'00000000110,
    0b11000000000'00000000110,
    0b11000000000'00000000110,
    0b11000000000'00000000110,
    0b11000000000'00000000110,
    0b11000000000'00000000110,
    0b11000000000'00000000110,
    0b11000000000'00000000110,
    0b11000000000'00000000110,
    0b11000000000'00000000110,
    0b11111111111'11111111110,
    0b11111111111'11111111110
  };
  static const Icon zip_large = {
    0b00000000000'00000000000,
    0b11111110000'00000000000,
    0b11111111000'00000000000,
    0b11111111100'00000000000,
    0b11111111111'11111111110,
    0b11111111111'11111111110,
    0b11000000000'00000000110,
    0b11000111111'11111000110,
    0b11000111111'11111000110,
    0b11000111111'11111000110,
    0b11000000000'11110000110,
    0b11000000001'11100000110,
    0b11000000011'11000000110,
    0b11000000111'10000000110,
    0b11000001111'00000000110,
    0b11000011110'00000000110,
    0b11000111111'11111000110,
    0b11000111111'11111000110,
    0b11000111111'11111000110,
    0b11000000000'00000000110,
    0b11111111111'11111111110,
    0b11111111111'11111111110
  };
  static const Icon cassette_large = {
    0b00000000000'00000000000,
    0b00000000000'00000000000,
    0b00000000000'00000000000,
    0b11111111111'11111111110,
    0b11111111111'11111111110,
    0b11100000000'00000001110,
    0b11100000000'00000001110,
    0b11111111111'11111111110,
    0b11111111111'11111111110,
    0b11110001111'11100011110,
    0b11100011000'00110001110,
    0b11100111000'00111001110,
    0b11100011000'00110001110,
    0b11110001111'11100011110,
    0b11111111111'11111111110,
    0b11111111111'11111111110,
    0b11111111111'11111111110,
    0b11111000000'00000111110,
    0b11110111111'11111011110,
    0b11101111111'11111101110,
    0b00000000000'00000000000,
    0b00000000000'00000000000,
  };
  static const Icon up_large = {
    0b00000000000'00000000000,
    0b11111110000'00000000000,
    0b11111111000'00000000000,
    0b11111111100'00000000000,
    0b11111111111'11111111110,
    0b11111111111'11111111110,
    0b11000000000'00000000110,
    0b11000000001'10000000110,
    0b11000000011'10000000110,
    0b11000000111'11000000110,
    0b11000001111'11100000110,
    0b11000011111'11110000110,
    0b11000111111'11111000110,
    0b11001111111'11111100110,
    0b11000000011'10000000110,
    0b11000000011'10000000110,
    0b11000000011'10000000110,
    0b11000000011'10000000110,
    0b11000000011'10000000110,
    0b11000000000'00000000110,
    0b11111111111'11111111110,
    0b11111111111'11111111110
  };
  constexpr int idx = static_cast<int>(IconType::numTypes);
  static const Icon* const small_icons[idx] = {
    &unknown_small, &rom_small, &directory_small, &zip_small, &cassette_small, &up_small
  };
  static const Icon* const large_icons[idx] = {
    &unknown_large, &rom_large, &directory_large, &zip_large, &cassette_large, &up_large,
  };
  const bool smallIcon = iconWidth() < 24;
  const int iconType = static_cast<int>(_iconTypeList[i]);

  assert(iconType < idx);

  return smallIcon ? small_icons[iconType] : large_icons[iconType];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string FileListWidget::getToolTip(const Common::Point& pos) const
{
  const Common::Rect& rect = getEditRect();
  const int idx = getToolTipIndex(pos);

  if(idx < 0)
    return {};

  if(_includeSubDirs && std::cmp_greater(_dirList.size(), idx))
    return _toolTipText + _dirList[idx];

  const string& value = _list[idx];

  if(static_cast<uInt32>(_font.getStringWidth(value)) > rect.w() - iconWidth())
    return _toolTipText + value;
  else
    return _toolTipText;
}
