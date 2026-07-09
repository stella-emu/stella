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

#include "DataGridRamWidget.hxx"
#include "EditTextWidget.hxx"
#include "GuiObject.hxx"
#include "InputTextDialog.hxx"
#include "OSystem.hxx"
#include "Debugger.hxx"
#include "Font.hxx"
#include "FBSurface.hxx"
#include "Widget.hxx"
#include "ScrollBarWidget.hxx"
#include "Layout.hxx"
#include "RamWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RamWidget::RamWidget(GuiObject* boss, const GUI::Font& lfont, const GUI::Font& nfont,
                     int x, int y, int w, int h,
                     uInt32 ramsize, uInt32 numrows, uInt32 pagesize,
                     string_view helpAnchor)
  : Widget(boss, lfont, x, y, w, h),
    CommandSender(boss),
    _nfont{nfont},
    myRamSize{ramsize},
    myNumRows{numrows},
    myPageSize{pagesize},
    myAutoHeight{h == 0}
{
  WidgetArray wid;

  // Create every widget at a placeholder position; reflow() positions/sizes them
  // NOLINTBEGIN(cppcoreguidelines-prefer-member-initializer)

  // RAM grid (with scrollbar for larger RAM)
  const bool useScrollbar = ramsize / numrows > 16;
  myRamGrid = new DataGridRamWidget(_boss, *this, _nfont, 0, 0,
                                    16, myNumRows, 2, 8, Common::Base::Fmt::_16, useScrollbar);
  myRamGrid->setHelpAnchor(helpAnchor, true);
  myRamGrid->setTarget(this);
  myRamGrid->setID(kRamGridID);
  addFocusWidget(myRamGrid);

  // Action buttons to the right of the RAM grid
  myUndoButton = new ButtonWidget(boss, lfont, 0, 0, 1, 1, "Undo", kUndoCmd);
  myUndoButton->setHelpAnchor("M6532Search", true);
  wid.push_back(myUndoButton);
  myUndoButton->setTarget(this);

  myRevertButton = new ButtonWidget(boss, lfont, 0, 0, 1, 1, "Revert", kRevertCmd);
  myRevertButton->setHelpAnchor("M6532Search", true);
  wid.push_back(myRevertButton);
  myRevertButton->setTarget(this);

  mySearchButton = new ButtonWidget(boss, lfont, 0, 0, 1, 1,
                                    "Search" + ELLIPSIS, kSearchCmd);
  mySearchButton->setHelpAnchor("M6532Search", true);
  mySearchButton->setToolTip("Search and highlight found values.");
  wid.push_back(mySearchButton);
  mySearchButton->setTarget(this);

  myCompareButton = new ButtonWidget(boss, lfont, 0, 0, 1, 1,
                                     "Compare" + ELLIPSIS, kCmpCmd);
  myCompareButton->setHelpAnchor("M6532Search", true);
  myCompareButton->setToolTip("Compare highlighted values.");
  wid.push_back(myCompareButton);
  myCompareButton->setTarget(this);

  myRestartButton = new ButtonWidget(boss, lfont, 0, 0, 1, 1, "Reset", kRestartCmd);
  myRestartButton->setHelpAnchor("M6532Search", true);
  myRestartButton->setToolTip("Reset search/compare mode.");
  wid.push_back(myRestartButton);
  myRestartButton->setTarget(this);

  addToFocusList(wid);

  // Row-address label and column headers for the RAM grid
  const int fontHeight = lfont.getFontHeight();

  myRamStart = new StaticTextWidget(_boss, lfont, 0, 0,
                                    lfont.getStringWidth("xxxx"), fontHeight,
                                    "00xx", TextAlign::Left);

  for(int col = 0; col < 16; ++col)
    myColHeaders[col] = new StaticTextWidget(_boss, lfont, 0, 0,
                          _fontWidth, fontHeight,
                          Common::Base::toString(col, Common::Base::Fmt::_16_1),
                          TextAlign::Left);

  for(uInt32 row = 0; row < myNumRows; ++row)
    myRamLabels[row] = new StaticTextWidget(_boss, _font, 0, 0,
                         _fontWidth, fontHeight, "", TextAlign::Left);

  // Detail row for the selected RAM cell (built from right to left originally,
  // but here just created; reflow() right-aligns the hex/dec/bin cluster)
  myBinPrefix = new StaticTextWidget(boss, lfont, 0, 0, "%");
  myBinValue = new DataGridWidget(boss, nfont, 0, 0, 1, 1, 8, 8, Common::Base::Fmt::_2);
  myBinValue->setHelpAnchor(helpAnchor, true);
  myBinValue->setTarget(this);
  myBinValue->setID(kRamBinID);

  myDecPrefix = new StaticTextWidget(boss, lfont, 0, 0, "#");
  myDecValue = new DataGridWidget(boss, nfont, 0, 0, 1, 1, 3, 8, Common::Base::Fmt::_10);
  myDecValue->setHelpAnchor(helpAnchor, true);
  myDecValue->setTarget(this);
  myDecValue->setID(kRamDecID);

  myHexValue = new DataGridWidget(boss, nfont, 0, 0, 1, 1, 2, 8, Common::Base::Fmt::_16);
  myHexValue->setHelpAnchor(helpAnchor, true);
  myHexValue->setTarget(this);
  myHexValue->setID(kRamHexID);

  addFocusWidget(myHexValue);
  addFocusWidget(myDecValue);
  addFocusWidget(myBinValue);

  myLabelText = new StaticTextWidget(boss, lfont, 0, 0, "Label");
  myLabel = new EditTextWidget(boss, nfont, 0, 0, 1, _lineHeight);
  myLabel->setEditable(false, true);
  // NOLINTEND(cppcoreguidelines-prefer-member-initializer)

  // Inputbox which will pop up when searching RAM
  const StringList labels = { "Value" };
  myInputBox = std::make_unique<InputTextDialog>(boss, lfont, nfont, labels, " ");
  myInputBox->setTextFilter([](char c) {
      return (c >= 'a' && c <= 'f') || (c >= '0' && c <= '9');
    }
  );
  myInputBox->setTarget(this);

  // Start with these buttons disabled
  myCompareButton->clearFlags(Widget::FLAG_ENABLED);
  myRestartButton->clearFlags(Widget::FLAG_ENABLED);

  reflow(w);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RamWidget::setArea(int x, int y, int w, int h)
{
  setPos(x, y);
  // The M6532 view sizes itself to its content; the cartridge view keeps the
  // fixed height it was given (it lives inside a tab)
  if(!myAutoHeight)
    _h = h;
  reflow(w);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RamWidget::reflow(int w)
{
  using GUI::BoxLayout;
  using GUI::anchoredItem;
  using GUI::indentedItem;
  using GUI::labelColumn;
  using GUI::vCentered;
  using Dir = BoxLayout::Dir;

  const int x = _x, y = _y;
  const int VGAP    = _font.getFontHeight() / 4;
  const int bheight = _lineHeight + 2;
  const int gridX   = x + _font.getStringWidth("xxxx");
  const int gridY   = y + _lineHeight;
  const int colWidth = myRamGrid->colWidth(),
            gridW    = myRamGrid->getWidth();

  _w = w;

  // A button keeps its size across a font change, so size them from the live
  // font here; they all share the widest label's width
  const int bwidth = _font.getStringWidth("Compare " + ELLIPSIS);
  for(auto* b: {myUndoButton, myRevertButton, mySearchButton, myCompareButton,
                myRestartButton})
  {
    b->setWidth(bwidth);
    b->setHeight(bheight);
  }

  // The grid (its scrollbar, if any, tracks it via DataGridWidget::setPos)
  myRamGrid->setPos(gridX, gridY);

  // Header row: row-address cell then the 16 column labels aligned to columns
  BoxLayout header(Dir::Horizontal);
  header.addFixed(anchoredItem(myRamStart), gridX - x);
  for(auto* h: myColHeaders)
    header.addFixed(indentedItem(h, 8), colWidth);
  header.doLayout(x, y, gridX - x + 16 * colWidth, _lineHeight);

  // Row-address labels down the left side of the grid, which insets each row's
  // text, so they start that far down to share its lines
  BoxLayout rowLabels(Dir::Vertical);
  rowLabels.addSpace(myRamGrid->textOffsetY());
  for(uInt32 row = 0; row < myNumRows; ++row)
    rowLabels.addFixed(anchoredItem(myRamLabels[row]), _lineHeight);
  rowLabels.doLayout(gridX - _font.getStringWidth("x "), gridY,
                     _fontWidth, myNumRows * _lineHeight);

  // Action buttons to the right of the grid (a wider gap sets Search apart)
  BoxLayout buttons(Dir::Vertical);
  buttons.addFixed(anchoredItem(myUndoButton), bheight);
  buttons.addSpace(VGAP);
  buttons.addFixed(anchoredItem(myRevertButton), bheight);
  buttons.addSpace(VGAP * 6);
  buttons.addFixed(anchoredItem(mySearchButton), bheight);
  buttons.addSpace(VGAP);
  buttons.addFixed(anchoredItem(myCompareButton), bheight);
  buttons.addSpace(VGAP);
  buttons.addFixed(anchoredItem(myRestartButton), bheight);
  buttons.doLayout(gridX + gridW + 4, gridY, bwidth,
                   bheight * 5 + VGAP * 10);

  // Detail row for the selected RAM cell: a "Label" caption plus a stretchy
  // label field, then the hex / #dec / %bin values right-aligned
  const uInt32 detailRow = myNumRows < 8 ? 9 : myNumRows + 1;
  const int detailY = gridY + (detailRow - 1) * _lineHeight + VGAP * 2;

  // The controls inset their own text, so the row is positioned by that text
  // line: the labels land on 'detailY' and the controls just above it
  BoxLayout detail(Dir::Horizontal);
  detail.addFixed(labelColumn(myLabelText, myLabel), myLabelText->getWidth());
  detail.addSpace(_fontWidth / 2);
  detail.addStretch(vCentered(myLabel, myLabel->getHeight()));
  detail.addSpace(_fontWidth * 3 / 2);
  detail.addFixed(anchoredItem(myHexValue), myHexValue->getWidth());
  detail.addSpace(_fontWidth);
  detail.addFixed(labelColumn(myDecPrefix, myDecValue), myDecPrefix->getWidth());
  detail.addFixed(anchoredItem(myDecValue), myDecValue->getWidth());
  detail.addSpace(_fontWidth);
  detail.addFixed(labelColumn(myBinPrefix, myBinValue), myBinPrefix->getWidth());
  detail.addFixed(anchoredItem(myBinValue), myBinValue->getWidth());
  detail.addSpace(9);
  detail.doLayout(x, detailY - myHexValue->textOffsetY(), w, _lineHeight);

  // The M6532 view fits its height to the content
  if(myAutoHeight)
    _h = detailY + _lineHeight - y;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RamWidget::~RamWidget() = default;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RamWidget::handleCommand(CommandSender* sender, int cmd, int data, int id)
{
  // We simply change the values in the DataGridWidget
  // It will then send the 'kDGItemDataChangedCmd' signal to change the actual
  // memory location
  int addr = 0, value = 0;

  switch(cmd)
  {
    case DataGridWidget::kItemDataChangedCmd:
    {
      switch(id)
      {
        case kRamGridID:
          addr  = myRamGrid->getSelectedAddr();
          value = myRamGrid->getSelectedValue();
          break;

        case kRamHexID:
          addr  = myRamGrid->getSelectedAddr();
          value = myHexValue->getSelectedValue();
          break;

        case kRamDecID:
          addr  = myRamGrid->getSelectedAddr();
          value = myDecValue->getSelectedValue();
          break;

        case kRamBinID:
          addr  = myRamGrid->getSelectedAddr();
          value = myBinValue->getSelectedValue();
          break;

        default:
          break;
      }

      const uInt8 oldval = getValue(addr);
      setValue(addr, value);

      myUndoAddress = addr;
      myUndoValue = oldval;

      myRamGrid->setValueInternal(addr - myCurrentRamBank*myPageSize, value, true);
      myHexValue->setValueInternal(0, value, true);
      myDecValue->setValueInternal(0, value, true);
      myBinValue->setValueInternal(0, value, true);

      myRevertButton->setEnabled(true);
      myUndoButton->setEnabled(true);
      break;
    }

    case DataGridWidget::kSelectionChangedCmd:
    {
      addr  = myRamGrid->getSelectedAddr();
      value = myRamGrid->getSelectedValue();
      const bool changed = myRamGrid->getSelectedChanged();

      myLabel->setText(getLabel(addr));
      myHexValue->setValueInternal(0, value, changed);
      myDecValue->setValueInternal(0, value, changed);
      myBinValue->setValueInternal(0, value, changed);
      break;
    }

    case kRevertCmd:
      for(uInt32 i = 0; i < myOldValueList.size(); ++i)
        setValue(i, myOldValueList[i]);
      fillGrid(true);
      break;

    case kUndoCmd:
      setValue(myUndoAddress, myUndoValue);
      myUndoButton->setEnabled(false);
      fillGrid(false);
      break;

    case kSearchCmd:
      showInputBox(kSValEntered);
      break;

    case kCmpCmd:
      showInputBox(kCValEntered);
      break;

    case kRestartCmd:
      doRestart();
      break;

    case kSValEntered:
    {
      const string_view result = doSearch(myInputBox->getResult());
      if(!result.empty())
        myInputBox->setMessage(result);
      else
        myInputBox->close();
      break;
    }

    case kCValEntered:
    {
      const string_view result = doCompare(myInputBox->getResult());
      if(!result.empty())
        myInputBox->setMessage(result);
      else
        myInputBox->close();
      break;
    }

    case GuiObject::kSetPositionCmd:
      myCurrentRamBank = data;
      showSearchResults();
      fillGrid(false);
      break;

    default:
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RamWidget::setOpsWidget(DataGridOpsWidget* w)
{
  myRamGrid->setOpsWidget(w);
  myHexValue->setOpsWidget(w);
  myBinValue->setOpsWidget(w);
  myDecValue->setOpsWidget(w);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RamWidget::loadConfig()
{
  fillGrid(true);

  const int value = myRamGrid->getSelectedValue();
  const bool changed = myRamGrid->getSelectedChanged();

  myHexValue->setValueInternal(0, value, changed);
  myDecValue->setValueInternal(0, value, changed);
  myBinValue->setValueInternal(0, value, changed);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RamWidget::fillGrid(bool updateOld)
{
  IntArray alist;
  IntArray vlist;
  BoolArray changed;

  const uInt32 start = myCurrentRamBank * myPageSize;
  fillList(start, myPageSize, alist, vlist, changed);

  if(updateOld)
    myOldValueList = currentRam(start);

  myRamGrid->setNumRows(myRamSize / myPageSize);
  myRamGrid->setList(alist, vlist, changed);
  if(updateOld)
  {
    myRevertButton->setEnabled(false);
    myUndoButton->setEnabled(false);
  }

  // Update RAM labels
  const uInt32 rport = readPort(start);
  int page = rport & 0xf0;
  string label = Common::Base::toString(rport, Common::Base::Fmt::_16_4);

  label[2] = label[3] = 'x';
  myRamStart->setLabel(label);
  for(uInt32 row = 0; row < myNumRows; ++row, page += 0x10)
    myRamLabels[row]->setLabel(Common::Base::toString(page>>4, Common::Base::Fmt::_16_1));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RamWidget::showInputBox(int cmd)
{
  // Add inputbox in the middle of the RAM widget
  const uInt32 x = getAbsX() + ((getWidth() - myInputBox->getWidth()) >> 1);
  const uInt32 y = getAbsY() + ((getHeight() - myInputBox->getHeight()) >> 1);

  myInputBox->show(x, y, dialog().surface().dstRect());
  myInputBox->setText("");
  myInputBox->setMessage("");
  myInputBox->setToolTip(cmd == kSValEntered
                         ? "Enter search value (leave blank for all)."
                         : "Enter relative or absolute value\nto compare with searched values.");
  myInputBox->setFocus(0);
  myInputBox->setEmitSignal(cmd);
  myInputBox->setTitle(cmd == kSValEntered ? "Search" : "Compare");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string_view RamWidget::doSearch(string_view str)
{
  bool comparisonSearch = true;

  if(str.empty())
  {
    // An empty field means return all memory locations
    comparisonSearch = false;
  }
  else if(str.find_first_of("+-", 0) != string::npos)
  {
    // Don't accept these characters here, only in compare
    return "Invalid input +|-";
  }

  const int searchVal = instance().debugger().stringToValue(str);

  // Clear the search array of previous items
  mySearchAddr.clear();
  mySearchValue.clear();
  mySearchState.clear();

  // Now, search all memory locations for this value, and add it to the
  // search array
  const ByteArray& ram = currentRam(0);
  bool hitfound = false;
  for(uInt32 addr = 0; addr < ram.size(); ++addr)
  {
    const int value = ram[addr];
    if(comparisonSearch && searchVal != value)
    {
      mySearchState.push_back(false);
    }
    else
    {
      mySearchAddr.push_back(addr);
      mySearchValue.push_back(value);
      mySearchState.push_back(true);
      hitfound = true;
    }
  }

  // If we have some hits, enable the comparison methods
  if(hitfound)
  {
    mySearchButton->setEnabled(false);
    myCompareButton->setEnabled(true);
    myRestartButton->setEnabled(true);
  }

  // Finally, show the search results in the list
  showSearchResults();

  return {};
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string_view RamWidget::doCompare(string_view str)
{
  bool comparativeSearch = false;
  int searchVal = 0, offset = 0;

  if(str.empty())
    return "Enter an absolute or comparative value";

  // Do some pre-processing on the string
  const string::size_type pos = str.find_first_of("+-", 0);
  if(pos > 0 && pos != string::npos)
  {
    // Only accept '+' or '-' at the start of the string
    return "Input must be [+|-]NUM";
  }

  // A comparative search searches memory for locations that have changed by
  // the specified amount, vs. for exact values
  if(str[0] == '+' || str[0] == '-')
  {
    comparativeSearch = true;
    bool negative = false;
    if(str[0] == '-')
      negative = true;

    string tmp{str};
    tmp.erase(0, 1);  // remove the operator
    offset = instance().debugger().stringToValue(tmp);
    if(negative)
      offset = -offset;
  }
  else
    searchVal = instance().debugger().stringToValue(str);

  // Now, search all memory locations previously 'found' for this value
  const ByteArray& ram = currentRam(0);
  bool hitfound = false;
  IntArray tempAddrList, tempValueList;
  mySearchState.clear();
  for(uInt32 i = 0; i < ram.size(); ++i)
    mySearchState.push_back(false);

  for(uInt32 i = 0; i < mySearchAddr.size(); ++i)
  {
    if(comparativeSearch)
    {
      searchVal = mySearchValue[i] + offset;
      if(searchVal < 0 || searchVal > 255)
        continue;
    }

    const int addr = mySearchAddr[i];
    if(std::cmp_equal(ram[addr], searchVal))
    {
      tempAddrList.push_back(addr);
      tempValueList.push_back(searchVal);
      mySearchState[addr] = hitfound = true;
    }
  }

  // Update the searchArray for the new addresses and data
  mySearchAddr = tempAddrList;
  mySearchValue = tempValueList;

  // If we have some hits, enable the comparison methods
  if(hitfound)
  {
    myCompareButton->setEnabled(true);
    myRestartButton->setEnabled(true);
  }

  // Finally, show the search results in the list
  showSearchResults();

  return {};
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RamWidget::doRestart()
{
  // Erase all search buffers, reset to start mode
  mySearchAddr.clear();
  mySearchValue.clear();
  mySearchState.clear();
  showSearchResults();

  mySearchButton->setEnabled(true);
  myCompareButton->setEnabled(false);
  myRestartButton->setEnabled(false);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RamWidget::showSearchResults()
{
  // Only update the search results for the bank currently being shown
  BoolArray temp;
  const uInt32 start = myCurrentRamBank * myPageSize;
  if(mySearchState.empty() || start > mySearchState.size())
  {
    for(uInt32 i = 0; i < myPageSize; ++i)
      temp.push_back(false);
  }
  else
  {
    for(uInt32 i = start; i < start + myPageSize; ++i)
      temp.push_back(mySearchState[i]);
  }
  myRamGrid->setHiliteList(temp);
}
