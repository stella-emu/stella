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
    myPageSize{pagesize}
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

  // Action buttons to the right of the RAM grid; each sizes itself to its own
  // label, and reflow() gives the group one width
  myUndoButton = new ButtonWidget(boss, lfont, 0, 0, "Undo", kUndoCmd);
  myUndoButton->setHelpAnchor("M6532Search", true);
  wid.push_back(myUndoButton);
  myUndoButton->setTarget(this);

  myRevertButton = new ButtonWidget(boss, lfont, 0, 0, "Revert", kRevertCmd);
  myRevertButton->setHelpAnchor("M6532Search", true);
  wid.push_back(myRevertButton);
  myRevertButton->setTarget(this);

  mySearchButton = new ButtonWidget(boss, lfont, 0, 0,
                                    "Search" + ELLIPSIS, kSearchCmd);
  mySearchButton->setHelpAnchor("M6532Search", true);
  mySearchButton->setToolTip("Search and highlight found values.");
  wid.push_back(mySearchButton);
  mySearchButton->setTarget(this);

  myCompareButton = new ButtonWidget(boss, lfont, 0, 0,
                                     "Compare" + ELLIPSIS, kCmpCmd);
  myCompareButton->setHelpAnchor("M6532Search", true);
  myCompareButton->setToolTip("Compare highlighted values.");
  wid.push_back(myCompareButton);
  myCompareButton->setTarget(this);

  myRestartButton = new ButtonWidget(boss, lfont, 0, 0, "Reset", kRestartCmd);
  myRestartButton->setHelpAnchor("M6532Search", true);
  myRestartButton->setToolTip("Reset search/compare mode.");
  wid.push_back(myRestartButton);
  myRestartButton->setTarget(this);

  addToFocusList(wid);

  // Row-address label and column headers for the RAM grid.  Each is built with
  // a value of the length it always shows, so it owns its own width
  myRamStart = new StaticTextWidget(_boss, lfont, 0, 0, "00xx");

  for(int col = 0; col < 16; ++col)
    myColHeaders[col] = new StaticTextWidget(_boss, lfont, 0, 0,
                          Common::Base::toString(col, Common::Base::Fmt::_16_1));

  for(uInt32 row = 0; row < myNumRows; ++row)
    myRamLabels[row] = new StaticTextWidget(_boss, _font, 0, 0, "0");

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
  myLabel = new EditTextWidget(boss, nfont, 0, 0, 1);
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

  reflow();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RamWidget::setArea(int x, int y, int w, int h)
{
  Widget::setArea(x, y, w, h);
  reflow();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
unique_ptr<GUI::BoxLayout> RamWidget::buildLayout() const
{
  using GUI::BoxLayout;
  using GUI::GridLayout;
  using GUI::anchoredItem;
  using GUI::centeredItem;
  using GUI::alignedItem;
  using GUI::stretchedItem;
  using GUI::HAlign;
  using GUI::VAlign;
  using Dir = BoxLayout::Dir;

  const int VGAP = _font.getFontHeight() / 4,
            HGAP = _fontWidth;

  // Every action button takes the widest label's width
  GUI::alignButtons({myUndoButton, myRevertButton, mySearchButton,
                     myCompareButton, myRestartButton});

  // A control that frames its text sits beside a label on the label's own line
  const auto onBaseline = [](Widget* wid) {
    return alignedItem(wid, HAlign::Left, VAlign::Baseline);
  };

  // The 16 column headings, each centred over the grid column it names
  auto colHeaders = std::make_unique<BoxLayout>(Dir::Horizontal);
  for(auto* h: myColHeaders)
    colHeaders->addFixed(centeredItem(h), myRamGrid->colWidth());

  // The row addresses down the left of the grid.  The grid insets each row's
  // text where a label centres its own, so the stack starts that much lower and
  // label i lands on grid row i's line
  auto digits = std::make_unique<BoxLayout>(Dir::Vertical);
  digits->addSpace(myRamGrid->firstTextY() - myRamLabels[0]->firstTextY());
  for(uInt32 row = 0; row < myNumRows; ++row)
    digits->addFixed(alignedItem(myRamLabels[row], HAlign::Right, VAlign::Center),
                     _lineHeight);

  // ...ending one character clear of the grid
  auto addrCol = std::make_unique<BoxLayout>(Dir::Horizontal);
  addrCol->addStretch(std::move(digits));
  addrCol->addSpace(HGAP);

  // The action buttons, set in from the grid (a wider gap sets Search apart)
  auto buttonCol = std::make_unique<BoxLayout>(Dir::Vertical, VGAP);
  buttonCol->addAuto(anchoredItem(myUndoButton));
  buttonCol->addAuto(anchoredItem(myRevertButton));
  buttonCol->addSpace(VGAP * 5);
  buttonCol->addAuto(anchoredItem(mySearchButton));
  buttonCol->addAuto(anchoredItem(myCompareButton));
  buttonCol->addAuto(anchoredItem(myRestartButton));

  auto buttons = std::make_unique<BoxLayout>(Dir::Horizontal);
  buttons->addSpace(HGAP / 2);
  buttons->addAuto(std::move(buttonCol));

  // The address column is shared by the page heading and the row digits, so an
  // Auto column sizes it from the pair and nobody measures an address.  The
  // grid area is never shorter than eight rows, which is what the old code's
  // "detail row 9 if fewer than 8 rows" was saying
  auto body = std::make_unique<GridLayout>(3, 2);
  body->columnAuto(0).columnAuto(1).columnAuto(2);
  body->rowAuto(0).rowStretch(1, 1, 8 * _lineHeight);

  body->place(0, 0, anchoredItem(myRamStart));
  body->place(1, 0, std::move(colHeaders));
  body->place(0, 1, std::move(addrCol));
  body->place(1, 1, alignedItem(myRamGrid, HAlign::Left, VAlign::Top));
  body->place(2, 1, std::move(buttons));

  // Detail row for the selected RAM cell: a "Label" caption plus a stretchy
  // label field, then the hex / #dec / %bin values
  auto detail = std::make_unique<BoxLayout>(Dir::Horizontal);
  detail->addAuto(onBaseline(myLabelText));
  detail->addSpace(HGAP / 2);
  detail->addStretch(alignedItem(myLabel, HAlign::Fill, VAlign::Baseline));
  detail->addSpace(HGAP * 3 / 2);
  detail->addAuto(onBaseline(myHexValue));
  detail->addSpace(HGAP);
  detail->addAuto(onBaseline(myDecPrefix));
  detail->addAuto(onBaseline(myDecValue));
  detail->addSpace(HGAP);
  detail->addAuto(onBaseline(myBinPrefix));
  detail->addAuto(onBaseline(myBinValue));
  detail->addSpace(HGAP);

  auto root = std::make_unique<BoxLayout>(Dir::Vertical);
  root->addAuto(std::move(body));
  root->addSpace(VGAP * 2);
  root->addAuto(std::move(detail));

  return root;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Common::Size RamWidget::naturalSize() const
{
  return buildLayout()->naturalSize();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RamWidget::reflow()
{
  buildLayout()->doLayout(_x, _y, _w, _h);
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
