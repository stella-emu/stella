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
// Copyright (c) 1995-2015 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

#include "DataGridWidget.hxx"
#include "EditTextWidget.hxx"
#include "GuiObject.hxx"
#include "InputTextDialog.hxx"
#include "OSystem.hxx"
#include "CartDebug.hxx"
#include "Widget.hxx"

#include "RamWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RamWidget::RamWidget(GuiObject* boss, const GUI::Font& lfont, const GUI::Font& nfont,
                     int x, int y, int w, int h,
                     uInt32 ramsize, uInt32 numrows, uInt32 pagesize)
  : Widget(boss, lfont, x, y, w, h),
    CommandSender(boss),
    _nfont(nfont),
    myFontWidth(lfont.getMaxCharWidth()),
    myFontHeight(lfont.getFontHeight()),
    myLineHeight(lfont.getLineHeight()),
    myButtonHeight(myLineHeight + 4),
    myCurrentRamBank(0),
    myRamSize(ramsize),
    myNumRows(numrows),
    myPageSize(pagesize)
{
  const int bwidth  = lfont.getStringWidth("Compare "),
            bheight = myLineHeight + 2;

  int ypos = y + myLineHeight;

  // Add RAM grid (with scrollbar)
  int xpos = x + _font.getStringWidth("xxxx");
  myRamGrid = new DataGridWidget(_boss, _nfont, xpos, ypos,
                                 16, myNumRows, 2, 8, Common::Base::F_16, true);
  myRamGrid->setTarget(this);
  myRamGrid->setID(kRamHexID);
  addFocusWidget(myRamGrid);

  // Create actions buttons to the left of the RAM grid
  int bx = xpos + myRamGrid->getWidth() + 4;
  int by = ypos;
  myUndoButton = new ButtonWidget(boss, lfont, bx, by, bwidth, bheight,
                                  "Undo", kUndoCmd);
  myUndoButton->setTarget(this);

  by += bheight + 4;
  myRevertButton = new ButtonWidget(boss, lfont, bx, by, bwidth, bheight,
                                    "Revert", kRevertCmd);
  myRevertButton->setTarget(this);

  by += 2 * bheight + 2;
  mySearchButton = new ButtonWidget(boss, lfont, bx, by, bwidth, bheight,
                                    "Search", kSearchCmd);
  mySearchButton->setTarget(this);

  by += bheight + 4;
  myCompareButton = new ButtonWidget(boss, lfont, bx, by, bwidth, bheight,
                                     "Compare", kCmpCmd);
  myCompareButton->setTarget(this);

  by += bheight + 4;
  myRestartButton = new ButtonWidget(boss, lfont, bx, by, bwidth, bheight,
                                     "Reset", kRestartCmd);
  myRestartButton->setTarget(this);

  // Labels for RAM grid
  myRamStart =
    new StaticTextWidget(_boss, lfont, xpos - _font.getStringWidth("xxxx"),
                         ypos - myLineHeight,
                         lfont.getStringWidth("xxxx"), myFontHeight,
                        "00xx", kTextAlignLeft);

  for(int col = 0; col < 16; ++col)
  {
    new StaticTextWidget(_boss, lfont, xpos + col*myRamGrid->colWidth() + 8,
                         ypos - myLineHeight,
                         myFontWidth, myFontHeight,
                         Common::Base::toString(col, Common::Base::F_16_1),
                         kTextAlignLeft);
  }

  uInt32 row;
  for(row = 0; row < myNumRows; ++row)
  {
    myRamLabels[row] =
      new StaticTextWidget(_boss, _font, xpos - _font.getStringWidth("x "),
                           ypos + row*myLineHeight + 2,
                           myFontWidth, myFontHeight, "", kTextAlignLeft);
  }

  // For smaller grids, make sure RAM cell detail fields are below the RESET button
  row = myNumRows < 8 ? 9 : myNumRows + 1;
  ypos += row * myLineHeight;

  // We need to define these widgets from right to left since the leftmost
  // one resizes as much as possible

  // Add Binary display of selected RAM cell
  xpos = x + w - 13*myFontWidth - 20;
  new StaticTextWidget(boss, lfont, xpos, ypos, 4*myFontWidth, myFontHeight,
                       "Bin:", kTextAlignLeft);
  myBinValue = new DataGridWidget(boss, nfont, xpos + 4*myFontWidth + 5, ypos-2,
                                  1, 1, 8, 8, Common::Base::F_2);
  myBinValue->setTarget(this);
  myBinValue->setID(kRamBinID);

  // Add Decimal display of selected RAM cell
  xpos -= 8*myFontWidth + 5 + 20;
  new StaticTextWidget(boss, lfont, xpos, ypos, 4*myFontWidth, myFontHeight,
                       "Dec:", kTextAlignLeft);
  myDecValue = new DataGridWidget(boss, nfont, xpos + 4*myFontWidth + 5, ypos-2,
                                  1, 1, 3, 8, Common::Base::F_10);
  myDecValue->setTarget(this);
  myDecValue->setID(kRamDecID);

  addFocusWidget(myDecValue);
  addFocusWidget(myBinValue);

  // Add Label of selected RAM cell
  int xpos_r = xpos - 20;
  xpos = x + 10;
  new StaticTextWidget(boss, lfont, xpos, ypos, 6*myFontWidth, myFontHeight,
                       "Label:", kTextAlignLeft);
  xpos += 6*myFontWidth + 5;
  myLabel = new EditTextWidget(boss, nfont, xpos, ypos-2, xpos_r-xpos,
                               myLineHeight);
  myLabel->setEditable(false);

  // Inputbox which will pop up when searching RAM
  StringList labels = { "Search: " };
  myInputBox = make_ptr<InputTextDialog>(boss, lfont, nfont, labels);
  myInputBox->setTarget(this);

  // Start with these buttons disabled
  myCompareButton->clearFlags(WIDGET_ENABLED);
  myRestartButton->clearFlags(WIDGET_ENABLED);

  // Calculate final height
  if(_h == 0)  _h = ypos + myLineHeight - y;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RamWidget::~RamWidget()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RamWidget::handleCommand(CommandSender* sender, int cmd, int data, int id)
{
  // We simply change the values in the DataGridWidget
  // It will then send the 'kDGItemDataChangedCmd' signal to change the actual
  // memory location
  int addr, value;

  switch(cmd)
  {
    case DataGridWidget::kItemDataChangedCmd:
    {
      switch(id)
      {
        case kRamHexID:
          addr  = myRamGrid->getSelectedAddr();
          value = myRamGrid->getSelectedValue();
          break;

        case kRamDecID:
          addr  = myRamGrid->getSelectedAddr();
          value = myDecValue->getSelectedValue();
          break;

        case kRamBinID:
          addr  = myRamGrid->getSelectedAddr();
          value = myBinValue->getSelectedValue();
          break;
      }

      uInt8 oldval = getValue(addr);
      setValue(addr, value);

      myUndoAddress = addr;
      myUndoValue = oldval;

      myRamGrid->setValueInternal(addr - myCurrentRamBank*myPageSize, value, true);
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

      myLabel->setText(getLabel(addr));
      myDecValue->setValueInternal(0, value, false);
      myBinValue->setValueInternal(0, value, false);
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
      const string& result = doSearch(myInputBox->getResult());
      if(result != "")
        myInputBox->setTitle(result);
      else
        myInputBox->close();
      break;
    }

    case kCValEntered:
    {
      const string& result = doCompare(myInputBox->getResult());
      if(result != "")
        myInputBox->setTitle(result);
      else
        myInputBox->close();
      break;
    }

    case kSetPositionCmd:
      myCurrentRamBank = data;
      showSearchResults();
      fillGrid(false);
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RamWidget::setOpsWidget(DataGridOpsWidget* w)
{
  myRamGrid->setOpsWidget(w);
  myBinValue->setOpsWidget(w);
  myDecValue->setOpsWidget(w);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RamWidget::loadConfig()
{
  fillGrid(true);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RamWidget::fillGrid(bool updateOld)
{
  IntArray alist;
  IntArray vlist;
  BoolArray changed;

  uInt32 start = myCurrentRamBank * myPageSize;
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
  uInt32 rport = readPort(start), page = rport & 0xf0;
  char buf[5];
  BSPF_snprintf(buf, 5, "%04X", rport);
  buf[2] = buf[3] = 'x';
  myRamStart->setLabel(buf);
  for(uInt32 row = 0; row < myNumRows; ++row, page += 0x10)
    myRamLabels[row]->setLabel(Common::Base::toString(page>>4, Common::Base::F_16_1));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RamWidget::showInputBox(int cmd)
{
  // Add inputbox in the middle of the RAM widget
  uInt32 x = getAbsX() + ((getWidth() - myInputBox->getWidth()) >> 1);
  uInt32 y = getAbsY() + ((getHeight() - myInputBox->getHeight()) >> 1);
  myInputBox->show(x, y);
  myInputBox->setText("");
  myInputBox->setTitle("");
  myInputBox->setFocus(0);
  myInputBox->setEmitSignal(cmd);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string RamWidget::doSearch(const string& str)
{
  bool comparisonSearch = true;

  if(str.length() == 0)
  {
    // An empty field means return all memory locations
    comparisonSearch = false;
  }
  else if(str.find_first_of("+-", 0) != string::npos)
  {
    // Don't accept these characters here, only in compare
    return "Invalid input +|-";
  }

  int searchVal = instance().debugger().stringToValue(str);

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
    int value = ram[addr];
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

  return EmptyString;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string RamWidget::doCompare(const string& str)
{
  bool comparitiveSearch = false;
  int searchVal = 0, offset = 0;

  if(str.length() == 0)
    return "Enter an absolute or comparitive value";

  // Do some pre-processing on the string
  string::size_type pos = str.find_first_of("+-", 0);
  if(pos > 0 && pos != string::npos)
  {
    // Only accept '+' or '-' at the start of the string
    return "Input must be [+|-]NUM";
  }

  // A comparitive search searches memory for locations that have changed by
  // the specified amount, vs. for exact values
  if(str[0] == '+' || str[0] == '-')
  {
    comparitiveSearch = true;
    bool negative = false;
    if(str[0] == '-')
      negative = true;

    string tmp = str;
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
    if(comparitiveSearch)
    {
      searchVal = mySearchValue[i] + offset;
      if(searchVal < 0 || searchVal > 255)
        continue;
    }

    int addr = mySearchAddr[i];
    if(ram[addr] == searchVal)
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

  return EmptyString;
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
  uInt32 start = myCurrentRamBank * myPageSize;
  if(mySearchState.size() == 0 || start > mySearchState.size())
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
