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

#include <cctype>

#include "ScrollBarWidget.hxx"
#include "FileListWidget.hxx"
#include "TimerManager.hxx"
#include "ProgressDialog.hxx"
#include "FBSurface.hxx"
#include "Bankswitch.hxx"

#include "bspf.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FileListWidget::FileListWidget(GuiObject* boss, const GUI::Font& font,
                               int x, int y, int w, int h)
  : StringListWidget(boss, font, x, y, w, h),
    _filter{[](const FilesystemNode& node) { return true; }}
{
  // This widget is special, in that it catches signals and redirects them
  setTarget(this);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FileListWidget::setDirectory(const FilesystemNode& node,
                                  const string& select)
{
  _node = node;

  // We always want a directory listing
  if(!_node.isDirectory() && _node.hasParent())
  {
    _selectedFile = _node.getName();
    _node = _node.getParent();
  }
  else
    _selectedFile = select;

  // Initialize history
  FilesystemNode tmp = _node;
  while(tmp.hasParent() && !_history.full())
  {
    string name = tmp.getName();
    if(name.back() == FilesystemNode::PATH_SEPARATOR)
      name.pop_back();
    _history.push(name);
    tmp = tmp.getParent();
  }
  // History is in reverse order; we need to fix that
  _history.reverse();

  // Finally, go to this location
  setLocation(_node, _selectedFile);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FileListWidget::setLocation(const FilesystemNode& node,
                                 const string& select)
{
  progress().resetProgress();
  progress().open();
  FilesystemNode::CancelCheck isCancelled = [this]() {
    return myProgressDialog->isCancelled();
  };

  _node = node;

  // Read in the data from the file system (start with an empty list)
  _fileList.clear();

  if(_includeSubDirs)
  {
    // Actually this could become HUGE
    _fileList.reserve(0x2000);
    _node.getAllChildren(_fileList, _fsmode, _filter, true, isCancelled);
  }
  else
  {
    _fileList.reserve(0x200);
    _node.getChildren(_fileList, _fsmode, _filter, false, true, isCancelled);
  }

  // Now fill the list widget with the names from the file list,
  // even if cancelled
  StringList l;
  size_t orgLen = _node.getShortPath().length();

  _dirList.clear();
  _iconList.clear();
  for(const auto& file : _fileList)
  {
    const string path = file.getShortPath();
    const string name = file.getName();

    l.push_back(name);
    // display only relative path in tooltip
    if(path.length() >= orgLen)
      _dirList.push_back(path.substr(orgLen));
    else
      _dirList.push_back(path);
    if(file.isDirectory())
    {
      if(BSPF::endsWithIgnoreCase(name, ".zip"))
        _iconList.push_back(IconType::zip);
      else
        _iconList.push_back(IconType::directory);
    }
    else if(file.isFile() && Bankswitch::isValidRomName(name))
      _iconList.push_back(IconType::rom);
    else
      _iconList.push_back(IconType::unknown);
  }

  setList(l);
  setSelected(select);
  ListWidget::recalc();

  progress().close();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FileListWidget::selectDirectory()
{
  _history.push(selected().getName());
  setLocation(selected(), _selectedFile);
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
  {
    _selectedFile = selected().getName();
    setLocation(_node, _selectedFile);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ProgressDialog& FileListWidget::progress()
{
  if(myProgressDialog == nullptr)
    myProgressDialog = make_unique<ProgressDialog>(this, _font, "");

  return *myProgressDialog;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FileListWidget::incProgress()
{
  progress().incProgress();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FileListWidget::handleText(char text)
{
  // Quick selection mode: Go to first list item starting with this key
  // (or a substring accumulated from the last couple key presses).
  // Only works in a useful fashion if the list entries are sorted.
  uInt64 time = TimerManager::getTicks() / 1000;

  if(_quickSelectTime < time)
    _quickSelectStr = text;
  else
    _quickSelectStr += text;
  _quickSelectTime = time + _QUICK_SELECT_DELAY;

  int selectedItem = 0;
  for(const auto& i : _list)
  {
    if(BSPF::startsWithIgnoreCase(i, _quickSelectStr))
      // Select directories when the first character is uppercase
      if(std::isupper(_quickSelectStr[0]) ==
          (_iconList[selectedItem] == IconType::directory))
        break;
    selectedItem++;
  }

  if(selectedItem > 0)
    setSelected(selectedItem);

  return true;
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
        _selectedFile = selected().getName();
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
  const Icon unknown_small = {
    0b00111111'1100000,
    0b00100000'0110000,
    0b00100000'0011000,
    0b00100000'0001100,
    0b00100000'0000100,
    0b00100000'0000100,
    0b00100000'0000100,
    0b00100000'0000100,
    0b00100000'0000100,
    0b00100000'0000100,
    0b00100000'0000100,
    0b00100000'0000100,
    0b00100000'0000100,
    0b00111111'1111100
  };
  const Icon rom_small = {
    0b00001111'1110000,
    0b00001010'1010000,
    0b00001010'1010000,
    0b00001010'1010000,
    0b00001010'1010000,
    0b00001010'1010000,
    0b00011010'1011000,
    0b00110010'1001100,
    0b00100110'1100100,
    0b11101110'1110111,
    0b10001010'1010001,
    0b10011010'1011001,
    0b11110011'1001111
  };
  const Icon directory_small = {
    0b11111000'0000000,
    0b11111100'0000000,
    0b11111111'1111111,
    0b10000000'0000001,
    0b10000000'0000001,
    0b10000000'0000001,
    0b10000000'0000001,
    0b10000000'0000001,
    0b10000000'0000001,
    0b10000000'0000001,
    0b10000000'0000001,
    0b10000000'0000001,
    0b11111111'1111111
  };
  const Icon zip_small = {
    //0b0011111'11111111,
    //0b0110000'11000111,
    //0b1111111'11111101,
    //0b1000001'00000111,
    //0b1000001'00000101,
    //0b1000001'00000111,
    //0b1000001'00000111,
    //0b1111111'11111101,
    //0b1000001'00000111,
    //0b1000001'00000101,
    //0b1000001'00000111,
    //0b1000001'00000110,
    //0b1111111'11111100
    0b11111000'0000000,
    0b11111100'0000000,
    0b11111111'1111111,
    0b10000000'0000001,
    0b10001111'1110001,
    0b10000000'1110001,
    0b10000001'1100001,
    0b10000011'1000001,
    0b10000111'0000001,
    0b10001110'0000001,
    0b10001111'1110001,
    0b10000000'0000001,
    0b11111111'1111111

  };
  const Icon unknown_large = {
    0b00111'11111111'11000000,
    0b00111'11111111'11100000,
    0b00110'00000000'01110000,
    0b00110'00000000'00111000,
    0b00110'00000000'00011100,
    0b00110'00000000'00001100,
    0b00110'00000000'00001100,
    0b00110'00000000'00001100,
    0b00110'00000000'00001100,
    0b00110'00000000'00001100,
    0b00110'00000000'00001100,
    0b00110'00000000'00001100,
    0b00110'00000000'00001100,
    0b00110'00000000'00001100,
    0b00110'00000000'00001100,
    0b00110'00000000'00001100,
    0b00110'00000000'00001100,
    0b00110'00000000'00001100,
    0b00110'00000000'00001100,
    0b00111'11111111'11111100,
    0b00111'11111111'11111100
  };
  const Icon rom_large = {
    0b00000'01111111'11000000,
    0b00000'01111111'11000000,
    0b00000'01101010'11000000,
    0b00000'01101010'11000000,
    0b00000'01101010'11000000,
    0b00000'01101010'11000000,
    0b00000'01101010'11000000,
    0b00000'01101010'11000000,
    0b00000'11101010'11100000,
    0b00000'11001010'01100000,
    0b00000'11001010'01100000,
    0b00001'11001010'01110000,
    0b00001'10001010'00110000,
    0b00011'10011011'00111000,
    0b00011'00011011'00011000,
    0b11111'00111011'10011111,
    0b11110'01111011'11001111,
    0b11000'01111011'11000011,
    0b11000'11111011'11100011,
    0b11111'11011111'01111111,
    0b11111'10011111'00111111
  };
  const Icon directory_large = {
    0b111111'10000000'0000000,
    0b111111'11000000'0000000,
    0b111111'11100000'0000000,
    0b111111'11111111'1111111,
    0b111111'11111111'1111111,
    0b110000'00000000'0000011,
    0b110000'00000000'0000011,
    0b110000'00000000'0000011,
    0b110000'00000000'0000011,
    0b110000'00000000'0000011,
    0b110000'00000000'0000011,
    0b110000'00000000'0000011,
    0b110000'00000000'0000011,
    0b110000'00000000'0000011,
    0b110000'00000000'0000011,
    0b110000'00000000'0000011,
    0b110000'00000000'0000011,
    0b110000'00000000'0000011,
    0b110000'00000000'0000011,
    0b111111'11111111'1111111,
    0b111111'11111111'1111111
  };
  const Icon zip_large = {
    0b111111'10000000'0000000,
    0b111111'11000000'0000000,
    0b111111'11100000'0000000,
    0b111111'11111111'1111111,
    0b111111'11111111'1111111,
    0b110000'00000000'0000011,
    0b110000'00000000'0000011,
    0b110000'11111111'1000011,
    0b110000'11111111'1000011,
    0b110000'00000011'0000011,
    0b110000'00000110'0000011,
    0b110000'00001100'0000011,
    0b110000'00011000'0000011,
    0b110000'00110000'0000011,
    0b110000'01100000'0000011,
    0b110000'11111111'1000011,
    0b110000'11111111'1000011,
    0b110000'00000000'0000011,
    0b110000'00000000'0000011,
    0b111111'11111111'1111111,
    0b111111'11111111'1111111
  };
  const bool smallIcon = iconWidth() < 24;
  const int iconGap = smallIcon ? 2 : 3;
  Icon icon = smallIcon ? unknown_small : unknown_large;

  switch(_iconList[i])
  {
    case IconType::rom:
      icon = smallIcon ? rom_small: rom_large;
      break;

    case IconType::directory:
      icon = smallIcon ? directory_small : directory_large;
      break;

    case IconType::zip:
      icon = smallIcon ? zip_small : zip_large;
      break;

    default:
      break;
  }

  FBSurface& s = _boss->dialog().surface();

  s.drawBitmap(icon.data(), x + 1 + iconGap, y + (_lineHeight - static_cast<int>(icon.size())) / 2,
    color, iconWidth() - iconGap * 2, static_cast<int>(icon.size()));

  return iconWidth();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int FileListWidget::iconWidth() const
{
  bool smallIcon = _lineHeight < 26;

  return smallIcon ? 16 + 4: 24 + 6;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string FileListWidget::getToolTip(const Common::Point& pos) const
{
  const Common::Rect rect = getEditRect();
  const int idx = getToolTipIndex(pos);

  if(idx < 0)
    return EmptyString;

  if(_includeSubDirs && static_cast<int>(_dirList.size()) > idx)
    return _toolTipText + _dirList[idx];

  const string value = _list[idx];

  if(uInt32(_font.getStringWidth(value)) > rect.w() - iconWidth())
    return _toolTipText + value;
  else
    return _toolTipText;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt64 FileListWidget::_QUICK_SELECT_DELAY = 300;
